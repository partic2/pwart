#ifndef _PWART_OPGEN_MEM_C
#define _PWART_OPGEN_MEM_C

// Variable access,Memory-related operators,Constants

#include "def.h"
#include "extfunc.c"
#include "opgen_utils.c"

static void opgen_GenLocalGet(Module *m, uint32_t idx) {
  push_stackvalue(m, dynarr_get(m->locals, StackValue, idx));
}

static void opgen_GenLocalSet(Module *m, uint32_t idx) {
  StackValue *sv, *sv2;
  StackValue *stack = m->stack;
  sv = dynarr_get(m->locals, StackValue, idx);
  // check if local variable still on stack, rarely.
  for (int i = 0; i < m->sp; i++) {
    if (stack[i].val.op == sv->val.op && stack[i].val.opw == sv->val.opw) {
      pwart_EmitSaveStack(m, &stack[i]);
    }
  }
  sv2 = &stack[m->sp];
  pwart_EmitStoreStackValue(m, sv2, sv->val.op, sv->val.opw);
  m->sp--;
}

static void opgen_GenLocalTee(Module *m, uint32_t idx) {
  StackValue *sv, *sv2;
  StackValue *stack = m->stack;
  sv = dynarr_get(m->locals, StackValue, idx);
  sv2 = &stack[m->sp];
  pwart_EmitStoreStackValue(m, sv2, sv->val.op, sv->val.opw);
}

static void opgen_GenGlobalGet(Module *m, uint32_t idx) {
  int32_t a;
  StackValue *sv;
  a = pwart_GetFreeReg(m, RT_BASE, 0);
  sv = dynarr_get(m->locals, StackValue, m->globals_base_local);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
  sv = push_stackvalue(m, dynarr_get(m->globals, StackValue, idx));
  sv->val.op = SLJIT_MEM1(a);
  sv->jit_type = SVT_GENERAL;
  pwart_EmitStackValueLoadReg(m, sv);
}

static void opgen_GenGlobalSet(Module *m, uint32_t idx) {
  int32_t a;
  StackValue *sv, *sv2;
  a = pwart_GetFreeReg(m, RT_BASE, 0);
  sv = dynarr_get(m->locals, StackValue, m->globals_base_local);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
  sv = dynarr_get(m->globals, StackValue, idx);
  sv2 = &m->stack[m->sp];
  pwart_EmitStoreStackValue(m, sv2, SLJIT_MEM1(a), sv->val.opw);
  m->sp--;
}

static void opgen_GenTableGet(Module *m, uint32_t idx) {
  int32_t a;
  StackValue *sv, *sv2;
  sv = &m->stack[m->sp];
  a = pwart_GetFreeReg(m, RT_INTEGER, 1);
  sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                 sizeof(void *) == 4 ? 2 : 3);
  sv2 = dynarr_get(m->locals, StackValue, m->table_entries_local);
  sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv2->val.op, sv2->val.opw);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_MEM1(a), 0);
  sv = &m->stack[m->sp];
  sv->wasm_type = WVT_FUNC;
  sv->jit_type = SVT_GENERAL;
  sv->val.op = a;
  sv->val.opw = 0;
}

static void opgen_GenTableSet(Module *m, uint32_t tidx) {
  int32_t a;
  StackValue *sv, *sv2;
  stackvalue_EmitSwapTopTwoValue(m);
  sv = &m->stack[m->sp];
  a = pwart_GetFreeReg(m, RT_INTEGER, 1);
  sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                 sizeof(void *) == 4 ? 2 : 3);
  m->sp--;
  sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
  sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv->val.op, sv->val.opw);
  sv = &m->stack[m->sp];
  pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(a), 0);
  m->sp--;
}

