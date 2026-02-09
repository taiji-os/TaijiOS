/*
 * Android Window Manager Context Implementation
 *
 * This file implements the Wmcontext layer that bridges Android input
 * events to the Tk widget system through channels (Queues).
 *
 * Channel Layout:
 * - kbd:   Android keyboard -> Tk widgets (keycodes)
 * - ptr:   Android touch -> Tk widgets (Pointer events)
 * - ctl:   WM -> Application (reshape, focus, etc.)
 * - wctl:  Application -> WM (reshape requests, etc.)
 * - images: Image exchange (for window content sharing)
 */

#include "dat.h"
#include "fns.h"
#include "error.h"
#include <cursor.h>

#include <android/log.h>
#define LOG_TAG "TaijiOS-WM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "wm.h"

/*
 * Global active wmcontext
 * Set when a window gains focus
 */
Wmcontext* g_active_wmcontext = nil;

/*
 * Forward declarations
 */
static void wmcontext_freeclose(Wmcontext* wm);

/*
 * Get current timestamp in milliseconds
 * Used for Pointer events
 */
int
wmcontext_msec(void)
{
	/* TODO: Use Android's clock_gettime(CLOCK_MONOTONIC) */
	/* For now, return 0 */
	return 0;
}

/*
 * Create a new wmcontext
 * Called from Dis VM when creating Draw->Wmcontext
 */
Wmcontext*
wmcontext_create(void* drawctxt)
{
	Wmcontext* wm;

	wm = mallocz(sizeof(Wmcontext), 1);
	if(wm == nil) {
		LOGE("wmcontext_create: malloc failed");
		return nil;
	}

	/* Initialize reference counting */
	wm->r.ref = 1;
	wm->refcount = 1;
	wm->closed = 0;
	wm->active = 0;
	wm->drawctxt = drawctxt;
	wm->windows = nil;

	/* Create channels (Queues) */
	/* Queue size: 256 events should be sufficient */
	/* Qmsg for message queue, nil for notify and aux */
	wm->kbd = qopen(256, Qmsg, nil, nil);
	if(wm->kbd == nil)
		goto error;

	wm->ptr = qopen(256, Qmsg, nil, nil);
	if(wm->ptr == nil)
		goto error;

	wm->ctl = qopen(256, Qmsg, nil, nil);
	if(wm->ctl == nil)
		goto error;

	wm->wctl = qopen(256, Qmsg, nil, nil);
	if(wm->wctl == nil)
		goto error;

	wm->images = qopen(64, Qmsg, nil, nil);
	if(wm->images == nil)
		goto error;

	LOGI("wmcontext_create: Created wmcontext %p", wm);
	return wm;

error:
	LOGE("wmcontext_create: Failed to allocate queues");
	wmcontext_freeclose(wm);
	free(wm);
	return nil;
}

/*
 * Close and free all resources
 * Called when reference count reaches zero
 */
static void
wmcontext_freeclose(Wmcontext* wm)
{
	if(wm == nil)
		return;

	LOGI("wmcontext_freeclose: Closing wmcontext %p", wm);

	if(wm->kbd != nil) {
		qclose(wm->kbd);
		wm->kbd = nil;
	}
	if(wm->ptr != nil) {
		qclose(wm->ptr);
		wm->ptr = nil;
	}
	if(wm->ctl != nil) {
		qclose(wm->ctl);
		wm->ctl = nil;
	}
	if(wm->wctl != nil) {
		qclose(wm->wctl);
		wm->wctl = nil;
	}
	if(wm->images != nil) {
		qclose(wm->images);
		wm->images = nil;
	}

	wm->closed = 1;
}

/*
 * Increment reference count
 */
void
wmcontext_ref(Wmcontext* wm)
{
	if(wm == nil)
		return;
	lock(&wm->lk);
	wm->refcount++;
	unlock(&wm->lk);
}

/*
 * Decrement reference count, free if reaches zero
 */
