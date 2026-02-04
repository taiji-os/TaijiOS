implement Test;

include "sys.m";

init()          # Keyword
{
    sys := load Sys Sys->PATH;  # Keyword, module type
    # This is a comment  # Comment
    x := 42;  # Number
    s := "string literal";  # String

    # Function call
    sys->print("Hello, world!\n");

    # for loop
    for(i := 0; i < 10; i++) {
        sys->print("i = %d\n", i);
    }
}
