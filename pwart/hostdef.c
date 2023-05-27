#ifndef _PWART_HOSTDEF_C
#define _PWART_HOSTDEF_C

#include <stddef.h>

#define __tostrInternal(s) #s
#define tostr(s) __tostrInternal(s)
static char *host_definition[]={
  #ifdef __linux__
  "__linux__",
  #endif
  #ifdef _WIN32
  "_WIN32",
  #endif
  #ifdef _WIN64
  "_WIN64",
  #endif
  #ifdef __x86__
  "__x86__",
  #endif
  #ifdef __x86_64__
  "__x86_64__",
  #endif
  #ifdef __ARM_ARCH
  "__ARM_ARCH="tostr(__ARM_ARCH),
  #endif
  #ifdef __riscv_xlen
  "__riscv_xlen="tostr(__riscv_xlen),
  #endif
  #ifdef __arm__
  "__arm__",
  #endif
  #ifdef __aarch64__
  "__aarch64__",
  #endif
  #ifdef __ANDROID__
  "__ANDROID__",
  #endif
  #ifdef __ANDROID_API__
  "__ANDROID_API__="tostr(__ANDROID_API__),
  #endif
  #ifdef __SOFTFP__
  "__SOFTFP__",
  #endif
  #ifdef __LITTLE_ENDIAN__
  "__LITTLE_ENDIAN__",
  #endif
  #ifdef __MIPSEL__
  "__MIPSEL__",
  #endif
  #ifdef __NetBSD__
  "__NetBSD__",
  #endif
  #ifdef __APPLE__
  "__APPLE__",
  #endif
  NULL
};

#undef tostr

#endif