static void opgen_GenCurrentMemory(Module *m) {
  int32_t a;
  StackValue *sv;
  a = pwart_GetFreeReg(m, RT_INTEGER, 0);
  sv = dynarr_get(m->locals, StackValue, m->runtime_ptr_local);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
  sljit_emit_op1(m->jitc, SLJIT_MOV32, a, 0, SLJIT_MEM1(a),
                 offsetof(RuntimeContext, memory.pages));
  sv = push_stackvalue(m, NULL);
  sv->jit_type = SVT_GENERAL;
  sv->wasm_type = WVT_I32;
  sv->val.op = a;
  sv->val.opw = 0;
}
static void opgen_GenMemoryGrow(Module *m) {
  pwart_EmitCallFunc(m, &func_type_i32_ret_i32, SLJIT_IMM,
                     (sljit_sw)&insn_memorygrow);
}
// a:memory access base register, can be result register. result StackValue will
// be pushed to stack.
static void opgen_GenI32Load(Module *m, int32_t opcode, int32_t a,
                             sljit_sw offset) {
  StackValue *sv = push_stackvalue(m, NULL);
  switch (opcode) {
  case 0x28: // i32.load32
    sljit_emit_op1(m->jitc, SLJIT_MOV32, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x2c: // i32.load8_s
    sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x2d: // i32.load8_u
    sljit_emit_op1(m->jitc, SLJIT_MOV_U8, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x2e: // i32.load16_s
    sljit_emit_op1(m->jitc, SLJIT_MOV_S16, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x2f: // i32.load16_u
    sljit_emit_op1(m->jitc, SLJIT_MOV_U16, a, 0, SLJIT_MEM1(a), offset);
    break;
  }
  sv->jit_type = SVT_GENERAL;
  sv->wasm_type = WVT_I32;
  sv->val.op = a;
  sv->val.opw = 0;
}
static void opgen_GenI64Load(Module *m, int32_t opcode, int32_t a,
                             sljit_sw offset) {
  int32_t b;
  b = pwart_GetFreeRegExcept(m, RT_INTEGER, a, 0);
  StackValue *sv = push_stackvalue(m, NULL);

  switch (opcode) {
  case 0x29: // i64.load64
    if (m->target_ptr_size == 32) {
      sv->wasm_type = WVT_I64;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_MEM1(a);
      sv->val.opw = offset;
      pwart_EmitStackValueLoadReg(m, sv);
    } else {
      sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_MEM1(a), offset);
    }
    break;
  case 0x30: // i64.load8_s
    sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x31: // i64.load8_u
    sljit_emit_op1(m->jitc, SLJIT_MOV_U8, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x32: // i64.load16_s
    sljit_emit_op1(m->jitc, SLJIT_MOV_S16, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x33: // i64.load16_u
    sljit_emit_op1(m->jitc, SLJIT_MOV_U16, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x34: // i64.load32_s
    sljit_emit_op1(m->jitc, SLJIT_MOV_S32, a, 0, SLJIT_MEM1(a), offset);
    break;
  case 0x35: // i64.load32_u
    sljit_emit_op1(m->jitc, SLJIT_MOV_U32, a, 0, SLJIT_MEM1(a), offset);
    break;
  }
  if (m->target_ptr_size == 32) {
    sv->jit_type = SVT_TWO_REG;
    sv->wasm_type = WVT_I64;
    sv->val.tworeg.opr1 = a;
    sv->val.tworeg.opr2 = b;
    // i64.load64
    if (opcode != 0x29) {
      sljit_emit_op1(m->jitc, SLJIT_MOV, b, 0, SLJIT_IMM, 0);
    }
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->wasm_type = WVT_I64;
    sv->val.op = a;
    sv->val.opw = 0;
  }
}

static void opgen_GenFloatLoad(Module *m, int32_t opcode, int32_t a,
                               uint32_t offset) {
  StackValue *sv = push_stackvalue(m, NULL);
  switch (opcode) {
  case 0x2a: // f32.load32
    sv->wasm_type = WVT_F32;
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(a);
    sv->val.opw = offset;
    pwart_EmitStackValueLoadReg(m, sv);
    break;
  case 0x2b: // f64.load64
    sv->wasm_type = WVT_F64;
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(a);
    sv->val.opw = offset;
    pwart_EmitStackValueLoadReg(m, sv);
    break;
  }
}

// size: result size. size2: memory size.
static void opgen_GenLoad(Module *m, int32_t opcode, sljit_sw offset,
                          sljit_sw align) {
  StackValue *sv, *sv2;
  int32_t a;
  sv2 = &m->stack[m->sp];                                    // addr
  sv = dynarr_get(m->locals, StackValue, m->mem_base_local); // memory base
  a = pwart_GetFreeReg(m, RT_BASE, 1);
  if (sv2->wasm_type == WVT_I32) {
    sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                   sv2->val.op, sv2->val.opw);
  } else {
    if (m->target_ptr_size == 64) {
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                     sv2->val.op, sv2->val.opw);
    } else {
      // TODO:memory64 support for 32 bit arch.
      SLJIT_UNREACHABLE();
    }
  }
  m->sp--;
  switch (opcode) {
  case 0x28: // i32.loadxx
  case 0x2c:
  case 0x2d:
  case 0x2e:
  case 0x2f:
    opgen_GenI32Load(m, opcode, a, offset);
    break;
  case 0x29: // i64.loadxx
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
    opgen_GenI64Load(m, opcode, a, offset);
    break;
  case 0x2a: // fxx.loadww
  case 0x2b:
    opgen_GenFloatLoad(m, opcode, a, offset);
    break;
  default:
    SLJIT_UNREACHABLE();
  }
}

static void opgen_GenStore(Module *m, int32_t opcode, sljit_sw offset,
                           sljit_sw align) {
  int32_t a;
  StackValue *sv, *sv2;
  stackvalue_EmitSwapTopTwoValue(m);
  sv2 = &m->stack[m->sp];
  sv = dynarr_get(m->locals, StackValue, m->mem_base_local); // memory base
  a = pwart_GetFreeReg(m, RT_BASE, 1);
  if (sv2->wasm_type == WVT_I32) {
    sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                   sv2->val.op, sv2->val.opw);
  } else {
    if (m->target_ptr_size == 64) {
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                     sv2->val.op, sv2->val.opw);
    } else {
      // TODO:memory64 support for 32 bit arch.
      SLJIT_UNREACHABLE();
    }
  }
  m->sp--;
  sv = &m->stack[m->sp];
  switch (opcode) {
  case 0x36: // i32.store
  case 0x37: // i64.store
  case 0x38: // f32.store
  case 0x39: // f64.store
    pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(a), offset);
    break;
  case 0x3a: // i32.store8
  case 0x3c: // i64.store8
    if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
      pwart_EmitSaveStack(m, sv);
    }
    sljit_emit_op1(m->jitc, SLJIT_MOV_U8, SLJIT_MEM1(a), offset, sv->val.op,
                   sv->val.opw);
    break;
  case 0x3b: // i32.store16
  case 0x3d: // i64.store16
    if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
      pwart_EmitSaveStack(m, sv);
    }
    sljit_emit_op1(m->jitc, SLJIT_MOV_U16, SLJIT_MEM1(a), offset, sv->val.op,
                   sv->val.opw);
    break;
  case 0x3e: // i64.store32
    if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
      pwart_EmitSaveStack(m, sv);
    }
    sljit_emit_op1(m->jitc, SLJIT_MOV_U32, SLJIT_MEM1(a), offset, sv->val.op,
                   sv->val.opw);
    break;
  }
  m->sp--;
}

