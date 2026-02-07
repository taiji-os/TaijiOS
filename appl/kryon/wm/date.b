implement Date;

include "sys.m";
	sys: Sys;

include "draw.m";
	draw: Draw;

include "tkclient.m";
	tkclient: Tkclient;

include "tk.m";
	tk: Tk;

include "daytime.m";
	daytime: Daytime;

Date: module
{
    init:	fn(ctxt: ref Draw->Context, argv: list of string);
    time: fn(): string;
};

tpid: int;


tkcmds := array[] of {
    "label .w0 -label {} -fg white",
    "pack .w0 -fill both -expand 1",
    "update"
};

init(ctxt: ref Draw->Context, argv: list of string)
{
    sys = load Sys Sys->PATH;
    draw = load Draw Draw->PATH;
    tk = load Tk Tk->PATH;
    tkclient = load Tkclient Tkclient->PATH;
    daytime = load Daytime Daytime->PATH;

    sys->pctl(Sys->NEWPGRP, nil);
    tkclient->init();

    (t, wmctl) := tkclient->toplevel(ctxt, "", "Date", 0);

    for (i := 0; i < len tkcmds; i++)
        tk->cmd(t, tkcmds[i]);

    tick := chan of int;
    spawn timer(tick);

    time_update(t);

    tkclient->onscreen(t, nil);
    tkclient->startinput(t, "kbd"::"ptr"::nil);

    for(;;) {
        alt {
        s := <-t.ctxt.kbd =>
            tk->keyboard(t, s);
        s := <-t.ctxt.ptr =>
            tk->pointer(t, *s);
        s := <-t.ctxt.ctl or
        s = <-t.wreq or
        s = <-wmctl =>
            tkclient->wmctl(t, s);
        <-tick =>
            time_update(t);
        }
    }
}
time_update(t: ref Tk->Toplevel)
{
    tk->cmd(t, ".w0 configure -label {"+time()+"};update");
}

timer(c: chan of int)
{
    tpid = sys->pctl(0, nil);
    for(;;) {
        c <-= 1;
        sys->sleep(1000);
    }
}


time(): string
{
    return daytime->time()[0:19];
}
