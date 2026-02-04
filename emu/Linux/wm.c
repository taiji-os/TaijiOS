#include "dat.h"
#include "fns.h"

/* Stub implementations for wmcontext functions
 * These are only used on Android; Linux doesn't need them.
 */

void*
wmcontext_create(void* drawctxt)
{
	return nil;
}

void
wmcontext_ref(void* wm)
{
	/* No-op on Linux */
}

void
wmcontext_unref(void* wm)
{
	/* No-op on Linux */
}

void
wmcontext_close(void* wm)
{
	/* No-op on Linux */
}

void
wmcontext_set_active(void* wm)
{
	/* No-op on Linux */
}

void
wmcontext_clear_active(void)
{
	/* No-op on Linux */
}