void
wmcontext_unref(Wmcontext* wm)
{
	int ref;

	if(wm == nil)
		return;

	lock(&wm->lk);
	ref = --wm->refcount;
	unlock(&wm->lk);

	if(ref <= 0) {
		LOGI("wmcontext_unref: Freeing wmcontext %p", wm);
		wmcontext_freeclose(wm);
		/* If this was active, clear it */
		if(g_active_wmcontext == wm) {
			g_active_wmcontext = nil;
		}
		free(wm);
	}
}

/*
 * Close all channels and mark as closed
 */
void
wmcontext_close(Wmcontext* wm)
{
	if(wm == nil)
		return;

	lock(&wm->lk);
	if(!wm->closed) {
		wm->closed = 1;
		/* Wake up any readers by sending nil */
		qwrite(wm->kbd, nil, 0);
		qwrite(wm->ptr, nil, 0);
		qwrite(wm->ctl, nil, 0);
		qwrite(wm->wctl, nil, 0);
		qwrite(wm->images, nil, 0);
	}
	unlock(&wm->lk);
}

/*
 * Send keyboard event to kbd channel
 * Called from input thread (deveia.c)
 */
void
wmcontext_send_kbd(Wmcontext* wm, int key)
{
	if(wm == nil || wm->closed)
		return;

	if(wm->kbd != nil) {
		/* Use qiwrite for input events - non-blocking, doesn't sleep */
		qiwrite(wm->kbd, (char*)&key, sizeof(key));
	}
}

/*
 * Send pointer event to ptr channel
 * Called from input thread (deveia.c)
 */
void
wmcontext_send_ptr(Wmcontext* wm, int buttons, int x, int y)
{
	WmPointer ptr;

	if(wm == nil || wm->closed)
		return;

	ptr.buttons = buttons;
	ptr.x = x;
	ptr.y = y;
	ptr.msec = wmcontext_msec();

	if(wm->ptr != nil) {
		/* Use qiwrite for input events - non-blocking, doesn't sleep */
		qiwrite(wm->ptr, (char*)&ptr, sizeof(ptr));
	}
}

/*
 * Send control message to ctl channel (WM -> app)
 */
void
wmcontext_send_ctl(Wmcontext* wm, const char* msg)
{
	int len;

	if(wm == nil || wm->closed || msg == nil)
		return;

	len = strlen(msg) + 1;
	if(wm->ctl != nil) {
		qwrite(wm->ctl, msg, len);
	}
}

/*
 * Receive keyboard event from kbd channel
 * Returns 1 if event received, 0 if queue empty/closed
 */
int
wmcontext_recv_kbd(Wmcontext* wm, int* key_out)
{
	int n;

	if(wm == nil || wm->closed || key_out == nil)
		return 0;

	if(wm->kbd == nil)
		return 0;

	n = qread(wm->kbd, (char*)key_out, sizeof(int));
	if(n != sizeof(int))
		return 0;

	return 1;
}

/*
 * Receive pointer event from ptr channel
 * Returns allocated WmPointer* or nil if queue empty/closed
 * Caller must free the returned pointer
 */
WmPointer*
wmcontext_recv_ptr(Wmcontext* wm)
{
	WmPointer* ptr;
	int n;

	if(wm == nil || wm->closed)
		return nil;

	if(wm->ptr == nil)
		return nil;

	ptr = mallocz(sizeof(WmPointer), 1);
	if(ptr == nil)
		return nil;

	n = qread(wm->ptr, (char*)ptr, sizeof(WmPointer));
	if(n != sizeof(WmPointer)) {
		free(ptr);
		return nil;
	}

	return ptr;
}

/*
 * Receive control message from ctl channel
 * Returns allocated string or nil if queue empty/closed
 * Caller must free the returned string
 */
char*
wmcontext_recv_ctl(Wmcontext* wm)
{
	char buf[256];
	int n;

	if(wm == nil || wm->closed)
		return nil;

	if(wm->ctl == nil)
		return nil;

	n = qread(wm->ctl, buf, sizeof(buf) - 1);
	if(n <= 0)
		return nil;

	buf[n] = '\0';
	return strdup(buf);
}

/*
 * Send wctl request (app -> WM)
 */
