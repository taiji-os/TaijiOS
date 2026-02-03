# TaijiOS Android Port - Multi-Phase Plan

## Philosophy

Following Inferno's design principle: **TaijiOS should run on any operating system**. Just as we have `emu/Linux/`, `emu/OpenBSD/`, and `emu/9front/`, we will add `emu/Android/` as a first-class platform port.

This is **NOT a rewrite** - it's a platform port, maintaining 100% compatibility with existing Limbo applications and Kryon UI framework.

---

# Overview

```
TaijiOS on Android Architecture:
┌─────────────────────────────────────────────┐
│         Android APK (NativeActivity)        │
│  ┌───────────────────────────────────────┐  │
│  │   emu/Android/ (Platform Layer)       │  │
│  │   - os.c (Android OS integration)     │  │
│  │   - win.c (OpenGL ES graphics)        │  │
│  │   - deveia.c (Input drivers)          │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │   Dis VM (libinterp/)                 │  │
│  │   - ARM64 JIT/interpreter             │  │
│  │   - Garbage collector                │  │
│  │   - Thread scheduler                 │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │   Applications (Limbo/Kryon)          │  │
│  │   - wm.b (Window Manager)             │  │
│  │   - toolbar.b, wmlib.b               │  │
│  │   - Kryon apps (.kry)                │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
         ↓
   OpenGL ES Renderer
   (Hardware Accelerated)
```

---

# Phase 1: Foundation - Platform Directory Structure

## Goal
Create the `emu/Android/` directory structure and basic build integration.

## Tasks

### 1.1 Create Directory Structure
```
emu/Android/
├── os.c              # Android OS abstraction layer
├── os.h              # Platform-specific headers
├── win.c             # OpenGL ES window system
├── win.h             # Window system interface
├── deveia.c          # Input device drivers (touch, keyboard)
├── devfs.c           # File system device layer
├── cmd.c             # Android-specific commands
├── segflush-arm.c    # ARM memory flush
├── segflush-arm64.c  # ARM64 memory flush
├── Android.mk        # NDK build file
├── CMakeLists.txt    # Alternative CMake build
└── README.md         # Android port documentation
```

### 1.2 Initial Build Configuration

**Create `emu/Android/Android.mk`:**
```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libemu
LOCAL_SRC_FILES := os.c win.c deveia.c devfs.c cmd.c
LOCAL_CFLAGS    := -Wall -O2 -DANDROID -D__ANDROID__
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3 -lOpenSLES
include $(BUILD_STATIC_LIBRARY)
```

**Create root-level `android-mkfile`:**
- Wraps NDK build process
- Integrates with existing mk system
- Provides `make android` target

### 1.3 Minimal os.c Implementation

Port essential functions from `emu/Linux/os.c`:
- `ossetclock()`, `osmillisec()` - Time management
- `oshostptr()` - Memory management
- `osyield()`, `ossleep()` - Threading primitives

### 1.4 Verification

**Success Criteria:**
- [ ] Directory structure created
- [ ] Android NDK builds libemu without errors
- [ ] `nm libemu.a` shows exported symbols
- [ ] Can be linked into a test Android binary

**Estimated Time**: 1-2 days

---

# Phase 2: Core OS Abstraction

## Goal
Implement the Android OS integration layer (`os.c`) to provide system services to the Dis VM.

## Tasks

### 2.1 Process Management

**Implement in `os.c`:**
```c
// Process creation and management
int newproc(char *args[]);
void procexit(int val);
void oslongjmp(void *regs, int val);

// Signal handling (Android has limited signals)
void osblocksig(uint sigs);
void osunblocksig(uint sigs);
```

**Android Adaptation:**
- Map POSIX signals to Android's signal subset
- Use pthreads for process emulation
- Handle Android's restrictive signal model

### 2.2 Memory Management

**Port from `emu/Linux/`:**
- `segflush-arm.c` / `segflush-arm64.c`
- Handle Android's ASLR and memory layout
- Implement `osheap()` and `osmsize()`

### 2.3 Threading primitives

