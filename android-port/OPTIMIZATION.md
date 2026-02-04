# Android Performance Optimization Guide

## Targets

| Metric | Target | Current | Notes |
|--------|--------|---------|-------|
| APK Size | < 50MB | TBD | Compress assets |
| RAM Usage | < 200MB | TBD | Monitor with profiler |
| Frame Rate | 60 FPS | TBD | Use OpenGL ES efficiently |
| Cold Start | < 2s | TBD | Lazy load modules |
| Warm Start | < 500ms | TBD | Cache in memory |

## Profiling Tools

### Android Profiler
```bash
# In Android Studio: View > Tool Windows > Profiler
# Or standalone:
adb shell am profile start org.taijos.os /data/local/profile.prof
# ... run app ...
adb shell am profile stop org.taijos.os
```

### GPU Profiling
```bash
# Enable GPU profiling
adb shell setprop debug.hwui.profile true
# View in dumpsys
adb shell dumpsys gfxinfo org.taijos.os
```

### Memory Analysis
```bash
# Memory stats
adb shell dumpsys meminfo org.taijos.os

# Heap dump
adb shell am dumpheap org.taijos.os /data/local/heap.hprof
```

## Optimization Strategies

### 1. Graphics Optimization

#### Texture Optimization
```c
// Use compressed textures where possible
glCompressedTexImage2D(GL_TEXTURE_2D, ...);

// Use mipmaps for better quality
glGenerateMipmap(GL_TEXTURE_2D);

// Use texture atlases to reduce draw calls
```

#### Batch Draw Calls
```c
// Group similar draw operations
// Minimize state changes
// Use vertex buffer objects
```

#### Framebuffer
```c
// Only update changed regions
// Use dirty rectangle tracking
// Full redraw only when necessary
```

### 2. Memory Optimization

#### Object Pooling
```c
// Reuse objects instead of allocating
// Implement pools for frequently allocated types
```

#### String Interning
```c
// Share common strings
// Reduce memory footprint
```

#### Limbo Module Caching
```c
// Keep frequently used modules loaded
// Lazy load others
```

### 3. CPU Optimization

#### Reduce Main Loop Work
```c
// Only process active windows
// Skip idle animations
// Use dirty flags
```

#### Optimize Dis Interpreter
```c
// Use JIT for hot paths (when ARM64 JIT implemented)
// Inline common operations
// Cache frequently accessed data
```

### 4. I/O Optimization

#### Asset Loading
```c
// Load assets asynchronously
// Compress assets in APK
// Use native asset manager
```

#### File System
```c
// Cache file reads
// Buffer writes
// Use memory-mapped files for large assets
```

## Platform-Specific Optimizations

### ARM64
```c
// Use NEON instructions for SIMD
// Optimize memory alignment
// Use 64-bit operations
```

### OpenGL ES 3.0
```c
// Use instanced rendering
// Use uniform buffers
// Use compute shaders where applicable
```

## Compiler Optimizations

### CMake Flags
```cmake
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math")
set(CMAKE_C_FLAGS_RELEASE "-flto")  # Link-time optimization
```

### NDK Flags
```makefile
LOCAL_CFLAGS := -O3 -DNDEBUG
LOCAL_LDFLAGS := -Wl,--gc-sections
```

## Build Optimizations

### Strip Symbols
```cmake
# In CMakeLists.txt
set(CMAKE_C_FLAGS_RELEASE "-s")
```

### Remove Unused Code
```bash
# Use linker garbage collection
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all
```

## Startup Optimization

### Deferred Initialization
```c
// Defer non-critical initialization
android_main() {
    // Init critical path only
    // Spawn thread for secondary init
}
```

### Module Lazy Loading
```c
// Load modules on demand
// Keep core modules preloaded
```

## Testing on Low-End Devices

### Device Targets
- 2GB RAM minimum
- ARM64 Cortex-A53 or better
- OpenGL ES 3.0 support

### Performance Levels
```
High-end: 1080p+, 60 FPS, all effects
Mid-end:   720p+,  30 FPS, reduced effects
Low-end:  480p+,  30 FPS, minimal effects
```

## Monitoring in Production

### Stats Collection
```c
// Collect key metrics
// Upload to analytics (with permission)
// Use to identify performance issues
```

### Crash Reporting
```c
// Use Android's crash reporting
// Log native crashes
// Symbolize stack traces
```

## Benchmark Results

### Baseline (Pre-optimization)
| Metric | Value | Date |
|--------|-------|------|
| APK Size | TBD | - |
| RAM | TBD | - |
| FPS | TBD | - |
| Start Time | TBD | - |

### After Optimization
| Metric | Value | Date | Improvement |
|--------|-------|------|-------------|
| APK Size | TBD | - | TBD% |
| RAM | TBD | - | TBD% |
| FPS | TBD | - | TBD% |
| Start Time | TBD | - | TBD% |
