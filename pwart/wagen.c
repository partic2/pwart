#ifndef _PWART_WAGEN_C
#define _PWART_WAGEN_C

#include "def.h"

#include "extfunc.c"

static void skip_immediates(uint8_t *bytes, uint32_t *pos) {
  uint32_t count, opcode = bytes[*pos];
  *pos = *pos + 1;
  switch (opcode) {
  // varuint1
  case 0x3f ... 0x40: // current_memory, grow_memory
    read_LEB(bytes, pos, 1);
    break;
  // varuint32, varint32
  case 0x0c ... 0x0d: // br, br_if
  case 0x10:          // call
  case 0x20 ... 0x24: // get/local.set, local.tee, get/global.set
  case 0x41:          // i32.const
    read_LEB(bytes, pos, 32);
    break;
  // varuint32 + varuint1
  case 0x11: // call_indirect
    read_LEB(bytes, pos, 32);
    read_LEB(bytes, pos, 1);
    break;
  case 0x1c: // select t
    read_LEB(bytes, pos, 32);
    break;
  // varint64
  case 0x42: // i64.const
    read_LEB(bytes, pos, 64);
    break;
  // uint32
  case 0x43: // f32.const
    *pos += 4;
    break;
  // uint64
  case 0x44: // f64.const
    *pos += 8;
    break;
  // block_type
  case 0x02 ... 0x04: // block, loop, if
    read_LEB(bytes, pos, 7);
    break;
  // memory_immediate
  case 0x28 ... 0x3e: // *.load*, *.store*
    read_LEB(bytes, pos, 32);
    read_LEB(bytes, pos, 32);
    break;
  // br_table
  case 0x0e:                          // br_table
    count = read_LEB(bytes, pos, 32); // target count
    for (uint32_t i = 0; i < count; i++) {
      read_LEB(bytes, pos, 32);
    }
    read_LEB(bytes, pos, 32); // default target
    break;
  default: // no immediates
    break;
  }
}

static int pwart_PrepareFunc(Module *m) {
  uint8_t *bytes = m->bytes;
  StackValue *stack = m->stack;
  StackValue *sv;

  uint32_t cur_pc;

  uint32_t arg, val, fidx, tidx, depth, count;
  uint32_t flags, offset, addr;
  uint8_t opcode, eof;
  uint32_t len = 0;
  uint32_t block_depth = 1;
  m->locals = NULL;
  dynarr_init(&m->locals, sizeof(StackValue));

  for (int i = 0; m->function_type->params[i] != 0; i++) {
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = m->function_type->params[i];
  }
  for (int i = 0; m->function_locals_type[i] != 0; i++) {
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = m->function_locals_type[i];
  }
  // initialize to -1
  // if local variable required, set to -2 and set to real value(>=0) after code
  // scan.
  m->mem_base_local = -1;
  m->table_entries_local = -1;
  m->functions_base_local = -1;
  m->globals_base_local = -1;
  while (!eof && m->pc < m->byte_count) {
    opcode = bytes[m->pc];
    cur_pc = m->pc;
    m->pc += 1;

    switch (opcode) {
    //
    // Control flow operators
    //
    case 0x02: // block
    case 0x03: // loop
    case 0x04: // if
      block_depth++;
      break;
    case 0x0b: // end
      block_depth--;
      if (block_depth <= 0) {
        eof = 1;
      }
      break;
    case 0x10: // call
      fidx = read_LEB(bytes, &m->pc, 32);
      m->functions_base_local = -2;
      break;
    case 0x11: // call_indirect
      tidx = read_LEB(bytes, &m->pc, 32);
      read_LEB(bytes, &m->pc, 1);
      m->table_entries_local = -2;
      break;
    case 0x23: // global.get
      arg = read_LEB(bytes, &m->pc, 32);
      m->globals_base_local = -2;
      break;
    case 0x24: // global.set
      arg = read_LEB(bytes, &m->pc, 32);
      m->globals_base_local = -2;
      break;
    case 0x25:                            // table.get
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      m->table_entries_local = -2;
      break;
    case 0x26:                            // table.set
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      m->table_entries_local = -2;
      break;
    // Memory load operators
    case 0x28 ... 0x35:
      flags = read_LEB(bytes, &m->pc, 32);
      offset = read_LEB(bytes, &m->pc, 32);
      m->mem_base_local = -2;
      break;

    // Memory store operators
    case 0x36 ... 0x3e:
      flags = read_LEB(bytes, &m->pc, 32);
      offset = read_LEB(bytes, &m->pc, 32);
      m->mem_base_local = -2;
      break;

    case 0xd2: // ref.func
      fidx = read_LEB(bytes, &m->pc, 32);
      m->functions_base_local = -2;
      break;

    default:
      m->pc--;
      skip_immediates(m->bytes, &m->pc);
      break;
    }
  }
  SLJIT_ASSERT(m->bytes[m->pc - 1] == 0xb);
  if (m->mem_base_local == -2) {
    m->mem_base_local = m->locals->len;
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = WVT_REF;
  }
  if (m->table_entries_local == -2) {
    m->table_entries_local = m->locals->len;
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = WVT_REF;
  }
  if (m->functions_base_local == -2) {
    m->functions_base_local = m->locals->len;
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = WVT_REF;
  }
  if (m->globals_base_local == -2) {
    m->globals_base_local = m->locals->len;
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = WVT_REF;
  }
  return 0;
}

//
// Stack machine (byte code related functions)
//

static int stackvalue_IsFloat(StackValue *sv) {
  return sv->wasm_type == WVT_F32 || sv->wasm_type == WVT_F64;
}

static uint32_t stackvalue_GetSizeAndAlign(StackValue *sv, uint32_t *align) {
  switch (sv->wasm_type) {
  case WVT_I32:
  case WVT_F32:
    if (align != NULL)
      *align = 4;
    return 4;
  case WVT_I64:
  case WVT_F64:
    if (align != NULL)
      *align = 8;
    return 8;
  default:
    if (align != NULL)
      *align = sizeof(void *);
    return sizeof(void *);
  }
}
// 32bit arch only, get most significant word in 64bit stackvalue
static void stackvalue_HighWord(Module *m, StackValue *sv, sljit_s32 *op,
                                sljit_sw *opw) {
  if (sv->jit_type == SVT_I64CONST) {
    *opw = (uint32_t)(sv->val.const64 >> 32);
    *op = SLJIT_IMM;
  } else if (sv->jit_type == SVT_TWO_REG) {
    *op = sv->val.tworeg.opr2;
    *opw = 0;
  } else if (sv->jit_type == SVT_GENERAL) {
    *op = sv->val.op;
    *opw = sv->val.opw + 4;
  } else {
    SLJIT_UNREACHABLE();
  }
}
static void stackvalue_LowWord(Module *m, StackValue *sv, sljit_s32 *op,
                               sljit_sw *opw) {
  if (sv->jit_type == SVT_I64CONST) {
    *opw = (uint32_t)(sv->val.const64);
    *op = SLJIT_IMM;
  } else if (sv->jit_type == SVT_TWO_REG) {
    *op = sv->val.tworeg.opr1;
    *opw = 0;
  } else if (sv->jit_type == SVT_GENERAL) {
    *op = sv->val.op;
    *opw = sv->val.opw;
  } else {
    SLJIT_UNREACHABLE();
  }
}
// find stackvalue is using register r, check for 0 to m->sp-upstack
static StackValue *stackvalue_FindSvalueUseReg(Module *m, sljit_s32 r,
                                               sljit_s32 regtype, int upstack) {
  StackValue *sv;
  int i, used = 0;
  for (i = 0; i <= m->sp - upstack; i++) {
    sv = &m->stack[i];
    if ((regtype == RT_FLOAT) != stackvalue_IsFloat(sv)) {
      continue;
    }
    if ((sv->jit_type == SVT_GENERAL || sv->jit_type == SVT_POINTER)) {
      if ((sv->val.op & 0xf) == r || ((sv->val.op >> 8) & 0xf) == r) {
        used = 1;
        break;
      }
    } else if (sv->jit_type == SVT_TWO_REG) {
      if (sv->val.tworeg.opr1 == r || sv->val.tworeg.opr2 == r) {
        used = 1;
        break;
      }
    }
  }
  if (used) {
    return sv;
  } else {
    return NULL;
  }
}

static int pwart_EmitStoreStackValue(Module *m, StackValue *sv, int memreg,
                                     int offset) {
  if (sv->jit_type == SVT_GENERAL) {
    if (m->target_ptr_size == 64) {
      switch (sv->wasm_type) {
      case WVT_I32:
        sljit_emit_op1(m->jitc, SLJIT_MOV32, memreg, offset, sv->val.op,
                       sv->val.opw);
        break;
      case WVT_F32:
        if (sv->val.op == SLJIT_IMM) {
          sljit_emit_op1(m->jitc, SLJIT_MOV32, memreg, offset, sv->val.op,
                         sv->val.opw);
        } else {
          sljit_emit_fop1(m->jitc, SLJIT_MOV_F32, memreg, offset, sv->val.op,
                          sv->val.opw);
        }
        break;
      case WVT_F64:
        if (sv->val.op == SLJIT_IMM) {
          sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, sv->val.op,
                         sv->val.opw);
        } else {
          sljit_emit_fop1(m->jitc, SLJIT_MOV_F64, memreg, offset, sv->val.op,
                          sv->val.opw);
        }
        break;
      case WVT_I64:
      default:
        sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, sv->val.op,
                       sv->val.opw);
        break;
      }
    } else if (m->target_ptr_size == 32) {
      switch (sv->wasm_type) {
      case WVT_I64:
        if (memreg & SLJIT_MEM) {
          sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, sv->val.op,
                         sv->val.opw);
          sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset + 4, sv->val.op,
                         sv->val.opw + 4);
        } else {
          SLJIT_UNREACHABLE();
        }
        break;
      case WVT_F32:
        if (sv->val.op == SLJIT_IMM) {
          sljit_emit_op1(m->jitc, SLJIT_MOV32, memreg, offset, sv->val.op,
                         sv->val.opw);
        } else {
          sljit_emit_fop1(m->jitc, SLJIT_MOV_F32, memreg, offset, sv->val.op,
                          sv->val.opw);
        }
        break;
      case WVT_F64:
        sljit_emit_fop1(m->jitc, SLJIT_MOV_F64, memreg, offset, sv->val.op,
                        sv->val.opw);
        break;
      case WVT_I32:
      default:
        sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, sv->val.op,
                       sv->val.opw);
        break;
      }
    } else {
      SLJIT_UNREACHABLE();
    }
  } else if (sv->jit_type == SVT_CMP) {
    sljit_emit_op_flags(m->jitc, SLJIT_MOV, memreg, offset, sv->val.cmp.flag);
  } else if (sv->jit_type == SVT_POINTER) {
    SLJIT_UNREACHABLE();
  } else if (sv->jit_type == SVT_TWO_REG) {
    // 32bit arch only
    if (memreg & SLJIT_MEM) {
      sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, sv->val.tworeg.opr1,
                     0);
      sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset + 4,
                     sv->val.tworeg.opr2, 0);
    } else {
      SLJIT_UNREACHABLE();
    }
  } else if (sv->jit_type == SVT_I64CONST) {
    if (memreg & SLJIT_MEM) {
      sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset, SLJIT_IMM,
                     (uint32_t)sv->val.const64);
      sljit_emit_op1(m->jitc, SLJIT_MOV, memreg, offset + 4, SLJIT_IMM,
                     (uint32_t)(sv->val.const64 >> 32));
    } else {
      SLJIT_UNREACHABLE();
    }
  }
}
// store value into [S0+IMM]
static int pwart_EmitSaveStack(Module *m, StackValue *sv) {
  if (sv->val.op == SLJIT_MEM1(SLJIT_S0) && sv->val.opw == sv->frame_offset) {
    return 1;
  }
  pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0), sv->frame_offset);
  sv->val.op = SLJIT_MEM1(SLJIT_S0);
  sv->val.opw = sv->frame_offset;
  sv->jit_type = SVT_GENERAL;
  return 1;
}