**Implement:**
```c
void ossemacquire(Sem *s);
void ossemrelease(Sem *s, int count);
void osyield(void);
```

**Android Approach:**
- Use pthread primitives
- Map to Android's threading model

### 2.4 Time and Timers

**Implement:**
```c
uvlong osfastticks(void);
uvlong osfastticks2ns(uvlong);
void ossleep(int ms);
```

**Android Approach:**
- Use `clock_gettime(CLOCK_MONOTONIC)`
- Handle Android's timer limitations

### 2.5 Verification

**Test Program (`test-android.c`):**
```c
#include "os.h"

void main() {
    print("osfastticks: %llun", osfastticks());
    print("osmillisec: %dn", osmillisec());
    test_threading();
    test_signals();
}
```

**Success Criteria:**
- [ ] All os.c functions implemented
- [ ] Unit tests pass on Android device
- [ ] No crashes in basic operations
- [ ] Memory management verified

**Estimated Time**: 3-5 days

---

# Phase 3: Graphics - OpenGL ES Window System

## Goal
Replace X11 with OpenGL ES 3.0 for hardware-accelerated graphics.

## Tasks

### 3.1 NativeActivity Setup

**Create `emu/Android/android_main.c`:**
```c
#include <android/native_activity.h>

void android_main(struct android_app* app) {
    // Initialize OpenGL ES
    // Start emu main loop
    // Handle Android lifecycle events
}
```

### 3.2 OpenGL ES Initialization

**Implement in `win.c`:**
```c
struct GLESContext {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    GLuint framebuffer;
    GLuint texture;
};

void win_init GLESContext* ctx) {
    // Initialize EGL
    // Create OpenGL ES 3.0 context
    // Setup framebuffer and texture
}

void win_swap(GLESContext* ctx) {
    // Swap buffers
}
```

### 3.3 Drawing Surface Integration

**Map libdraw operations to OpenGL ES:**

| libdraw Operation | OpenGL ES Equivalent |
|-------------------|----------------------|
| `memimage` | GL_TEXTURE_2D |
| `memdraw` | Framebuffer blit |
| `line()` | GL_LINES |
| `poly()` | GL_TRIANGLE_FAN |
| `text()` | Texture atlas + quads |

### 3.4 Input Handling - Touch to Mouse

**Implement in `deveia.c`:**
```c
void handle_touch_event(AInputEvent* event) {
    int action = AMotionEvent_getAction(event);
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    // Map touch to mouse events
    switch(action) {
        case AMOTION_EVENT_ACTION_DOWN:
            mouse_button = 1;
            mouse_pos = (Point){x, y};
            break;
        case AMOTION_EVENT_ACTION_UP:
            mouse_button = 0;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            mouse_pos = (Point){x, y};
            break;
    }

    // Send to WM via channel
    send(mouse_event_chan, &mouse_event);
}
```

### 3.5 Virtual Keyboard Support

**Implement soft keyboard:**
```c
void show_keyboard() {
    JavaVM* vm;
    (*vm)->GetEnv(vm, &env, JNI_VERSION_1_6);
    jclass clazz = (*env)->FindClass(env, "android/view/inputmethod/InputMethodManager");
    // Show IME
}

int32_t handle_input_event(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        // Map Android keycode to Plan 9/Inferno keysym
        return 1;
    }
    return 0;
}
```

### 3.6 Verification

**Test Program (`drawtest.kry`):**
```kryon
App {
    Column {
        Button "Click Me!" onClick @limbo {
            sys->print("Button clicked!n");
        }
        Text "OpenGL ES rendering test" size 24 color "#FF0000"
    }
}
```

**Success Criteria:**
- [ ] OpenGL ES context created successfully
- [ ] Can draw colored rectangles
- [ ] Touch input works as mouse
- [ ] Virtual keyboard shows/hides
- [ ] 60 FPS rendering on target device
- [ ] Kryon button app works

**Estimated Time**: 1-2 weeks

---

# Phase 4: Device Drivers

## Goal
Implement Android device drivers for audio, networking, and storage.

