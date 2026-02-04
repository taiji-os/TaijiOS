# Window Manager (WM) Android Integration

## Status: The WM should work without modification

The Window Manager (`appl/wm/wm.b`) is written in **Limbo**, which is platform-independent. It uses:
- `draw.m` - Drawing library (we provide via OpenGL ES in `win.c`)
- `tk.m` - Toolkit library
- `wmsrv.m` - Window manager server
- `wmclient.m` - Window manager client

## What Works

Since `win.c` provides the standard `libdraw` interface using OpenGL ES:
- Window creation (`wmclient->window()`)
- Screen drawing (`makescreen()`)
- Input handling (`win.startinput("kbd" :: "ptr" :: nil)`)
- Touch events are mapped to mouse events in `deveia.c`

## Android-Specific Considerations

### Screen Size
Android devices have varying screen sizes. The WM should auto-detect:
```limbo
win := wmclient->window(ctxt, "Wm", buts);
```

The window size comes from the OpenGL ES context which we set in `win_init()`.

### Touch Gestures
Single touch is mapped to mouse events in `deveia.c`:
- ACTION_DOWN → mouse button 1 press
- ACTION_UP → mouse button 1 release
- ACTION_MOVE → mouse move

For multi-touch gestures (swipe, pinch), future enhancements would be needed.

### Orientation
Android orientation changes trigger `APP_CMD_CONFIG_CHANGED` in `android_main.c`:
```c
case APP_CMD_CONFIG_CHANGED:
    int32_t width = ANativeWindow_getWidth(app->window);
    int32_t height = ANativeWindow_getHeight(app->window);
    win_resize(width, height);
```

The WM will receive the new screen size through `libdraw`.

### Virtual Keyboard
The `android_show_keyboard()` function in `deveia.c` can show/hide the IME.

## Testing

To verify WM works on Android:

1. **Compile WM to Dis bytecode:**
   ```bash
   cd appl/wm
   mk wm.dis
   ```

2. **Package into APK:**
   Add `wm.dis` to APK assets

3. **Load from android_main.c:**
   ```c
   // After libinit("emu-g");
   loadmodule("wm.dis");
   ```

4. **Verify:**
   - Window appears
   - Touch works as mouse
   - Keyboard input works
   - Orientation change is handled

## Known Limitations

1. **Multi-touch**: Only single touch mapped to mouse
2. **Gestures**: No pinch-to-zoom, swipe (future work)
3. **Keyboard**: Virtual keyboard works but needs testing
4. **Screen size**: WM may need adjustment for small screens

## Files Involved

- `appl/wm/wm.b` - Main WM (Limbo, no changes needed)
- `emu/Android/win.c` - OpenGL ES graphics backend
- `emu/Android/deveia.c` - Touch input driver
- `libdraw/` - Drawing library (unchanged)

## Next Steps

1. Build `wm.dis` bytecode
2. Package into APK assets
3. Test on device
4. Add touch gesture support if needed
