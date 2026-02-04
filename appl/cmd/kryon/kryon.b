implement Kryon;

include "sys.m";
    sys: Sys;
include "draw.m";
    draw: Draw;
include "bufio.m";
    bufio: Bufio;

include "lexer.m";
    lexer: Lexer;
include "parser.m";
    parser: Parser;
include "codegen.m";
    codegen: Codegen;

# Show usage message
show_usage()
{
    sys->fprint(sys->fildes(2), "Usage: kryon [-o output] input.kry\n");
    sys->fprint(sys->fildes(2), "\nOptions:\n");
    sys->fprint(sys->fildes(2), "  -o <output>  Specify output file (default: input.b)\n");
    sys->fprint(sys->fildes(2), "  -h           Show this help message\n");
}

# Derive module name from input file
derive_module_name(input_file: string): string
{
    if (input_file == nil)
        return "Module";

    # Find basename
    basename := input_file;
    for (i := len input_file - 1; i >= 0; i--) {
        if (input_file[i] == '/') {
            basename = input_file[i+1:];
            break;
        }
    }

    # Remove extension
    module_name := basename;
    dot := len module_name - 1;

    while (dot >= 0 && module_name[dot] != '.')
        dot--;

    if (dot > 0)
        module_name = module_name[0:dot];

    # Capitalize first letter
    if (len module_name > 0) {
        first := module_name[0];
        if (first >= 'a' && first <= 'z') {
            # Capitalize
            c := first - ('a' - 'A');
            module_name = string c + module_name[1:];
        }
    }

    return module_name;
}

# Derive output filename from input
derive_output_file(input_file: string): string
{
    if (input_file == nil)
        return nil;

    output := input_file;
    dot := len output - 1;

    while (dot >= 0 && output[dot] != '.')
        dot--;

    if (dot > 0)
        output = output[0:dot] + ".b";
    else
        output = output + ".b";

    return output;
}

# Read entire file into string
read_file(path: string): (string, string)
{
    fd := sys->open(path, Sys->OREAD);
    if (fd == nil)
        return (nil, sys->sprint("cannot open input file: %s", path));

    # Get file size
    (ok, stat) := sys->fstat(fd);
    if (!ok) {
        return (nil, sys->sprint("cannot stat file: %s", path));
    }

    size := stat.length;
    buf := array[size] of byte;

    n := sys->read(fd, buf, len buf);
    if (n < 0) {
        return (nil, sys->sprint("error reading file: %s", path));
    }

    fd = nil;

    # Convert bytes to string
    data := string buf[0:n];

    return (data, nil);
}

# Parse command line arguments
parse_args(argv: list of string): (string, string, string)
{
    input_file := "";
    output_file := "";

    # Skip program name
    args := tl argv;

    while (args != nil) {
        arg := hd args;
        args = tl args;

        if (arg == "-o") {
            if (args == nil)
                return (nil, nil, "missing argument for -o");

            output_file = hd args;
            args = tl args;
        } else if (arg == "-h" || arg == "--help") {
            show_usage();
            raise "success:help";
        } else if (arg[0] == '-') {
            return (nil, nil, "unknown option: " + arg);
        } else {
            if (input_file != nil && input_file != "")
                return (nil, nil, "multiple input files specified");

            input_file = arg;
        }
    }

    if (input_file == nil || input_file == "")
        return (nil, nil, "no input file specified");

    # Derive output file if not specified
    if (output_file == nil || output_file == "")
        output_file = derive_output_file(input_file);

    return (input_file, output_file, nil);
}

init(ctxt: ref Draw->Context, argv: list of string)
{
    sys = load Sys Sys->PATH;
    bufio = load Bufio Bufio->PATH;

    # Parse arguments
    (input, output, err) := parse_args(argv);

    if (err != nil) {
        if (err == "success:help")
            return;

        sys->fprint(sys->fildes(2), "Error: %s\n", err);
        show_usage();
        raise "fail:args";
    }

    sys->print("Parsing %s...\n", input);

    # Read input file
    (contents, read_err) := read_file(input);
    if (read_err != nil) {
        sys->fprint(sys->fildes(2), "Error: %s\n", read_err);
        raise "fail:read";
    }

    # Create lexer
    l := lexer.Lexer.create(input, contents);

    # Parse
    (prog, parse_err) := parser.Parser.parse_program(l);

    if (parse_err != nil) {
        sys->fprint(sys->fildes(2), "Parse error: %s\n", parse_err);
        raise "fail:parse";
    }

    # Derive module name
    module_name := derive_module_name(input);

    sys->print("Generating %s from %s...\n", output, input);

    # Generate code
    gen_err := codegen.Codegen.generate(output, prog, module_name);

    if (gen_err != nil) {
        sys->fprint(sys->fildes(2), "Code generation error: %s\n", gen_err);
        raise "fail:codegen";
    }

    sys->print("Successfully generated %s\n", output);
}
