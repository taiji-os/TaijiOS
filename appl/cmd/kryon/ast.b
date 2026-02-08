implement Ast;

include "sys.m";
    sys: Sys;
include "ast.m";

# AST construction functions

program_create(): ref Program
{
    return ref Program (nil, nil, nil, nil, nil, nil, nil, nil, nil, 0);
}

constdecl_create(name: string, value: string): ref ConstDecl
{
    return ref ConstDecl (name, value, nil);
}

var_decl_create(name: string, typ: string, init_expr: string, init: ref Value): ref VarDecl
{
    return ref VarDecl (name, typ, init_expr, init, nil);
}

code_block_create(typ: int, code: string): ref CodeBlock
{
    return ref CodeBlock (typ, code, nil);
}

component_create(name: string): ref ComponentDef
{
    return ref ComponentDef (name, nil, nil, nil, nil, nil);
}

app_decl_create(): ref AppDecl
{
    return ref AppDecl ("", nil, nil);
}

widget_create(typ: int): ref Widget
{
    return ref Widget (typ, "", nil, nil, nil, 0);
}

property_create(name: string): ref Property
{
    return ref Property (name, nil, nil);
}

value_create_string(s: string): ref Value
{
    return ref Value.String (Ast->VALUE_STRING, s);
}

value_create_number(n: big): ref Value
{
    return ref Value.Number (Ast->VALUE_NUMBER, n);
}

value_create_color(c: string): ref Value
{
    return ref Value.Color (Ast->VALUE_COLOR, c);
}

value_create_ident(id: string): ref Value
{
    return ref Value.Identifier (Ast->VALUE_IDENTIFIER, id);
}

value_create_array(items: array of ref Value): ref Value
{
    return ref Value.Array (Ast->VALUE_ARRAY, items);
}

param_create(name: string, typ: string, default_val: string): ref Param
{
    return ref Param (name, typ, default_val, nil);
}

# Widget linking functions

program_add_const(prog: ref Program, cd: ref ConstDecl)
{
    if (prog == nil || cd == nil)
        return;

    if (prog.consts == nil) {
        prog.consts = cd;
    } else {
        c := prog.consts;
        while (c.next != nil)
            c = c.next;
        c.next = cd;
    }
}

widget_add_child(parent: ref Widget, child: ref Widget)
{
    if (parent == nil || child == nil)
        return;

    if (parent.children == nil) {
        parent.children = child;
    } else {
        w := parent.children;
        while (w.next != nil)
            w = w.next;
        w.next = child;
    }
}

widget_add_property(w: ref Widget, prop: ref Property)
{
    if (w == nil || prop == nil)
        return;

    if (w.props == nil) {
        w.props = prop;
    } else {
        p := w.props;
        while (p.next != nil)
            p = p.next;
        p.next = prop;
    }
}

program_add_var(prog: ref Program, var: ref VarDecl)
{
    if (prog == nil || var == nil)
        return;

    if (prog.vars == nil) {
        prog.vars = var;
    } else {
        v := prog.vars;
        while (v.next != nil)
            v = v.next;
        v.next = var;
    }
}

program_add_code_block(prog: ref Program, code: ref CodeBlock)
{
    if (prog == nil || code == nil)
        return;

    if (prog.code_blocks == nil) {
        prog.code_blocks = code;
    } else {
        cb := prog.code_blocks;
        while (cb.next != nil)
            cb = cb.next;
        cb.next = code;
    }
}

program_add_component(prog: ref Program, comp: ref ComponentDef)
{
    if (prog == nil || comp == nil)
        return;

    if (prog.components == nil) {
        prog.components = comp;
    } else {
        c := prog.components;
        while (c.next != nil)
            c = c.next;
        c.next = comp;
    }
}

program_set_app(prog: ref Program, app: ref AppDecl)
{
    prog.app = app;
}

program_add_reactive_fn(prog: ref Program, rfn: ref ReactiveFunction)
{
    if (prog == nil || rfn == nil)
        return;

    if (prog.reactive_fns == nil) {
        prog.reactive_fns = rfn;
    } else {
        r := prog.reactive_fns;
        while (r.next != nil)
            r = r.next;
        r.next = rfn;
    }
}

# List building functions

constdecl_list_add(listhd: ref ConstDecl, item: ref ConstDecl): ref ConstDecl
{
    if (listhd == nil)
        return item;

    c := listhd;
    while (c.next != nil)
        c = c.next;
    c.next = item;
    return listhd;
}

property_list_add(listhd: ref Property, item: ref Property): ref Property
{
    if (listhd == nil)
        return item;

    p := listhd;
    while (p.next != nil)
        p = p.next;
    p.next = item;
    return listhd;
}

widget_list_add(listhd: ref Widget, item: ref Widget): ref Widget
{
    if (listhd == nil)
        return item;

    w := listhd;
    while (w.next != nil)
        w = w.next;
    w.next = item;
    return listhd;
}