static int pwart_EmitSaveStackAll(Module *m) {
  int i;
  for (i = 0; i <= m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
}

static int pwart_EmitLoadReg(Module *m, StackValue *sv, int reg) {

  if (sv->jit_type == SVT_GENERAL) {
    if (m->target_ptr_size != 32) {
      switch (sv->wasm_type) {
      case WVT_I32:
        sljit_emit_op1(m->jitc, SLJIT_MOV32, reg, 0, sv->val.op, sv->val.opw);
        break;
      case WVT_F32:
        sljit_emit_fop1(m->jitc, SLJIT_MOV_F32, reg, 0, sv->val.op,
                        sv->val.opw);
        break;
      case WVT_F64:
        sljit_emit_fop1(m->jitc, SLJIT_MOV_F64, reg, 0, sv->val.op,
                        sv->val.opw);
        break;
      case WVT_I64:
      default:
        sljit_emit_op1(m->jitc, SLJIT_MOV, reg, 0, sv->val.op, sv->val.opw);
        break;
      }
    } else {
      switch (sv->wasm_type) {
      case WVT_I64:
        SLJIT_UNREACHABLE();
        break;
      case WVT_F32:
        sljit_emit_fop1(m->jitc, SLJIT_MOV_F32, reg, 0, sv->val.op,
                        sv->val.opw);
        break;
      case WVT_F64:
        sljit_emit_fop1(m->jitc, SLJIT_MOV_F64, reg, 0, sv->val.op,
                        sv->val.opw);
        break;
      case WVT_I32:
      default:
        sljit_emit_op1(m->jitc, SLJIT_MOV, reg, 0, sv->val.op, sv->val.opw);
        break;
      }
    }
  } else if (sv->jit_type == SVT_CMP) {
    sljit_emit_op_flags(m->jitc, SLJIT_MOV, reg, 0, sv->val.cmp.flag);
  } else if (sv->jit_type == SVT_POINTER) {
    SLJIT_UNREACHABLE();
  } else if (sv->jit_type == SVT_TWO_REG) {
    // 32bit arch only
    SLJIT_UNREACHABLE();
  } else if (sv->jit_type == SVT_I64CONST) {
    // 32bit arch only
    SLJIT_UNREACHABLE();
  }
}

static inline size_t get_memorybytes_offset(Module *m) {
  return offsetof(RuntimeContext, memory.bytes);
}
static inline size_t get_tableentries_offset(Module *m) {
  return offsetof(RuntimeContext, table.entries);
}
static inline size_t get_funcarr_offset(Module *m) {
  return offsetof(RuntimeContext, funcentries);
}
static inline size_t get_globalsbuf_offset(Module *m) {
  return offsetof(RuntimeContext, globals);
}

// Don't emit any code here.
// Don't write other stackvalue (due to stackvalue_EmitSwapTopTwoValue
// function).
static StackValue *push_stackvalue(Module *m, StackValue *sv) {
  StackValue *sv2;
  sv2 = &m->stack[++m->sp];
  if (sv != NULL) {
    *sv2 = *sv;
  } else {
    sv2->jit_type = SVT_GENERAL;
  }
  if (sv2 - m->stack == 0) {
    // first stack value
    sv2->frame_offset = m->first_stackvalue_offset;
  } else {
    sv = sv2 - 1;
    sv2->frame_offset = sv->frame_offset + stackvalue_GetSizeAndAlign(sv, NULL);
  }
  return sv2;
}

static void stackvalue_EmitSwapTopTwoValue(Module *m) {
  StackValue *sv;
  // copy stack[sp-1]
  sv = push_stackvalue(m, &m->stack[m->sp - 1]);
  if (sv->jit_type == SVT_GENERAL && sv->val.op == SLJIT_MEM1(SLJIT_S0) &&
      sv->val.opw >= m->first_stackvalue_offset) {
    // value are on stack, we need save it otherwhere.
    pwart_EmitStackValueLoadReg(m, sv);
  }
  m->sp -= 3;
  push_stackvalue(m, &m->stack[m->sp + 2]);
  push_stackvalue(m, &m->stack[m->sp + 2]);
  // recover value on stack
  sv = &m->stack[m->sp - 1];
  if (sv->jit_type == SVT_GENERAL && sv->val.op == SLJIT_MEM1(SLJIT_S0) &&
      sv->val.opw >= m->first_stackvalue_offset) {
    pwart_EmitSaveStack(m, sv);
  }
  sv = &m->stack[m->sp];
  if (sv->jit_type == SVT_GENERAL && sv->val.op == SLJIT_MEM1(SLJIT_S0) &&
      sv->val.opw >= m->first_stackvalue_offset) {
    pwart_EmitSaveStack(m, sv);
  }
}

static int pwart_EmitCallFunc(Module *m, Type *type, sljit_s32 memreg,
                              sljit_sw offset) {
  StackValue *sv;
  uint32_t a, b, len;
  pwart_EmitSaveStackAll(m);
  if ((memreg & (SLJIT_MEM - 1)) == SLJIT_R0 ||
      (memreg & (SLJIT_MEM - 1)) == SLJIT_R1) {
    sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R2, 0, memreg, offset);
    memreg = SLJIT_R2;
    offset = 0;
  }
  sv = dynarr_get(m->locals, StackValue,
                  m->runtime_ptr_local); // RuntimeContext *
  sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R1, 0, sv->val.op, sv->val.opw);
  m->sp -= strlen(type->params);
  sv = &m->stack[m->sp + 1]; // first argument
  sljit_emit_op2(m->jitc, SLJIT_ADD, SLJIT_R0, 0, SLJIT_S0, 0, SLJIT_IMM,
                 sv->frame_offset);
  sljit_emit_icall(m->jitc, SLJIT_CALL, SLJIT_ARGS2(VOID, W, W), memreg,
                   offset);
  len = strlen(type->results);
  for (a = 0; a < len; a++) {
    b = type->results[a];
    sv = push_stackvalue(m, NULL);
    sv->jit_type = SVT_GENERAL;
    sv->wasm_type = b;
    sv->val.op = SLJIT_MEM1(SLJIT_S0);
    sv->val.opw = sv->frame_offset;
  }
}

static int pwart_EmitFuncReturn(Module *m) {
  int idx, len, off;
  StackValue *sv;
  off = 0;

  len = strlen(m->function_type->results);
  for (idx = 0; idx < len; idx++) {
    sv = &m->stack[m->sp - len + idx + 1];
    pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0), off);
    off += stackvalue_GetSizeAndAlign(sv, NULL);
  }
  sljit_emit_return_void(m->jitc);
}

// get free register. check up to upstack.
static sljit_s32 pwart_GetFreeReg(Module *m, sljit_s32 regtype, int upstack) {
  int i, used;
  sljit_s32 fr;
  StackValue *sv;

  if (regtype != RT_FLOAT) {
// XXX: on x86-32, R3 - R6 (same as S3 - S6) are emulated, and cannot be used
// for memory addressing
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    if (regtype == RT_BASE) {
      for (fr = SLJIT_R0; fr < SLJIT_R0 + 3; fr++) {
        if (m->registers_status[fr - SLJIT_R0] & RS_RESERVED) {
          continue;
        }
        sv = stackvalue_FindSvalueUseReg(m, fr, regtype, upstack);
        if (sv == NULL) {
          return fr;
        }
      }
    } else {
      for (fr = SLJIT_R0; fr < SLJIT_R0 + SLJIT_NUMBER_OF_SCRATCH_REGISTERS;
           fr++) {
        if (m->registers_status[fr - SLJIT_R0] & RS_RESERVED) {
          continue;
        }
        sv = stackvalue_FindSvalueUseReg(m, fr, regtype, upstack);
        if (sv == NULL) {
          return fr;
        }
      }
    }
    pwart_EmitSaveStack(m, sv);
    return SLJIT_R0;
#else
    for (fr = SLJIT_R0; fr < SLJIT_R0 + SLJIT_NUMBER_OF_SCRATCH_REGISTERS;
         fr++) {
      if (m->registers_status[fr - SLJIT_R0] & RS_RESERVED) {
        continue;
      }
      sv = stackvalue_FindSvalueUseReg(m, fr, regtype, upstack);
      if (sv == NULL) {
        return fr;
      }
    }
    pwart_EmitSaveStack(m, sv);
    return SLJIT_R0;
#endif
  } else {
    for (fr = SLJIT_FR0;
         fr <= SLJIT_FR0 + SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS; fr++) {
      if (m->float_registers_status[fr - SLJIT_FR0] & RS_RESERVED) {
        continue;
      }
      sv = stackvalue_FindSvalueUseReg(m, fr, regtype, upstack);
      if (sv == NULL) {
        return fr;
      }
    }
    pwart_EmitSaveStack(m, sv);
    return SLJIT_FR0;
  }
}

