#ifndef _PWART_EXTFUNC_C
#define _PWART_EXTFUNC_C

#include <stdlib.h>
#include "def.h"

static void insn_i64eqz(void *fp){
  uint32_t *result = fp;
  uint64_t *arg1 = fp;
  *result = *arg1 == 0;
}

static void insn_i64clz(void *fp){
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
static void insn_i64ctz(void *fp){
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
static void insn_i64popcount(void *fp){
  uint64_t x = *(uint64_t *)fp;
  x -= (x & 0xaaaaaaaaaaaaaaaaull) >> 1;
  x = (x & 0x3333333333333333ull) + ((x & 0xccccccccccccccccull) >> 2);
  x = ((x * 0x11) & 0xf0f0f0f0f0f0f0f0ull) >> 4;
  *(uint32_t *)fp = (uint32_t)((x * 0x0101010101010101ull) >> 56);
}

static void insn_i32clz(void *fp){
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
static void insn_i32ctz(void *fp){
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
static void insn_i32popcount(void *fp){
  uint32_t x = *(uint32_t *)fp;
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  *(uint32_t *)fp = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

static void insn_i64eq(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 == *arg2;
}
static void insn_i64ne(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 != *arg2;
}
static void insn_i64lts(void *fp){
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 < *arg2;
}
static void insn_i64ltu(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 < *arg2;
}
static void insn_i64gts(void *fp){
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 > *arg2;
}
static void insn_i64gtu(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 > *arg2;
}
static void insn_i64les(void *fp){
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 <= *arg2;
}
static void insn_i64leu(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 <= *arg2;
}
static void insn_i64ges(void *fp){
  int64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 >= *arg2;
}
static void insn_i64geu(void *fp){
  uint64_t *arg1, *arg2;
  uint32_t *result=fp;
  arg1 = fp;
  arg2 = arg1 + 1;
  *result = *arg1 >= *arg2;
}

#include <math.h>

static void insn_f32ceil(float *fp){ *fp = ceilf(*fp); }
static void insn_f32floor(float *fp){ *fp = floorf(*fp); }
static void insn_f32trunc(float *fp){ *fp = truncf(*fp); }
static void insn_f32nearest(float *fp){ *fp = rintf(*fp); }
static void insn_f32sqrt(float *fp){ *fp = sqrtf(*fp); }

static void insn_f64ceil(double *fp){ *fp = ceil(*fp); }
static void insn_f64floor(double *fp){ *fp = floor(*fp); }
static void insn_f64trunc(double *fp){ *fp = trunc(*fp); }
static void insn_f64nearest(double *fp){ *fp = rint(*fp); }
static void insn_f64sqrt(double *fp){ *fp = sqrt(*fp); }

static void insn_i32rotl(uint32_t *fp){
  uint32_t x = *fp;
  uint32_t y = *(fp + 1);
  y = y & 31;
  *fp = (x << y) | (x >> (32 - y));
}
static void insn_i32rotr(uint32_t *fp){
  uint32_t x = *fp;
  uint32_t y = *(fp + 1);
  y = y & 31;
  *fp = (x >> y) | (x << (32 - y));
}

static void insn_i64rotl(uint64_t *fp){
  uint64_t x = *fp;
  uint64_t y = *(fp + 1);
  y = y & 63;
  *fp = (x << y) | (x >> (64 - y));
}
static void insn_i64rotr(uint64_t *fp){
  uint64_t x = *fp;
  uint64_t y = *(fp + 1);
  y = y & 63;
  *fp = (x >> y) | (x << (64 - y));
}
static void insn_i64mul(uint64_t *fp){ *fp = (*fp) * (*(fp + 1)); }
static void insn_i64divs(int64_t *fp){ *fp = (*fp) / (*(fp + 1)); }
static void insn_i64divu(uint64_t *fp){ *fp = (*fp) / (*(fp + 1)); }
static void insn_i64rems(int64_t *fp){ *fp = (*fp) % (*(fp + 1)); }
static void insn_i64remu(uint64_t *fp){ *fp = (*fp) % (*(fp + 1)); }
static void insn_i64shl(uint64_t *fp){ *fp = (*fp) << (*(fp + 1)); }
static void insn_i64shrs(int64_t *fp){ *fp = (*fp) >> (*(fp + 1)); }
static void insn_i64shru(uint64_t *fp){ *fp = (*fp) >> (*(fp + 1)); }

static void insn_f32max(float *fp){
  *fp = (*fp) > *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f32min(float *fp){
  *fp = (*fp) < *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f32copysign(float *fp){
  *fp = signbit(*fp) ? -fabs(*(fp + 1)) : fabs(*(fp + 1));
}

static void insn_f64max(double *fp){
  *fp = (*fp) > *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f64min(double *fp){
  *fp = (*fp) < *(fp + 1) ? (*fp) : *(fp + 1);
}
static void insn_f64copysign(double *fp){
  *fp = signbit(*fp) ? -fabs(*(fp + 1)) : fabs(*(fp + 1));
}

static void insn_i32truncf32u(uint32_t *fp){
  *fp = (uint32_t)(*(float *)fp);
}

static void insn_i32truncf64u(uint32_t *fp){
  *fp = (uint32_t)(*(double *)fp);
}

static void insn_i64truncf32s(int64_t *fp){
  *fp = (int64_t)(*(float *)fp);
}
static void insn_i64truncf32u(uint64_t *fp){
  *fp = (uint64_t)(*(float *)fp);
}

static void insn_i64truncf64s(int64_t *fp){
  *fp = (int64_t)(*(double *)fp);
}
static void insn_i64truncf64u(uint64_t *fp){
  *fp = (uint64_t)(*(double *)fp);
}

static void insn_f32converti32u(float *fp){
  *fp = (float)(*(uint32_t *)fp);
}

static void insn_f32converti64s(float *fp){
  *fp = (float)(*(int64_t *)fp);
}
static void insn_f32converti64u(float *fp){
  *fp = (float)(*(uint64_t *)fp);
}

static void insn_f64converti32u(double *fp){
  *fp = (double)(*(uint32_t *)fp);
}

static void insn_f64converti64s(double *fp){
  *fp = (double)(*(int64_t *)fp);
}
static void insn_f64converti64u(double *fp){
  *fp = (double)(*(uint64_t *)fp);
}
// keep argument aligned, to avoid effect from PWART_STACK_FLAGS_AUTO_ALIGN.
// (i32 delta, i32 memory_index,ref runtime_context)
static void insn_memorygrow(void *fp){
  void *sp=fp;
  uint32_t delta = pwart_rstack_get_i32(&sp);
  uint32_t index=pwart_rstack_get_i32(&sp); // memory index
  RuntimeContext *m=pwart_rstack_get_ref(&sp);
  Memory *mem=*dynarr_get(m->memories,Memory *,index);
  uint32_t prev_pages = mem->pages;
  if (delta == 0) {
    return; // No change
  } else if (delta + prev_pages > mem->maximum) {
    prev_pages = 0xffffffff;
    return;
  }
  mem->pages += delta;
  if(mem->fixed!=0){
    mem->bytes = wa_realloc(mem->bytes, mem->pages * PAGE_SIZE);
  }
  sp=fp;
  pwart_rstack_put_i32(&sp,prev_pages);
  return;
}
//get pwart version
static void insn_version(uint32_t *fp){
  *fp=pwart_get_version();
}
//allocate memory
static void insn_malloc32(uint32_t *fp){
  *fp=(uint32_t)(size_t)malloc(*fp);
}
static void insn_malloc64(uint64_t *fp){
  *fp=(uint64_t)(size_t)malloc(*fp);
}


//(i32 destination,i32 source,i32 count,i32 memory_indexA,i32 memory_indexB,ref runtime_context)
static void insn_memorycopy32(void *fp){
  void *sp=fp;
  uint32_t d=pwart_rstack_get_i32(&sp);
  uint32_t s=pwart_rstack_get_i32(&sp);
  uint32_t n=pwart_rstack_get_i32(&sp);
  uint32_t dmi=pwart_rstack_get_i32(&sp);
  uint32_t smi=pwart_rstack_get_i32(&sp);

  RuntimeContext *m=(RuntimeContext *)pwart_rstack_get_ref(&sp);
  
  Memory *dmem=*dynarr_get(m->memories,Memory *,dmi);
  Memory *smem=*dynarr_get(m->memories,Memory *,smi);
  #ifdef DEBUG_BUILD
  printf("test_aid:insn_memorycopy32(%d,%d,%d,%p)\n",d,s,n,m);
  #endif
  memmove(dmem->bytes+d,smem->bytes+s,n);
}

//(i32 destination,i32 source,i32 count,i32 table_indexA,i32 table_indexB,ref runtime_context)
static void insn_tablecopy32(void *fp){
  void *sp=fp;
  uint32_t d=pwart_rstack_get_i32(&sp);
  uint32_t s=pwart_rstack_get_i32(&sp);
  uint32_t n=pwart_rstack_get_i32(&sp);
  uint32_t dtab=pwart_rstack_get_i32(&sp);
  uint32_t stab=pwart_rstack_get_i32(&sp);
  RuntimeContext *m=(RuntimeContext *)pwart_rstack_get_ref(&sp);
  
  memmove(dynarr_get(m->tables,Table,dtab)+d,dynarr_get(m->tables,Table,stab)+s,n*sizeof(void *));
}
//(i32 destination,i32 value,i32 count,i32 memory_index,i32 reserve,ref runtime_context)
static void insn_memoryfill32(void *fp){
  void *sp=fp;
  uint32_t d=pwart_rstack_get_i32(&sp);
  uint32_t val=pwart_rstack_get_i32(&sp);
  uint32_t n=pwart_rstack_get_i32(&sp);
  uint32_t dmi=pwart_rstack_get_i32(&sp);
  pwart_rstack_get_i32(&sp);
  RuntimeContext *m=(RuntimeContext *)pwart_rstack_get_ref(&sp);
  Memory *dmem=*dynarr_get(m->memories,Memory *,dmi);
  #ifdef DEBUG_BUILD
  printf("test_aid:insn_insn_memoryfill32(%d,%d,%d,%p)\n",d,val,n);
  #endif
  memset(dmem->bytes+d,val,n);
}
//(i32 destination,ref value,i32 count,i32 table_index,i32 reserve,ref runtime_context)
static void insn_tablefill32(void *fp){
  void *sp=fp;
  uint32_t d=pwart_rstack_get_i32(&sp);
  void *val=pwart_rstack_get_ref(&sp);
  uint32_t n=pwart_rstack_get_i32(&sp);
  uint32_t tabidx=pwart_rstack_get_i32(&sp);
  pwart_rstack_get_i32(&sp);
  RuntimeContext *m=(RuntimeContext *)pwart_rstack_get_ref(&sp);
  void **ent=dynarr_get(m->tables,Table,tabidx)->entries;
  for(int i1=d;i1<d+n;i1++){
    ent[i1]=val;
  }
}
static void insn_get_self_runtime_context(void *fp){
  wa_assert(0,"This is stub function. can only call directly.");
}

static void insn_native_index_size(void *fp){
  pwart_rstack_put_i32(&fp,sizeof(void *)*8);
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

static uint8_t types_i32x5_ref[] = {WVT_I32, WVT_I32, WVT_I32, WVT_I32,WVT_I32, WVT_REF,0};

static uint8_t types_i32_i32_ref[] = {WVT_I32, WVT_I32, WVT_REF,0};

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


static Type func_type_i32_i32_ref_ret_i32 = {.params = types_i32_i32_ref, .results = types_i32};
static Type func_type_i32x5_ref_ret_void={.params=types_i32x5_ref,.results=types_void};

static void waexpr_run_const(ModuleCompiler *m, void *result) {
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
  case 0xd0: //ref.null
    *(void **)result=NULL;
    break;
  case 0xd2: //ref.func
  {
    uint32_t arg = read_LEB(m->bytes, &m->pc, 32);
    *(WasmFunction **)result = dynarr_get(m->functions,WasmFunction,arg);
  }
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