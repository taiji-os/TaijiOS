implement Getfile;

include "sys.m";
	sys: Sys;
	stderr: ref Sys->FD;
include "draw.m";
	draw: Draw;
include "selectfile.m";
	selectfile: Selectfile;
include "arg.m";

Getfile: module
{
	init:	fn(ctxt: ref Draw->Context, argv: list of string);
};

usage()
{
	sys->fprint(stderr, "usage: getfile [-d startdir] [-t title] [pattern...]\n");
	raise "fail:usage";
}

init(ctxt: ref Draw->Context, argv: list of string)
{
	sys = load Sys Sys->PATH;
	stderr = sys->fildes(2);
	draw = load Draw Draw->PATH;
	selectfile = load Selectfile Selectfile->PATH;
	if (selectfile == nil) {
		sys->fprint(stderr, "getfile: cannot load %s: %r\n", Selectfile->PATH);
		raise "fail:bad module";
	}
	arg := load Arg Arg->PATH;
	if (arg == nil) {
		sys->fprint(stderr, "getfile: cannot load %s: %r\n", Arg->PATH);
		raise "fail:bad module";
	}

	if (ctxt == nil) {
		sys->fprint(stderr, "getfile: no window context\n");
		raise "fail:bad context";
	}

	sys->pctl(Sys->NEWPGRP, nil);
	selectfile->init();

	startdir := ".";
	title := "Select a file";
	arg->init(argv);
	while (opt := arg->opt()) {
		case opt {
		'd' =>
			startdir = arg->arg();
		't' =>
			title = arg->arg();
		* =>
			sys->fprint(stderr, "getfile: unknown option -%c\n", opt);
			usage();
		}
	}
	if (startdir == nil || title == nil)
		usage();
	argv = arg->argv();
	arg = nil;
	sys->print("%s\n", selectfile->filename(ctxt, nil, title, argv, startdir));
}
