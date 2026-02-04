# TaijiOS Android Port

This directory contains the Android platform implementation for TaijiOS, following Inferno's philosophy of running on any operating system.

## Directory Structure

```
emu/Android/
├── os.c                 # Android OS abstraction layer
├── asm-arm64.S          # ARM64 assembly support
├── segflush-arm64.c     # ARM64 cache flush (for JIT)
├── mkfile               # Build configuration for mk
├── mkfile-arm64         # ARM64-specific build rules
├── Android.mk           # NDK build file (ndk-build)
├── CMakeLists.txt       # CMake build file (Android Studio)
├── emu-g.c              # Graphics emu configuration
├── emu-g.root.h         # Root filesystem configuration
├── emu.root.h           # Root filesystem for emu
└── README.md            # This file
```

## Building

### Using mk (native build)

```bash
cd /mnt/storage/Projects/TaijiOS
mk 'SYSTARG=Android' 'OBJTYPE=arm64'
```

### Using Android NDK

```bash
export ANDROID_NDK_ROOT=/path/to/ndk
cd emu/Android
$ANDROID_NDK_ROOT/ndk-build
```

### Using CMake (Android Studio)

```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_NATIVE_API_LEVEL=21 \
      ..
make
```

## Platform Notes

### Android-Specific Adaptations

1. **Signal Handling**: Android has a more restricted signal model than Linux. We use pthread primitives for inter-process signaling.

2. **Graphics**: Uses OpenGL ES 3.0 instead of X11. Window system will be implemented in `win.c`.

3. **Input**: Touch events are mapped to mouse events for compatibility with existing applications.

4. **Storage**: Uses Android's internal storage model with sandbox restrictions.

### ARM64 Support

The Dis VM already has ARM64 support (`libinterp/comp-arm64.c`), so the JIT compiler works out of the box. The `segflush-arm64.c` file handles instruction cache flushing for dynamically generated code.

### Thread Model

Uses pthreads with `USE_PTHREADS` defined, similar to the Linux port. Each Inferno process is a pthread.

## Status

### Phase 1: Foundation (In Progress)
- [x] Directory structure created
- [x] Basic os.c implementation
- [x] ARM64 assembly support
- [x] NDK/CMake build files
- [ ] Full build integration

### Phase 2: OS Abstraction (Pending)
- [ ] Complete os.c implementation
- [ ] Memory management
- [ ] Threading primitives
- [ ] Time functions

### Phase 3: Graphics (Pending)
- [ ] win.c with OpenGL ES
- [ ] EGL context management
- [ ] Drawing surface integration

### Phase 4: Device Drivers (Pending)
- [ ] Audio (OpenSLES)
- [ ] Network (BSD sockets)
- [ ] File system

## Dependencies

- Android NDK r21 or later
- OpenGL ES 3.0
- Android API level 21+ (Android 5.0 Lollipop)

## References

- `emu/Linux/` - Linux implementation (primary reference)
- `emu/9front/` - Plan 9 port for additional reference
- `libinterp/comp-arm64.c` - ARM64 JIT compiler
- `android-port/README.md` - Full implementation plan

## Contributing

When adding new features:

1. Check `emu/Linux/` for the Linux implementation
2. Adapt for Android's restrictions
3. Use pthread primitives instead of Linux-specific syscalls
4. Test with Android NDK toolchain
