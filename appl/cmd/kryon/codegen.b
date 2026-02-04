implement Codegen;

include "sys.m";
    sys: Sys;
include "bufio.m";
    bufio: Bufio;
include "ast.m";
    ast: Ast;
include "codegen.m";

# Code generation state
Codegen: adt {
    module_name: string;
    output: ref Sys->FD;
    tk_cmds: list of string;
    widget_counter: int;
    callbacks: list of (string, string);  # (name, event) pairs
};

# Create a new code generator
create_codegen(output: ref Sys->FD, module_name: string): ref Codegen
{
    return ref Codegen (module_name, output, nil, 0, nil);
}

# Escape a string for Tk
escape_tk_string(s: string): string
{
    if (s == nil)
        return "{}";

    # Check if string needs braces
    needs_braces := 0;
    for (i := 0; i < len s; i++) {
        c := s[i];
        if (c == ' ' || c == '{' || c == '}' || c == '\\' ||
            c == '$' || c == '[' || c == ']') {
            needs_braces = 1;
            break;
        }
    }

    if (!needs_braces)
        return s;

    # Build {value} with escapes
    result := "{";

    for (i := 0; i < len s; i++) {
        c := s[i];
        if (c == '}' || c == '\\')
            result[len result] = '\\';
        result[len result] = c;
    }

    result[len result] = '}';

    return result;
}

# Map Kryon property names to Tk property names
map_property_name(prop_name: string): string
{
    if (prop_name == "color" || prop_name == "textColor")
        return "fg";

    if (prop_name == "backgroundColor")
        return "bg";

    return prop_name;
}

# Convert widget type to Tk widget type
widget_type_to_tk(typ: int): string
{
    case typ {
    Ast->WIDGET_BUTTON =>
        return "button";
    Ast->WIDGET_TEXT =>
        return "label";
    Ast->WIDGET_INPUT =>
        return "entry";
    Ast->WIDGET_WINDOW =>
        return "toplevel";
    * =>
        return "frame";
    }
}

# Check if a property is a callback (returns event name or nil)
is_callback_property(prop_name: string): string
{
    if (len prop_name >= 2 && prop_name[0:2] == "on")
        return prop_name;

    return nil;
}

# Append a Tk command to the commands list
append_tk_cmd(cg: ref Codegen, cmd: string)
{
    cg.tk_cmds = cmd :: cg.tk_cmds;
}

# Add a callback to the callback list
add_callback(cg: ref Codegen, name: string, event: string)
{
    cg.callbacks = (name, event) :: cg.callbacks;
}

# Generate a property value as Tk string
value_to_tk(v: ref Ast->Value): string
{
    if (v == nil)
        return "{}";

    case v.valtype {
    Ast->VALUE_STRING =>
        return escape_tk_string(v.string_val);

    Ast->VALUE_NUMBER =>
        return sys->sprint("%bd", v.number_val);

    Ast->VALUE_COLOR =>
        return escape_tk_string(v.color_val);

    Ast->VALUE_IDENTIFIER =>
        return v.ident_val;

    * =>
        return "{}";
    }
}

# Process a property and add to command buffer
process_property(cg: ref Codegen, prop: ref Ast->Property, cmd: ref list of string, callbacks: ref list of (string, string)): string
{
    if (prop == nil || prop.value == nil)
        return nil;

    cb_event := is_callback_property(prop.name);

    if (cb_event != nil && prop.value.valtype == Ast->VALUE_IDENTIFIER) {
        # This is a callback
        callbacks = (prop.value.ident_val, prop.name) :: callbacks;
        return nil;
    }

    # Regular property
    tk_prop := map_property_name(prop.name);
    val_str := value_to_tk(prop.value);

    prop_cmd := sys->sprint("-%s %s", tk_prop, val_str);
    cmd = prop_cmd :: cmd;

    return nil;
}

