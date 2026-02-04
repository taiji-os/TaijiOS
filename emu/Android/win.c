/*
 * Android OpenGL ES window system for TaijiOS
 * Replaces X11 with OpenGL ES 3.0 for hardware-accelerated graphics
 */

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "dat.h"
#include "fns.h"
#include "error.h"
#include "draw.h"
#include "memdraw.h"

#define LOG_TAG "TaijiOS-WIN"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

/*
 * OpenGL ES context structure
 */
typedef struct {
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	ANativeWindow* window;
	int width;
	int height;
	GLuint framebuffer;
	GLuint texture;
	GLuint vao;
	GLuint vbo;
	uchar* screen_data;	/* Software framebuffer */
	int screen_dirty;
} GLESContext;

static GLESContext gles_ctx = {0};

/*
 * External references from deveia.c
 */
extern int32_t android_handle_input_event(struct android_app* app, AInputEvent* event);

/*
 * Initialize OpenGL ES
 */
int
win_init(struct android_app* app)
{
	EGLint attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 16,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	EGLint format, num_configs;
	EGLConfig config;

	/* Get display */
	gles_ctx.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(gles_ctx.display == EGL_NO_DISPLAY) {
		LOGE("eglGetDisplay failed");
		return -1;
	}

	/* Initialize EGL */
	if(!eglInitialize(gles_ctx.display, NULL, NULL)) {
		LOGE("eglInitialize failed");
		return -1;
	}

	/* Choose config */
	if(!eglChooseConfig(gles_ctx.display, attribs, &config, 1, &num_configs) || num_configs < 1) {
		LOGE("eglChooseConfig failed");
		return -1;
	}
	gles_ctx.config = config;

	/* Get native window format */
	if(!eglGetConfigAttrib(gles_ctx.display, config, EGL_NATIVE_VISUAL_ID, &format)) {
		LOGE("eglGetConfigAttrib failed");
		return -1;
	}

	/* Set window format */
	ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);
	gles_ctx.window = app->window;

	/* Create surface */
	gles_ctx.surface = eglCreateWindowSurface(gles_ctx.display, config, app->window, NULL);
	if(gles_ctx.surface == EGL_NO_SURFACE) {
		LOGE("eglCreateWindowSurface failed");
		return -1;
	}

	/* Create context */
	gles_ctx.context = eglCreateContext(gles_ctx.display, config, EGL_NO_CONTEXT, context_attribs);
	if(gles_ctx.context == EGL_NO_CONTEXT) {
		LOGE("eglCreateContext failed");
		return -1;
	}

	/* Make current */
	if(!eglMakeCurrent(gles_ctx.display, gles_ctx.surface, gles_ctx.surface, gles_ctx.context)) {
		LOGE("eglMakeCurrent failed");
		return -1;
	}

	/* Get window size */
	eglQuerySurface(gles_ctx.display, gles_ctx.surface, EGL_WIDTH, &gles_ctx.width);
	eglQuerySurface(gles_ctx.display, gles_ctx.surface, EGL_HEIGHT, &gles_ctx.height);

	LOGI("OpenGL ES initialized: %dx%d", gles_ctx.width, gles_ctx.height);

	/* Allocate software framebuffer */
	gles_ctx.screen_data = malloc(gles_ctx.width * gles_ctx.height * 4);
	if(gles_ctx.screen_data == nil) {
		LOGE("Failed to allocate framebuffer");
		return -1;
	}

	/* Set up GL resources for blitting */
	glGenVertexArrays(1, &gles_ctx.vao);
	glGenBuffers(1, &gles_ctx.vbo);
	glGenTextures(1, &gles_ctx.texture);
	glGenFramebuffers(1, &gles_ctx.framebuffer);

	/* Set up texture for framebuffer */
	glBindTexture(GL_TEXTURE_2D, gles_ctx.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gles_ctx.width, gles_ctx.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Set up VBO for fullscreen quad */
	float vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
	};
	glBindBuffer(GL_ARRAY_BUFFER, gles_ctx.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Set viewport */
	glViewport(0, 0, gles_ctx.width, gles_ctx.height);

	/* Clear screen */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(gles_ctx.display, gles_ctx.surface);

	return 0;
}