## Tasks

### 4.1 Audio Driver (OpenSLES)

**Create `emu/Android/devaudio.c`:**
```c
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

void audio_init() {
    // Create OpenSLES engine
    // Configure audio output (44.1kHz, stereo, 16-bit)
}

void audio_write(void* buf, int len) {
    // Queue audio buffer
}
```

### 4.2 Network Driver

**Create `emu/Android/devip.c`:**
```c
// Use Android's BSD socket API
int dial(char *host, int port) {
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // Connect
    return fd;
}
```

**Android Permissions:**
```xml
<uses-permission android:name="android.permission.INTERNET" />
```

### 4.3 File System Driver

**Create `emu/Android/devfs.c`:**
- Map to Android's internal storage
- Handle sandbox restrictions
- Support `/mnt/sdcard` for external storage
- Implement permission model mapping

### 4.4 Verification

**Test Components:**
- [ ] Play audio file
- [ ] Network socket connects
- [ ] File read/write works
- [ ] Permissions granted correctly

**Estimated Time**: 1 week

---

# Phase 5: Dis VM Integration

## Goal
Ensure the Dis VM works correctly on Android ARM64.

## Tasks

### 5.1 Verify ARM64 Support

**Check existing ARM64 backend:**
- `libinterp/comp-arm64.c` - Already exists!
- Verify it generates correct code for Android ARM64

### 5.2 Memory Barrier Handling

**Adapt `segflush-arm64.c`:**
```c
void segflush(void *a, uint n) {
    __builtin___clear_cache(a, (char*)a + n);
}
```

### 5.3 JNI Bridge

**Create Java wrapper if needed:**
```java
// TaijiOSActivity.java
public class TaijiOSActivity extends NativeActivity {
    static {
        System.loadLibrary("emu");
        System.loadLibrary("limbo");
    }

    public native void startEmu(String[] args);
}
```

### 5.4 Verification

**Test Program:**
```limbo
implement Test;

include "sys.m";
sys: Sys;

init(nil: ref Draw->Context, nil: list of string) {
    sys = load Sys Sys->PATH;
    sys->print("Dis VM running on Android!n");

    # Test threading
    spawn test_thread();
}

test_thread() {
    sys->print("Thread running!n");
}
```

**Success Criteria:**
- [ ] .dis files execute correctly
- [ ] Threading works
- [ ] Garbage collection works
- [ ] Channel communication works
- [ ] No crashes or memory leaks

**Estimated Time**: 3-5 days

---

# Phase 6: Window Manager Port

## Goal
Get the full WM running on Android.

## Tasks

### 6.1 Verify WM Compatibility

**Test `appl/wm/wm.b`:**
- Should work without modification (uses libdraw)
- Verify window creation/destruction
- Test focus management

### 6.2 Android-Specific WM Adjustments

**Modify if needed:**
- Screen size handling (Android has varying resolutions)
- Orientation changes
- Multi-window support (Android split-screen)

### 6.3 Touch Gesture Support

**Add to WM:**
```limbo
# Touch-specific gestures
swipe(w: ref Window, dx, dy: int) {
    # Handle swipe gestures
}

pinch(w: ref Window, scale: real) {
    # Handle pinch-to-zoom
}
```

### 6.4 Verification

**Test WM:**
- [ ] WM starts successfully
- [ ] Can create windows
- [ ] Window focus works
- [ ] Window stacking works
- [ ] Resize/move with touch

**Estimated Time**: 1 week

---

# Phase 7: Kryon UI Framework

## Goal
Get Kryon apps running on Android.

## Tasks

### 7.1 Verify Kryon Runtime

**Test `module/kryon.m`:**
- Compiler should work unchanged
- Runtime needs OpenGL ES backend (already done in Phase 3)

### 7.2 Touch-Optimized Components

**Add Kryon components:**
```kryon
# New touch-friendly components
Swipeable {
    # Content that responds to swipe gestures
}

PinchZoom {
    # Content that can be zoomed
}
```