# Generate code for a single widget
generate_widget(cg: ref Codegen, w: ref Ast->Widget, parent: string, is_root: int): string
{
    if (w == nil)
        return nil;

    # Skip wrapper widgets and layout helpers
    if (w.is_wrapper || w.wtype == Ast->WIDGET_CENTER ||
        w.wtype == Ast->WIDGET_COLUMN || w.wtype == Ast->WIDGET_ROW) {
        return generate_widget_list(cg, w.children, parent, is_root);
    }

    # Build widget path
    widget_path := "";

    if (is_root) {
        widget_path = sys->sprint(".w%d", cg.widget_counter);
    } else {
        widget_path = sys->sprint("%s.w%d", parent, cg.widget_counter);
    }
    cg.widget_counter++;

    # Build widget creation command
    tk_type := widget_type_to_tk(w.wtype);
    cmd_parts: list of string = nil;
    cmd_parts = tk_type :: widget_path :: cmd_parts;

    # Generate properties
    callbacks: list of (string, string) = nil;

    prop := w.props;
    while (prop != nil) {
        cb_event := is_callback_property(prop.name);

        if (cb_event != nil && prop.value != nil && prop.value.valtype == Ast->VALUE_IDENTIFIER) {
            # This is a callback
            callbacks = (prop.value.ident_val, prop.name) :: callbacks;
        } else {
            # Regular property
            tk_prop := map_property_name(prop.name);
            val_str := value_to_tk(prop.value);
            prop_cmd := sys->sprint("-%s", tk_prop);
            cmd_parts = prop_cmd :: cmd_parts;
            cmd_parts = val_str :: cmd_parts;
        }

        prop = prop.next;
    }

    # Reverse and join command parts
    cmd := "";
    parts := cmd_parts;
    while (parts != nil) {
        if (cmd != nil)
            cmd += " ";
        cmd += hd parts;
        parts = tl parts;
    }

    append_tk_cmd(cg, cmd);

    # Process children FIRST (they need to be packed before this widget)
    if (w.children != nil) {
        err := generate_widget_list(cg, w.children, widget_path, 0);
        if (err != nil)
            return err;
    }

    # Pack widget into parent AFTER children are packed
    pack_cmd := sys->sprint("pack %s", widget_path);
    append_tk_cmd(cg, pack_cmd);

    # Store callbacks for later
    while (callbacks != nil) {
        (name, event) := hd callbacks;
        add_callback(cg, name, event);
        callbacks = tl callbacks;
    }

    return nil;
}

# Process widget list
generate_widget_list(cg: ref Codegen, w: ref Ast->Widget, parent: string, is_root: int): string
{
    while (w != nil) {
        if (w.is_wrapper) {
            # Process children of wrapper with same parent
            child := w.children;
            while (child != nil) {
                if (child.is_wrapper) {
                    err := generate_widget_list(cg, child.children, parent, is_root);
                    if (err != nil)
                        return err;
                } else {
                    err := generate_widget(cg, child, parent, is_root);
                    if (err != nil)
                        return err;
                }
                child = child.next;
            }
        } else {
            err := generate_widget(cg, w, parent, is_root);
            if (err != nil)
                return err;
        }
        w = w.next;
    }

    return nil;
}

# Collect widget commands
collect_widget_commands(cg: ref Codegen, prog: ref Ast->Program): string
{
    cg.widget_counter = 0;

    if (prog == nil || prog.app == nil || prog.app.body == nil)
        return nil;

    return generate_widget_list(cg, prog.app.body, ".", 1);
}

# Generate prologue
generate_prologue(cg: ref Codegen, prog: ref Ast->Program): string
{
    buf := "";

    buf += sys->sprint("implement %s;\n\n", cg.module_name);

    buf += "include \"sys.m\";\n";
    buf += "include \"draw.m\";\n";
    buf += "include \"tk.m\";\n";
    buf += "include \"tkclient.m\";\n\n";

    buf += "sys: Sys;\n";
    buf += "draw: Draw;\n";
    buf += "tk: Tk;\n";
    buf += "tkclient: Tkclient;\n\n";

    # Generate module declaration
    buf += sys->sprint("%s: module\n{\n", cg.module_name);
    buf += "    init: fn(ctxt: ref Draw->Context, nil: list of string);\n";

    # Add function signatures from code blocks
    cb := prog.code_blocks;
    while (cb != nil) {
        if (cb.cbtype == Ast->CODE_LIMBO && cb.code != nil) {
            # Try to extract function name from code like "funcName: fn(...) {"
            code := cb.code;
            colon := 0;

            # Find colon
            for (i = 0; i < len code; i++) {
                if (code[i] == ':') {
                    colon = i;
                    break;
                }
            }

            if (colon > 0 && colon + 4 < len code) {
                if (code[colon+1] == ' ' && code[colon+2] == 'f' &&
                    code[colon+3] == 'n' && code[colon+4] == '(') {

                    # Find function name start
                    start := colon - 1;
                    while (start > 0 && (code[start] == '\n' || code[start] == ' ' || code[start] == '\t'))
                        start--;

                    # Find end of name
                    name_end := start;
                    while (name_end > 0 && code[name_end] != '\n' &&
                           code[name_end] != ' ' && code[name_end] != '\t')
                        name_end--;

                    func_name := code[name_end+1 : start+1];

                    if (len func_name > 0 && func_name[0] >= 'a' && func_name[0] <= 'z') {
                        buf += sys->sprint("    %s: fn();\n", func_name);
                    }
                }
            }
        }
        cb = cb.next;
    }

    buf += "};\n";

    # Write prologue
    sys->fprint(cg.output, "%s", buf);

    return nil;
}