void
wmcontext_send_wctl(Wmcontext* wm, const char* request)
{
	int len;

	if(wm == nil || wm->closed || request == nil)
		return;

	len = strlen(request) + 1;
	if(wm->wctl != nil) {
		qwrite(wm->wctl, request, len);
	}
}

/*
 * Receive wctl response (WM -> app)
 * Returns allocated string or nil if queue empty
 */
char*
wmcontext_recv_wctl(Wmcontext* wm)
{
	char buf[256];
	int n;

	if(wm == nil || wm->closed)
		return nil;

	if(wm->wctl == nil)
		return nil;

	n = qread(wm->wctl, buf, sizeof(buf) - 1);
	if(n <= 0)
		return nil;

	buf[n] = '\0';
	return strdup(buf);
}

/*
 * Process wctl request and send response via ctl channel
 * This is called by the WM thread to handle reshape, move, etc.
 *
 * For Android, we act as both WM and app, so this is simpler
 */
void
wmcontext_process_wctl(Wmcontext* wm)
{
	char* request;
	char response[256];

	if(wm == nil || wm->closed)
		return;

	/* Read request from wctl channel */
	request = wmcontext_recv_wctl(wm);
	if(request == nil)
		return;

	LOGI("wmcontext_process_wctl: Request: %s", request);

	/* Parse request and send response via ctl */
	/* Common requests:
	 * - "reshape name x y w h"
	 * - "move name x y"
	 * - "size name w h"
	 */

	/* For now, just acknowledge */
	snprint(response, sizeof(response), "ok");
	wmcontext_send_ctl(wm, response);

	free(request);
}

/*
 * Set as active context (receives input events)
 */
void
wmcontext_set_active(Wmcontext* wm)
{
	if(g_active_wmcontext == wm)
		return;

	LOGI("wmcontext_set_active: Setting %p as active", wm);
	g_active_wmcontext = wm;
}

/*
 * Get active context
 */
Wmcontext*
wmcontext_get_active(void)
{
	return g_active_wmcontext;
}

/*
 * Clear active context
 */
void
wmcontext_clear_active(void)
{
	LOGI("wmcontext_clear_active: Clearing active context");
	g_active_wmcontext = nil;
}

/*
 * Check if context is valid
 */
int
wmcontext_is_valid(Wmcontext* wm)
{
	return (wm != nil && !wm->closed);
}

/*
 * Initialize WM subsystem (called at startup)
 * Creates a default wmcontext for Android so that /dev/wmctx-* devices work
 */
void
wm_init(void)
{
	Wmcontext* wm;

	LOGI("wm_init: ENTRY - Initializing Window Manager subsystem");

	/* Create a default wmcontext for Android */
	wm = wmcontext_create(nil);
	if(wm == nil) {
		__android_log_print(ANDROID_LOG_ERROR, "TaijiOS", "wm_init: FAILED to create wmcontext");
		return;
	}

	/* Set it as active so input events are routed to it */
	wmcontext_set_active(wm);

	__android_log_print(ANDROID_LOG_INFO, "TaijiOS", "wm_init: SUCCESS wmcontext %p created", wm);
	LOGI("wm_init: Default wmcontext %p created and set as active, queues: kbd=%p ptr=%p ctl=%p",
	     wm, wm->kbd, wm->ptr, wm->ctl);
}

/*
 * Cleanup WM subsystem (called at shutdown)
 */
void
wm_shutdown(void)
{
	LOGI("wm_shutdown: Window Manager subsystem shutting down");
	g_active_wmcontext = nil;
}

/*
 * External reference to screenimage from devdraw.c
 * This is the Memimage* that holds the actual screen buffer
 * When Tk draws, it modifies screenimage->data->bdata directly
 */
extern Memimage* screenimage;

/*
 * External function from win.c to get screen buffer
 */
extern void win_get_screendata(uchar **data, int *width, int *height);

/*
 * Process and display images from wmcontext
 * This should be called regularly from the main event loop
 * Returns 1 if an image was displayed, 0 otherwise
 *
 * Note: The images in the queue are Image* pointers from Tk drawing operations.
 * The actual pixel data has already been written to screenimage->data->bdata
 * by the memdraw operations. We just need to signal that a refresh is needed.
 */