### 7.3 Virtual Keyboard Integration

**Modify Kryon text input:**
- Auto-show keyboard on text focus
- Handle IME events
- Support autocomplete

### 7.4 Verification

**Test Apps:**
- [ ] `button.kry` works
- [ ] `hello.kry` works
- [ ] `simple.kry` works
- [ ] Text input works
- [ ] All UI components render correctly

**Estimated Time**: 1 week

---

# Phase 8: APK Packaging

## Goal
Create a distributable APK with proper Android integration.

## Tasks

### 8.1 AndroidManifest.xml

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="org.taijos.os">

    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />

    <application
        android:label="TaijiOS"
        android:hasCode="true"
        android:theme="@android:style/Theme.NoTitleBar.Fullscreen">

        <activity android:name="android.app.NativeActivity"
            android:label="TaijiOS"
            android:configChanges="orientation|keyboardHidden"
            android:launchMode="singleTask">

            <meta-data android:name="android.app.lib_name"
                android:value="taijos" />

            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
```

### 8.2 Asset Packaging

**Package in APK assets:**
- `/dis/*.dis` - All compiled Limbo bytecode
- `/appl/wm/wm.dis` - Window manager
- `/module/*.dis` - Runtime modules
- `/appl/examples/*.kry` - Example apps
- `/fonts/*` - Font files

### 8.3 Gradle Build Integration

**Create `build.gradle`:**
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++17"
                arguments "-DANDROID_STL=c++_shared"
            }
        }
    }
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
        }
    }
}
```

### 8.4 Icons and Branding

**Create:**
- App icons (mdpi, hdpi, xhdpi, xxhdpi, xxxhdpi)
- Splash screen
- Banner for TV

### 8.5 Signing and Release

**Create release build:**
```bash
# Generate signing key
keytool -genkey -v -keystore taijos-release.keystore
  -alias taijos -keyalg RSA -keysize 2048 -validity 10000

# Build release APK
./gradlew assembleRelease

# Sign APK
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1
  -keystore taijos-release.keystore
  app-release-unsigned.apk taijos

# Align APK
zipalign -v 4 app-release-unsigned.apk TaijiOS-release.apk
```

### 8.6 Verification

**Success Criteria:**
- [ ] APK installs on device
- [ ] App launches successfully
- [ ] WM starts automatically
- [ ] All example apps work
- [ ] No permission errors
- [ ] Survives orientation changes
- [ ] Properly handles app lifecycle (pause/resume)

**Estimated Time**: 3-5 days

---

# Phase 9: Testing and Optimization

## Goal
Ensure stability and performance on Android.

## Tasks

### 9.1 Performance Profiling

**Use Android Profiler:**
- CPU usage
- Memory usage
- GPU rendering
- Battery impact

### 9.2 Device Testing

**Test on:**
- [ ] Phone (ARM64)
- [ ] Tablet (different screen size)
- [ ] Android emulator (x86)
- [ ] Various Android versions (8, 10, 12, 14)

### 9.3 Optimization Targets

**Goals:**
- APK size < 50MB
- RAM usage < 200MB
- 60 FPS rendering
- < 2 second cold start
- < 500ms warm start

### 9.4 Known Issues to Address

1. **Software Fallback**: If OpenGL ES unavailable
2. **Low Memory Mode**: For < 2GB devices
3. **Orientation**: Handle rotation gracefully
4. **Multi-Window**: Android split-screen support
5. **Accessibility**: Screen reader support

### 9.5 Verification

**Success Criteria:**
- [ ] Passes all tests on 3+ devices
- [ ] No crashes in 1 hour of testing
- [ ] Meets performance targets
- [ ] Battery drain acceptable

**Estimated Time**: 1-2 weeks

---

# Phase 10: Distribution

## Goal
Make TaijiOS available to users.

## Tasks

### 10.1 F-Droid Publishing

- Prepare build recipe
- Ensure FOSS compliance
- Submit to F-Droid

### 10.2 GitHub Releases

- Tag release version
- Upload APK to releases
- Create installation instructions

### 10.3 Documentation

**Create:**
- User guide
- Developer guide (building from source)
- API documentation for Android-specific features

### 10.4 Verification

**Success Criteria:**
- [ ] Published on F-Droid
- [ ] GitHub release created
- [ ] Documentation complete

**Estimated Time**: 1 week

---

# File Checklist

## New Files to Create

### Platform Layer (emu/Android/)
```
emu/Android/os.c
emu/Android/os.h
emu/Android/win.c
emu/Android/win.h
emu/Android/deveia.c
emu/Android/devaudio.c
emu/Android/devfs.c
emu/Android/devip.c
emu/Android/cmd.c
emu/Android/segflush-arm.c
emu/Android/segflush-arm64.c
emu/Android/Android.mk
emu/Android/CMakeLists.txt
emu/Android/README.md
```

### Build System
```
android-mkfile
build-android.sh
app/src/main/cpp/CMakeLists.txt
app/src/main/AndroidManifest.xml
app/build.gradle
build.gradle
settings.gradle
```

### Java/Kotlin Wrapper
```
app/src/main/java/org/taijos/TaijiOSActivity.java
app/src/main/java/org/taijos/NativeLibraryLoader.java
```

### Resources
```
app/src/main/res/mipmap-*/*.png (icons)
app/src/main/res/values/strings.xml
app/src/main/res/drawable/splash.xml
```

### Documentation
```
android-port/ANDROID.md
android-port/BUILD.md
android-port/TESTING.md
android-port/KNOWN_ISSUES.md
```

## Existing Files to Modify

### Build System
- `mkfile` - Add Android target
- `config/*` - Add Android architecture configs

### Graphics Libraries
- `include/draw.h` - Add OpenGL ES definitions
- `libdraw/` - May need minor adaptations

### Root Configuration
- Add Android-specific configuration options

---

# Risk Assessment

## High Risk Items

| Risk | Impact | Mitigation |
|------|--------|------------|
| OpenGL ES compatibility | Graphics may not render correctly | Test on multiple devices, fallback to software |
| Performance on low-end devices | May be too slow | Implement LOD, optimize drawing |
| Android lifecycle complexity | State loss on rotation | Save/restore VM state |
| NDK build complexity | Build may fail | Use CMake as fallback |
| Input mapping issues | Touch != mouse | Add gesture recognition |

## Medium Risk Items

| Risk | Impact | Mitigation |
|------|--------|------------|
| Memory constraints | OOM crashes | Monitor usage, add limits |
| Permission model | Features blocked | Graceful degradation |
| APK size limits | Too large to distribute | Asset compression |
| Screen size variety | Layout breaks | Responsive design |

## Low Risk Items

- Dis VM compatibility (ARM support exists)
- Limbo apps (platform-independent)
- Build system integration (well-understood)

---

# Success Metrics

## Phase Completion Criteria

- **Phase 1-5**: Can run `emu` on Android via ADB shell
- **Phase 6**: WM displays windows on Android screen
- **Phase 7**: Kryon apps run with touch input
- **Phase 8**: Installable APK works end-to-end
- **Phase 9**: Stable and performant on target devices
- **Phase 10**: Published and available to users

## Overall Success

TaijiOS on Android is successful when:
1. APK installs and launches
2. WM runs at 60 FPS
3. All Kryon examples work
4. Touch input is natural
5. Battery usage is acceptable
6. Works on Android 8+ (ARM64)

---

# Summary

This plan creates a **first-class Android port** of TaijiOS that:
- Runs native ARM64 code via the Dis VM
- Uses hardware-accelerated OpenGL ES graphics
- Supports touch input naturally
- Packages as a standard APK
- Maintains full compatibility with existing Limbo/Kryon apps

**Total Estimated Time**: 8-12 weeks for a full working port.

**Parallel to AppImage Work**: Just as AppImage made TaijiOS portable on Linux desktop, this Android port makes it portable on Android devices.

**Next Step**: Begin Phase 1 by creating the `emu/Android/` directory structure and initial build configuration.