# Generate code blocks (Limbo functions)
generate_code_blocks(cg: ref Codegen, prog: ref Ast->Program): string
{
    cb := prog.code_blocks;

    while (cb != nil) {
        if (cb.cbtype == Ast->CODE_LIMBO && cb.code != nil) {
            code := cb.code;
            current := 0;

            # Process each function in the code block
            while (current < len code) {
                # Skip leading whitespace
                while (current < len code &&
                       (code[current] == '\n' || code[current] == ' ' || code[current] == '\t'))
                    current++;

                if (current >= len code)
                    break;

                # Find function name (ends with ':')
                colon := current;
                found := 0;

                while (colon < len code) {
                    if (code[colon] == ':') {
                        found = 1;
                        break;
                    }
                    colon++;
                }

                if (!found)
                    break;

                # Find opening brace
                lbrace := colon;
                while (lbrace < len code && code[lbrace] != '{')
                    lbrace++;

                if (lbrace >= len code)
                    break;

                # Find matching closing brace
                rbrace := lbrace + 1;
                brace_count := 1;

                while (rbrace < len code && brace_count > 0) {
                    if (code[rbrace] == '{')
                        brace_count++;
                    else if (code[rbrace] == '}')
                        brace_count--;
                    rbrace++;
                }

                if (brace_count != 0)
                    break;

                # Extract function name
                name_start := current;
                while (name_start < colon &&
                       (code[name_start] == '\n' || code[name_start] == ' ' || code[name_start] == '\t'))
                    name_start++;

                func_name := code[name_start : colon];

                # Output function
                sys->fprint(cg.output, "\n%s()\n", func_name);

                # Find body start
                body_start := lbrace + 1;
                while (body_start < rbrace &&
                       (code[body_start] == '\n' || code[body_start] == ' ' || code[body_start] == '\t'))
                    body_start++;

                if (rbrace - 1 > body_start) {
                    body_end := rbrace - 1;
                    while (body_end > body_start &&
                           (code[body_end] == '\n' || code[body_end] == ' ' || code[body_end] == '\t'))
                        body_end--;

                    body := code[body_start : body_end + 1];

                    # Indent body
                    sys->fprint(cg.output, "{\n");

                    lines := sys->tokenize(body, "\n");
                    while (lines != nil) {
                        line := hd lines;
                        if (line != "")
                            sys->fprint(cg.output, "    %s\n", line);
                        else
                            sys->fprint(cg.output, "\n");
                        lines = tl lines;
                    }

                    sys->fprint(cg.output, "}\n");
                }

                current = rbrace;

                # Skip semicolons and whitespace
                while (current < len code &&
                       (code[current] == ';' || code[current] == '\n' ||
                        code[current] == ' ' || code[current] == '\t'))
                    current++;
            }
        }
        cb = cb.next;
    }

    return nil;
}

# Generate tkcmds array
generate_tkcmds_array(cg: ref Codegen): string
{
    sys->fprint(cg.output, "\ntkcmds := array[] of {\n");

    # Reverse commands to get correct order
    cmds := cg.tk_cmds;
    rev: list of string = nil;

    while (cmds != nil) {
        rev = hd cmds :: rev;
        cmds = tl cmds;
    }

    while (rev != nil) {
        sys->fprint(cg.output, "    \"%s\",\n", hd rev);
        rev = tl rev;
    }

    sys->fprint(cg.output, "    \"pack propagate . 0\",\n");
    sys->fprint(cg.output, "    \"update\"\n");
    sys->fprint(cg.output, "};\n\n");

    return nil;
}

# Get string property value
get_string_prop(props: ref Ast->Property, name: string): string
{
    while (props != nil) {
        if (props.name == name && props.value != nil) {
            case props.value.valtype {
            Ast->VALUE_STRING =>
                return props.value.string_val;
            Ast->VALUE_COLOR =>
                return props.value.color_val;
            }
        }
        props = props.next;
    }
    return nil;
}

# Get number property value
get_number_prop(props: ref Ast->Property, name: string): (int, int)
{
    while (props != nil) {
        if (props.name == name && props.value != nil &&
            props.value.valtype == Ast->VALUE_NUMBER) {
            return (int props.value.number_val, 1);
        }
        props = props.next;
    }
    return (0, 0);
}

