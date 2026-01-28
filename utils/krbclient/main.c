/*
 * KRB Client Main Entry Point
 *
 * This program loads and runs KRB files as WM client applications.
 */

#include <u.h>
#include <libc.h>
#include <stdlib.h>
#include <string.h>
#include "krbclient.h"

void
usage(void)
{
    fprint(2, "usage: krbclient [-t title] [-W width] [-H height] file.krb\n");
    exits("usage");
}

void
main(int argc, char *argv[])
{
    KrbClientContext *ctx;
    char *krb_path;
    char *title;
    int width, height;
    int i;

    title = nil;
    width = 800;
    height = 600;

    ARGBEGIN {
    case 't':
        title = ARGF();
        break;
    case 'W':
        width = atoi(ARGF());
        break;
    case 'H':
        height = atoi(ARGF());
        break;
    default:
        usage();
    } ARGEND;

    if (argc < 1)
        usage();

    krb_path = argv[0];

    /* Use filename as title if not specified */
    if (title == nil) {
        /* Extract basename from path */
        char *slash = strrchr(krb_path, '/');
        if (slash != nil)
            title = slash + 1;
        else
            title = krb_path;
    }

    /* Initialize client */
    ctx = krb_client_init(title, width, height);
    if (ctx == nil) {
        sysfatal("krb_client_init: %r");
    }

    /* Load and run KRB file */
    if (krb_client_run(ctx, krb_path) < 0) {
        krb_client_cleanup(ctx);
        sysfatal("krb_client_run: %r");
    }

    /* Run event loop */
    krb_client_event_loop(ctx);

    /* Cleanup */
    krb_client_cleanup(ctx);

    exits(nil);
}
