/*
 * Android window/graphics implementation for TaijiOS
 * Uses OpenGL ES 2.0 for rendering
 *
 * Phase 2 COMPLETE - Full OpenGL ES renderer implemented
 *
 * This file implements the screen buffer and rendering functions
 * required by the draw device (devdraw.c)
 *
 * - attachscreen(): Allocates screen buffer, initializes OpenGL ES
 * - flushmemscreen(): Renders screen buffer to texture, draws fullscreen quad
 * - drawcursor(): Stub for cursor rendering (TODO)
 */

#include "dat.h"
#include "fns.h"
#include "error.h"
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "keyboard.h"

#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#define LOG_TAG "TaijiOS-Win"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* EGL context - accessible by android_test.c */
extern EGLDisplay g_display;
extern EGLSurface g_surface;
extern EGLContext g_context;

/* Screen buffer data */
static int screenwidth = 0;
static int screenheight = 0;
static int screensize = 0;
static uchar *screendata = NULL;
static Memdata screendata_struct;

/* OpenGL ES resources */
static GLuint texture = 0;
static GLuint shader_program = 0;
static GLuint position_buffer = 0;
static GLuint texcoord_buffer = 0;
static GLuint index_buffer = 0;

/* Shader sources */
static const char vertex_shader_src[] =
	"attribute vec2 a_position;\n"
	"attribute vec2 a_texcoord;\n"
	"varying vec2 v_texcoord;\n"
	"void main() {\n"
	"	gl_Position = vec4(a_position, 0.0, 1.0);\n"
	"	v_texcoord = a_texcoord;\n"
	"}\n";

static const char fragment_shader_src[] =
	"precision mediump float;\n"
	"varying vec2 v_texcoord;\n"
	"uniform sampler2D u_texture;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(u_texture, v_texcoord);\n"
	"}\n";

/* Fullscreen quad vertices */
static const float vertices[] = {
	-1.0f, -1.0f,
	 1.0f, -1.0f,
	-1.0f,  1.0f,
	 1.0f,  1.0f,
};

static const float texcoords[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
};

static const GLushort indices[] = {
	0, 1, 2,
	1, 3, 2,
};

/* Compile shader */
static GLuint
compile_shader(GLenum type, const char *src)
{
	LOGI("compile_shader: Creating shader type=%d", type);
	GLuint shader = glCreateShader(type);
	if (shader == 0) {
		LOGE("compile_shader: glCreateShader failed, error=0x%x", glGetError());
		return 0;
	}

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint log_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		LOGE("compile_shader: Compilation failed, log_len=%d", log_len);
		if (log_len > 0) {
			char *log = malloc(log_len);
			glGetShaderInfoLog(shader, log_len, NULL, log);
			LOGE("Shader compile error: %s", log);
			free(log);
		}
		glDeleteShader(shader);
		return 0;
	}
	LOGI("compile_shader: Success, shader=%u", shader);
	return shader;
}

/* Initialize OpenGL ES resources */
static int
init_gl_resources(void)
{
	/* Compile shaders */
	GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
	GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
	if (!vs || !fs) {
		LOGE("Failed to compile shaders");
		return 0;
	}

	/* Link program */
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vs);
	glAttachShader(shader_program, fs);
	glLinkProgram(shader_program);

	GLint linked;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint log_len = 0;
		glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &log_len);
		if (log_len > 0) {
			char *log = malloc(log_len);
			glGetProgramInfoLog(shader_program, log_len, NULL, log);
			LOGE("Program link error: %s", log);
			free(log);
		}
		glDeleteProgram(shader_program);
		shader_program = 0;
		glDeleteShader(vs);
		glDeleteShader(fs);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	/* Create buffers */
	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &texcoord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	/* Create texture */
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	LOGI("OpenGL ES resources initialized");
	return 1;
}

/*
 * attachscreen - Create the screen buffer
 * Called by devdraw.c to initialize the screen
 * Returns pointer to Memdata, or nil on failure
 */
Memdata*
attachscreen(Rectangle *r, ulong *chan, int *d, int *width, int *softscreen)
{
	LOGI("attachscreen: Initializing screen buffer");

	if (g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) {
		LOGE("attachscreen: EGL not initialized");
		return nil;
	}

	/* Get screen dimensions from EGL */
	EGLint w, h;
	eglQuerySurface(g_display, g_surface, EGL_WIDTH, &w);
	eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &h);
	screenwidth = w;
	screenheight = h;

	LOGI("attachscreen: Screen size %dx%d", screenwidth, screenheight);

	/* Allocate screen buffer (RGBA format) */
	screensize = screenwidth * screenheight * 4;
	screendata = malloc(screensize);
	if (screendata == NULL) {
		LOGE("attachscreen: Failed to allocate screen buffer");
		return nil;
	}

	/* Initialize to dark blue background */
	memset(screendata, 0x10, screensize);  /* Dark blue: 0x10, 0x10, 0x30 */

	/* Set up Memdata structure */
	/* base is a back-pointer to Memdata itself (for compaction) */
	screendata_struct.base = (uintptr*)&screendata_struct;
	screendata_struct.bdata = screendata;
	screendata_struct.ref = 1;
	screendata_struct.imref = 0;
	screendata_struct.allocd = 1;

	/* Initialize OpenGL ES resources if not already done */
	if (shader_program == 0) {
		if (!init_gl_resources()) {
			LOGE("attachscreen: Failed to initialize OpenGL ES resources");
			free(screendata);
			screendata = NULL;
			return nil;
		}
	}

	/* Return screen parameters */
	r->min.x = 0;
	r->min.y = 0;
	r->max.x = screenwidth;
	r->max.y = screenheight;
	*chan = XRGB32;  /* 32-bit RGBA */
	*d = 32;  /* Depth */
	*width = screenwidth * 4;  /* Bytes per row */
	*softscreen = 1;  /* Software rendering */

	LOGI("attachscreen: Screen buffer initialized successfully");
	return &screendata_struct;
}

