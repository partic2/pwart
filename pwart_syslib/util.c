#ifndef _PWART_SYSLIB_UTILS_C
#define _PWART_SYSLIB_UTILS_C

#define _wargref(vname) void *vname=pwart_rstack_get_ref(&sp);
#define _wargi32(vname) uint32_t vname=pwart_rstack_get_i32(&sp);
#define _wargi64(vname) uint64_t vname=pwart_rstack_get_i64(&sp);

//XXX: Can we use pwart internal code?
#include "../pwart/util.c"


#endif