static void opgen_GenI32Const(Module *m, uint32_t c) {
  StackValue *sv = push_stackvalue(m, NULL);
  sv->wasm_type = WVT_I32;
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = c;
}
static void opgen_GenI64Const(Module *m, uint64_t c) {
  StackValue *sv = push_stackvalue(m, NULL);
  sv->wasm_type = WVT_I64;
  if (m->target_ptr_size == 32) {
    sv->jit_type = SVT_I64CONST;
    sv->val.const64 = c;
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_IMM;
    sv->val.opw = c;
  }
}
static void opgen_GenF32Const(Module *m, float *c) {
  StackValue *sv = push_stackvalue(m, NULL);
  sv->wasm_type = WVT_F32;
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = *(sljit_s32 *)c;
  // sljit only allow load float from memory
  pwart_EmitSaveStack(m, sv);
}

static void opgen_GenF64Const(Module *m, double *c) {
  StackValue *sv = push_stackvalue(m, NULL);
  sv->wasm_type = WVT_F64;
  if (m->target_ptr_size == 32) {
    sv->jit_type = SVT_I64CONST;
    sv->val.const64 = *(uint64_t *)c;
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_IMM;
    sv->val.opw = *(sljit_sw *)c;
  }
  // sljit only allow load float from memory
  pwart_EmitSaveStack(m, sv);
}

static void opgen_GenMemOp(Module *m, int opcode) {
  int32_t arg, tidx;
  uint64_t arg64;
  sljit_sw align, offset;
  switch (opcode) {
    //
    // Variable access
    //
  case 0x20: // local.get
    arg = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenLocalGet(m, arg);
    break;
  case 0x21: // local.set
    arg = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenLocalSet(m, arg);
    break;
  case 0x22: // local.tee
    arg = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenLocalTee(m, arg);
    break;
  case 0x23: // global.get
    arg = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenGlobalGet(m, arg);
    break;
  case 0x24: // global.set
    arg = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenGlobalSet(m, arg);
    break;
  case 0x25:                               // table.get
    tidx = read_LEB(m->bytes, &m->pc, 32); // table index
    opgen_GenTableGet(m, tidx);
    break;
  case 0x26:                               // table.set
    tidx = read_LEB(m->bytes, &m->pc, 32); // table index
    opgen_GenTableSet(m, tidx);
    break;
  //
  // Memory-related operators
  //
  case 0x3f:                        // current_memory
    read_LEB(m->bytes, &m->pc, 32); // ignore reserved
    opgen_GenCurrentMemory(m);
    break;
  case 0x40:                        // memory.grow
    read_LEB(m->bytes, &m->pc, 32); // ignore reserved
    opgen_GenMemoryGrow(m);
    break;
  // Memory load operators
  case 0x28 ... 0x35:
    align = read_LEB(m->bytes, &m->pc, 32);
    offset = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenLoad(m, opcode, offset, align);
    break;
  case 0x36 ... 0x3e:
    align = read_LEB(m->bytes, &m->pc, 32);
    offset = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenStore(m, opcode, offset, align);
    break;
  //
  // Constants
  //
  case 0x41: // i32.const
    arg = read_LEB_signed(m->bytes, &m->pc, 32);
    opgen_GenI32Const(m, arg);
    break;
  case 0x42: // i64.const
    arg64 = read_LEB_signed(m->bytes, &m->pc, 64);
    opgen_GenI64Const(m, arg64);
    break;
  case 0x43: // f32.const
    opgen_GenF32Const(m, (float *)(m->bytes + m->pc));
    m->pc += 4;
    break;
  case 0x44: // f64.const
    opgen_GenF64Const(m, (double *)(m->bytes + m->pc));
    m->pc += 8;
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    exit(1);
    break;
  }
}

#endif