# Graphics module for Kryon - provides clean drawing API

Graphics: module
{
    PATH: con "$Graphics";

    # Import Draw module for types
    # Draw->Image, Draw->Display, Draw->Point, Draw->Rect are used

    # Context type - holds drawing state
    Context: adt {
        screen: ref Draw->Image;      # Drawing surface
        display: ref Draw->Display;   # Display for color allocation
        fill_color: ref Draw->Image;  # Current fill color
        stroke_color: ref Draw->Image; # Current stroke color
        line_width: int;              # Line width
        font: ref Draw->Font;         # Current font
    };

    # Create a new graphics context
    create: fn(screen: ref Draw->Image, display: ref Draw->Display): ref Context;

    # Clear the screen with a color
    clear: fn(ctx: ref Context, color: ref Draw->Image);

    # Set fill color
    setfill: fn(ctx: ref Context, color: ref Draw->Image);

    # Set stroke color
    setstroke: fn(ctx: ref Context, color: ref Draw->Image);

    # Set line width
    setlinewidth: fn(ctx: ref Context, width: int);

    # Draw filled rectangle
    rect: fn(ctx: ref Context, x, y, w, h: int);

    # Draw filled circle
    circle: fn(ctx: ref Context, x, y, r: int);

    # Draw filled ellipse
    ellipse: fn(ctx: ref Context, x, y, rx, ry: int);

    # Draw line
    line: fn(ctx: ref Context, x1, y1, x2, y2: int);

    # Draw text
    text: fn(ctx: ref Context, str: string, x, y: int);

    # Flush drawing to screen
    flush: fn(ctx: ref Context);

    # Helper: allocate a color from RGB values
    rgb: fn(display: ref Draw->Display, r, g, b: int): ref Draw->Image;

    # Helper: allocate a color from hex string (#rrggbb or #rgb)
    hexcolor: fn(display: ref Draw->Display, hex: string): ref Draw->Image;
};