var_list_add(listhd: ref VarDecl, item: ref VarDecl): ref VarDecl
{
    if (listhd == nil)
        return item;

    v := listhd;
    while (v.next != nil)
        v = v.next;
    v.next = item;
    return listhd;
}

code_block_list_add(listhd: ref CodeBlock, item: ref CodeBlock): ref CodeBlock
{
    if (listhd == nil)
        return item;

    cb := listhd;
    while (cb.next != nil)
        cb = cb.next;
    cb.next = item;
    return listhd;
}

component_list_add(listhd: ref ComponentDef, item: ref ComponentDef): ref ComponentDef
{
    if (listhd == nil)
        return item;

    c := listhd;
    while (c.next != nil)
        c = c.next;
    c.next = item;
    return listhd;
}

param_list_add(listhd: ref Param, item: ref Param): ref Param
{
    if (listhd == nil)
        return item;

    p := listhd;
    while (p.next != nil)
        p = p.next;
    p.next = item;
    return listhd;
}

# Value helper functions - safe field access for pick ADT

value_get_string(v: ref Value): string
{
    if (v == nil || v.valtype != VALUE_STRING)
        return "";
    pick sv := v {
    String => return sv.string_val;
    * => return "";
    }
}

value_get_number(v: ref Value): big
{
    if (v == nil || v.valtype != VALUE_NUMBER)
        return big 0;
    pick nv := v {
    Number => return nv.number_val;
    * => return big 0;
    }
}

value_get_color(v: ref Value): string
{
    if (v == nil || v.valtype != VALUE_COLOR)
        return "";
    pick cv := v {
    Color => return cv.color_val;
    * => return "";
    }
}

value_get_ident(v: ref Value): string
{
    if (v == nil || v.valtype != VALUE_IDENTIFIER)
        return "";
    pick iv := v {
    Identifier => return iv.ident_val;
    * => return "";
    }
}

value_create_fn_call(fn_name: string): ref Value
{
    return ref Value.FnCall (Ast->VALUE_FN_CALL, fn_name);
}

# Module import helper functions

moduleimport_create(module_name: string, alias: string): ref ModuleImport
{
    return ref ModuleImport (module_name, alias, nil);
}

moduleimport_list_add(head: ref ModuleImport, imp: ref ModuleImport): ref ModuleImport
{
    if (head == nil)
        return imp;

    m := head;
    while (m.next != nil)
        m = m.next;
    m.next = imp;
    return head;
}

program_add_module_import(prog: ref Program, imp: ref ModuleImport)
{
    if (prog == nil || imp == nil)
        return;

    if (prog.module_imports == nil) {
        prog.module_imports = imp;
    } else {
        m := prog.module_imports;
        while (m.next != nil)
            m = m.next;
        m.next = imp;
    }
}

program_add_function_decl(prog: ref Program, fn_decl: ref FunctionDecl)
{
    if (prog == nil || fn_decl == nil)
        return;

    if (prog.function_decls == nil) {
        prog.function_decls = fn_decl;
    } else {
        f := prog.function_decls;
        while (f.next != nil)
            f = f.next;
        f.next = fn_decl;
    }
}

# Reactive function helper functions

watchvar_create(name: string): ref WatchVar
{
    return ref WatchVar (name, nil);
}

watchvar_list_add(head: ref WatchVar, wv: ref WatchVar): ref WatchVar
{
    if (head == nil)
        return wv;
    last := head;
    while (last.next != nil)
        last = last.next;
    last.next = wv;
    return head;
}

reactivefn_create(name: string, expr: string, interval: int, watch_vars: ref WatchVar): ref ReactiveFunction
{
    return ref ReactiveFunction (name, expr, interval, watch_vars, nil);
}

reactivefn_list_add(head: ref ReactiveFunction, rfn: ref ReactiveFunction): ref ReactiveFunction
{
    if (head == nil)
        return rfn;

    r := head;
    while (r.next != nil)
        r = r.next;
    r.next = rfn;
    return head;
}

reactivebinding_create(widget_path: string, property_name: string, fn_name: string): ref ReactiveBinding
{
    return ref ReactiveBinding (widget_path, property_name, fn_name, nil);
}

reactivebinding_list_add(head: ref ReactiveBinding, binding: ref ReactiveBinding): ref ReactiveBinding
{
    if (head == nil)
        return binding;

    r := head;
    while (r.next != nil)
        r = r.next;
    r.next = binding;
    return head;
}

# Regular function declaration helper functions

functiondecl_create(name: string, body: ref Statement): ref FunctionDecl
{
    return ref FunctionDecl (name, "", body, "", 0, nil);
}

functiondecl_create_with_body(name: string, params: string, body: ref Statement, return_type: string, interval: int): ref FunctionDecl
{
    return ref FunctionDecl (name, params, body, return_type, interval, nil);
}

functiondecl_list_add(head: ref FunctionDecl, fn_decl: ref FunctionDecl): ref FunctionDecl
{
    if (head == nil)
        return fn_decl;

    f := head;
    while (f.next != nil)
        f = f.next;
    f.next = fn_decl;
    return head;
}

