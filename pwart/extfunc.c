#ifndef _PWART_EXTFUNC_C
#define _PWART_EXTFUNC_C

#include "def.h"

static void insn_i64eqz(void *fp, RuntimeContext *m) {
  uint32_t *result = fp;
  uint64_t *arg1 = fp;
  *result = *arg1 == 0;
}

static void insn_i64clz(void *fp, RuntimeContext *m) {
  uint64_t x = *(uint64_t *)fp;
  if (x != 0) {
    int c = 63;
    if (x >> 32)
      x >>= 32, c -= 32;
    if (x >> 16)
      x >>= 16, c -= 16;
    if (x >> 8)
      x >>= 8, c -= 8;
    if (x >> 4)
      x >>= 4, c -= 4;
    if (x >> 2)
      x >>= 2, c -= 2;
    *(uint32_t *)fp = c - (x >> 1);
  } else {
    *(uint32_t *)fp = 64;
  }
}
static void insn_i64ctz(void *fp, RuntimeContext *m) {
  uint64_t x = *(uint64_t *)fp;
  if (x != 0) {
    int c = 63;
    if (x << 32)
      x <<= 32, c -= 32;
    if (x << 16)
      x <<= 16, c -= 16;
    if (x << 8)
      x <<= 8, c -= 8;
    if (x << 4)
      x <<= 4, c -= 4;
    if (x << 2)
      x <<= 2, c -= 2;
    if (x << 1)
      c -= 1;
    *(uint32_t *)fp = c;
  } else {
    *(uint32_t *)fp = 64;
  }
}
static void insn_i64popcount(void *fp, RuntimeContext *m) {
  uint64_t x = *(uint64_t *)fp;
  x -= (x & 0xaaaaaaaaaaaaaaaaull) >> 1;
  x = (x & 0x3333333333333333ull) + ((x & 0xccccccccccccccccull) >> 2);
  x = ((x * 0x11) & 0xf0f0f0f0f0f0f0f0ull) >> 4;
  *(uint32_t *)fp = (uint32_t)((x * 0x0101010101010101ull) >> 56);
}