int
wmcontext_update_display(Wmcontext* wm)
{
	Image* img;
	int display_updated = 0;

	if(wm == nil || wm->closed || wm->images == nil)
		return 0;

	/* Try to read an image from the queue without blocking */
	while(qcanread(wm->images)) {
		Image* img_ptr;
		long n = qread(wm->images, (char*)&img_ptr, sizeof(Image*));
		if(n == sizeof(Image*) && img_ptr != nil) {
			LOGI("wmcontext_update_display: Received image %p", img_ptr);
			LOGI("  Image rect: (%d,%d)-(%d,%d)",
			     img_ptr->r.min.x, img_ptr->r.min.y,
			     img_ptr->r.max.x, img_ptr->r.max.y);
			LOGI("  Image depth=%d, chan=0x%x", img_ptr->depth, img_ptr->chan);

			/*
			 * The drawing has already been done to screenimage by memdraw.
			 * We just need to indicate that we have content to display.
			 * The win_swap() function will call flushmemscreen() to render.
			 */
			display_updated = 1;
		}
	}

	return display_updated;
}

/*
 * Update display from active wmcontext
 * Convenience function to call from main loop
 */
int
wm_update_active_display(void)
{
	if(g_active_wmcontext != nil) {
		return wmcontext_update_display(g_active_wmcontext);
	}
	return 0;
}

/*
 * ============================================================================
 * Window Management - Compositing wmclient windows to screen
 * ============================================================================
 */

/*
 * External reference to flushmemscreen from win.c
 */
extern void flushmemscreen(Rectangle r);

/*
 * Global window ID counter
 */
static ulong g_next_window_id = 1;

/*
 * Register a wmclient window with the wmcontext
 * Called when wmclient creates a new window
 * Returns window ID on success, -1 on failure
 */
int
wmcontext_register_window(Wmcontext* wm, Memimage* winimg, Rectangle r)
{
	WmWindow* win;
	ulong winid;

	if(wm == nil || winimg == nil) {
		LOGE("wmcontext_register_window: nil arguments");
		return -1;
	}

	lock(&wm->lk);

	if(wm->closed) {
		LOGE("wmcontext_register_window: wmcontext is closed");
		unlock(&wm->lk);
		return -1;
	}

	/* Allocate new window structure */
	win = mallocz(sizeof(WmWindow), 1);
	if(win == nil) {
		LOGE("wmcontext_register_window: malloc failed");
		unlock(&wm->lk);
		return -1;
	}

	/* Assign window ID */
	winid = g_next_window_id++;
	win->id = winid;
	win->image = winimg;
	win->screenimg = screenimage;
	win->r = r;
	win->visible = 1;
	win->zorder = (int)winid;  /* Use ID as initial z-order */
	win->next = nil;

	/* Add to front of window list */
	win->next = (WmWindow*)wm->windows;
	wm->windows = win;

	LOGI("wmcontext_register_window: Registered window %lu at (%d,%d)-(%d,%d)",
	     winid, r.min.x, r.min.y, r.max.x, r.max.y);

	unlock(&wm->lk);
	return (int)winid;
}

/*
 * Unregister a window from the wmcontext
 */
void
wmcontext_unregister_window(Wmcontext* wm, int winid)
{
	WmWindow *win, *prev;

	if(wm == nil || winid <= 0)
		return;

	lock(&wm->lk);

	prev = nil;
	win = (WmWindow*)wm->windows;

	while(win != nil) {
		if(win->id == (ulong)winid) {
			/* Remove from list */
			if(prev == nil)
				wm->windows = win->next;
			else
				prev->next = win->next;

			LOGI("wmcontext_unregister_window: Unregistered window %lu", win->id);
			free(win);
			break;
		}
		prev = win;
		win = win->next;
	}

	unlock(&wm->lk);
}

/*
 * Mark a window region as dirty (needs redraw)
 * Called when application draws to its window
 */
