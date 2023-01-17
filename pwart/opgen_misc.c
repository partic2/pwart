#ifndef _PWART_OPGEN_MISC_C
#define _PWART_OPGEN_MISC_C

#include "def.h"
#include "extfunc.c"
#include "opgen_num.c"
#include "opgen_mem.c"
#include "opgen_utils.c"

static void opgen_GenRefNull(Module *m, int32_t tidx) {
  StackValue *sv = stackvalue_Push(m, tidx);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = 0;
}
static void opgen_GenIsNull(Module *m) {
  StackValue *sv = &m->stack[m->sp];
  if (sv->wasm_type == WVT_REF || sv->wasm_type == WVT_FUNC) {
    sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op, sv->val.opw,
                    SLJIT_IMM, 0);
    m->sp--;
    stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_CMP;
    sv->val.cmp.flag = SLJIT_EQUAL;
  } else {
    m->sp--;
    stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_GENERAL;
    sv->val.opw = 0;
    sv->val.op = SLJIT_IMM;
  }
}

static void opgen_GenRefFunc(Module *m, int32_t fidx) {
  int32_t a;
  StackValue *sv;
  a = pwart_GetFreeReg(m, RT_INTEGER, 0);
  sv = dynarr_get(m->locals, StackValue, m->functions_base_local);
  sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                 sizeof(void *) * fidx);
  sv = stackvalue_Push(m, WVT_FUNC);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_MEM1(a);
  sv->val.opw = 0;
}

static void opgen_GenMiscOp_FC(Module *m,int opcode){
  #if DEBUG_BUILD
  switch(opcode){
    case 0 ... 7:
    wa_debug("op fc %x:%s\n", m->pc, "ixx.trunc_sat_fxx_s|u");
    break;
    case 0x0a:
    wa_debug("op fc %x:%s\n", m->pc, "memory.copy");
    break;
    case 0x0b:
    wa_debug("op fc %x:%s\n", m->pc, "memory.fill");
    break;
  }
  #endif
  switch(opcode){
    //Non-trapping Float-to-int Conversions
    //the behavior when convert is overflow is depend on sljit implementation.
    case 0x00: //i32.trunc_sat_f32_s
    opgen_GenNumOp(m,0xa8);
    break;
    case 0x01: //i32.trunc_sat_f32_u
    opgen_GenNumOp(m,0xa9);
    break;
    case 0x02: //i32.trunc_sat_f64_s
    opgen_GenNumOp(m,0xaa);
    break;
    case 0x03: //i32.trunc_sat_f64_u
    opgen_GenNumOp(m,0xab);
    break; 
    case 0x04: //i64.trunc_sat_f32_s
    opgen_GenNumOp(m,0xae);
    break;
    case 0x05: //i64.trunc_sat_f32_u
    opgen_GenNumOp(m,0xaf);
    break;
    case 0x06: //i64.trunc_sat_f64_s
    opgen_GenNumOp(m,0xb0);
    break;
    case 0x07: //i64.trunc_sat_f64_u
    opgen_GenNumOp(m,0xb1);
    break; 
    case 0x0a: //memory.copy
    read_LEB_signed(m->bytes,&m->pc,32); //source memory index
    read_LEB_signed(m->bytes,&m->pc,32); //destination memory index
    opgen_GenMemoryCopy(m);
    break;
    case 0x0b: //memory.fill
    read_LEB_signed(m->bytes,&m->pc,32); //destination memory index
    opgen_GenMemoryFill(m);
    break;
    default:
    wa_debug("unrecognized misc opcode 0xfc 0x%x at %d", opcode, m->pc);
    exit(1);
  }
}

static void opgen_GenMiscOp(Module *m, int opcode) {
  int32_t tidx,fidx,opcode2;
  uint8_t *bytes = m->bytes;
  switch (opcode) {
  case 0xd0:                            // ref.null
    tidx = read_LEB(bytes, &m->pc, 32); // ref type
    opgen_GenRefNull(m, tidx);
    break;
  case 0xd1: // ref.isnull
    opgen_GenIsNull(m);
    break;
  case 0xd2: // ref.func
    fidx = read_LEB(bytes, &m->pc, 32);
    opgen_GenRefFunc(m, fidx);
    break;
  case 0xfc: // misc prefix
    opcode2=m->bytes[m->pc];
    opgen_GenMiscOp_FC(m,opcode2);
    m->pc++;
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    exit(1);
    break;
  }
}
#endif