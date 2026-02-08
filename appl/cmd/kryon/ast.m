# AST type definitions for Kryon compiler

Ast: module
{
    # Widget type constants
    WIDGET_APP: con 1;
    WIDGET_WINDOW: con 2;
    WIDGET_FRAME: con 3;
    WIDGET_BUTTON: con 4;
    WIDGET_LABEL: con 5;
    WIDGET_ENTRY: con 6;
    WIDGET_CHECKBUTTON: con 7;
    WIDGET_RADIOBUTTON: con 8;
    WIDGET_LISTBOX: con 9;
    WIDGET_CANVAS: con 10;
    WIDGET_SCALE: con 11;
    WIDGET_MENUBUTTON: con 12;
    WIDGET_MESSAGE: con 13;
    WIDGET_COLUMN: con 14;
    WIDGET_ROW: con 15;
    WIDGET_CENTER: con 16;
    WIDGET_IMG: con 17;

    # Value type constants
    VALUE_STRING: con 1;
    VALUE_NUMBER: con 2;
    VALUE_COLOR: con 3;
    VALUE_IDENTIFIER: con 4;
    VALUE_ARRAY: con 5;
    VALUE_FN_CALL: con 6;
    VALUE_REAL: con 7;

    # Code block type constants
    CODE_LIMBO: con 1;
    CODE_TCL: con 2;
    CODE_LUA: con 3;

    # Value ADT - uses pick to handle different value types
    Value: adt {
        valtype: int;

        pick {
        String =>
            string_val: string;
        Number =>
            number_val: big;
        Real =>
            real_val: real;
        Color =>
            color_val: string;
        Identifier =>
            ident_val: string;
        Array =>
            array_val: array of ref Value;
        FnCall =>
            fn_name: string;
        }
    };

    # Property ADT
    Property: adt {
        name: string;
        value: ref Value;
        next: ref Property;
    };

    # Widget ADT
    Widget: adt {
        wtype: int;
        id: string;
        props: ref Property;
        children: ref Widget;
        next: ref Widget;
        is_wrapper: int;
    };

    # Code block ADT
    CodeBlock: adt {
        cbtype: int;
        code: string;
        next: ref CodeBlock;
    };

    # Variable declaration ADT
    VarDecl: adt {
        name: string;
        typ: string;
        init_expr: string;          # initialization expression as string
        init_value: ref Value;
        next: ref VarDecl;
    };

    # Constant declaration ADT
    ConstDecl: adt {
        name: string;
        value: string;
        next: ref ConstDecl;
    };

    # Parameter ADT
    Param: adt {
        name: string;
        typ: string;
        default_value: string;
        next: ref Param;
    };

    # Component definition ADT
    ComponentDef: adt {
        name: string;
        params: ref Param;
        vars: ref VarDecl;
        handlers: ref CodeBlock;
        body: ref Widget;
        next: ref ComponentDef;
    };

    # App declaration ADT
    AppDecl: adt {
        title: string;
        props: ref Property;
        body: ref Widget;
    };

    # Watch variable ADT for variable-based reactivity
    WatchVar: adt {
        name: string;
        next: ref WatchVar;
    };

    # Reactive function ADT
    ReactiveFunction: adt {
        name: string;
        expression: string;
        interval: int;              # 0 for var-based, >0 for time-based
        watch_vars: ref WatchVar;   # list of variables to watch
        next: ref ReactiveFunction;
    };

    # Regular function declaration ADT (for callbacks)
    FunctionDecl: adt {
        name: string;
        params: string;               # parameter list string (e.g., "x: int, y: int")
        body: ref Statement;          # parsed statement body (instead of string)
        return_type: string;          # return type annotation (e.g., "string")
        reactive_interval: int;       # reactive binding interval (0 for non-reactive)
        next: ref FunctionDecl;
    };

    # Statement ADT for function bodies
    Statement: adt {
        lineno: int;
        next: ref Statement;

        pick {
        VarDecl =>
            var_decl: ref Ast->VarDecl;
        Block =>
            statements: ref Statement;
        If =>
            condition: string;        # expression as string
            then_stmt: ref Statement;
            else_stmt: ref Statement;
        For =>
            init: ref Statement;      # usually VarDecl
            condition: string;
            increment: string;
            body: ref Statement;
        While =>
            condition: string;
            body: ref Statement;
        Return =>
            expression: string;       # empty string for no value
        Expr =>
            expression: string;       # expression statement
        }
    };

    # Reactive binding ADT (tracks widget -> reactive function relationships)
    ReactiveBinding: adt {
        widget_path: string;
        property_name: string;
        fn_name: string;
        next: ref ReactiveBinding;
    };

    # Module import ADT
    ModuleImport: adt {
        module_name: string;
        alias: string;
        next: ref ModuleImport;
    };

    # Struct field ADT
    StructField: adt {
        name: string;
        typename: string;
        next: ref StructField;
    };

    # Struct declaration ADT
    StructDecl: adt {
        name: string;
        fields: ref StructField;
        next: ref StructDecl;
    };

    # Spawn statement ADT
    SpawnStmt: adt {
        fn_name: string;
        args: string;  # argument expression string
    };

    # Channel type ADT (for type annotations)
    ChanType: adt {
        elem_type: string;  # element type (e.g., "int" for "chan of int")
    };

    # Symbol table for tracking variables in scope during validation
    SymbolTable: adt {
        vars: list of string;        # Local variables
        module_vars: list of string; # Module-level vars
        params: list of string;      # Function parameters
        imports: list of string;     # Module imports (known identifiers)
    };

    # Program ADT (root node)
    Program: adt {
        consts: ref ConstDecl;
        vars: ref VarDecl;
        code_blocks: ref CodeBlock;
        components: ref ComponentDef;
        app: ref AppDecl;
        reactive_fns: ref ReactiveFunction;
        module_imports: ref ModuleImport;
        function_decls: ref FunctionDecl;  # regular function declarations
        struct_decls: ref StructDecl;      # struct type declarations
        window_type: int;    # 0=Tk, 1=Draw/wmclient
    };

    # AST construction functions
    program_create: fn(): ref Program;
    constdecl_create: fn(name: string, value: string): ref ConstDecl;
    var_decl_create: fn(name: string, typ: string, init_expr: string, init: ref Value): ref VarDecl;
    code_block_create: fn(typ: int, code: string): ref CodeBlock;
    component_create: fn(name: string): ref ComponentDef;
    app_decl_create: fn(): ref AppDecl;
    widget_create: fn(typ: int): ref Widget;
    property_create: fn(name: string): ref Property;
    value_create_string: fn(s: string): ref Value;
    value_create_number: fn(n: big): ref Value;
    value_create_color: fn(c: string): ref Value;
    value_create_ident: fn(id: string): ref Value;
    value_create_array: fn(items: array of ref Value): ref Value;
    value_create_fn_call: fn(fn_name: string): ref Value;
    value_create_real: fn(r: real): ref Value;
    param_create: fn(name: string, typ: string, default_val: string): ref Param;

    # Struct functions
    structfield_create: fn(name: string, typename: string): ref StructField;
    structfield_list_add: fn(head: ref StructField, item: ref StructField): ref StructField;
    structdecl_create: fn(name: string): ref StructDecl;
    structdecl_list_add: fn(head: ref StructDecl, item: ref StructDecl): ref StructDecl;

    # Module import functions
    moduleimport_create: fn(module_name: string, alias: string): ref ModuleImport;
    moduleimport_list_add: fn(head: ref ModuleImport, imp: ref ModuleImport): ref ModuleImport;

    # Symbol table functions for variable validation
    symboltable_create: fn(): ref SymbolTable;
    symboltable_add_var: fn(st: ref SymbolTable, name: string);
    symboltable_has_var: fn(st: ref SymbolTable, name: string): int;
    symboltable_add_module_var: fn(st: ref SymbolTable, name: string);
    symboltable_add_param: fn(st: ref SymbolTable, name: string);
    symboltable_add_import: fn(st: ref SymbolTable, name: string);

    # Watch variable functions
    watchvar_create: fn(name: string): ref WatchVar;
    watchvar_list_add: fn(head: ref WatchVar, wv: ref WatchVar): ref WatchVar;

    # Reactive function functions
    reactivefn_create: fn(name: string, expr: string, interval: int, watch_vars: ref WatchVar): ref ReactiveFunction;
    reactivefn_list_add: fn(head: ref ReactiveFunction, rfn: ref ReactiveFunction): ref ReactiveFunction;

    # Regular function declaration functions
    functiondecl_create: fn(name: string, body: ref Statement): ref FunctionDecl;
    functiondecl_create_with_body: fn(name: string, params: string, body: ref Statement, return_type: string, interval: int): ref FunctionDecl;
    functiondecl_list_add: fn(head: ref FunctionDecl, fn_decl: ref FunctionDecl): ref FunctionDecl;
    reactivebinding_create: fn(widget_path: string, property_name: string, fn_name: string): ref ReactiveBinding;
    reactivebinding_list_add: fn(head: ref ReactiveBinding, binding: ref ReactiveBinding): ref ReactiveBinding;

    # Statement constructor functions
    statement_create_vardecl: fn(lineno: int, var_decl: ref VarDecl): ref Statement;
    statement_create_block: fn(lineno: int, statements: ref Statement): ref Statement;
    statement_create_if: fn(lineno: int, condition: string, then_stmt: ref Statement, else_stmt: ref Statement): ref Statement;
    statement_create_for: fn(lineno: int, init: ref Statement, condition: string, increment: string, body: ref Statement): ref Statement;
    statement_create_while: fn(lineno: int, condition: string, body: ref Statement): ref Statement;
    statement_create_return: fn(lineno: int, expression: string): ref Statement;
    statement_create_expr: fn(lineno: int, expression: string): ref Statement;
    statement_list_add: fn(head: ref Statement, stmt: ref Statement): ref Statement;

    # Widget linking functions
    widget_add_child: fn(parent: ref Widget, child: ref Widget);
    widget_add_property: fn(w: ref Widget, prop: ref Property);
    program_add_const: fn(prog: ref Program, cd: ref ConstDecl);
    program_add_var: fn(prog: ref Program, var: ref VarDecl);
    program_add_code_block: fn(prog: ref Program, code: ref CodeBlock);
    program_add_component: fn(prog: ref Program, comp: ref ComponentDef);
    program_set_app: fn(prog: ref Program, app: ref AppDecl);
    program_add_reactive_fn: fn(prog: ref Program, rfn: ref ReactiveFunction);
    program_add_module_import: fn(prog: ref Program, imp: ref ModuleImport);
    program_add_function_decl: fn(prog: ref Program, fn_decl: ref FunctionDecl);
    program_add_struct_decl: fn(prog: ref Program, decl: ref StructDecl);

    # List building functions
    constdecl_list_add: fn(listhd: ref ConstDecl, item: ref ConstDecl): ref ConstDecl;
    property_list_add: fn(listhd: ref Property, item: ref Property): ref Property;
    widget_list_add: fn(listhd: ref Widget, item: ref Widget): ref Widget;
    var_list_add: fn(listhd: ref VarDecl, item: ref VarDecl): ref VarDecl;
    code_block_list_add: fn(listhd: ref CodeBlock, item: ref CodeBlock): ref CodeBlock;
    component_list_add: fn(listhd: ref ComponentDef, item: ref ComponentDef): ref ComponentDef;
    param_list_add: fn(listhd: ref Param, item: ref Param): ref Param;

    # Value helper functions - safe field access for pick ADT
    value_get_string: fn(v: ref Value): string;
    value_get_number: fn(v: ref Value): big;
    value_get_color: fn(v: ref Value): string;
    value_get_ident: fn(v: ref Value): string;
};
