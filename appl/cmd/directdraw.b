implement Directdraw;

include "sys.m";
	sys: Sys;
include "draw.m";
	draw: Draw;
	Display, Image, Screen, Rect, Point, Context: import draw;

Directdraw: module {
	init:	fn(ctxt: ref Draw->Context, argv: list of string);
};

init(ctxt: ref Draw->Context, argv: list of string)
{
	sys = load Sys Sys->PATH;
	draw = load Draw Draw->PATH;

	sys->print("Directdraw: Starting direct draw test\n");

	# Don't rely on ctxt parameter - it's always nil when run from schedmod()
	# Create our own Display using Display.allocate(nil)
	display := Display.allocate(nil);
	if(display == nil) {
		sys->fprint(sys->fildes(2), "Directdraw: Display.allocate failed: %r\n");
		return;
	}

	sys->print("Directdraw: Display allocated successfully\n");

	screen := display.image;
	if(screen == nil) {
		sys->fprint(sys->fildes(2), "Directdraw: screen image is nil\n");
		return;
	}

	sys->print("Directdraw: Screen rect is %s\n",
		sys->sprint("%d %d %d %d",
			screen.r.min.x, screen.r.min.y,
			screen.r.max.x, screen.r.max.y));

	# Create a red color
	red := display.color(Draw->Red);
	if(red == nil) {
		sys->fprint(sys->fildes(2), "Directdraw: color() failed for red\n");
		return;
	}

	sys->print("Directdraw: Red color created\n");

	# Draw a red rectangle covering the entire screen
	screen.draw(screen.r, red, nil, (0,0));

	sys->print("Directdraw: Drew red rectangle\n");

	# Flush to make it visible using Image.flush()
	screen.flush(Draw->Flushnow);

	sys->print("Directdraw: Flush called - screen should be red now\n");

	# Wait 2 seconds so user can see it
	sys->sleep(2000);

	# Now draw a green circle in the center
	green := display.color(Draw->Green);
	if(green != nil) {
		center := Point((screen.r.min.x + screen.r.max.x) / 2,
			       (screen.r.min.y + screen.r.max.y) / 2);
		radius := (screen.r.max.x - screen.r.min.x) / 4;

		# Draw a filled circle using fillellipse
		screen.fillellipse(center, radius, radius, green, (0,0));
		screen.flush(Draw->Flushnow);

		sys->print("Directdraw: Drew green circle\n");
		sys->sleep(2000);
	}

	# Finally draw a blue rectangle in the upper left
	blue := display.color(Draw->Blue);
	if(blue != nil) {
		blue_rect := Rect((100, 100), (300, 300));
		screen.draw(blue_rect, blue, nil, (0,0));
		screen.flush(Draw->Flushnow);

		sys->print("Directdraw: Drew blue rectangle\n");
		sys->sleep(2000);
	}

	sys->print("Directdraw: Test complete\n");
}