void
wmcontext_mark_dirty(Wmcontext* wm, int winid, Rectangle r)
{
	WmWindow* win;

	USED(winid);
	(void)r;  /* Can't use USED with non-scalar types */

	if(wm == nil)
		return;

	/* For now, we'll composite all windows on each flush */
	/* This could be optimized to only composite the dirty region */
	LOGI("wmcontext_mark_dirty: window %d region (%d,%d)-(%d,%d)",
	     winid, r.min.x, r.min.y, r.max.x, r.max.y);
}

/*
 * Get screenimage for compositing
 * Returns the global screenimage from devdraw.c
 */
Memimage*
wmcontext_get_screenimage(void)
{
	return screenimage;
}

/*
 * Flush screen after compositing
 * Triggers flushmemscreen() with the composited region
 */
void
wmcontext_flush_screen(Rectangle r)
{
	flushmemscreen(r);
}

/*
 * ============================================================================
 * Window Notification - Called by devdraw.c when wmclient windows are created
 * ============================================================================
 */

/*
 * Track wmclient windows globally for compositing
 * This is a simple linked list of all wmclient window layers
 */
typedef struct WmClientWindow WmClientWindow;

struct WmClientWindow {
	Memimage*		layerimg;	/* The layer image (what wmclient draws to) */
	Rectangle		screenr;	/* Position on screen */
	int				visible;	/* Visibility flag */
	WmClientWindow*	next;
};

static WmClientWindow* g_wmclient_windows = nil;
static Lock g_wmclient_windows_lock = { 0 };

/*
 * Called by devdraw.c when a wmclient window (layer) is created
 * This allows the Android WM to track wmclient windows for compositing
 */
void
wmcontext_notify_window_created(Memimage* layerimg, Rectangle screenr)
{
	WmClientWindow* wcw;

	if(layerimg == nil)
		return;

	lock(&g_wmclient_windows_lock);

	/* Check if already tracked */
	for(wcw = g_wmclient_windows; wcw != nil; wcw = wcw->next) {
		if(wcw->layerimg == layerimg) {
			/* Update existing entry */
			wcw->screenr = screenr;
			wcw->visible = 1;
			unlock(&g_wmclient_windows_lock);
			LOGI("wmcontext_notify_window_created: Updated existing window %p at (%d,%d)-(%d,%d)",
			     layerimg, screenr.min.x, screenr.min.y, screenr.max.x, screenr.max.y);
			return;
		}
	}

	/* Create new entry */
	wcw = mallocz(sizeof(WmClientWindow), 1);
	if(wcw == nil) {
		unlock(&g_wmclient_windows_lock);
		LOGE("wmcontext_notify_window_created: malloc failed");
		return;
	}

	wcw->layerimg = layerimg;
	wcw->screenr = screenr;
	wcw->visible = 1;
	wcw->next = g_wmclient_windows;
	g_wmclient_windows = wcw;

	LOGI("wmcontext_notify_window_created: Registered window %p at (%d,%d)-(%d,%d)",
	     layerimg, screenr.min.x, screenr.min.y, screenr.max.x, screenr.max.y);

	unlock(&g_wmclient_windows_lock);
}

/*
 * Called by devdraw.c when a wmclient window is destroyed
 */
void
wmcontext_notify_window_destroyed(Memimage* layerimg)
{
	WmClientWindow *wcw, *prev;

	if(layerimg == nil)
		return;

	lock(&g_wmclient_windows_lock);

	prev = nil;
	wcw = g_wmclient_windows;

	while(wcw != nil) {
		if(wcw->layerimg == layerimg) {
			/* Remove from list */
			if(prev == nil)
				g_wmclient_windows = wcw->next;
			else
				prev->next = wcw->next;

			LOGI("wmcontext_notify_window_destroyed: Unregistered window %p", layerimg);
			free(wcw);
			break;
		}
		prev = wcw;
		wcw = wcw->next;
	}

	unlock(&g_wmclient_windows_lock);
}

/*
 * Update wmcontext_composite_windows to also composite wmclient windows
 * This replaces the previous version to handle both WmWindow and WmClientWindow
 */
