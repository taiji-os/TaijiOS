/*
 * KRB WM Client Implementation
 *
 * This is a simplified WM client that creates a window and renders KRB content.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include "krbclient.h"

KrbClientContext*
krb_client_init(const char *window_title, int width, int height)
{
    KrbClientContext *ctx;

    ctx = mallocz(sizeof(KrbClientContext), 1);
    if (ctx == nil)
        return nil;

    /* Initialize display */
    ctx->display = initdisplay(nil, nil, nil);
    if (ctx->display == nil) {
        free(ctx);
        return nil;
    }

    /* Get screen */
    ctx->screen = ctx->display->image;
    if (ctx->screen == nil) {
        closedisplay(ctx->display);
        free(ctx);
        return nil;
    }

    /* Save title and dimensions */
    if (window_title != nil)
        ctx->title = strdup(window_title);
    ctx->width = width;
    ctx->height = height;

    /* Create window image */
    ctx->window = allocwindow(ctx->screen, Rect(0, 0, width, height), Refbackup);
    if (ctx->window == nil) {
        if (ctx->title != nil)
            free(ctx->title);
        closedisplay(ctx->display);
        free(ctx);
        return nil;
    }

    ctx->screenr = ctx->window->r;
    ctx->running = 1;

    return ctx;
}

int
krb_client_run(KrbClientContext *ctx, const char *krb_path)
{
    KrbFile *krb_file;
    KrbRuntime *runtime;
    KrbDrawContext *draw_ctx;

    if (ctx == nil || krb_path == nil)
        return -1;

    /* Load KRB file */
    krb_file = krb_load(krb_path);
    if (krb_file == nil) {
        fprint(2, "Failed to load KRB file: %s\n", krb_path);
        return -1;
    }

    ctx->krb_file = krb_file;

    /* Initialize runtime */
    runtime = krb_runtime_init(krb_file);
    if (runtime == nil) {
        krb_free(krb_file);
        return -1;
    }

    ctx->runtime = runtime;

    /* Initialize renderer */
    draw_ctx = krb_render_init(ctx->display, ctx->screen, ctx->window);
    if (draw_ctx == nil) {
        krb_runtime_cleanup(runtime);
        krb_free(krb_file);
        return -1;
    }

    ctx->draw_ctx = draw_ctx;
    draw_ctx->runtime = runtime;

    /* Calculate initial layout */
    krb_runtime_calculate_layout(runtime, ctx->width, ctx->height);

    /* Set window title if supported */
    if (ctx->title != nil) {
        /* TODO: Set window title via WM protocol */
    }

    return 0;
}

void
krb_client_render(KrbClientContext *ctx)
{
    if (ctx == nil || ctx->draw_ctx == nil || ctx->runtime == nil)
        return;

    /* Clear window */
    draw(ctx->window, ctx->window->r,
         ctx->display->white, nil, ZP);

    /* Render widget tree */
    if (ctx->runtime->root != nil) {
        krb_render_widget_tree(ctx->draw_ctx, ctx->runtime->root);
    }
}

void
krb_client_resize(KrbClientContext *ctx, int new_width, int new_height)
{
    if (ctx == nil || ctx->runtime == nil)
        return;

    ctx->width = new_width;
    ctx->height = new_height;

    /* Recalculate layout */
    krb_runtime_calculate_layout(ctx->runtime, new_width, new_height);

    /* Re-render */
    krb_client_render(ctx);
}

void
krb_client_handle_pointer(KrbClientContext *ctx, Point pos, int buttons)
{
    KrbWidget *widget;

    if (ctx == nil || ctx->runtime == nil)
        return;

    /* Update mouse state */
    ctx->mouse.pos = pos;
    ctx->mouse.buttons = buttons;

    /* Hit test */
    widget = krb_runtime_widget_at(ctx->runtime, pos);

    /* Handle hover */
    if (widget != ctx->hovered) {
        ctx->hovered = widget;
        /* TODO: Trigger hover events */
    }

    /* Handle click */
    if (buttons != 0 && ctx->mouse.buttons == 0) {
        /* Mouse down */
    } else if (buttons == 0 && ctx->mouse.buttons != 0) {
        /* Mouse up - potential click */
        if (widget != nil) {
            /* Trigger click event */
            krb_runtime_trigger_event(ctx->runtime, widget, "onClick", nil);
        }
    }

    /* Update and render */
    krb_client_render(ctx);
}

void
krb_client_handle_keyboard(KrbClientContext *ctx, int key)
{
    if (ctx == nil || ctx->runtime == nil)
        return;

    /* Route keyboard event to focused widget */
    if (ctx->focused != nil) {
        /* TODO: Trigger key press event */
        krb_runtime_trigger_event(ctx->runtime, ctx->focused, "onKeyPress", &key);
    }

    /* Update and render */
    krb_client_render(ctx);
}

void
krb_client_handle_control(KrbClientContext *ctx, const char *msg)
{
    if (ctx == nil || msg == nil)
        return;

    /* Parse control messages */
    if (strcmp(msg, "resize") == 0) {
        /* TODO: Get new dimensions */
        /* krb_client_resize(ctx, new_width, new_height); */
    } else if (strcmp(msg, "quit") == 0) {
        ctx->running = 0;
    } else if (strcmp(msg, "exit") == 0) {
        ctx->running = 0;
    }
}

void
krb_client_event_loop(KrbClientContext *ctx)
{
    Event e;

    if (ctx == nil)
        return;

    /* Initial render */
    krb_client_render(ctx);

    /* Event loop */
    while (ctx->running) {
        /* Wait for events */
        /* This is a simplified version - real implementation would use
           the proper event handling mechanisms */

        /* For now, just sleep and render */
        sleep(100);

        /* In a real implementation, we would:
           1. Use eread() to get events
           2. Dispatch to appropriate handlers
           3. Handle Ekeyboard for keyboard events
           4. Handle Emouse for mouse events
        */
    }
}

void
krb_client_cleanup(KrbClientContext *ctx)
{
    if (ctx == nil)
        return;

    /* Cleanup renderer */
    if (ctx->draw_ctx != nil)
        krb_render_cleanup(ctx->draw_ctx);

    /* Cleanup runtime */
    if (ctx->runtime != nil)
        krb_runtime_cleanup(ctx->runtime);

    /* Free KRB file */
    if (ctx->krb_file != nil)
        krb_free(ctx->krb_file);

    /* Free window */
    if (ctx->window != nil)
        freeimage(ctx->window);

    /* Free title */
    if (ctx->title != nil)
        free(ctx->title);

    /* Close display */
    if (ctx->display != nil)
        closedisplay(ctx->display);

    free(ctx);
}
