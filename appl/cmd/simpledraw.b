implement Simpledraw;

#
# Simple test program that bypasses wmclient
# Tests Display.allocate(nil) and direct drawing to screen
#

include "sys.m";
	sys: Sys;
include "draw.m";
	draw: Draw;
	Display, Image, Rect, Point: import draw;

Simpledraw: module
{
	init:	fn(nil: ref Draw->Context, argv: list of string);
};

init(nil: ref Draw->Context, argv: list of string)
{
	sys = load Sys Sys->PATH;
	draw = load Draw Draw->PATH;

	sys->print("simpledraw: Starting\n");

	# Create Display
	display := Display.allocate(nil);
	if(display == nil) {
		sys->fprint(sys->fildes(2), "simpledraw: Display.allocate failed: %r\n");
		return;
	}

	sys->print("simpledraw: Display allocated\n");

	screen := display.image;
	if(screen == nil) {
		sys->fprint(sys->fildes(2), "simpledraw: screen is nil\n");
		return;
	}

	sys->print("simpledraw: screen.r = (%d,%d)-(%d,%d)\n",
		screen.r.min.x, screen.r.min.y,
		screen.r.max.x, screen.r.max.y);

	# Fill screen with red
	red := display.color(Draw->Red);
	sys->print("simpledraw: Drawing red to entire screen\n");
	screen.draw(screen.r, red, nil, (0,0));
	screen.flush(Draw->Flushnow);

	sys->print("simpledraw: Done, sleeping 3 seconds\n");
	sys->sleep(3000);

	# Now draw green
	sys->print("simpledraw: Drawing green\n");
	green := display.color(Draw->Green);
	screen.draw(screen.r, green, nil, (0,0));
	screen.flush(Draw->Flushnow);

	sys->print("simpledraw: Done, sleeping 3 seconds\n");
	sys->sleep(3000);

	# Now draw blue
	sys->print("simpledraw: Drawing blue\n");
	blue := display.color(Draw->Blue);
	screen.draw(screen.r, blue, nil, (0,0));
	screen.flush(Draw->Flushnow);

	sys->print("simpledraw: Done, sleeping 3 seconds\n");
	sys->sleep(3000);

	sys->print("simpledraw: Exiting\n");
}