/*
 * flushmemscreen - Flush screen rectangle to display
 * Called by devdraw.c when screen content changes
 */
void
flushmemscreen(Rectangle r)
{
	if (g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) {
		return;
	}

	if (screendata == NULL) {
		return;
	}

	/* Make context current if needed */
	if (eglGetCurrentContext() != g_context) {
		if (!eglMakeCurrent(g_display, g_surface, g_surface, g_context)) {
			LOGE("flushmemscreen: eglMakeCurrent failed");
			return;
		}
	}

	/* Clamp rectangle to screen bounds */
	if (r.min.x < 0) r.min.x = 0;
	if (r.min.y < 0) r.min.y = 0;
	if (r.max.x > screenwidth) r.max.x = screenwidth;
	if (r.max.y > screenheight) r.max.y = screenheight;

	/* Update texture from screen data */
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenwidth, screenheight,
	             0, GL_RGBA, GL_UNSIGNED_BYTE, screendata);

	/* Set viewport */
	glViewport(0, 0, screenwidth, screenheight);

	/* Use shader program */
	glUseProgram(shader_program);

	/* Set up vertex attributes */
	GLint pos_attr = glGetAttribLocation(shader_program, "a_position");
	glEnableVertexAttribArray(pos_attr);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

	GLint texcoord_attr = glGetAttribLocation(shader_program, "a_texcoord");
	glEnableVertexAttribArray(texcoord_attr);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

	/* Draw fullscreen quad */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	/* Swap buffers */
	eglSwapBuffers(g_display, g_surface);
}

/*
 * drawcursor - Draw cursor on screen (stub for Android)
 * TODO: Implement cursor overlay rendering
 */
void
drawcursor(Drawcursor *c)
{
	(void)c;
	/* Cursor drawing not yet implemented for Android */
}

/*
 * android_initdisplay - Create a Display structure for Android
 * Returns a Display* backed by the EGL surface, or nil on failure
 *
 * This creates a minimal Display that wraps the EGL surface without
 * requiring /dev/draw/new which doesn't exist on Android.
 */
Display*
android_initdisplay(void (*error)(Display*, char*))
{
	Display *disp;
	Image *image;
	void *q;
	EGLint w, h;

	LOGI("android_initdisplay: Starting");

	/* Allocate lock */
	q = libqlalloc();
	if(q == nil) {
		LOGE("android_initdisplay: libqlalloc failed");
		return nil;
	}
	LOGI("android_initdisplay: libqlalloc succeeded");

	/* Allocate Display structure */
	disp = malloc(sizeof(Display));
	if(disp == 0) {
		LOGE("android_initdisplay: malloc Display failed");
		libqlfree(q);
		return nil;
	}

	/* Allocate root image */
	image = malloc(sizeof(Image));
	if(image == 0) {
		LOGE("android_initdisplay: malloc Image failed");
		free(disp);
		libqlfree(q);
		return nil;
	}

	/* Initialize to zeros */
	memset(disp, 0, sizeof(Display));
	memset(image, 0, sizeof(Image));

	/* Check EGL is initialized */
	if(g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) {
		LOGE("android_initdisplay: EGL not initialized");
		free(image);
		free(disp);
		libqlfree(q);
		return nil;
	}

	/* Get screen dimensions from EGL */
	eglQuerySurface(g_display, g_surface, EGL_WIDTH, &w);
	eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &h);
	LOGI("android_initdisplay: EGL surface %dx%d", w, h);

	/* Set up root image */
	image->display = disp;
	image->id = 0;
	image->chan = XRGB32;
	image->depth = 32;
	image->repl = 1;
	image->r = Rect(0, 0, w, h);
	image->clipr = image->r;
	image->screen = nil;
	image->next = nil;

	/* Set up Display fields */
	disp->image = image;
	disp->local = 1;  /* Local display, minimal locking */
	disp->depth = 32;
	disp->chan = XRGB32;
	disp->error = error;
	disp->devdir = strdup("/dev");
	disp->windir = strdup("/dev");
	disp->bufsize = Displaybufsize;
	disp->bufp = disp->buf;
	disp->qlock = q;

	LOGI("android_initdisplay: Allocating color images");

	/* NOTE: Don't lock the display during initialization since disp->local = 1
	 * and allocimage will check disp->local before attempting to lock.
	 * This avoids a potential deadlock since we're in the middle of init. */

	/* Allocate standard colors - On Android, these may fail if draw device isn't ready
	 * We still return a valid display even if color allocation fails */
	disp->white = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DWhite);
	disp->black = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DBlack);
	disp->opaque = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DWhite);
	disp->transparent = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DBlack);

	if(disp->white == nil || disp->black == nil ||
	   disp->opaque == nil || disp->transparent == nil) {
		/* Color allocation failed, but display is still usable */
		LOGE("android_initdisplay: Color images not available (draw device may not be ready)");
		LOGE("android_initdisplay: Display will work but color convenience functions are limited");
		/* Don't fail - the display is still functional without pre-allocated colors */
	}

	LOGI("android_initdisplay: Display created %dx%d", w, h);
	return disp;
}
