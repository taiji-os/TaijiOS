Tkclient: module
{
	PATH:		con "/dis/lib/tkclient.dis";

	Resize,
	Hide,
	Help,
	OK,
	Popup,		# XXX is this useful?
	Plain:		con 1 << iota;

	Appl:		con Resize | Hide;

	init:		fn();
	makedrawcontext: fn():	ref Draw->Context;
	toplevel:	fn(ctxt: ref Draw->Context, topconfig: string,
				title: string, buts: int): (ref Tk->Toplevel, chan of string);
	onscreen:		fn(top: ref Tk->Toplevel, how: string);
	startinput:		fn(top: ref Tk->Toplevel, devs: list of string);
	wmctl:		fn(top: ref Tk->Toplevel, request: string): string;
	settitle:		fn(top: ref Tk->Toplevel, name: string): string;
	handler:		fn(top: ref Tk->Toplevel, stop: chan of int);
	tkcmds:		fn(top: ref Tk->Toplevel, a: array of string);
	filename:	fn(ctxt: ref Draw->Context, parent: ref Draw->Image,
				title: string, pat: list of string, dir: string): string;
	dialog:		fn(top: ref Tk->Toplevel, ico: string, title: string, msg: string,
				dflt: int, labs: list of string): int;
	geom:		fn(top: ref Tk->Toplevel): string;

	snarfput:	fn(buf: string);
	snarfget:	fn(): string;
};
