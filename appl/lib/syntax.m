Syntax : module {
	PATH : con "/dis/lib/syntax.dis";

	# Token type constants
	TKWD, TSTR, TCHR, TNUM, TCOM, TTYPE, TFN, TOP, TPRE, TID : con iota;
	SYN_NCOL : con 10;

	init : fn(mods : ref Dat->Mods);
	enabled : fn() : int;  # Returns 0 (disabled) initially
};
