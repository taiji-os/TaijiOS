# Kryon UI Framework Android Integration

## Status: Kryon should work without modification

Kryon is a **declarative UI language** that compiles to Limbo. The compiler (`module/kryon.m`) is pure Limbo, which is platform-independent.

## How Kryon Works

1. **Compilation**: `.kry` files are compiled to Limbo (`.b`) by the Kryon compiler
2. **Execution**: Compiled Limbo code runs on the Dis VM
3. **Graphics**: Uses `draw.m` for rendering (we provide via OpenGL ES)

## Kryon Example Apps

The following example apps should work on Android:

| File | Description |
|------|-------------|
| `hello.kry` | Simple "Hello World" button |
| `button.kry` | Multiple buttons with handlers |
| `simple.kry` | Layout demonstration |

## Android-Specific Considerations

### Touch Input
Kryon Button clicks work through touch events:
1. Touch on button → `deveia.c` maps to mouse event
2. Mouse event → Tk widget click handler
3. Handler executes (Limbo code in Dis VM)

### Screen Size
Android devices have smaller screens. Consider:
- Reduce `width` and `height` in Kryon apps
- Use responsive layouts
- Test on both phones and tablets

### Virtual Keyboard
For text input widgets:
```kryon
TextField {
    placeholder = "Enter text"
    onFocus = showKeyboard  // Call android_show_keyboard()
}
```

### Performance
OpenGL ES hardware acceleration should provide smooth 60 FPS rendering.

## Testing Kryon on Android

### 1. Compile Kryon Examples
```bash
cd appl/examples
../../utils/kryonc hello.kry > hello.b
../../utils/kryonc button.kry > button.b
../../utils/kryonc simple.kry > simple.b
```

### 2. Compile to Dis Bytecode
```bash
limbo hello.b
limbo button.b
limbo simple.b
```

### 3. Package into APK
Add `.dis` files to APK assets:
```
assets/
  dis/
    hello.dis
    button.dis
    simple.dis
```

### 4. Load from android_main.c
```c
// After libinit("emu-g")
loadmodule("hello.dis");
```

### 5. Verify
- [ ] App displays correctly
- [ ] Buttons respond to touch
- [ ] Text is readable
- [ ] Layout fits on screen

## Future Enhancements

### Touch-Optimized Components
```kryon
// New components for Android
Swipeable {
    content = ...  // Content that can be swiped
    onSwipe = handler
}

PinchZoom {
    content = ...  // Content that can be zoomed
    minScale = 0.5
    maxScale = 3.0
}

ViewPager {
    pages = [...]  // Swipeable pages
}
```

### Android-Specific Features
```kryon
// Android back button handling
onBackPressed = handler

// Android lifecycle
onResume = handler
onPause = handler

// Permissions
requestPermission = "android.permission.INTERNET"
```

## Files Involved

- `module/kryon.m` - Kryon compiler (Limbo, no changes needed)
- `appl/examples/*.kry` - Example Kryon apps
- `emu/Android/win.c` - OpenGL ES graphics backend
- `emu/Android/deveia.c` - Touch input driver
- `libdraw/` - Drawing library
- `libtk/` - Toolkit library

## Known Limitations

1. **Small screens**: May need to adjust layout sizes
2. **Text input**: Virtual keyboard integration needs testing
3. **Gestures**: Only basic touch, no multi-touch gestures yet
4. **Performance**: Needs optimization for low-end devices

## Next Steps

1. Build example `.dis` files
2. Test on device
3. Adjust sizes for mobile screens
4. Add touch gesture support if needed
