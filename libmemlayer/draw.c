#include "lib9.h"
#include "draw.h"
#include "memdraw.h"
#include "memlayer.h"
#include "pool.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

struct Draw
{
	Point	deltas;
	Point	deltam;
	Memlayer		*dstlayer;
	Memimage	*src;
	Memimage	*mask;
	s32	op;
};

static
void
ldrawop(Memimage *dst, Rectangle screenr, Rectangle clipr, void *etc, int insave)
{
	struct Draw *d;
	Point p0, p1;
	Rectangle oclipr, srcr, r, mr;
	int ok;

	d = etc;
	if(insave && d->dstlayer->save==nil)
		return;

	p0 = addpt(screenr.min, d->deltas);
	p1 = addpt(screenr.min, d->deltam);

	if(insave){
		r = rectsubpt(screenr, d->dstlayer->delta);
		clipr = rectsubpt(clipr, d->dstlayer->delta);
	}else
		r = screenr;

	/* now in logical coordinates */

	/* clipr may have narrowed what we should draw on, so clip if necessary */
	if(!rectinrect(r, clipr)){
		oclipr = dst->clipr;
		dst->clipr = clipr;
		ok = drawclip(dst, &r, d->src, &p0, d->mask, &p1, &srcr, &mr);
		dst->clipr = oclipr;
		if(!ok)
			return;
	}
	memdraw(dst, r, d->src, p0, d->mask, p1, d->op);
}

void
memdraw(Memimage *dst, Rectangle r, Memimage *src, Point p0, Memimage *mask, Point p1, int op)
{
	struct Draw d;
	Rectangle srcr, tr, mr;
	Memlayer *dl, *sl;

#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
		"memdraw: ENTRY dst=%p, r=(%d,%d)-(%d,%d), src=%p, p0=(%d,%d), mask=%p, p1=(%d,%d), op=%d",
		dst, r.min.x, r.min.y, r.max.x, r.max.y, src, p0.x, p0.y, mask, p1.x, p1.y, op);
#endif

	if(drawdebug)
		iprint("memdraw %p %R %p %P %p %P\n", dst, r, src, p0, mask, p1);

	if(mask == nil)
		mask = memopaque;

	if(mask->layer){
if(drawdebug)	iprint("mask->layer != nil\n");
		return;	/* too hard, at least for now */
	}

#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
		"memdraw: dst->layer=%p, src->layer=%p, mask->layer=%p",
		dst->layer, src->layer, mask->layer);
#endif

    Top:
	if(dst->layer==nil && src->layer==nil){
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: calling memimagedraw (no layers)");
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: dst data=%p, src data=%p, dst->data=%p, src->data=%p",
			(dst->data ? dst->data->bdata : 0), (src->data ? src->data->bdata : 0), dst->data, src->data);
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: dst chan=0x%x, src chan=0x%x", dst->chan, src->chan);
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: dst->r=(%d,%d)-(%d,%d), dst->clipr=(%d,%d)-(%d,%d)",
			dst->r.min.x, dst->r.min.y, dst->r.max.x, dst->r.max.y,
			dst->clipr.min.x, dst->clipr.min.y, dst->clipr.max.x, dst->clipr.max.y);
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: src->r=(%d,%d)-(%d,%d), src->clipr=(%d,%d)-(%d,%d)",
			src->r.min.x, src->r.min.y, src->r.max.x, src->r.max.y,
			src->clipr.min.x, src->clipr.min.y, src->clipr.max.x, src->clipr.max.y);
#endif
		memimagedraw(dst, r, src, p0, mask, p1, op);
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
			"memdraw: memimagedraw returned");
		/* Sample center pixel to see if it changed */
		if(dst->data && dst->data->bdata && dst->width > 0) {
			int center_offset = (1140 * dst->width + 540) * 4;
			uchar *pixel = (uchar*)dst->data->bdata + dst->zero + center_offset;
			__android_log_print(ANDROID_LOG_INFO, "TaijiOS-memdraw",
				"memdraw: center pixel B,G,R,A = [%d,%d,%d,%d]",
				pixel[0], pixel[1], pixel[2], pixel[3]);
		}
