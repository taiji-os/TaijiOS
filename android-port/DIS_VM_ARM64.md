# Dis VM ARM64 Support for Android

## Current Status

The TaijiOS Dis VM currently has JIT compiler support for:
- 386/amd64 (x86/x86_64)
- ARM (32-bit) - `libinterp/comp-arm.c`
- PowerPC
- MIPS
- SPARC

**ARM64 (aarch64) JIT compiler does NOT exist yet.**

## How Dis Works

The Dis VM (Dis bytecode interpreter) has two modes:
1. **JIT mode**: Compiles Dis bytecode to native machine code for fast execution
2. **Interpreter mode**: Executes Dis bytecode directly (slower but portable)

## Android ARM64 Strategy

For the initial Android port, we use **interpreter mode** which works on any architecture.

### Option 1: Interpreter Mode (Current - Works)
- No JIT compilation
- Portable to any architecture
- Slower execution (3-10x slower than JIT)
- Works immediately

### Option 2: ARM64 JIT Compiler (Future - Faster)
- Need to create `libinterp/comp-arm64.c`
- Need to create `libinterp/das-arm64.c` (disassembler)
- 10-100x faster than interpreter
- Requires significant development work

## Build Configuration

In the mkfile and CMakeLists.txt, set:
```
OBJTYPE=arm64
```

The Dis VM will automatically fall back to interpreter mode if `comp-arm64.$O` doesn't exist.

## Implementing ARM64 JIT (Future Work)

If implementing `libinterp/comp-arm64.c`:

1. **Register mapping** - ARM64 has 31 general-purpose registers (x0-x30)
2. **Instruction encoding** - AArch64 instruction format
3. **Calling convention** - Follow ARM64 AAPCS64
4. **Floating point** - 32 SIMD/FP registers (v0-v31)

Reference existing `comp-arm.c` (32-bit ARM) as a template.

## Testing

To verify the Dis VM works on Android ARM64:

1. Build with `OBJTYPE=arm64`
2. Run a simple Dis program: `echo "print(42);" | dis`
3. Check that it produces correct output

## Files to Check

- `libinterp/mkfile` - Compiler selection
- `libinterp/comp-arm.c` - ARM 32-bit JIT (reference)
- `libinterp/dis.c` - Core interpreter (always works)
- `emu/Android/segflush-arm64.c` - Cache flush for JIT (when implemented)
