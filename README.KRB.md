# KRB Integration for TaijiOS

This document describes the KRB (Kryon Binary) integration for TaijiOS.

## Overview

KRB is a binary UI serialization format that enables compiled KRY (Kryon) applications to run as native windows in the TaijiOS window manager, completely independent of Limbo and DIS.

## Architecture

```
KRY Source → KRY Parser → KIR (JSON) → KRB Codegen → KRB binary → KRB Runtime
                                                                          ↓
                                                                    TaijiOS WM
```

## Components

### Libraries

1. **libkrb.a** - KRB file parser
   - Location: `/libkrb/`
   - Purpose: Load and parse KRB binary files
   - API: `krb_load()`, `krb_free()`, `krb_find_widget_instance()`, etc.

2. **libkrb_runtime.a** - Widget runtime
   - Location: `/libkrb_runtime/`
   - Purpose: Manage widget tree, state, layout, and events
   - API: `krb_runtime_init()`, `krb_runtime_calculate_layout()`, etc.

3. **libkrb_render.a** - Renderer
   - Location: `/libkrb_render/`
   - Purpose: Render widgets using TaijiOS draw library
   - API: `krb_render_init()`, `krb_render_widget_tree()`, etc.

### Programs

1. **krbclient** - WM client
   - Location: `/utils/krbclient/`
   - Purpose: Run KRB files as WM windows
   - Usage: `krbclient [-t title] [-W width] [-H height] file.krb`

2. **krbexec** - Convenience wrapper
   - Location: `/dis/krbexec`
   - Purpose: Simplified interface to run KRB files
   - Usage: `krbexec file.krb`

## Building

### Build all KRB components:

```sh
mk krb-install
```

### Build individual components:

```sh
# Build libraries
cd libkrb && mk install
cd ../libkrb_runtime && mk install
cd ../libkrb_render && mk install

# Build client
cd ../../utils/krbclient && mk install
```

### Clean:

```sh
mk krb-clean
```

## Usage

### Running KRB applications:

```sh
# Using krbclient directly
krbclient myapp.krb

# With custom title and size
krbclient -t "My App" -W 1024 -H 768 myapp.krb

# Using krbexec wrapper
krbexec myapp.krb
```

### From the WM:

```sh
# If WM has .krb file association
wm myapp.krb
```

## KRB File Format

KRB files contain:
- Header (magic, version, section offsets)
- String table
- Widget definitions
- Widget instances
- Styles
- Themes
- Properties
- Events
- Scripts (not yet executed)

Magic number: `0x4B52594E` ("KRYN")

## Widget Types

Supported widget types:
- `Container` - Basic container widget
- `Column` - Vertical layout
- `Row` - Horizontal layout
- `Text` - Text display
- `Button` - Clickable button
- `Image` - Image display (placeholder)

## Features

### Implemented:
- ✅ KRB file parsing
- ✅ Widget tree construction
- ✅ Layout (Column, Row, Container)
- ✅ Basic rendering
- ✅ Color resolution
- ✅ Dimension resolution
- ✅ Hit testing
- ✅ Event registration (not yet executed)

### Not yet implemented:
- ⏳ Style inheritance
- ⏳ Theme variable resolution
- ⏳ Script execution
- ⏳ Event handler execution
- ⏳ Advanced text features (wrapping, ellipsis)
- ⏳ Image loading and rendering
- ⏳ Animations
- ⏳ Accessibility

## Example KRB Application

```kry
// simple.kry
export default function() {
    return Column({
        style: {
            background: "#ffffff",
            padding: 20,
        },
        children: [
            Text({
                text: "Hello, World!",
                style: {
                    fontSize: 24,
                    color: "#000000",
                }
            }),
            Button({
                text: "Click me",
                onClick: () => {
                    console.log("Button clicked!");
                }
            })
        ]
    });
}
```

Compile and run:
```sh
kryon build simple.kry -o simple.krb
krbexec simple.krb
```

## Development

### Adding new widget types:

1. Update `KrbWidgetDefinition` in `libkrb/krb_types.h`
2. Add renderer in `libkrb_render/widget_renderers.c`
3. Add layout logic in `libkrb_runtime/layout_engine.c` if needed

### Adding new properties:

1. Update `KrbWidget` in `include/krb_runtime.h`
2. Add property resolver in `libkrb_runtime/property_resolver.c`
3. Apply in renderer if needed

### Adding new event types:

1. Update `KrbEvent` in `libkrb/krb_types.h`
2. Add to `KrbWidget.events` in `include/krb_runtime.h`
3. Handle in `utils/krbclient/client.c`

## Testing

Run unit tests:
```sh
cd libkrb/test && mk test
```

Run integration test:
```sh
# Create a simple KRB file and run it
krbexec test_simple.krb
```

## Troubleshooting

### Build errors:
- Ensure all libraries are built in order: libkrb → libkrb_runtime → libkrb_render → krbclient
- Check that `$ROOT` and `$OBJDIR` are set correctly

### Runtime errors:
- Check that KRB file is valid: `file.krb` should start with magic number
- Ensure display is initialized
- Check WM is running

## Future Work

1. **Script Execution**: Integrate Lua or QBE for event handlers
2. **JIT Compilation**: Compile KRB to native code at load time
3. **Hot Reload**: Reload KRB files without restart
4. **Debugger**: Visual inspector for KRB apps
5. **Internationalization**: RTL text, locale support
6. **Accessibility**: Screen reader support
7. **Performance**: Dirty region tracking, layout caching

## References

- KRB Format Specification: See Kryon documentation
- TaijiOS WM Protocol: See `/appl/wm/`
- libdraw API: See `/libdraw/`

## License

Same as TaijiOS.
