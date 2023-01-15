# PWART

## 
**PWART** is a light-weight JIT runtime for WebAssembly.

PWART use sljit to generate native code, no other dependency. All code is writen in C, include sljit.

PWART support 32bit and 64bit architecture, test only on x86_64, i686 and armv7l, but should also work on aarch64,risc-v etc. thanks to sljit.

Currently, PWART still need more test to improve and verify the stablity. 


about sljit: https://github.com/zherczeg/sljit

## Usage

Currently only support GCC-like toolchain, consider use MinGW on Windows please.

1. set environment variable PWART_SOURCE_ROOT to the project directory.

2. In your Makefile, write
```shell
include $(PWART_SOURCE_ROOT)/pwart/make_config.mk
```

3. Put flags to build your target.
```shell
your_target:build_pwart
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o your_output your_source.c $(PWART_LDFLAGS) $(LDFLAGS)
```

4. In your source, include "pwart.h". simple demo show below.

```C
FILE *f = fopen("test1.wasm", "rb");
uint8_t *data = malloc(1024 * 1024 * 8);
void *sp;
int len = fread(data, 1, 1024 * 1024 * 8, f);
fclose(f);
pwart_module m = pwart_new_module();
struct pwart_inspect_result1 ctxinfo;
pwart_load(m, data, len);
pwart_runtime_context ctx = pwart_get_runtime_context(m);
pwart_inspect_runtime_context(ctx, &ctxinfo);
pwart_wasmfunction test1 = pwart_get_export_function(m, "test1");
pwart_free_module(m);
sp = ctxinfo.runtime_stack;
test1(sp, ctx);
```
See [include/pwart.h](include/pwart.h) , [tests/testmain.c](tests/testmain.c) and [tests/Makefile](tests/Makefile) for detail usage.

## Test

To run the test, you need WAT2WASM to compile wat file. You can get it by compile [wabt](https://github.com/WebAssembly/wabt),Then
```
export PWART_SOURCE_ROOT=<the project directory>
export WAT2WASM=<the executable wat2wasm path>
cd $PWART_SOURCE_ROOT/tests
make
```

## Features


### Implemented

WebAssembly MVP

Sign-extension operators

Multi-value

Non-trapping Float-to-int Conversions (For performance, the behavior depend on the host.)

Bulk Memory Operations(currently only implement memory.copy, memory.fill)

Memory64(support i64 index, offset and align are still 32bit.)


### NOT Implemented

Conditional Segment Initialization

Fixed-width SIMD.

Multiple memories and tables


## Roadmap

1. More test , and fix.

2. Performance optimization.

3. Implement more feature.

4. WASI Support.