/*
 * KRB Window Manager Client
 *
 * Enables KRB applications to run as windows in the TaijiOS window manager.
 */

#ifndef KRB_CLIENT_H
#define KRB_CLIENT_H

#include <u.h>
#include <libc.h>
#include <draw.h>
#include "krb_runtime.h"
#include "krb_render.h"

/* Client context */
typedef struct {
    Display *display;
    Screen *screen;
    Image *window;            /* Window image */
    Rectangle screenr;        /* Screen rectangle */

    KrbRuntime *runtime;
    KrbDrawContext *draw_ctx;
    KrbFile *krb_file;

    char *title;
    int width;
    int height;

    /* Event state */
    int running;
    KrbWidget *hovered;
    KrbWidget *focused;
    struct {
        Point pos;
        int buttons;
    } mouse;
} KrbClientContext;

/*
 * Client lifecycle
 */

/* Initialize WM client connection */
KrbClientContext* krb_client_init(const char *window_title, int width, int height);

/* Load and run KRB file */
int krb_client_run(KrbClientContext *ctx, const char *krb_path);

/* Main event loop */
void krb_client_event_loop(KrbClientContext *ctx);

/* Cleanup */
void krb_client_cleanup(KrbClientContext *ctx);

/*
 * Event handlers
 */

/* Handle pointer (mouse) event */
void krb_client_handle_pointer(KrbClientContext *ctx, Point pos, int buttons);

/* Handle keyboard event */
void krb_client_handle_keyboard(KrbClientContext *ctx, int key);

/* Handle control message from WM */
void krb_client_handle_control(KrbClientContext *ctx, const char *msg);

/*
 * Rendering
 */

/* Render the entire UI */
void krb_client_render(KrbClientContext *ctx);

/* Handle window resize */
void krb_client_resize(KrbClientContext *ctx, int new_width, int new_height);

#endif /* KRB_CLIENT_H */