# Generate init function
generate_init(cg: ref Codegen, prog: ref Ast->Program): string
{
    sys->fprint(cg.output, "init(ctxt: ref Draw->Context, nil: list of string)\n");
    sys->fprint(cg.output, "{\n");

    sys->fprint(cg.output, "    sys = load Sys Sys->PATH;\n");
    sys->fprint(cg.output, "    draw = load Draw Draw->PATH;\n");
    sys->fprint(cg.output, "    tk = load Tk Tk->PATH;\n");
    sys->fprint(cg.output, "    tkclient = load Tkclient Tkclient->PATH;\n\n");
    sys->fprint(cg.output, "    tkclient->init();\n\n");

    # Get app properties
    title := "Application";
    width := 400;
    height := 300;
    bg := "#191919";

    if (prog.app != nil && prog.app.props != nil) {
        t := get_string_prop(prog.app.props, "title");
        if (t != nil)
            title = t;

        bg = get_string_prop(prog.app.props, "background");
        if (bg == nil)
            bg = get_string_prop(prog.app.props, "backgroundColor");
        if (bg == nil)
            bg = "#191919";

        (w, ok) := get_number_prop(prog.app.props, "width");
        if (ok)
            width = w;

        (h, ok) = get_number_prop(prog.app.props, "height");
        if (ok)
            height = h;
    }

    sys->fprint(cg.output, "    (toplevel, menubut) := tkclient->toplevel(ctxt, \"\", \"%s\", 0);\n\n", title);

    # Create command channel if we have callbacks
    has_callbacks := (cg.callbacks != nil);

    if (has_callbacks) {
        sys->fprint(cg.output, "    cmd := chan of string;\n");
        sys->fprint(cg.output, "    tk->namechan(toplevel, cmd, \"cmd\");\n\n");
    }

    # Execute tk commands
    sys->fprint(cg.output, "    for (i := 0; i < len tkcmds; i++)\n");
    sys->fprint(cg.output, "        tk->cmd(toplevel, tkcmds[i]);\n\n");

    # Show window
    sys->fprint(cg.output, "    tkclient->onscreen(toplevel, nil);\n");
    sys->fprint(cg.output, "    tkclient->startinput(toplevel, \"ptr\"::nil);\n\n");
    sys->fprint(cg.output, "    stop := chan of int;\n");
    sys->fprint(cg.output, "    spawn tkclient->handler(toplevel, stop);\n");

    if (has_callbacks) {
        sys->fprint(cg.output, "    for(;;) {\n");
        sys->fprint(cg.output, "        alt {\n");
        sys->fprint(cg.output, "        msg := <-menubut =>\n");
        sys->fprint(cg.output, "            if(msg == \"exit\")\n");
        sys->fprint(cg.output, "                break;\n");
        sys->fprint(cg.output, "            tkclient->wmctl(toplevel, msg);\n");
        sys->fprint(cg.output, "        s := <-cmd =>\n");

        # Generate callback cases
        cbs := cg.callbacks;
        while (cbs != nil) {
            (name, event) := hd cbs;
            sys->fprint(cg.output, "            if(s == \"%s\")\n", name);
            sys->fprint(cg.output, "                %s();\n", name);
            cbs = tl cbs;
        }

        sys->fprint(cg.output, "        }\n");
        sys->fprint(cg.output, "    }\n");
    } else {
        sys->fprint(cg.output, "    while((msg := <-menubut) != \"exit\")\n");
        sys->fprint(cg.output, "        tkclient->wmctl(toplevel, msg);\n");
    }

    sys->fprint(cg.output, "    stop <-= 1;\n");
    sys->fprint(cg.output, "}\n");

    return nil;
}

# Main generation function
generate(output: string, prog: ref Ast->Program, module_name: string): string
{
    if (prog == nil)
        return "nil program";

    if (output == nil || output == "")
        return "nil output path";

    # Open output file
    fd := sys->create(output, Sys->OWRITE, 8r666);
    if (fd == nil)
        return sys->sprint("cannot create output file: %s", output);

    cg := create_codegen(fd, module_name);

    # Generate code
    err := generate_prologue(cg, prog);
    if (err != nil) {
        fd = nil;
        return err;
    }

    err = generate_code_blocks(cg, prog);
    if (err != nil) {
        fd = nil;
        return err;
    }

    err = collect_widget_commands(cg, prog);
    if (err != nil) {
        fd = nil;
        return err;
    }

    err = generate_tkcmds_array(cg);
    if (err != nil) {
        fd = nil;
        return err;
    }

    err = generate_init(cg, prog);
    if (err != nil) {
        fd = nil;
        return err;
    }

    fd = nil;

    return nil;
}
