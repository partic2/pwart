# PWART

## 
**PWART** is a light-weight JIT runtime for WebAssembly.

PWART use sljit to generate native code, no other dependency. All code is writen in C, include sljit.

PWART support 32bit and 64bit architecture, test only on x86_64, i686 and armv7l, but should also work on aarch64,risc-v etc. thanks to sljit.

Currently, PWART still need more test to improve and verify the stablity. 


about sljit: https://github.com/zherczeg/sljit

## Usage

Currently only support GCC-like toolchain, consider use MinGW on Windows please.

### CMake mode
```shell
cmake -S . -B build
```
```shell
cmake -S . -B build -DPWART_SYSLIB_ENABLED=ON
```

### Makefile mode

(*Makefile mode do not support to build pwart_syslib component yet.*) 

1. Set environment variable PWART_SOURCE_ROOT to the project directory.

2. In your Makefile, write
```shell
include $(PWART_SOURCE_ROOT)/pwart/make_config.mk
```

1. Put flags to build your target.
```shell
your_target:build_pwart
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o your_output your_source.c $(PWART_LDFLAGS) $(LDFLAGS)
```

1. In your source, include "pwart.h". A simple demo show below.

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

## Test

To run the test, you need WAT2WASM to compile wat file. You can get it by compile [wabt](https://github.com/WebAssembly/wabt),Then
```
export PWART_SOURCE_ROOT=<the project directory>
export WAT2WASM=<the executable wat2wasm path>
cd $PWART_SOURCE_ROOT/tests
make
```
or , If you use cmake
```
cmake -S . -B build -DBUILD_TESTING=ON
cd build
make test
```


## Implemented

WebAssembly MVP

Import/Export of Mutable Globals

Sign-extension operators

Multi-value

Non-trapping Float-to-int Conversions (For performance, the behavior depend on the host.)

Bulk Memory Operations(Conditional Segment Initialization is not support yet.)

Memory64(support i64 index, offset and align are still 32bit.)

Reference types

Multiple memories

Simple namespace support

## NOT Implemented

Fixed-width SIMD.

## Roadmap

1. More test , and fix.

2. Performance optimization.

3. libuv and libffi binding.

4. WASI Support.