void
wmcontext_composite_windows(Wmcontext* wm)
{
	WmClientWindow* wcw;
	Rectangle screenr;
	int composite_count = 0;

	/* Composite WmWindow registered via wmcontext_register_window */
	if(wm != nil && wm->windows != nil) {
		WmWindow* win;
		lock(&wm->lk);

		if(screenimage == nil) {
			unlock(&wm->lk);
			goto composite_wmclient;
		}
		screenr = screenimage->r;

		/* Composite each visible WmWindow */
		for(win = (WmWindow*)wm->windows; win != nil; win = win->next) {
			if(win->visible && win->image != nil) {
				Rectangle srcr, dstr;

				/* Clip window rectangle to screen bounds */
				srcr = win->image->r;
				dstr = win->r;

				if(!rectclip(&dstr, screenr))
					continue;

				/* Clip source rectangle accordingly */
				srcr.min.x += dstr.min.x - win->r.min.x;
				srcr.min.y += dstr.min.y - win->r.min.y;
				srcr.max.x += dstr.max.x - win->r.max.x;
				srcr.max.y += dstr.max.y - win->r.max.y;

				/* Composite using memimagedraw */
				if(srcr.min.x < srcr.max.x && srcr.min.y < srcr.max.y) {
					LOGI("wmcontext_composite_windows: Compositing WmWindow %lu from (%d,%d)-(%d,%d) to (%d,%d)-(%d,%d)",
					     win->id,
					     srcr.min.x, srcr.min.y, srcr.max.x, srcr.max.y,
					     dstr.min.x, dstr.min.y, dstr.max.x, dstr.max.y);

					/* Use memimagedraw to composite window to screen */
					memimagedraw(screenimage, dstr, win->image, srcr.min, nil, ZP, SoverD);
					composite_count++;
				}
			}
		}

		unlock(&wm->lk);
	}

composite_wmclient:
	/* Now composite all wmclient windows (layers) */
	if(g_wmclient_windows == nil)
		goto done;

	lock(&g_wmclient_windows_lock);

	/* Get screen bounds */
	if(screenimage == nil) {
		unlock(&g_wmclient_windows_lock);
		goto done;
	}
	screenr = screenimage->r;

	/* Composite each visible wmclient window */
	for(wcw = g_wmclient_windows; wcw != nil; wcw = wcw->next) {
		if(wcw->visible && wcw->layerimg != nil) {
			Memimage* srcimg;
			Rectangle srcr, dstr;
			Point srcpt;

			/*
			 * For a layer, we need to get the actual image data.
			 * The layerimg is the layer wrapper, we need to extract
			 * the actual image content.
			 */
			srcimg = wcw->layerimg;  /* Use the layer image directly */
			srcpt = ZP;
			dstr = wcw->screenr;

			/* Clip to screen bounds */
			if(!rectclip(&dstr, screenr))
				continue;

			/* Source rectangle is the window's content */
			srcr = wcw->layerimg->r;

			/* Adjust source point based on clipping */
			srcpt.x = srcr.min.x + (dstr.min.x - wcw->screenr.min.x);
			srcpt.y = srcr.min.y + (dstr.min.y - wcw->screenr.min.y);

			/* Composite using memimagedraw */
			/* We use SoverD to composite the window on top of existing content */
			memimagedraw(screenimage, dstr, wcw->layerimg, srcpt, nil, ZP, SoverD);
			composite_count++;

			static int log_count = 0;
			if(log_count < 3 || (log_count % 60) == 0) {
				LOGI("wmcontext_composite_windows: Composited wmclient window %p from (%d,%d) to (%d,%d)-(%d,%d)",
				     wcw->layerimg, srcpt.x, srcpt.y,
				     dstr.min.x, dstr.min.y, dstr.max.x, dstr.max.y);
			}
			log_count++;
		}
	}

	unlock(&g_wmclient_windows_lock);

done:
	if(composite_count > 0) {
		static int total_log_count = 0;
		if(total_log_count < 3 || (total_log_count % 60) == 0) {
			LOGI("wmcontext_composite_windows: Composited %d windows total", composite_count);
		}
		total_log_count++;
	}
}
