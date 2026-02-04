# Android Port Testing Checklist

## Build Verification

### NDK Build
```bash
cd /mnt/storage/Projects/TaijiOS/emu/Android
$NDK/ndk-build
```
- [ ] No compilation errors
- [ ] `libemu.a` created
- [ ] All symbols exported

### Gradle/CMake Build
```bash
cd /mnt/storage/Projects/TaijiOS/android-port
./build-android.sh build
```
- [ ] Gradle build succeeds
- [ ] `app-debug.apk` created
- [ ] APK size is reasonable (< 50MB target)

## Installation

### Install to Device
```bash
./build-android.sh install
```
- [ ] APK installs without errors
- [ ] App icon appears in launcher
- [ ] No permission errors

## Basic Functionality

### Launch
- [ ] App launches successfully
- [ ] OpenGL ES context created (check logcat)
- [ ] No crashes on startup
- [ ] Loading screen appears (if any)

### Graphics
- [ ] Screen renders correctly
- [ ] Colors are accurate
- [ ] Text is readable
- [ ] No rendering artifacts

### Touch Input
- [ ] Single touch works
- [ ] Touch position is accurate
- [ ] Button clicks register
- [ ] Dragging works

### Virtual Keyboard
- [ ] Keyboard appears when needed
- [ ] Text input works
- [ ] Keyboard dismisses correctly

## Performance

### Frame Rate
- [ ] 60 FPS on target device
- [ ] No dropped frames
- [ ] Smooth animations

### Memory
- [ ] RAM usage < 200MB
- [ ] No memory leaks
- [ ] Proper GC working

### Battery
- [ ] No excessive battery drain
- [ ] CPU usage reasonable when idle

## Compatibility

### Screen Sizes
- [ ] Works on phone (small screen)
- [ ] Works on tablet (large screen)
- [ ] Orientation changes handled

### Android Versions
- [ ] Android 8.0 (API 26)
- [ ] Android 10 (API 29)
- [ ] Android 12 (API 31)
- [ ] Android 14 (API 34)

### Device Types
- [ ] ARM64 phone
- [ ] ARM64 tablet
- [ ] x86 emulator (if supported)

## Application Tests

### Window Manager
- [ ] `wm.dis` loads
- [ ] Windows create correctly
- [ ] Window focus works
- [ ] Window stacking correct

### Kryon Apps
- [ ] `hello.kry` works
- [ ] `button.kry` works
- [ ] `simple.kry` works
- [ ] Button clicks respond

### Dis VM
- [ ] `.dis` files execute
- [ ] Threading works
- [ ] Garbage collection works
- [ ] Channels work

## Device Drivers

### Audio
- [ ] `audio_init()` succeeds
- [ ] `audio_write()` works
- [ ] Sound plays correctly

### Network
- [ ] Socket creation works
- [ ] DNS resolution works
- [ ] HTTP requests work

### File System
- [ ] File creation works
- [ ] File read/write works
- [ ] Directory listing works
- [ ] Permissions correct

## Logcat Analysis

### Check for Errors
```bash
adb logcat | grep TaijiOS
```

### Expected Output
```
I/TaijiOS ( ####): TaijiOS Android port starting...
I/TaijiOS ( ####): Window ready, initializing file system...
I/TaijiOS ( ####): FS paths: internal=... external=...
I/TaijiOS ( ####): Initializing audio...
I/TaijiOS ( ####): Audio initialized: 44100 Hz, 2 channels
I/TaijiOS ( ####): Initializing Inferno...
I/TaijiOS ( ####): Inferno initialized, entering main loop...
```

### Look For
- No ERROR or FATAL messages
- No native crashes
- Reasonable memory usage

## Optimization Checklist

### Code Size
- [ ] Strip debug symbols
- [ ] Enable compiler optimizations
- [ ] Remove unused code

### Assets
- [ ] Compress textures
- [ ] Optimize fonts
- [ ] Remove unused assets

### Rendering
- [ ] Use texture atlases
- [ ] Batch draw calls
- [ ] Optimize shaders

### Startup Time
- [ ] Cold start < 2 seconds
- [ ] Warm start < 500ms

## Regression Testing

After making changes:
- [ ] Rebuild APK
- [ ] Reinstall on device
- [ ] Run all tests above
- [ ] No new issues introduced

## Known Issues to Track

1. ___
2. ___
3. ___

## Test Results

| Date | Device | Android | Build | Pass/Fail | Notes |
|------|--------|---------|-------|-----------|-------|
|      |        |         |       |           |       |