static sljit_s32 pwart_GetFreeRegExcept(Module *m, sljit_s32 regtype,
                                        sljit_s32 except, int upstack) {
  sljit_s32 r1;
  if (regtype == RT_FLOAT) {
    if (m->float_registers_status[except - SLJIT_R0] & RS_RESERVED) {
      return pwart_GetFreeReg(m, regtype, upstack);
    } else {
      m->float_registers_status[except - SLJIT_R0] |= RS_RESERVED;
      r1 = pwart_GetFreeReg(m, regtype, upstack);
      m->float_registers_status[except - SLJIT_R0] &= ~RS_RESERVED;
      return r1;
    }
  } else {
    if (m->registers_status[except - SLJIT_R0] & RS_RESERVED) {
      return pwart_GetFreeReg(m, regtype, upstack);
    } else {
      m->registers_status[except - SLJIT_R0] |= RS_RESERVED;
      r1 = pwart_GetFreeReg(m, regtype, upstack);
      m->registers_status[except - SLJIT_R0] &= ~RS_RESERVED;
      return r1;
    }
  }
}

// load stack value into reg
static void pwart_EmitStackValueLoadReg(Module *m, StackValue *sv) {
  sljit_s32 r1, r2;
  if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
    if (sv->jit_type == SVT_TWO_REG) {
      return;
    }
    r1 = pwart_GetFreeReg(m, RT_INTEGER, 0);
    r2 = pwart_GetFreeRegExcept(m, RT_INTEGER, r1, 0);
    if (sv->jit_type == SVT_GENERAL) {
      sljit_emit_op1(m->jitc, SLJIT_MOV, r1, 0, sv->val.op, sv->val.opw);
      sljit_emit_op1(m->jitc, SLJIT_MOV, r2, 0, sv->val.op, sv->val.opw + 4);
    } else if (sv->jit_type == SVT_I64CONST) {
      sljit_emit_op1(m->jitc, SLJIT_MOV, r1, 0, SLJIT_IMM, sv->val.const64);
      sljit_emit_op1(m->jitc, SLJIT_MOV, r2, 0, SLJIT_IMM,
                     sv->val.const64 >> 32);
    }
    sv->val.tworeg.opr1 = r1;
    sv->val.tworeg.opr2 = r2;
    sv->jit_type = SVT_TWO_REG;
  } else {
    if ((sv->val.op & SLJIT_MEM) == 0 && sv->val.op != SLJIT_IMM) {
      return;
    }
    if (stackvalue_IsFloat(sv)) {
      r1 = pwart_GetFreeReg(m, RT_FLOAT, 0);
    } else {
      r1 = pwart_GetFreeReg(m, RT_INTEGER, 0);
    }
    pwart_EmitLoadReg(m, sv, r1);
    sv->val.op = r1;
    sv->val.opw = 0;
    sv->jit_type = SVT_GENERAL;
  }
}

static int pwart_EmitFuncEnter(Module *m) {
  int nextSr = SLJIT_S2;
  int nextSfr = SLJIT_FS0;
  int i, len;
  int nextLoc; // next local offset to store locals variable, base on SLJIT_SP;
  StackValue *sv;

  // temprorary use nextLoc to represent offset of argument, base on SLJIT_S0
  nextLoc = 0;
  len = strlen(m->function_type->params);
  for (i = 0; i < len; i++) {
    sv = dynarr_get(m->locals, StackValue, i);
    sv->wasm_type = m->function_type->params[i];
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(SLJIT_S0);
    sv->val.opw = nextLoc;
    nextLoc += stackvalue_GetSizeAndAlign(sv, NULL);
  }
  m->first_stackvalue_offset = nextLoc;

  for (i = len; i < m->locals->len; i++) {
    sv = dynarr_get(m->locals, StackValue, i);
    sv->jit_type = SVT_GENERAL;
    if (m->target_ptr_size == 32) {
      if (sv->wasm_type == WVT_F32 || sv->wasm_type == WVT_F64) {
        if (nextSfr >= SLJIT_FIRST_SAVED_FLOAT_REG) {
          sv->val.op = nextSfr;
          sv->val.opw = 0;
          nextSfr--;
          continue;
        }
      } else if (stackvalue_GetSizeAndAlign(sv, NULL) == 4) {
        if (nextSr >= SLJIT_FIRST_SAVED_REG) {
          sv->val.op = nextSr;
          sv->val.opw = 0;
          nextSr--;
          continue;
        }
      }
      sv->val.op = SLJIT_MEM1(SLJIT_SP);
      sv->val.opw = nextLoc;
      nextLoc += stackvalue_GetSizeAndAlign(sv, NULL);
    } else {
      if (stackvalue_IsFloat(sv)) {
        if (nextSfr >= SLJIT_FIRST_SAVED_FLOAT_REG) {
          sv->val.op = nextSfr;
          sv->val.opw = 0;
          nextSfr--;
          continue;
        }
      } else {
        if (nextSr >= SLJIT_FIRST_SAVED_REG) {
          sv->val.op = nextSr;
          sv->val.opw = 0;
          nextSr--;
          continue;
        }
      }
      sv->val.op = SLJIT_MEM1(SLJIT_SP);
      sv->val.opw = nextLoc;
      nextLoc += stackvalue_GetSizeAndAlign(sv, NULL);
    }
  }
  sljit_emit_enter(m->jitc, 0, SLJIT_ARGS2(VOID, W, W),
                   SLJIT_NUMBER_OF_SCRATCH_REGISTERS, SLJIT_S0 - nextSr,
                   SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, SLJIT_FS0 - nextSfr,
                   nextLoc);

  if (m->table_entries_local >= 0) {
    sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
    sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw,
                   SLJIT_MEM1(SLJIT_S1), get_tableentries_offset(m));
  }
  if (m->functions_base_local >= 0) {
    sv = dynarr_get(m->locals, StackValue, m->functions_base_local);
    sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw,
                   SLJIT_MEM1(SLJIT_S1), get_funcarr_offset(m));
  }
  if (m->globals_base_local >= 0) {
    sv = dynarr_get(m->locals, StackValue, m->globals_base_local);
    sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S1),
                   get_globalsbuf_offset(m));
    sljit_emit_op2(m->jitc, SLJIT_ADD, sv->val.op, sv->val.opw, SLJIT_R0, 0,
                   SLJIT_IMM, offsetof(struct dynarr, data));
  }

  m->runtime_ptr_local = m->locals->len;
  sv = dynarr_push_type(&m->locals, StackValue);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_S1;
  sv->val.opw = 0;

  // for performance, put memory base to SLJIT_S1, and save RuntimeContext
  // pointer. Must be the last step.
  if (m->mem_base_local >= 0) {
    int16_t t;
    sv = dynarr_get(m->locals, StackValue, m->mem_base_local);
    sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_S1),
                   get_memorybytes_offset(m));
    sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw, SLJIT_S1, 0);
    sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_S1, 0, SLJIT_R0, 0);
    t = m->mem_base_local;
    m->mem_base_local = m->runtime_ptr_local;
    m->runtime_ptr_local = m->mem_base_local;
  }
}