/*
 * Resize handler
 */
void
win_resize(int width, int height)
{
	if(width == gles_ctx.width && height == gles_ctx.height)
		return;

	gles_ctx.width = width;
	gles_ctx.height = height;

	/* Reallocate framebuffer */
	free(gles_ctx.screen_data);
	gles_ctx.screen_data = malloc(width * height * 4);

	/* Update texture */
	glBindTexture(GL_TEXTURE_2D, gles_ctx.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glViewport(0, 0, width, height);
	LOGI("Window resized: %dx%d", width, height);
}

/*
 * Blit software framebuffer to OpenGL ES texture
 */
static void
blit_to_texture(void)
{
	glBindTexture(GL_TEXTURE_2D, gles_ctx.texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, gles_ctx.width, gles_ctx.height,
		GL_RGBA, GL_UNSIGNED_BYTE, gles_ctx.screen_data);
}

/*
 * Draw fullscreen quad with texture
 */
static void
draw_quad(void)
{
	GLuint program;
	GLint pos_loc, tex_loc;

	/* Simple shader program */
	static const char* vs_src =
		"#version 300 es\n"
		"in vec2 pos;\n"
		"in vec2 texcoord;\n"
		"out vec2 v_texcoord;\n"
		"void main() {\n"
		"	gl_Position = vec4(pos, 0.0, 1.0);\n"
		"	v_texcoord = texcoord;\n"
		"}\n";

	static const char* fs_src =
		"#version 300 es\n"
		"precision mediump float;\n"
		"in vec2 v_texcoord;\n"
		"uniform sampler2D tex;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = texture(tex, v_texcoord);\n"
		"}\n";

	/* TODO: Compile shaders once at init */
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_src, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_src, NULL);
	glCompileShader(fs);

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glUseProgram(program);

	pos_loc = glGetAttribLocation(program, "pos");
	tex_loc = glGetAttribLocation(program, "texcoord");

	glBindVertexArray(gles_ctx.vao);
	glBindBuffer(GL_ARRAY_BUFFER, gles_ctx.vbo);
	glEnableVertexAttribArray(pos_loc);
	glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);
	glEnableVertexAttribArray(tex_loc);
	glVertexAttribPointer(tex_loc, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteProgram(program);
}

/*
 * Swap buffers (display)
 */
void
win_swap(void)
{
	if(gles_ctx.screen_dirty) {
		blit_to_texture();
		draw_quad();
		gles_ctx.screen_dirty = 0;
	}
	eglSwapBuffers(gles_ctx.display, gles_ctx.surface);
}

/*
 * Get screen memory for direct drawing
 */
Memimage*
win_attach(int color, int width, int height)
{
	Memimage *m;
	Rectangle r;

	r = Rect(0, 0, width, height);
	m = allocmemimage(r, color);
	if(m == nil)
		return nil;

	/* Attach software framebuffer */
	m->data->base = gles_ctx.screen_data;
	m->data->bdata = gles_ctx.screen_data;

	return m;
}

/*
 * Mark screen as dirty (needs redraw)
 */
void
win_flush(void)
{
	gles_ctx.screen_dirty = 1;
}

/*
 * Cleanup
 */
void
win_cleanup(void)
{
	if(gles_ctx.display != EGL_NO_DISPLAY) {
		eglMakeCurrent(gles_ctx.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if(gles_ctx.context != EGL_NO_CONTEXT)
			eglDestroyContext(gles_ctx.display, gles_ctx.context);

		if(gles_ctx.surface != EGL_NO_SURFACE)
			eglDestroySurface(gles_ctx.display, gles_ctx.surface);

		eglTerminate(gles_ctx.display);
	}

	if(gles_ctx.screen_data)
		free(gles_ctx.screen_data);

	memset(&gles_ctx, 0, sizeof(gles_ctx));
}
