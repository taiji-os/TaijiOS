Kryon: module
{
    PATH: con "$Kryon";

    # Kryon runtime support (optional, for advanced features)
    # Most functionality is compiled directly into the generated Limbo code

    # Initialize Kryon runtime (for Tcl/Lua handler support)
    init: fn(ctxt: ref Draw->Context): (ref Runtime, string);

    # Runtime ADT for managing handler execution
    Runtime: adt {
        display: ref Draw->Display;
        top: ref Tk->Toplevel;
        evtch: chan of string;
    };

    # Handler descriptor for multi-language handlers
    Handler: adt {
        name: string;
        language: string;  # "limbo", "tcl", or "lua"
        code: string;
    };
};