/*
//no need for code generate
Type *block_ParseBlockType(Module *m,int blktype){
  if(blktype==0x40){
     return &func_type_ret_void;
  }else if(blktype>=0){
    return dynarr_get(m->types,Type,blktype);
  }else{
    switch(blktype){
      case WVT_I32-0x80:
      return &func_type_ret_i32;
      break;
      case WVT_I64-0x80:
      return &func_type_ret_i64;
      break;
      case WVT_F32-0x80:
      return &func_type_ret_f32;
      break;
      case WVT_F64-0x80:
      return &func_type_ret_f64;
      break;
    }
  }
}
*/
/*
r0,r1,r2 is scratch registers(at least three registers are required.).
s0(arg0) contains runtime frame pointer.
s1(arg1) contains the pointer refer to Module m.
*/
static int pwart_GenCode(Module *m) {
  uint8_t *bytes = m->bytes;
  StackValue *stack = m->stack;
  StackValue *sv, *sv2;
  uint32_t cur_pc;
  Block *block;
  uint32_t arg, val, fidx, tidx, depth, count;
  uint32_t *i32p;
  uint32_t flags, offset, addr;
  // uint32_t    *depths;
  uint8_t opcode;
  uint8_t eof;      // end of function
  uint32_t a, b, c; // temp
  sljit_s32 op1, op2;
  sljit_sw opw1, opw2;
  Type *type;
  struct sljit_jump *jump;

  m->jitc = sljit_create_compiler(NULL, NULL);

#if DEBUG_BUILD
  m->jitc->verbose = stdout;
#endif

  pwart_EmitFuncEnter(m);

  m->blocks = NULL;
  dynarr_init(&m->blocks, sizeof(Block));
  m->br_table = NULL;
  dynarr_init(&m->br_table, sizeof(uint32_t));

  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x00;
  eof = 0;

  while (!eof && m->pc < m->byte_count) {
    opcode = bytes[m->pc];
    cur_pc = m->pc;
    m->pc += 1;

    // XXX: save flag if next op is not if or br_if.
    if (stack[m->sp].jit_type == SVT_CMP && opcode != 0x04 && opcode != 0x0d) {
      pwart_EmitSaveStack(m, &stack[m->sp]);
    }
#if DEBUG_BUILD
    wa_debug("op %d:%s\n", m->pc, OPERATOR_INFO[opcode]);
#endif
    switch (opcode) {

    //
    // Control flow operators
    //
    case 0x00: // unreachable
      sprintf(exception, "%s", "unreachable");
      sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)&m);
      sljit_emit_call(m->jitc, SLJIT_CALL, SLJIT_ARGS1(VOID, P));
      break;
    case 0x01: // nop
      break;
    case 0x02: // block
    {
      int32_t blktype;
      blktype = read_LEB_signed(bytes, &m->pc, 33);
      pwart_EmitSaveStackAll(m);
      block = dynarr_push_type(&m->blocks, Block);
      block->block_type = 0x2;
      dynarr_init(&block->br_jump, sizeof(struct sljit_jump *));
      block->else_jump=NULL;
    } break;
    case 0x03: // loop
    {
      int32_t blktype;
      blktype = read_LEB_signed(bytes, &m->pc, 33);
      pwart_EmitSaveStackAll(m);
      block = dynarr_push_type(&m->blocks, Block);
      block->block_type = 0x3;
      block->br_label = sljit_emit_label(m->jitc);
    } break;
    case 0x04: // if
    {
      int32_t blktype;
      blktype = read_LEB_signed(bytes, &m->pc, 33);
      //stack top will be remove soon. Don't save it here.
      for (int i = 0; i < m->sp; i++) {
        pwart_EmitSaveStack(m, &m->stack[i]);
      }
      block = dynarr_push_type(&m->blocks, Block);
      block->block_type = 0x4;
      dynarr_init(&block->br_jump, sizeof(struct sljit_jump *));
      sv = &stack[m->sp];
      if (sv->jit_type == SVT_CMP) {
        int freer = pwart_GetFreeReg(m, RT_INTEGER, 0);
        sljit_emit_op_flags(m->jitc, SLJIT_MOV, freer, 0, sv->val.cmp.flag);
        jump = sljit_emit_cmp(m->jitc, SLJIT_EQUAL, sv->val.op, sv->val.opw,
                              SLJIT_IMM, 0);
      } else {
        //sljit_emit_cmp always use machine word length, So load to register first.
        pwart_EmitStackValueLoadReg(m,sv);
        jump = sljit_emit_cmp(m->jitc, SLJIT_EQUAL, sv->val.op, sv->val.opw,
                              SLJIT_IMM, 0);
      }
      block->else_jump = jump;
      m->sp--;
    } break;
    case 0x05: // else
      pwart_EmitSaveStackAll(m);
      block = dynarr_get(m->blocks, Block, m->blocks->len - 1);
      *dynarr_push_type(&block->br_jump,struct sljit_jump *)=sljit_emit_jump(m->jitc,SLJIT_JUMP);
      sljit_set_label(block->else_jump, sljit_emit_label(m->jitc));
      block->else_jump=NULL;
      break;
    case 0x0b: // end
    {
      struct sljit_jump *sj;
      block = dynarr_pop_type(&m->blocks, Block);
      if (block->block_type == 0x2 || block->block_type == 0x4) {
        pwart_EmitSaveStackAll(m);
        // if block
        block->br_label = sljit_emit_label(m->jitc);
        for (a = 0; a < block->br_jump->len; a++) {
          sj = *dynarr_get(block->br_jump, struct sljit_jump *, a);
          sljit_set_label(sj, block->br_label);
        }
        //if block, ensure else_jump have target.
        if(block->else_jump!=NULL){
          sljit_set_label(block->else_jump, block->br_label);
        }

      } else if (block->block_type == 0x00) {
        // function
        pwart_EmitFuncReturn(m);
        SLJIT_ASSERT(m->blocks->len == 0);
        eof = 1;
      }
      if (block->br_jump != NULL) {
        dynarr_free(&block->br_jump);
      }
    } break;
    case 0x0c: // br
      depth = read_LEB(bytes, &m->pc, 32);
      pwart_EmitSaveStackAll(m);
      block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
      if (block->br_label != NULL) {
        sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_JUMP), block->br_label);
      } else {
        *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
            sljit_emit_jump(m->jitc, SLJIT_JUMP);
      }
      break;
    case 0x0d: // br_if
      depth = read_LEB(bytes, &m->pc, 32);
      sv = &stack[m->sp];
      //stack top will be remove soon. Don't save it here.
      for (int i = 0; i < m->sp; i++) {
        pwart_EmitSaveStack(m, &m->stack[i]);
      }
      block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
      if (sv->jit_type == SVT_CMP) {
        if (block->br_label != NULL) {
          sljit_set_label(sljit_emit_jump(m->jitc, sv->val.cmp.flag),
                          block->br_label);
        } else {
          *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
              sljit_emit_jump(m->jitc, sv->val.cmp.flag);
        }
      } else if (sv->jit_type == SVT_GENERAL) {
        //sljit_emit_cmp always use machine word length, So load to register first.
        pwart_EmitStackValueLoadReg(m,sv);
        if (block->br_label != NULL) {
          sljit_set_label(sljit_emit_cmp(m->jitc, SLJIT_NOT_EQUAL, sv->val.op,
                                         sv->val.opw, SLJIT_IMM, 0),
                          block->br_label);
        } else {
          *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
              sljit_emit_cmp(m->jitc, SLJIT_NOT_EQUAL, sv->val.op,
                                         sv->val.opw, SLJIT_IMM, 0);
        }
      } else {
        SLJIT_UNREACHABLE();
      }
      m->sp--;
      break;
    case 0x0e: // br_table
      count = read_LEB(bytes, &m->pc, 32);
      // reset br_table
      m->br_table->len = 0;
      for (uint32_t i = 0; i <= count; i++) {
        i32p = dynarr_push_type(&m->br_table, uint32_t);
        *i32p = read_LEB(bytes, &m->pc, 32);
      }
      sv = &stack[m->sp];
      //stack top will be remove soon. Don't save it here.
      for (int i = 0; i < m->sp; i++) {
        pwart_EmitSaveStack(m, &m->stack[i]);
      }
      pwart_EmitStackValueLoadReg(m,sv);
      for (a = 0; a < count; a++) {
        depth = *dynarr_get(m->br_table, uint32_t, a);
        block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
        if (a == 0) {
          sljit_emit_op2u(m->jitc, SLJIT_AND | SLJIT_SET_Z, sv->val.op, sv->val.opw,
                         sv->val.op, sv->val.opw);
        } else {
          sljit_emit_op2(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z, sv->val.op,
                         sv->val.opw, sv->val.op, sv->val.opw, SLJIT_IMM, 1);
        }
        if (block->br_label != NULL) {
          sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_ZERO),
                          block->br_label);
        } else {
          *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
              sljit_emit_jump(m->jitc, SLJIT_ZERO);
        }
      }

      // default
      depth = *dynarr_get(m->br_table,uint32_t,count);
      block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
      if (block->br_label != NULL) {
        sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_JUMP), block->br_label);
      } else {
        *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
            sljit_emit_jump(m->jitc, SLJIT_JUMP);
      }
      m->sp--;
      break;
    case 0x0f: // return
      pwart_EmitFuncReturn(m);
      break;

    //
    // Call operators
    //
    case 0x10: // call
    {
      WasmFunction *fn;
      fidx = read_LEB(bytes, &m->pc, 32);
      fn = dynarr_get(m->functions, WasmFunction, fidx);
      a = pwart_GetFreeReg(m, RT_INTEGER, 0);
      sv = dynarr_get(m->locals, StackValue, m->functions_base_local);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, sizeof(void *) * fidx);
      type = dynarr_get(m->types, Type, fn->tidx);
      pwart_EmitCallFunc(m, type, SLJIT_MEM1(a), 0);
    } break;
    case 0x11: // call_indirect
      tidx = read_LEB(bytes, &m->pc, 32);
      type = dynarr_get(m->types, Type, tidx);
      read_LEB(bytes, &m->pc, 1); // tableidx. only support 1 table
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, sizeof(void *) == 4 ? 2 : 3);

      sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv->val.op, sv->val.opw);
      m->sp--;

      pwart_EmitCallFunc(m, type, SLJIT_MEM1(a), 0);

      break;
    //
    // Parametric operators
    //
    case 0x1a: // drop
      m->sp--;
      break;
    case 0x1c:                     // select t
      read_LEB(bytes, &m->pc, 32); // ignore type
    case 0x1b:                     // select
      // must save to stack.
      pwart_EmitSaveStack(m, &stack[m->sp - 2]);
      sv = &stack[m->sp--];

      if (sv->jit_type == SVT_CMP) {
        jump = sljit_emit_jump(m->jitc, sv->val.cmp.flag);
      } else if (sv->jit_type == SVT_GENERAL) {
        jump = sljit_emit_cmp(m->jitc, SLJIT_NOT_EQUAL, sv->val.op, sv->val.opw,
                              SLJIT_IMM, 0);
      } else {
        SLJIT_UNREACHABLE();
      }
      sv2 = &stack[m->sp--];
      sv = &stack[m->sp];
      sv->val = sv2->val;
      sv->jit_type = sv2->jit_type;
      pwart_EmitSaveStack(m, sv);
      sljit_set_label(jump, sljit_emit_label(m->jitc));
      break;
    //
    // Variable access
    //
    case 0x20: // local.get
      arg = read_LEB(bytes, &m->pc, 32);
      push_stackvalue(m, dynarr_get(m->locals, StackValue, arg));
      break;
    case 0x21: // local.set
      arg = read_LEB(bytes, &m->pc, 32);
      sv = dynarr_get(m->locals, StackValue, arg);
      // check if local variable still on stack, rarely.
      for (int i = 0; i < m->sp; i++) {
        if (stack[i].val.op == sv->val.op && stack[i].val.opw == sv->val.opw) {
          pwart_EmitSaveStack(m, &stack[i]);
        }
      }
      sv2 = &stack[m->sp];
      pwart_EmitStoreStackValue(m, sv2, sv->val.op, sv->val.opw);
      m->sp--;
      break;
    case 0x22: // local.tee
      arg = read_LEB(bytes, &m->pc, 32);
      sv = dynarr_get(m->locals, StackValue, arg);
      sv2 = &stack[m->sp];
      pwart_EmitStoreStackValue(m, sv2, sv->val.op, sv->val.opw);
      break;
    case 0x23: // global.get
      a = pwart_GetFreeReg(m, RT_BASE, 0);
      sv = dynarr_get(m->locals, StackValue, m->globals_base_local);
      sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
      arg = read_LEB(bytes, &m->pc, 32);
      sv = push_stackvalue(m, dynarr_get(m->globals, StackValue, arg));
      sv->val.op = SLJIT_MEM1(a);
      sv->jit_type = SVT_GENERAL;
      pwart_EmitStackValueLoadReg(m, sv);
      break;
    case 0x24: // global.set
      a = pwart_GetFreeReg(m, RT_BASE, 0);
      sv = dynarr_get(m->locals, StackValue, m->globals_base_local);
      sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
      arg = read_LEB(bytes, &m->pc, 32);
      sv = dynarr_get(m->globals, StackValue, arg);
      sv2 = &stack[m->sp];
      pwart_EmitStoreStackValue(m, sv2, SLJIT_MEM1(a), sv->val.opw);
      m->sp--;
      break;
    case 0x25:                            // table.get
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, sizeof(void *) == 4 ? 2 : 3);
      sv2 = dynarr_get(m->locals, StackValue, m->table_entries_local);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv2->val.op, sv2->val.opw);
      sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_MEM1(a), 0);
      sv = &stack[m->sp];
      sv->wasm_type = WVT_FUNC;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0x26:                            // table.set
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      stackvalue_EmitSwapTopTwoValue(m);
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, sizeof(void *) == 4 ? 2 : 3);
      m->sp--;
      sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv->val.op, sv->val.opw);
      sv = &stack[m->sp];
      pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(a), 0);
      m->sp--;
      break;
    //
    // Memory-related operators
    //
    case 0x3f:                     // current_memory
      read_LEB(bytes, &m->pc, 32); // ignore reserved
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
      break;
    case 0x40:                     // grow_memory
      read_LEB(bytes, &m->pc, 32); // ignore reserved
      pwart_EmitCallFunc(m, &func_type_i32_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_memorygrow);
      break;
    // Memory load operators
    case 0x28 ... 0x35:
      flags = read_LEB(bytes, &m->pc, 32); // align
      offset = read_LEB(bytes, &m->pc, 32);
      *sv2 = stack[m->sp];                                       // addr
      sv = dynarr_get(m->locals, StackValue, m->mem_base_local); // memory base
      a = pwart_GetFreeReg(m, RT_BASE, 1);
      if (sv2->wasm_type == WVT_I32) {
        sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
      } else {
        // TODO:memory64 support
        SLJIT_UNREACHABLE();
      }
      m->sp--;
      sv = push_stackvalue(m, NULL);
      sv->jit_type = SVT_GENERAL;
      switch (opcode) {
      case 0x28: // i32.load32
      case 0x29: // i64.load64
      case 0x2a: // f32.load32
      case 0x2b: // f64.load64
        sv->jit_type = SVT_GENERAL;
        sv->val.op = SLJIT_MEM1(a);
        sv->val.opw = offset;
        pwart_EmitStackValueLoadReg(m, sv);
        break;
      case 0x2c: // i32.load8_s
        sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, SLJIT_MEM1(a), offset);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_I32;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      case 0x2d: // i32.load8_u
        sljit_emit_op1(m->jitc, SLJIT_MOV_U8, a, 0, SLJIT_MEM1(a), offset);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_I32;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      case 0x2e: // i32.load16_s
        sljit_emit_op1(m->jitc, SLJIT_MOV_S16, a, 0, SLJIT_MEM1(a), offset);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_I32;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      case 0x2f: // i32.load16_u
        sljit_emit_op1(m->jitc, SLJIT_MOV_U16, a, 0, SLJIT_MEM1(a), offset);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_I32;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      case 0x30 ... 0x35:
        // i64.loadxx
        if (m->target_ptr_size == 32) {
          sv->jit_type = SVT_TWO_REG;
          sv->wasm_type = WVT_I64;
          sv->val.tworeg.opr1 = a;
          b = pwart_GetFreeRegExcept(m, RT_INTEGER, a, 0);
          sljit_emit_op1(m->jitc, SLJIT_MOV, b, 0, SLJIT_IMM, 0);
        } else {
          sv->jit_type = SVT_GENERAL;
          sv->wasm_type = WVT_I64;
          sv->val.op = a;
          sv->val.opw = 0;
        }
        switch (opcode) {
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
        break;
      }
      break;
    case 0x36 ... 0x3e:
      flags = read_LEB(bytes, &m->pc, 32); // align
      offset = read_LEB(bytes, &m->pc, 32);
      stackvalue_EmitSwapTopTwoValue(m);
      sv2 = &m->stack[m->sp];
      sv = dynarr_get(m->locals, StackValue, m->mem_base_local); // memory base
      a = pwart_GetFreeReg(m, RT_BASE, 1);
      if (sv2->wasm_type == WVT_I32) {
        sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
      } else {
        // TODO:memory64 support
        SLJIT_UNREACHABLE();
      }
      m->sp--;
      sv = &stack[m->sp];
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
        sljit_emit_op1(m->jitc, SLJIT_MOV_U16, SLJIT_MEM1(a), offset,
                       sv->val.op, sv->val.opw);
        break;
      case 0x3e: // i64.store32
        if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
          pwart_EmitSaveStack(m, sv);
        }
        sljit_emit_op1(m->jitc, SLJIT_MOV_U32, SLJIT_MEM1(a), offset,
                       sv->val.op, sv->val.opw);
        break;
      }
      m->sp--;
      break;
    //
    // Constants
    //
    case 0x41: // i32.const
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_IMM;
      sv->val.opw = read_LEB_signed(bytes, &m->pc, 32);
      break;
    case 0x42: // i64.const
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_I64;
      if (m->target_ptr_size == 32) {
        sv->jit_type = SVT_I64CONST;
        sv->val.const64 = read_LEB_signed(bytes, &m->pc, 64);
      } else {
        sv->jit_type = SVT_GENERAL;
        sv->val.op = SLJIT_IMM;
        sv->val.opw = read_LEB_signed(bytes, &m->pc, 64);
      }
      break;
    case 0x43: // f32.const
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_F32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_IMM;
      sv->val.opw = *(sljit_s32 *)(bytes + m->pc);
      // sljit only allow load float from memory
      pwart_EmitSaveStack(m, sv);
      m->pc += 4;
      break;
    case 0x44: // f64.const
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_F64;
      if (m->target_ptr_size == 32) {
        sv->jit_type = SVT_I64CONST;
        sv->val.const64 = *(uint64_t *)(bytes + m->pc);
      } else {
        sv->jit_type = SVT_GENERAL;
        sv->val.op = SLJIT_IMM;
        sv->val.opw = *(sljit_sw *)(bytes + m->pc);
      }
      // sljit only allow load float from memory
      pwart_EmitSaveStack(m, sv);
      m->pc += 8;
      break;
    //
    // Comparison operators
    //

    // unary
    case 0x45: // i32.eqz
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z, sv->val.op,
                      sv->val.opw, SLJIT_IMM, 0);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_CMP;
      sv->val.cmp.flag = SLJIT_EQUAL;
      break;
    case 0x50: // i64.eqz
      if (m->target_ptr_size == 32) {
        pwart_EmitCallFunc(m, &func_type_i64_ret_i32, SLJIT_IMM,
                           (sljit_sw)&insn_i64eqz);
      } else {
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                        sv->val.opw, SLJIT_IMM, 0);
        sv->wasm_type = WVT_I32;
        sv->jit_type = SVT_CMP;
        sv->val.cmp.flag = SLJIT_EQUAL;
      }
      break;

    // i32 binary
    case 0x46 ... 0x4f:
      sv2 = &stack[m->sp];
      m->sp -= 1;
      sv = &stack[m->sp];

      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_CMP;

      switch (opcode) {
      case 0x46:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z | SLJIT_CURRENT_FLAGS_COMPARE, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_EQUAL;
        break; // i32.eq
      case 0x47:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_NOT_EQUAL;
        break; // i32.ne
      case 0x48:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_LESS, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_SIG_LESS;
        break; // i32.lt_s
      case 0x49:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_LESS, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_LESS;
        break; // i32.lt_u
      case 0x4a:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_GREATER,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_SIG_GREATER;
        break; // i32.gt_s
      case 0x4b:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_GREATER, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_GREATER;
        break; // i32.gt_u
      case 0x4c:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_LESS_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_SIG_LESS_EQUAL;
        break; // i32.le_s
      case 0x4d:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_LESS_EQUAL, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_LESS_EQUAL;
        break; // i32.le_u
      case 0x4e:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_GREATER_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_SIG_GREATER_EQUAL;
        break; // i32.ge_s
      case 0x4f:
        sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_GREATER_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        sv->val.cmp.flag = SLJIT_GREATER_EQUAL;
        break; // i32.ge_u
      }
      break;
    case 0x51 ... 0x5a:
      if (m->target_ptr_size == 32) {
        switch (opcode) {
        case 0x51: // i64.eq
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64eq);
          break;
        case 0x52: // i64.ne
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64ne);
          break;
        case 0x53: // i64.lt_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64lts);
          break;
        case 0x54: // i64.lt_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64ltu);
          break;
        case 0x55: // i64.gt_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64gts);
          break;
        case 0x56: // i64.gt_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64gtu);
          break;
        case 0x57: // i64.le_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64les);
          break;
        case 0x58: // i64.le_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64leu);
          break;
        case 0x59: // i64.ge_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64ges);
          break;
        case 0x5a: // i64.ge_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i32, SLJIT_IMM,
                             (sljit_sw)&insn_i64geu);
          break;
        }
      } else {
        sv2 = &stack[m->sp];
        m->sp -= 1;
        sv = &stack[m->sp];

        sv->wasm_type = WVT_I32;
        sv->jit_type = SVT_CMP;

        switch (opcode) {
        case 0x51:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_EQUAL;
          break; // i64.eq
        case 0x52:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_NOT_EQUAL;
          break; // i64.ne
        case 0x53:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_LESS, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_SIG_LESS;
          break; // i64.lt_s
        case 0x54:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_LESS, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_LESS;
          break; // i64.lt_u
        case 0x55:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_GREATER,
                          sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_SIG_GREATER;
          break; // i64.gt_s
        case 0x56:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_GREATER, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_GREATER;
          break; // i64.gt_u
        case 0x57:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL,
                          sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_SIG_LESS_EQUAL;
          break; // i64.le_s
        case 0x58:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_LESS_EQUAL, sv->val.op,
                          sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_LESS_EQUAL;
          break; // i64.le_u
        case 0x59:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL,
                          sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_SIG_GREATER_EQUAL;
          break; // i64.ge_s
        case 0x5a:
          sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_GREATER_EQUAL,
                          sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
          sv->val.cmp.flag = SLJIT_GREATER_EQUAL;
          break; // i64.ge_u
        }
      }
      break;
    case 0x5b ... 0x60:
      sv2 = &stack[m->sp];
      sv = &stack[m->sp - 1];
      // pop arg1 to allow arg1 to be the result.

      switch (opcode) {
      case 0x5b: // f32.eq
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_EQUAL;
        break;
      case 0x5c: // f32.ne
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_NOT_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_NOT_EQUAL;
        break;
      case 0x5d: // f32.lt
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_LESS),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_LESS;
        break;
      case 0x5e: // f32.gt
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_GREATER),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_GREATER;
        break;
      case 0x5f: // f32.le
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_LESS_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_LESS_EQUAL;
        break;
      case 0x60: // f32.ge
        sljit_emit_fop1(m->jitc,
                        SLJIT_CMP_F32 | SLJIT_SET(SLJIT_F_GREATER_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_GREATER_EQUAL;
        break;
      }
      m->sp -= 2;
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_CMP;
      sv->val.cmp.flag = b;
      break;
    case 0x61 ... 0x66:
      sv2 = &stack[m->sp];
      sv = &stack[m->sp];

      switch (opcode) {
      case 0x61: // f64.eq
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_EQUAL;
        break;
      case 0x62: // f64.ne
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_NOT_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_NOT_EQUAL;
        break;
      case 0x63: // f64.lt
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_LESS),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_LESS;
        break;
      case 0x64: // f64.gt
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_GREATER),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_GREATER;
        break;
      case 0x65: // f64.le
        sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_LESS_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_LESS_EQUAL;
        break;
      case 0x66: // f64.ge
        sljit_emit_fop1(m->jitc,
                        SLJIT_CMP_F64 | SLJIT_SET(SLJIT_F_GREATER_EQUAL),
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        b = SLJIT_F_GREATER_EQUAL;
        break;
      }
      m->sp -= 2;
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_CMP;
      sv->val.cmp.flag = b;
      break;

    //
    // Numeric operators
    //

    // unary i32
    case 0x67: // i32.clz
      pwart_EmitCallFunc(m, &func_type_i32_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i32clz);
      break;
    case 0x68: // i32.ctz
      pwart_EmitCallFunc(m, &func_type_i32_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i32ctz);
      break;
    case 0x69: // i32.popcount
      pwart_EmitCallFunc(m, &func_type_i32_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i32popcount);
      break;
    case 0x79: // i64.clz
      pwart_EmitCallFunc(m, &func_type_i64_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64clz);
      break;
    case 0x7a: // i64.ctz
      pwart_EmitCallFunc(m, &func_type_i64_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64ctz);
      break;
    case 0x7b: // i64.popcount
      pwart_EmitCallFunc(m, &func_type_i64_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64popcount);
      break;

    // unary f32
    case 0x8b: // f32.abs
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1); // free register
      sljit_emit_fop1(m->jitc, SLJIT_ABS_F32, a, 0, sv->val.op, sv->val.opw);
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0x8c: // f32.neg
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1); // free register
      sljit_emit_fop1(m->jitc, SLJIT_NEG_F32, a, 0, sv->val.op, sv->val.opw);
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;

    case 0x8d: // f32.ceil
      pwart_EmitCallFunc(m, &func_type_f32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32ceil);
      break;
    case 0x8e: // f32.floor
      pwart_EmitCallFunc(m, &func_type_f32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32floor);
      break;
    case 0x8f: // f32.trunc
      pwart_EmitCallFunc(m, &func_type_f32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32trunc);
      break;
    case 0x90: // f32.nearest
      pwart_EmitCallFunc(m, &func_type_f32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32nearest);
      break;
    case 0x91: // f32.sqrt
      pwart_EmitCallFunc(m, &func_type_f32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32sqrt);
      break;

    // unary f64
    case 0x99: // f64.abs
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1); // free register
      sljit_emit_fop1(m->jitc, SLJIT_ABS_F64, a, 0, sv->val.op, sv->val.opw);
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_F64;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0x9a: // f64.neg
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1); // free register
      sljit_emit_fop1(m->jitc, SLJIT_NEG_F64, a, 0, sv->val.op, sv->val.opw);
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_F64;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0x9b: // f64.ceil
      pwart_EmitCallFunc(m, &func_type_f64_ret_f64, SLJIT_IMM,
                         (sljit_sw)&insn_f64ceil);
      break;
    case 0x9c: // f64.floor
      pwart_EmitCallFunc(m, &func_type_f64_ret_f64, SLJIT_IMM,
                         (sljit_sw)&insn_f64floor);
      break;
    case 0x9d: // f64.trunc
      pwart_EmitCallFunc(m, &func_type_f64_ret_f64, SLJIT_IMM,
                         (sljit_sw)&insn_f64trunc);
      break;
    case 0x9e: // f64.nearest
      pwart_EmitCallFunc(m, &func_type_f64_ret_f64, SLJIT_IMM,
                         (sljit_sw)&insn_f64nearest);
      break;
    case 0x9f: // f64.sqrt
      pwart_EmitCallFunc(m, &func_type_f64_ret_f64, SLJIT_IMM,
                         (sljit_sw)&insn_f64sqrt);
      break;

    // i32 binary
    case 0x6a ... 0x78:
      if (opcode != 0x77 && opcode != 0x78) {
        sv = &stack[m->sp - 1];
        sv2 = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_INTEGER, 2);
      }
      switch (opcode) {
      case 0x6a: // i32.add
        sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x6b: // i32.sub
        sljit_emit_op2(m->jitc, SLJIT_SUB32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x6c: // i32.mul
        sljit_emit_op2(m->jitc, SLJIT_MUL32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x6d: // i32.div_s
      {
        StackValue *sv3;
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
        if (sv3 != sv) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv, SLJIT_R0);
        }
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
        if (sv3 != sv2) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv2, SLJIT_R1);
        }
        a = SLJIT_R0;
        sljit_emit_op0(m->jitc, SLJIT_DIV_S32);
      } break;
      case 0x6e: // i32.div_u
      {
        StackValue *sv3;
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
        if (sv3 != sv) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv, SLJIT_R0);
        }
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
        if (sv3 != sv2) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv2, SLJIT_R1);
        }
        a = SLJIT_R0;
        sljit_emit_op0(m->jitc, SLJIT_DIV_U32);
      } break;
      case 0x6f: // i32.rem_s
      {
        StackValue *sv3;
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
        if (sv3 != sv) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv, SLJIT_R0);
        }
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
        if (sv3 != sv2) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv2, SLJIT_R1);
        }
        a = SLJIT_R1;
        sljit_emit_op0(m->jitc, SLJIT_DIVMOD_S32);
      } break;
      case 0x70: // i32.rem_u
      {
        StackValue *sv3;
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
        if (sv3 != sv) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv, SLJIT_R0);
        }
        sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
        if (sv3 != sv2) {
          if (sv3 != NULL) {
            pwart_EmitSaveStack(m, sv3);
          }
          pwart_EmitLoadReg(m, sv2, SLJIT_R1);
        }
        a = SLJIT_R1;
        sljit_emit_op0(m->jitc, SLJIT_DIVMOD_U32);
      } break;
      case 0x71: // i32.and
        sljit_emit_op2(m->jitc, SLJIT_AND32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x72: // i32.or
        sljit_emit_op2(m->jitc, SLJIT_OR32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x73: // i32.xor
        sljit_emit_op2(m->jitc, SLJIT_XOR32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x74: // i32.shl
        sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x75: // i32.shr_s
        sljit_emit_op2(m->jitc, SLJIT_ASHR32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x76: // i32.shr_u
        sljit_emit_op2(m->jitc, SLJIT_LSHR32, a, 0, sv->val.op, sv->val.opw,
                       sv2->val.op, sv2->val.opw);
        break;
      case 0x77: // i32.rotl
        pwart_EmitCallFunc(m, &func_type_i32_i32_ret_i32, SLJIT_IMM,
                           (sljit_sw)&insn_i32rotl);
        break;
      case 0x78: // i32.rotr
        pwart_EmitCallFunc(m, &func_type_i32_i32_ret_i32, SLJIT_IMM,
                           (sljit_sw)&insn_i32rotr);
        break;
      }
      if (opcode != 0x77 && opcode != 0x78) {
        m->sp -= 2;
        sv = push_stackvalue(m, NULL);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_I32;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;

    // i64 binary
    case 0x7c ... 0x8a:
      if (m->target_ptr_size == 64) {
        if (opcode != 0x89 && opcode != 0x8a) {
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          a = pwart_GetFreeReg(m, RT_INTEGER, 2);
        }
        switch (opcode) {
        case 0x7c: // i64.add
          sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x7d: // i64.sub
          sljit_emit_op2(m->jitc, SLJIT_SUB, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x7e: // i64.mul
          sljit_emit_op2(m->jitc, SLJIT_MUL, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x7f: // i64.div_s
        {
          StackValue *sv3;
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
          if (sv3 != sv) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv, SLJIT_R0);
          }
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
          if (sv3 != sv2) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv2, SLJIT_R1);
          }
          a = SLJIT_R0;
          sljit_emit_op0(m->jitc, SLJIT_DIV_SW);
        } break;
        case 0x80: // i64.div_u
        {
          StackValue *sv3;
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
          if (sv3 != sv) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv, SLJIT_R0);
          }
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
          if (sv3 != sv2) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv2, SLJIT_R1);
          }
          a = SLJIT_R0;
          sljit_emit_op0(m->jitc, SLJIT_DIV_UW);
        } break;
        case 0x81: // i64.rem_s
        {
          StackValue *sv3;
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
          if (sv3 != sv) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv, SLJIT_R0);
          }
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
          if (sv3 != sv2) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv2, SLJIT_R1);
          }
          a = SLJIT_R1;
          sljit_emit_op0(m->jitc, SLJIT_DIVMOD_SW);
        } break;
        case 0x82: // i64.rem_u
        {
          StackValue *sv3;
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R0, RT_INTEGER, 0);
          if (sv3 != sv) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv, SLJIT_R0);
          }
          sv3 = stackvalue_FindSvalueUseReg(m, SLJIT_R1, RT_INTEGER, 0);
          if (sv3 != sv2) {
            if (sv3 != NULL) {
              pwart_EmitSaveStack(m, sv3);
            }
            pwart_EmitLoadReg(m, sv2, SLJIT_R1);
          }
          a = SLJIT_R1;
          sljit_emit_op0(m->jitc, SLJIT_DIVMOD_UW);
        } break;
        case 0x83: // i64.and
          sljit_emit_op2(m->jitc, SLJIT_AND, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x84: // i64.or
          sljit_emit_op2(m->jitc, SLJIT_OR, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x85: // i64.xor
          sljit_emit_op2(m->jitc, SLJIT_XOR, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x86: // i64.shl
          sljit_emit_op2(m->jitc, SLJIT_SHL, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x87: // i64.shr_s
          sljit_emit_op2(m->jitc, SLJIT_ASHR, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x88: // i64.shr_u
          sljit_emit_op2(m->jitc, SLJIT_LSHR, a, 0, sv->val.op, sv->val.opw,
                         sv2->val.op, sv2->val.opw);
          break;
        case 0x89: // i64.rotl
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64rotl);
          break;
        case 0x8a: // i64.rotr
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64rotr);
          break;
        }
        if (opcode != 0x89 && opcode != 0x8a) {
          m->sp -= 2;
          sv = push_stackvalue(m, NULL);
          sv->jit_type = SVT_GENERAL;
          sv->wasm_type = WVT_I64;
          sv->val.op = a;
          sv->val.opw = 0;
        }
      } else if (m->target_ptr_size == 32) {
        switch (opcode) {
        case 0x7c: // i64.add
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          if (sv->jit_type == SVT_TWO_REG) {
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          } else if (sv2->jit_type == SVT_TWO_REG) {
            a = sv2->val.tworeg.opr1;
            b = sv2->val.tworeg.opr2;
          } else {
            pwart_EmitStackValueLoadReg(m, sv);
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          }
          stackvalue_LowWord(m, sv, &op1, &opw1);
          stackvalue_LowWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_ADD | SLJIT_SET_CARRY, a, 0, op1, opw1,
                         op2, opw2);
          stackvalue_HighWord(m, sv, &op1, &opw1);
          stackvalue_HighWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_ADDC, b, 0, op1, opw1, op2, opw2);
          m->sp--;
          sv->jit_type = SVT_TWO_REG;
          sv->val.tworeg.opr1 = a;
          sv->val.tworeg.opr2 = b;
          break;
        case 0x7d: // i64.sub
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          if (sv->jit_type == SVT_TWO_REG) {
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          } else if (sv2->jit_type == SVT_TWO_REG) {
            a = sv2->val.tworeg.opr1;
            b = sv2->val.tworeg.opr2;
          } else {
            pwart_EmitStackValueLoadReg(m, sv);
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          }
          stackvalue_LowWord(m, sv, &op1, &opw1);
          stackvalue_LowWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_SUB | SLJIT_SET_CARRY, a, 0, op1, opw1,
                         op2, opw2);
          stackvalue_HighWord(m, sv, &op1, &opw1);
          stackvalue_HighWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_SUBC, b, 0, op1, opw1, op2, opw2);
          m->sp--;
          sv->jit_type = SVT_TWO_REG;
          sv->val.tworeg.opr1 = a;
          sv->val.tworeg.opr2 = b;
          break;
        case 0x7e: // i64.mul
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64mul);
          break;
        case 0x7f: // i64.div_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64divs);
          break;
        case 0x80: // i64.div_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64divu);
          break;
        case 0x81: // i64.rem_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64rems);
          break;
        case 0x82: // i64.rem_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64remu);
          break;
        case 0x83: // i64.and
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          if (sv->jit_type == SVT_TWO_REG) {
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          } else if (sv2->jit_type == SVT_TWO_REG) {
            a = sv2->val.tworeg.opr1;
            b = sv2->val.tworeg.opr2;
          } else {
            pwart_EmitStackValueLoadReg(m, sv);
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          }
          stackvalue_LowWord(m, sv, &op1, &opw1);
          stackvalue_LowWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_AND, a, 0, op1, opw1, op2, opw2);
          stackvalue_HighWord(m, sv, &op1, &opw1);
          stackvalue_HighWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_AND, b, 0, op1, opw1, op2, opw2);
          m->sp--;
          sv->jit_type = SVT_TWO_REG;
          sv->val.tworeg.opr1 = a;
          sv->val.tworeg.opr2 = b;
          break;
        case 0x84: // i64.or
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          if (sv->jit_type == SVT_TWO_REG) {
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          } else if (sv2->jit_type == SVT_TWO_REG) {
            a = sv2->val.tworeg.opr1;
            b = sv2->val.tworeg.opr2;
          } else {
            pwart_EmitStackValueLoadReg(m, sv);
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          }
          stackvalue_LowWord(m, sv, &op1, &opw1);
          stackvalue_LowWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_OR, a, 0, op1, opw1, op2, opw2);
          stackvalue_HighWord(m, sv, &op1, &opw1);
          stackvalue_HighWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_OR, b, 0, op1, opw1, op2, opw2);
          m->sp--;
          sv->jit_type = SVT_TWO_REG;
          sv->val.tworeg.opr1 = a;
          sv->val.tworeg.opr2 = b;
          break;
        case 0x85: // i64.xor
          sv = &stack[m->sp - 1];
          sv2 = &stack[m->sp];
          if (sv->jit_type == SVT_TWO_REG) {
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          } else if (sv2->jit_type == SVT_TWO_REG) {
            a = sv2->val.tworeg.opr1;
            b = sv2->val.tworeg.opr2;
          } else {
            pwart_EmitStackValueLoadReg(m, sv);
            a = sv->val.tworeg.opr1;
            b = sv->val.tworeg.opr2;
          }
          stackvalue_LowWord(m, sv, &op1, &opw1);
          stackvalue_LowWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_XOR, a, 0, op1, opw1, op2, opw2);
          stackvalue_HighWord(m, sv, &op1, &opw1);
          stackvalue_HighWord(m, sv2, &op2, &opw2);
          sljit_emit_op2(m->jitc, SLJIT_XOR, b, 0, op1, opw1, op2, opw2);
          m->sp--;
          sv->jit_type = SVT_TWO_REG;
          sv->val.tworeg.opr1 = a;
          sv->val.tworeg.opr2 = b;
          break;
        case 0x86: // i64.shl
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64shl);
          break;
        case 0x87: // i64.shr_s
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64shrs);
          break;
        case 0x88: // i64.shr_u
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64shru);
          break;
        case 0x89: // i64.rotl
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64rotl);
          break;
        case 0x8a: // i64.rotr
          pwart_EmitCallFunc(m, &func_type_i64_i64_ret_i64, SLJIT_IMM,
                             (sljit_sw)&insn_i64rotr);
          break;
        }
      }
      break;
    // f32 binary
    case 0x92 ... 0x98:
      if (opcode <= 0x95) {
        sv = &stack[m->sp - 1];
        sv2 = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_FLOAT, 2);
      }
      switch (opcode) {
      case 0x92: // f32.add
        sljit_emit_fop2(m->jitc, SLJIT_ADD_F32, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0x93: // f32.sub
        sljit_emit_fop2(m->jitc, SLJIT_SUB_F32, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0x94: // f32.mul
        sljit_emit_fop2(m->jitc, SLJIT_MUL_F32, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0x95: // f32.div
        sljit_emit_fop2(m->jitc, SLJIT_DIV_F32, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0x96: // f32.min
        pwart_EmitCallFunc(m, &func_type_f32_f32_ret_f32, SLJIT_IMM,
                           (sljit_sw)&insn_f32min);
        break;
      case 0x97: // f32.max
        pwart_EmitCallFunc(m, &func_type_f32_f32_ret_f32, SLJIT_IMM,
                           (sljit_sw)&insn_f32max);
        break;
      case 0x98: // f32.copysign
        pwart_EmitCallFunc(m, &func_type_f32_f32_ret_f32, SLJIT_IMM,
                           (sljit_sw)&insn_f32copysign);
        break;
      }
      if (opcode <= 0x95) {
        m->sp -= 2;
        sv = push_stackvalue(m, NULL);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_F64;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;

    // f64 binary
    case 0xa0 ... 0xa6:
      if (opcode <= 0xa3) {
        sv = &stack[m->sp - 1];
        sv2 = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_FLOAT, 2);
      }
      switch (opcode) {
      case 0xa0: // f64.add
        sljit_emit_fop2(m->jitc, SLJIT_ADD_F64, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0xa1: // f64.sub
        sljit_emit_fop2(m->jitc, SLJIT_SUB_F64, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0xa2: // f64.mul
        sljit_emit_fop2(m->jitc, SLJIT_MUL_F64, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0xa3: // f64.div
        sljit_emit_fop2(m->jitc, SLJIT_DIV_F64, a, 0, sv->val.op, sv->val.opw,
                        sv2->val.op, sv2->val.opw);
        break;
      case 0xa4: // f64.min
        pwart_EmitCallFunc(m, &func_type_f64_f64_ret_f64, SLJIT_IMM,
                           (sljit_sw)&insn_f64min);
        break;
      case 0xa5: // f64.max
        pwart_EmitCallFunc(m, &func_type_f64_f64_ret_f64, SLJIT_IMM,
                           (sljit_sw)&insn_f64max);
        break;
      case 0xa6: // f64.copysign
        pwart_EmitCallFunc(m, &func_type_f64_f64_ret_f64, SLJIT_IMM,
                           (sljit_sw)&insn_f64copysign);
        break;
      }
      if (opcode <= 0xa3) {
        m->sp -= 2;
        sv = push_stackvalue(m, NULL);
        sv->jit_type = SVT_GENERAL;
        sv->wasm_type = WVT_F64;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;

    // conversion operations
    // case 0xa7 ... 0xbb:
    case 0xa7: // i32.wrap_i64
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
        if (sv->jit_type == SVT_TWO_REG) {
          sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.op, sv->val.opw);
        } else if (sv->jit_type == SVT_I64CONST) {
          sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_IMM,
                         (sljit_sw)sv->val.const64 & 0xffffffff);
        }
      } else {
        sljit_emit_op2(m->jitc, SLJIT_AND, a, 0, sv->val.op, sv->val.opw,
                       SLJIT_IMM, 0xffffffff);
      }
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xa8: // i32.trunc_f32_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_S32_FROM_F32, a, 0, sv->val.op,
                      sv->val.opw);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xa9: // i32.trunc_f32_u
      pwart_EmitCallFunc(m, &func_type_f32_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i32truncf32u);
      break;
    case 0xaa: // i32.trunc_f64_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_S32_FROM_F64, a, 0, sv->val.op,
                      sv->val.opw);
      sv->wasm_type = WVT_I32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xab: // i32.trunc_f64_u
      pwart_EmitCallFunc(m, &func_type_f64_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i32truncf64u);
      break;
    case 0xac: // i64.extend_i32_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op1(m->jitc, SLJIT_MOV_S32, a, 0, sv->val.op, sv->val.opw);
      sv->wasm_type = WVT_I64;
      if (m->target_ptr_size == 32) {
        b = pwart_GetFreeRegExcept(m, RT_INTEGER, a, 1);
        sljit_emit_op2(m->jitc, SLJIT_ASHR, b, 0, a, 0, SLJIT_IMM, 31);
        sv->jit_type = SVT_TWO_REG;
        sv->val.tworeg.opr1 = a;
        sv->val.tworeg.opr2 = b;
      } else {
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xad: // i64.extend_i32_u
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op1(m->jitc, SLJIT_MOV_U32, a, 0, sv->val.op, sv->val.opw);
      sv->wasm_type = WVT_I64;
      if (m->target_ptr_size == 32) {
        b = pwart_GetFreeRegExcept(m, RT_INTEGER, a, 1);
        sljit_emit_op1(m->jitc, SLJIT_MOV, b, 0, SLJIT_IMM, 0);
        sv->jit_type = SVT_TWO_REG;
        sv->val.tworeg.opr1 = a;
        sv->val.tworeg.opr2 = b;
      } else {
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xae: // i64.trunc_f32_s
      if (m->target_ptr_size == 32) {
        pwart_EmitCallFunc(m, &func_type_f32_ret_i64, SLJIT_IMM,
                           (sljit_sw)&insn_i64truncf32s);
      } else {
        sv = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_INTEGER, 1);
        sljit_emit_fop1(m->jitc, SLJIT_CONV_SW_FROM_F32, a, 0, sv->val.op,
                       sv->val.opw);
        sv->wasm_type = WVT_I64;
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      }
      break;
    case 0xaf: // i64.trunc_f32_u
      pwart_EmitCallFunc(m, &func_type_f32_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64truncf32u);
      break;
    case 0xb0: // i64.trunc_f64_s
      if (m->target_ptr_size == 32) {
        pwart_EmitCallFunc(m, &func_type_f32_ret_i64, SLJIT_IMM,
                           (sljit_sw)&insn_i64truncf64s);
      } else {
        sv = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_INTEGER, 1);
        sljit_emit_fop1(m->jitc, SLJIT_CONV_SW_FROM_F64, a, 0, sv->val.op,
                        sv->val.opw);
        sv->wasm_type = WVT_I64;
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
        break;
      }
    case 0xb1: // i64.trunc_f64_u
      pwart_EmitCallFunc(m, &func_type_f32_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64truncf64u);
      break;
    case 0xb2: // f32.convert_i32_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_F32_FROM_S32, a, 0, sv->val.op,
                     sv->val.opw);
      sv->wasm_type = WVT_F32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xb3: // f32.convert_i32_u
      pwart_EmitCallFunc(m, &func_type_i32_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32converti32u);
      break;
    case 0xb4: // f32.convert_i64_s
      if (m->target_ptr_size == 32) {
        pwart_EmitCallFunc(m, &func_type_i64_ret_f32, SLJIT_IMM,
                           (sljit_sw)&insn_f32converti64s);
      } else {
        sv = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_FLOAT, 1);
        sljit_emit_fop1(m->jitc, SLJIT_CONV_F32_FROM_SW, a, 0, sv->val.op,
                       sv->val.opw);
        sv->wasm_type = WVT_F32;
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xb5: // f32.convert_i64_u
      pwart_EmitCallFunc(m, &func_type_i64_ret_f32, SLJIT_IMM,
                         (sljit_sw)&insn_f32converti64u);
      break;
    case 0xb6: // f32.demote_f64
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_F32_FROM_F64, a, 0, sv->val.op,
                     sv->val.opw);
      sv->wasm_type = WVT_F32;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xb7: // f64.convert_i32_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_F64_FROM_S32, a, 0, sv->val.op,
                     sv->val.opw);
      sv->wasm_type = WVT_F64;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xb8: // f64.convert_i32_u
      pwart_EmitCallFunc(m, &func_type_i32_ret_f64, SLJIT_IMM,
                         (sljit_sw)insn_f64converti32u);
      break;
    case 0xb9: // f64.convert_i64_s
      if (m->target_ptr_size == 32) {
        pwart_EmitCallFunc(m, &func_type_i64_ret_f64, SLJIT_IMM,
                           (sljit_sw)insn_f64converti64s);
      } else {
        sv = &stack[m->sp];
        a = pwart_GetFreeReg(m, RT_FLOAT, 1);
        sljit_emit_fop1(m->jitc, SLJIT_CONV_F64_FROM_SW, a, 0, sv->val.op,
                       sv->val.opw);
        sv->wasm_type = WVT_F64;
        sv->jit_type = SVT_GENERAL;
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xba: // f64.convert_i64_u
      pwart_EmitCallFunc(m, &func_type_i64_ret_f64, SLJIT_IMM,
                         (sljit_sw)insn_f64converti64u);
      break;
    case 0xbb: // f64.promote_f32
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_FLOAT, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_F64_FROM_F32, a, 0, sv->val.op,
                     sv->val.opw);
      sv->wasm_type = WVT_F64;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
      break;

    // reinterpretations
    case 0xbc: // i32.reinterpret_f32
      sv = &stack[m->sp];
      if (!(sv->val.op & SLJIT_MEM)) {
        pwart_EmitSaveStack(m, sv);
      }
      sv->wasm_type = WVT_I32;
      break;
    case 0xbd: // i64.reinterpret_f64
      sv = &stack[m->sp];
      if (!(sv->val.op & SLJIT_MEM)) {
        pwart_EmitSaveStack(m, sv);
      }
      sv->wasm_type = WVT_I64;
      break;
    case 0xbe: // f32.reinterpret_i32
      sv = &stack[m->sp];
      if (!(sv->val.op & SLJIT_MEM)) {
        pwart_EmitSaveStack(m, sv);
      }
      sv->wasm_type = WVT_F32;
      break;
    case 0xbf: // f64.reinterpret_i64
      sv = &stack[m->sp];
      if (!(sv->jit_type = SVT_GENERAL && (sv->val.op & SLJIT_MEM))) {
        pwart_EmitSaveStack(m, sv);
      }
      sv->wasm_type = WVT_I64;
      break;
    case 0xc0: // i32.extend8_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op1(m->jitc, SLJIT_MOV32_S8, a, 0, sv->val.op, sv->val.opw);
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xc1: // i32.extend16_s
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_op1(m->jitc, SLJIT_MOV32_S16, a, 0, sv->val.op, sv->val.opw);
      sv->val.op = a;
      sv->val.opw = 0;
      break;
    case 0xc2: // i64.extend8_s
      sv = &stack[m->sp];
      if (m->target_ptr_size == 32) {
        pwart_EmitStackValueLoadReg(m, sv);
        sljit_emit_op1(m->jitc, SLJIT_MOV_S8, sv->val.tworeg.opr1, 0,
                       sv->val.op, sv->val.opw);
        sljit_emit_op2(m->jitc, SLJIT_ASHR, sv->val.tworeg.opr2, 0,
                       sv->val.tworeg.opr1, 0, SLJIT_IMM, 31);
      } else {
        a = pwart_GetFreeReg(m, RT_INTEGER, 1);
        sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, sv->val.op, sv->val.opw);
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xc3: // i64.extend16_s
      sv = &stack[m->sp];
      if (m->target_ptr_size == 32) {
        pwart_EmitStackValueLoadReg(m, sv);
        sljit_emit_op1(m->jitc, SLJIT_MOV_S16, sv->val.tworeg.opr1, 0,
                       sv->val.op, sv->val.opw);
        sljit_emit_op2(m->jitc, SLJIT_ASHR, sv->val.tworeg.opr2, 0,
                       sv->val.tworeg.opr1, 0, SLJIT_IMM, 31);
      } else {
        a = pwart_GetFreeReg(m, RT_INTEGER, 1);
        sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, sv->val.op, sv->val.opw);
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;
    case 0xc4: // i64.extend32_s
      sv = &stack[m->sp];
      if (m->target_ptr_size == 32) {
        pwart_EmitStackValueLoadReg(m, sv);
        sljit_emit_op2(m->jitc, SLJIT_ASHR, sv->val.tworeg.opr2, 0,
                       sv->val.tworeg.opr1, 0, SLJIT_IMM, 31);
      } else {
        a = pwart_GetFreeReg(m, RT_INTEGER, 1);
        sljit_emit_op1(m->jitc, SLJIT_MOV_S8, a, 0, sv->val.op, sv->val.opw);
        sv->val.op = a;
        sv->val.opw = 0;
      }
      break;

    case 0xd0:                            // ref.null
      tidx = read_LEB(bytes, &m->pc, 32); // ref type
      sv = push_stackvalue(m, sv);
      sv->wasm_type = tidx;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_IMM;
      sv->val.opw = 0;
      break;
    case 0xd1: // ref.isnull
      sv = &stack[m->sp];
      if (sv->wasm_type == WVT_REF || sv->wasm_type == WVT_FUNC) {
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                        sv->val.opw, SLJIT_IMM, 0);
        sv->wasm_type = WVT_I32;
        sv->jit_type = SVT_CMP;
        sv->val.cmp.flag = SLJIT_EQUAL;
      } else {
        sv->wasm_type = WVT_I32;
        sv->jit_type = SVT_GENERAL;
        sv->val.opw = 0;
        sv->val.op = SLJIT_IMM;
      }
      break;
    case 0xd2: // ref.func
      fidx = read_LEB(bytes, &m->pc, 32);
      a = pwart_GetFreeReg(m, RT_INTEGER, 0);
      sv = dynarr_get(m->locals, StackValue, m->functions_base_local);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, sizeof(void *) * fidx);
      sv = push_stackvalue(m, NULL);
      sv->wasm_type = WVT_FUNC;
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_MEM1(a);
      sv->val.opw = 0;
      break;
      break;

    default:
      wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
      exit(1);
      break;
    }
  }
  dynarr_free(&m->br_table);
  dynarr_free(&m->blocks);
  return 0;
}

static WasmFunctionEntry pwart_EmitFunction(Module *m, WasmFunction *func) {
  WasmFunctionEntry code;
  uint32_t savepos = m->pc;

  // Empty stacks
  m->sp = -1;

  m->function_type = dynarr_get(m->types, Type, func->tidx);
  m->function_locals_type = func->locals_type;

  m->locals = NULL;
  dynarr_init(&m->locals, sizeof(StackValue));

  pwart_PrepareFunc(m);
  m->pc = savepos;
  pwart_GenCode(m);
  code = (WasmFunctionEntry)sljit_generate_code(m->jitc);
  sljit_free_compiler(m->jitc);
  dynarr_free(&m->locals);
  func->func_ptr = code;
  return code;
}
static void pwart_FreeFunction(WasmFunctionEntry code) {
  sljit_free_code(code, NULL);
}

#endif