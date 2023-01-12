#ifndef _PWART_OPGEN_MISC_C
#define _PWART_OPGEN_MISC_C

#include "def.h"
#include "extfunc.c"
#include "opgen_utils.c"

static void opgen_GenRefNull(Module *m, int32_t tidx) {
  StackValue *sv = push_stackvalue(m, sv);
  sv->wasm_type = tidx;
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = 0;
}
static void opgen_GenIsNull(Module *m) {
  StackValue *sv = &m->stack[m->sp];
  if (sv->wasm_type == WVT_REF || sv->wasm_type == WVT_FUNC) {
    sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op, sv->val.opw,
                    SLJIT_IMM, 0);
    sv->wasm_type = WVT_I32;
    sv->jit_type = SVT_CMP;
    sv->val.cmp.flag = SLJIT_EQUAL;
  } else {
    sv->wasm_type = WVT_I32;
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
  sv = push_stackvalue(m, NULL);
  sv->wasm_type = WVT_FUNC;
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_MEM1(a);
  sv->val.opw = 0;
}

static void opgen_GenMiscOp(Module *m, int opcode) {
  int32_t tidx,fidx;
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
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    exit(1);
    break;
  }
}
#endif