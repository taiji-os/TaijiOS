Syntax : module {
	PATH : con "/dis/lib/syntax.dis";

	init : fn(mods : ref Dat->Mods);

	# Token type constants
	TKWD, TSTR, TCHR, TNUM, TCOM, TTYPE, TFN, TOP, TPRE, TID : con iota;

	# Language detection
	detect : fn(filename : string, content : string) : string;

	# Theme loading
	loadtheme : fn(name : string) : int;
	getcolor : fn(tokentype : int) : ref Draw->Image;

	# Tokenization
	gettokens : fn(lang : string, text : string, max : int) : array of (int, int, int);

	# Configuration check
	enabled : fn() : int;
};