#endif
		return;
	}

	if(drawclip(dst, &r, src, &p0, mask, &p1, &srcr, &mr) == 0){
if(drawdebug)	iprint("drawclip dstcr %R srccr %R maskcr %R\n", dst->clipr, src->clipr, mask->clipr);
		return;
	}

	/*
 	 * Convert to screen coordinates.
	 */
	dl = dst->layer;
	if(dl != nil){
		r.min.x += dl->delta.x;
		r.min.y += dl->delta.y;
		r.max.x += dl->delta.x;
		r.max.y += dl->delta.y;
	}
    Clearlayer:
	if(dl!=nil && dl->clear){
		if(src == dst){
			p0.x += dl->delta.x;
			p0.y += dl->delta.y;
			src = dl->screen->image;
		}
		dst = dl->screen->image;
		goto Top;
	}

	sl = src->layer;
	if(sl != nil){
		p0.x += sl->delta.x;
		p0.y += sl->delta.y;
		srcr.min.x += sl->delta.x;
		srcr.min.y += sl->delta.y;
		srcr.max.x += sl->delta.x;
		srcr.max.y += sl->delta.y;
	}

	/*
	 * Now everything is in screen coordinates.
	 * mask is an image.  dst and src are images or obscured layers.
	 */

	/*
	 * if dst and src are the same layer, just draw in save area and expose.
	 */
	if(dl!=nil && dst==src){
		if(dl->save == nil)
			return;	/* refresh function makes this case unworkable */
		if(rectXrect(r, srcr)){
			tr = r;
			if(srcr.min.x < tr.min.x){
				p1.x += tr.min.x - srcr.min.x;
				tr.min.x = srcr.min.x;
			}
			if(srcr.min.y < tr.min.y){
				p1.y += tr.min.x - srcr.min.x;
				tr.min.y = srcr.min.y;
			}
			if(srcr.max.x > tr.max.x)
				tr.max.x = srcr.max.x;
			if(srcr.max.y > tr.max.y)
				tr.max.y = srcr.max.y;
			memlhide(dst, tr);
		}else{
			memlhide(dst, r);
			memlhide(dst, srcr);
		}
		memdraw(dl->save, rectsubpt(r, dl->delta), dl->save,
			subpt(srcr.min, src->layer->delta), mask, p1, op);
		memlexpose(dst, r);
		return;
	}

	if(sl){
		if(sl->clear){
			src = sl->screen->image;
			if(dl != nil){
				r.min.x -= dl->delta.x;
				r.min.y -= dl->delta.y;
				r.max.x -= dl->delta.x;
				r.max.y -= dl->delta.y;
			}
			goto Top;
		}
		/* relatively rare case; use save area */
		if(sl->save == nil)
			return;	/* refresh function makes this case unworkable */
		memlhide(src, srcr);
		/* convert back to logical coordinates */
		p0.x -= sl->delta.x;
		p0.y -= sl->delta.y;
		srcr.min.x -= sl->delta.x;
		srcr.min.y -= sl->delta.y;
		srcr.max.x -= sl->delta.x;
		srcr.max.y -= sl->delta.y;
		src = src->layer->save;
	}

	/*
	 * src is now an image.  dst may be an image or a clear layer
	 */
	if(dst->layer==nil)
		goto Top;
	if(dst->layer->clear)
		goto Clearlayer;

	/*
	 * dst is an obscured layer
	 */
	d.deltas = subpt(p0, r.min);
	d.deltam = subpt(p1, r.min);
	d.dstlayer = dl;
	d.src = src;
	d.mask = mask;
	d.op = op;
	_memlayerop(ldrawop, dst, r, r, &d);
}