static void insn_i32clz(void *fp, RuntimeContext *m) {
  uint32_t x = *(uint32_t *)fp;
  if (x != 0) {
    int c = 31;
    if (x >> 16)
      x >>= 16, c -= 16;
    if (x >> 8)
      x >>= 8, c -= 8;
    if (x >> 4)
      x >>= 4, c -= 4;
    if (x >> 2)
      x >>= 2, c -= 2;
    *(uint32_t *)fp = c - (x >> 1);
  } else {
    *(uint32_t *)fp = 32;
  }
}
static void insn_i32ctz(void *fp, RuntimeContext *m) {
  uint32_t x = *(uint32_t *)fp;
  if (x != 0) {
    int c = 31;
    if (x << 16)
      x <<= 16, c -= 16;
    if (x << 8)
      x <<= 8, c -= 8;
    if (x << 4)
      x <<= 4, c -= 4;
    if (x << 2)
      x <<= 2, c -= 2;
    if (x << 1)
      c -= 1;
    *(uint32_t *)fp = c;
  } else {
    *(uint32_t *)fp = 32;
  }
}
static void insn_i32popcount(void *fp, RuntimeContext *m) {
  uint32_t x = *(uint32_t *)fp;
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  *(uint32_t *)fp = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

static void insn_i64eq(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 == *arg2;
}
static void insn_i64ne(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 != *arg2;
}
static void insn_i64lts(void *fp, RuntimeContext *m) {
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 < *arg2;
}
static void insn_i64ltu(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 < *arg2;
}
static void insn_i64gts(void *fp, RuntimeContext *m) {
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 > *arg2;
}
static void insn_i64gtu(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 > *arg2;
}
static void insn_i64les(void *fp, RuntimeContext *m) {
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 <= *arg2;
}
static void insn_i64leu(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 <= *arg2;
}
static void insn_i64ges(void *fp, RuntimeContext *m) {
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 >= *arg2;
}
static void insn_i64geu(void *fp, RuntimeContext *m) {
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 >= *arg2;
}

#include <math.h>

static void insn_f32ceil(float *fp, RuntimeContext *m) { *fp = ceilf(*fp); }
static void insn_f32floor(float *fp, RuntimeContext *m) { *fp = floorf(*fp); }
static void insn_f32trunc(float *fp, RuntimeContext *m) { *fp = truncf(*fp); }
static void insn_f32nearest(float *fp, RuntimeContext *m) { *fp = rintf(*fp); }
static void insn_f32sqrt(float *fp, RuntimeContext *m) { *fp = sqrtf(*fp); }

static void insn_f64ceil(double *fp, RuntimeContext *m) { *fp = ceil(*fp); }
static void insn_f64floor(double *fp, RuntimeContext *m) { *fp = floor(*fp); }
static void insn_f64trunc(double *fp, RuntimeContext *m) { *fp = trunc(*fp); }
static void insn_f64nearest(double *fp, RuntimeContext *m) { *fp = rint(*fp); }
static void insn_f64sqrt(double *fp, RuntimeContext *m) { *fp = sqrt(*fp); }

static void insn_i32rotl(uint32_t *fp, RuntimeContext *m) {
  uint32_t x = *fp;
  uint32_t y = *(fp + 1);
  y = y & 31;
  *fp = (x << y) | (x >> (32 - y));
}
static void insn_i32rotr(uint32_t *fp, RuntimeContext *m) {
  uint32_t x = *fp;
  uint32_t y = *(fp + 1);
  y = y & 31;
  *fp = (x >> y) | (x << (32 - y));
}

static void insn_i64rotl(uint64_t *fp, RuntimeContext *m) {
  uint64_t x = *fp;
  uint64_t y = *(fp + 1);
  y = y & 63;
  *fp = (x << y) | (x >> (64 - y));
}
static void insn_i64rotr(uint64_t *fp, RuntimeContext *m) {
  uint64_t x = *fp;
  uint64_t y = *(fp + 1);
  y = y & 63;
  *fp = (x >> y) | (x << (64 - y));
}
static void insn_i64mul(uint64_t *fp, RuntimeContext *m) { *fp = (*fp) * (*(fp + 1)); }
static void insn_i64divs(int64_t *fp, RuntimeContext *m) { *fp = (*fp) / (*(fp + 1)); }
static void insn_i64divu(uint64_t *fp, RuntimeContext *m) { *fp = (*fp) / (*(fp + 1)); }
static void insn_i64rems(int64_t *fp, RuntimeContext *m) { *fp = (*fp) % (*(fp + 1)); }
static void insn_i64remu(uint64_t *fp, RuntimeContext *m) { *fp = (*fp) % (*(fp + 1)); }
static void insn_i64shl(uint64_t *fp, RuntimeContext *m) { *fp = (*fp) << (*(fp + 1)); }
static void insn_i64shrs(int64_t *fp, RuntimeContext *m) { *fp = (*fp) >> (*(fp + 1)); }
static void insn_i64shru(uint64_t *fp, RuntimeContext *m) { *fp = (*fp) >> (*(fp + 1)); }

static void insn_f32max(float *fp, RuntimeContext *m) {
  *fp = (*fp) > *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f32min(float *fp, RuntimeContext *m) {
  *fp = (*fp) < *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f32copysign(float *fp, RuntimeContext *m) {
  *fp = signbit(*fp) ? -fabs(*(fp + 1)) : fabs(*(fp + 1));
}

static void insn_f64max(double *fp, RuntimeContext *m) {
  *fp = (*fp) > *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f64min(double *fp, RuntimeContext *m) {
  *fp = (*fp) < *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f64copysign(double *fp, RuntimeContext *m) {
  *fp = signbit(*fp) ? -fabs(*(fp + 1)) : fabs(*(fp + 1));
}

static void insn_i32truncf32u(uint32_t *fp, RuntimeContext *m) {
  *fp = (uint32_t)(*(float *)fp);
}

static void insn_i32truncf64u(uint32_t *fp, RuntimeContext *m) {
  *fp = (uint32_t)(*(double *)fp);
}

static void insn_i64truncf32s(int64_t *fp, RuntimeContext *m) {
  *fp = (int64_t)(*(float *)fp);
}
static void insn_i64truncf32u(uint64_t *fp, RuntimeContext *m) {
  *fp = (uint64_t)(*(float *)fp);
}

static void insn_i64truncf64s(int64_t *fp, RuntimeContext *m) {
  *fp = (int64_t)(*(double *)fp);
}
static void insn_i64truncf64u(uint64_t *fp, RuntimeContext *m) {
  *fp = (uint64_t)(*(double *)fp);
}

static void insn_f32converti32u(float *fp, RuntimeContext *m) {
  *fp = (float)(*(uint32_t *)fp);
}

static void insn_f32converti64s(float *fp, RuntimeContext *m) {
  *fp = (float)(*(int64_t *)fp);
}
static void insn_f32converti64u(float *fp, RuntimeContext *m) {
  *fp = (float)(*(uint64_t *)fp);
}

static void insn_f64converti32u(double *fp, RuntimeContext *m) {
  *fp = (double)(*(uint32_t *)fp);
}

static void insn_f64converti64s(double *fp, RuntimeContext *m) {
  *fp = (double)(*(int64_t *)fp);
}
static void insn_f64converti64u(double *fp, RuntimeContext *m) {
  *fp = (double)(*(uint64_t *)fp);
}

static void insn_memorygrow(void *fp, RuntimeContext *m) {
  uint32_t prev_pages = m->memory.pages;
  uint32_t *arg1 = (uint32_t *)fp;
  uint32_t delta = *arg1;
  *arg1 = prev_pages;
  if (delta == 0) {
    return; // No change
  } else if (delta + prev_pages > m->memory.maximum) {
    *arg1 = 0xffffffff;
    return;
  }
  m->memory.pages += delta;
  if(m->memory_model==PWART_MEMORY_MODEL_GROW_ENABLED){
    m->memory.bytes = wa_realloc(m->memory.bytes, m->memory.pages * PAGE_SIZE);
  }
  return;
}

static void insn_malloc32(uint32_t *fp,RuntimeContext *m){
  *fp=(uint32_t)(size_t)malloc(*fp);
}
static void insn_malloc64(uint64_t *fp,RuntimeContext *m){
  *fp=(uint64_t)(size_t)malloc(*fp);
}

static uint8_t types_void[] = {0};

static uint8_t types_i32[] = {WVT_I32, 0};
static uint8_t types_i64[] = {WVT_I64, 0};
static uint8_t types_f32[] = {WVT_F32, 0};
static uint8_t types_f64[] = {WVT_F64, 0};

static uint8_t types_i32_i32[] = {WVT_I32, WVT_I32, 0};
static uint8_t types_i64_i64[] = {WVT_I64, WVT_I64, 0};
static uint8_t types_f32_f32[] = {WVT_F32, WVT_F32, 0};
static uint8_t types_f64_f64[] = {WVT_F64, WVT_F64, 0};

static Type func_type_ret_void = {.params = types_void, .results = types_void};
static Type func_type_ret_i32 = {.params = types_void, .results = types_i32};
static Type func_type_ret_i64 = {.params = types_void, .results = types_i64};
static Type func_type_ret_f32 = {.params = types_void, .results = types_i32};
static Type func_type_ret_f64 = {.params = types_void, .results = types_f64};
static Type func_type_i64_ret_i32 = {.params = types_i64, .results = types_i32};
static Type func_type_i64_ret_i64 = {.params = types_i64, .results = types_i64};
static Type func_type_i32_ret_i32 = {.params = types_i32, .results = types_i32};
static Type func_type_i64_i64_ret_i32 = {.params = types_i64_i64,
                                         .results = types_i32};
static Type func_type_f32_ret_f32 = {.params = types_f32, .results = types_f32};
static Type func_type_f64_ret_f64 = {.params = types_f64, .results = types_f64};
static Type func_type_i32_i32_ret_i32 = {.params = types_i32_i32,
                                         .results = types_i32};
static Type func_type_i64_i64_ret_i64 = {.params = types_i64_i64,
                                         .results = types_i64};
static Type func_type_f32_f32_ret_f32 = {.params = types_f32_f32,
                                         .results = types_f32};
static Type func_type_f64_f64_ret_f64 = {.params = types_f64_f64,
                                         .results = types_f64};
static Type func_type_f32_ret_i32 = {.params = types_f32, .results = types_i32};
static Type func_type_f64_ret_i32 = {.params = types_f64, .results = types_i32};
static Type func_type_f32_ret_i64 = {.params = types_f32, .results = types_i64};
static Type func_type_f64_ret_i64 = {.params = types_f64, .results = types_i64};
static Type func_type_i32_ret_f32 = {.params = types_i32, .results = types_f32};
static Type func_type_i64_ret_f32 = {.params = types_i64, .results = types_f32};
static Type func_type_i32_ret_f64 = {.params = types_i32, .results = types_f64};
static Type func_type_i64_ret_f64 = {.params = types_i64, .results = types_f64};

static void waexpr_run_const(Module *m, void *result) {
  int opcode=m->bytes[m->pc];
  m->pc++;
  switch (opcode) {
  case 0x41: // i32.const
    *(uint32_t *)result = read_LEB_signed(m->bytes, &m->pc, 32);
    break;
  case 0x42: // i64.const
    *(uint64_t *)result = read_LEB_signed(m->bytes, &m->pc, 64);
    break;
  case 0x43: // f32.const
    *(float *)result = *(float *)(m->bytes + m->pc);
    m->pc += 4;
    break;
  case 0x44: // f64.const
    *(double *)result = *(double *)(m->bytes + m->pc);
    m->pc += 8;
    break;
  case 0x23: // global.get
  {
    uint32_t arg = read_LEB(m->bytes, &m->pc, 32);
    StackValue *sv = dynarr_get(m->globals, StackValue, arg);
    if (stackvalue_GetSizeAndAlign(sv, NULL) == 8) {
      *(uint64_t *)result = *(uint64_t *)(&m->context->globals->data+sv->val.opw);
    } else {
      *(uint32_t *)result = *(uint32_t *)(&m->context->globals->data+sv->val.opw);
    }
  } break;
  default:
    SLJIT_UNREACHABLE();
    break;
  }
  SLJIT_ASSERT(m->bytes[m->pc]==0xb);
  m->pc++;
}

static void debug_printtypes(char *t){
  for(char *t2=t;*t2!=0;*t2++){
    switch(*t2){
      case WVT_I32:
      wa_debug("i32,");
      break;
      case WVT_I64:
      wa_debug("i64,");
      break;
      case WVT_F32:
      wa_debug("f32,");
      break;
      case WVT_F64:
      wa_debug("f64,");
      break;
      case WVT_FUNC:
      wa_debug("func,");
      break;
      case WVT_REF:
      wa_debug("ref,");
      break;
      default :
      wa_debug("unknown,");
      break;
    }
  }
}

static void debug_printfunctype(Type *type){
  wa_debug("type->form:%x\n",type->form);
  wa_debug("type->params at %p:",type->params);
  debug_printtypes(type->params);
  wa_debug("\n");
  wa_debug("type->result at %p:",type->results);
  debug_printtypes(type->results);
  wa_debug("\n");
}
#endif