# Android Port - Quick Start Guide

## First Steps (Phase 1)

The very first thing to do is create the Android platform directory structure and get the build system working.

### Step 1: Create Directories

```bash
cd /mnt/storage/Projects/TaijiOS
mkdir -p emu/Android
mkdir -p android-port/build
```

### Step 2: Copy Reference Files

Use Linux implementation as reference:

```bash
cp emu/Linux/os.c emu/Android/os.c.reference
cp emu/Linux/segflush-arm.c emu/Android/segflush-arm.c
cp emu/Linux/os.h emu/Android/os.h
```

### Step 3: Create Minimal Android.mk

Create `emu/Android/Android.mk`:

```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := emu-android
LOCAL_SRC_FILES := os.c
LOCAL_CFLAGS    := -Wall -O2 -DANDROID -D__ANDROID__ -I../include
LOCAL_LDLIBS    := -llog -landroid
include $(BUILD_STATIC_LIBRARY)
```

### Step 4: Create NDK Standalone Toolchain Test

```bash
# Create NDK toolchain
$NDK/build/tools/make_standalone_toolchain.py
  --arch arm64
  --api 21
  --install-dir ./android-toolchain

# Test compile simple program
$ANDROID_TOOLCHAIN/bin/aarch64-linux-android-gcc
  -DANDROID -c emu/Android/os.c -o test.o
```

### Step 5: Verify ARM Support

The Dis VM already has ARM64 support:

```bash
ls -la libinterp/comp-arm*
# Should show: comp-arm.c comp-arm64.c
```

This is good - means the VM can run on Android ARM64!

### Step 6: Create First Android Test App

Create minimal NativeActivity to test integration:

`android-port/build/test-android.c`:
```c
#include <android/log.h>
#include <android/native_activity.h>

void android_main(struct android_app* state) {
    __android_log_print(ANDROID_LOG_INFO, "TaijiOS",
        "TaijiOS Android port - first boot!");
}
```

Build and deploy via Android Studio or gradle.

---

## Reference: What Works Already

From the exploration, these components should work **without modification** on Android:

1. **Dis VM** - ARM64 backend exists (`libinterp/comp-arm64.c`)
2. **Limbo apps** - All `.b` and `.dis` files are platform-independent
3. **Kryon compiler** - Pure Limbo, will run once VM works
4. **Memory allocation** - Standard C malloc/free works

## What Needs to Be Written

### Must Implement from Scratch:
1. `emu/Android/win.c` - OpenGL ES window system (replaces X11)
2. `emu/Android/deveia.c` - Touch input drivers
3. OpenGL ES drawing backend

### Can Adapt from Linux:
1. `emu/Android/os.c` - Adapt from Linux, change signal handling
2. `emu/Android/devfs.c` - Adapt for Android storage model

### Complexity Ranking:
1. **Easiest**: Build system, os.c adaptation
2. **Medium**: Device drivers, VM integration
3. **Hardest**: Graphics (win.c), touch input

---

## Development Environment Setup

```bash
# Install Android NDK
sudo apt install android-ndk

# Or download from:
# https://developer.android.com/ndk/downloads

# Set environment
export ANDROID_NDK_ROOT=/path/to/ndk
export PATH=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

# Verify
aarch64-linux-android21-clang --version
```

---

## Recommended Starting Order

1. **Build system** first (get something to compile)
2. **os.c** next (get basic OS services)
3. **win.c** last (hardest, requires OpenGL ES)

This way you can test progress incrementally via ADB shell before tackling graphics.