# Symbol table helper functions for variable validation

symboltable_create(): ref SymbolTable
{
    return ref SymbolTable (nil, nil, nil, nil);
}

symboltable_add_var(st: ref SymbolTable, name: string)
{
    if (st == nil || name == nil)
        return;

    # Check if already in list to avoid duplicates
    for (l := st.vars; l != nil; l = tl l) {
        if (hd l == name)
            return;
    }
    st.vars = name :: st.vars;
}

symboltable_add_module_var(st: ref SymbolTable, name: string)
{
    if (st == nil || name == nil)
        return;

    # Check if already in list to avoid duplicates
    for (l := st.module_vars; l != nil; l = tl l) {
        if (hd l == name)
            return;
    }
    st.module_vars = name :: st.module_vars;
}

symboltable_add_param(st: ref SymbolTable, name: string)
{
    if (st == nil || name == nil)
        return;

    # Check if already in list to avoid duplicates
    for (l := st.params; l != nil; l = tl l) {
        if (hd l == name)
            return;
    }
    st.params = name :: st.params;
}

symboltable_add_import(st: ref SymbolTable, name: string)
{
    if (st == nil || name == nil)
        return;

    # Check if already in list to avoid duplicates
    for (l := st.imports; l != nil; l = tl l) {
        if (hd l == name)
            return;
    }
    st.imports = name :: st.imports;
}

symboltable_has_var(st: ref SymbolTable, name: string): int
{
    if (st == nil || name == nil)
        return 0;

    # Check local variables
    {
    l := st.vars;
    while (l != nil) {
        if (hd l == name)
            return 1;
        l = tl l;
    }
    }

    # Check function parameters
    {
    l := st.params;
    while (l != nil) {
        if (hd l == name)
            return 1;
        l = tl l;
    }
    }

    # Check module-level variables
    {
    l := st.module_vars;
    while (l != nil) {
        if (hd l == name)
            return 1;
        l = tl l;
    }
    }

    # Check imports
    {
    l := st.imports;
    while (l != nil) {
        if (hd l == name)
            return 1;
        l = tl l;
    }
    }

    return 0;
}

# Struct field and declaration functions

structfield_create(name: string, typename: string): ref StructField
{
    return ref StructField (name, typename, nil);
}

structfield_list_add(head: ref StructField, item: ref StructField): ref StructField
{
    if (head == nil)
        return item;

    f := head;
    while (f.next != nil)
        f = f.next;
    f.next = item;
    return head;
}

structdecl_create(name: string): ref StructDecl
{
    return ref StructDecl (name, nil, nil);
}

structdecl_list_add(head: ref StructDecl, item: ref StructDecl): ref StructDecl
{
    if (head == nil)
        return item;

    d := head;
    while (d.next != nil)
        d = d.next;
    d.next = item;
    return head;
}

program_add_struct_decl(prog: ref Program, decl: ref StructDecl)
{
    if (prog == nil || decl == nil)
        return;

    if (prog.struct_decls == nil) {
        prog.struct_decls = decl;
    } else {
        d := prog.struct_decls;
        while (d.next != nil)
            d = d.next;
        d.next = decl;
    }
}

# Real value helper functions

value_create_real(r: real): ref Value
{
    return ref Value.Real (Ast->VALUE_REAL, r);
}

value_get_real(v: ref Value): real
{
    if (v == nil || v.valtype != VALUE_REAL)
        return 0.0;
    pick rv := v {
    Real => return rv.real_val;
    * => return 0.0;
    }
}

# Statement constructor functions

statement_create_vardecl(lineno: int, var_decl: ref VarDecl): ref Statement
{
    return ref Statement.VarDecl (lineno, nil, var_decl);
}

statement_create_block(lineno: int, statements: ref Statement): ref Statement
{
    return ref Statement.Block (lineno, nil, statements);
}

statement_create_if(lineno: int, condition: string, then_stmt: ref Statement, else_stmt: ref Statement): ref Statement
{
    return ref Statement.If (lineno, nil, condition, then_stmt, else_stmt);
}

statement_create_for(lineno: int, init: ref Statement, condition: string, increment: string, body: ref Statement): ref Statement
{
    return ref Statement.For (lineno, nil, init, condition, increment, body);
}

statement_create_while(lineno: int, condition: string, body: ref Statement): ref Statement
{
    return ref Statement.While (lineno, nil, condition, body);
}

statement_create_return(lineno: int, expression: string): ref Statement
{
    return ref Statement.Return (lineno, nil, expression);
}

statement_create_expr(lineno: int, expression: string): ref Statement
{
    return ref Statement.Expr (lineno, nil, expression);
}

statement_list_add(head: ref Statement, stmt: ref Statement): ref Statement
{
    if (head == nil)
        return stmt;

    s := head;
    while (s.next != nil)
        s = s.next;
    s.next = stmt;
    return head;
}
