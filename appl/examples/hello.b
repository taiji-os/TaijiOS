implement Hello;

include "sys.m";
include "tk.m";
include "tkclient.m";
include "draw.m";

sys: Sys;
tk: Tk;
tkclient: Tkclient;
draw: Draw;

evtch: chan of string;

handleClick: fn(top: ref Tk->Toplevel) {
    sys->print("Hello from Kryon!\n");
};


init(ctxt: ref Draw->Context, nil: list of string)
{
    sys = load Sys Sys->PATH;
    tk = load Tk Tk->PATH;
    tkclient = load Tkclient Tkclient->PATH;
    draw = load Draw Draw->PATH;
    tkclient->init();

    sys->pctl(Sys->NEWPGRP, nil);

    evtch = chan of string;

    (top, titlech) := tkclient->toplevel(ctxt, "", "Hello World", Tkclient->Appl);

    # Build UI
    tk->cmd(top, "frame ." );
    tk->cmd(top, "button ..w1 -text {Click Me}" );
    tk->cmd(top, "..w1 configure -color {white}" );
    tk->cmd(top, "..w1 configure -backgroundColor {#404080}" );
    tk->cmd(top, "..w1 configure -onClick {send evtch handleClick}" );
    # Center layout
    tk->cmd(top, "pack ..w0" );

    tkclient->onscreen(top, nil);
    tkclient->startinput(top, "kbd" :: "ptr" :: nil);

    # Event loop
    for(;;) alt {
        s := <-top.ctxt.kbd =>
            tk->keyboard(top, s);
        s := <-top.ctxt.ptr =>
            tk->pointer(top, *s);
        s := <-top.wreq or s = <-titlech =>
            tkclient->wmctl(top, s);
        msg := <-evtch =>
            # Handle event: msg
            ;
    }
}
