# PWART

## 
**PWART** is a light-weight JIT runtime for WebAssembly.

PWART use sljit to generate native code, no other dependency. All code is writen in C, include sljit.

PWART support 32bit and 64bit architecture, test only on x86_64, i686 ,aarch64 and armv7l, but should also work on risc-v etc. thanks to sljit.

Currently, PWART still need more test to improve and verify the stablity. 


about sljit: https://github.com/zherczeg/sljit

## Usage

Currently only support GCC-like toolchain, consider use MinGW on Windows please.

### CMake mode
```shell
cmake -S . -B build
```

```shell
python deps/pull_deps.py
cmake -S . -B build -DPWART_SYSLIB_ENABLED=ON
```

### Makefile mode

(*Makefile mode do not support to build pwart_syslib component yet.*) 

1. Set environment variable PWART_SOURCE_DIR to the project directory.

2. In your Makefile, write
```shell
include $(PWART_SOURCE_DIR)/pwart/make_config.mk
```

3. Put flags to build your target.
```shell
your_target:build_pwart
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o your_output your_source.c $(PWART_LDFLAGS) $(LDFLAGS)
```

4. In your source, include "pwart.h". A simple demo show below.

```C
void *stackbase = pwart_allocate_stack(64 * 1024);
pwart_module_state ctx=pwart_load_module(data,len,&err);
if(ctx==NULL){printf("load module failed:%s\n",err);return 0;};
pwart_wasmfunction test1 = pwart_get_export_function(ctx, "test1");
pwart_call_wasm_function(test1,stackbase);
...
```

See [include/pwart.h](include/pwart.h) , [tests/testmain.c](tests/testmain.c)  [tests/testmain2.c](tests/testmain2.c)
and [tests/Makefile](tests/Makefile) for detail usage.

## SYSLIB and WASI Support

There are experimental Runtime Core Library(SYSLIB) and WASI Support, can be enabled by `-DPWART_SYSLIB_ENABLED=ON` in CMake. And you need to pull the dependencies ([libuv](https://github.com/libuv/libuv), [libffi with cmake(optional)](https://github.com/partic2/libffi), [uvwasi](https://github.com/nodejs/uvwasi)) by using `python deps/pull_deps`.py*

The `pwart_wasiexec` can be used to execute a WASI module, and you can also use `libpwart_syslib.a` to embed the library. See also [include/pwart_syslib.h](include/pwart_syslib.h) 


## Test

To run the test, you need WAT2WASM to compile wat file. You can get it by compile [wabt](https://github.com/WebAssembly/wabt),Then
```
export PWART_SOURCE_DIR=<the project directory>
export WAT2WASM=<the executable wat2wasm path>
cd $PWART_SOURCE_DIR/tests
make
```
or , If you use cmake
```
cmake -S . -B build -DBUILD_TESTING=ON -DPWART_SYSLIB_ENABLED=ON
cd build
make test
```

## Implemented

WebAssembly MVP

Import/Export of Mutable Globals

Sign-extension operators

Multi-value

Non-trapping Float-to-int Conversions (For performance, the behavior depend on the host.)

Bulk Memory Operations(Conditional Segment Initialization is not supported yet.)

Memory64(support i64 index, offset and align are still 32bit.)

Reference types

Multiple memories

Extended Constant Expressions

Tail Call

Simple namespace support

## NOT Implemented

Fixed-width SIMD.


## Benchmarks

The source code are here [tests/benchsort.c](tests/benchsort.c).

Until May 18, 2023

Time Consumed

| | GCC (-O2) | PWART (fixed memory) | PWART (dynamic memory) | PWART (native memory) | TinyCC | V8(Chrome v113) |
| ---- | ----: | ----: | ----: | ----: | ----: | ----: |
| Windows10 x86_64 | 2823ms | 3622ms | 2720ms | Not test yet | 6330ms | 2618ms |
| Linux aarch64 | 1561ms | 1997ms | 2079ms | Not test yet | 7681ms | 1465ms |

**fixed memory** mean wasm module use memory with maximum limit size (expected to be faster). **native memory** mean memory is mapped to host memory directly, This feature is not supported by any toolchain, So we need write .wat by hand before testing.

See [include/pwart.h](include/pwart.h) for detail.


## Roadmap

1. More test , and fix.

2. Performance optimization.

3. More libuv and libffi binding.
