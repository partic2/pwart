#ifndef _PWART_OPGEN_CTL_C
#define _PWART_OPGEN_CTL_C

// Control flow operators,Call operators ,Parametric operators

#include "def.h"
#include "opgen_utils.c"

static void opgen_GenUnreachable(Module *m) {
  sprintf(exception, "%s", "unreachable");
  sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)&m);
  sljit_emit_call(m->jitc, SLJIT_CALL, SLJIT_ARGS1(VOID, P));
}

static void opgen_GenBlock(Module *m) {
  Block *block;
  uint8_t *bytes = m->bytes;
  pwart_EmitSaveStackAll(m);
  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x2;
  dynarr_init(&block->br_jump, sizeof(struct sljit_jump *));
  block->else_jump = NULL;
}

static void opgen_GenLoop(Module *m) {
  Block *block;
  uint8_t *bytes = m->bytes;
  pwart_EmitSaveStackAll(m);
  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x3;
  block->br_label = sljit_emit_label(m->jitc);
}

static void opgen_GenIf(Module *m) {
  Block *block;
  StackValue *sv;
  struct sljit_jump *jump;
  uint8_t *bytes = m->bytes;
  StackValue *stack = m->stack;
  // stack top will be remove soon. Don't save it here.
  for (int i = 0; i < m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x4;
  dynarr_init(&block->br_jump, sizeof(struct sljit_jump *));
  sv = &stack[m->sp];
  if (sv->jit_type == SVT_CMP) {
    //inverse flag, we supposed inverse flag are always set.
    jump = sljit_emit_jump(m->jitc,sv->val.cmp.flag^0x1);
  } else {
    // sljit_emit_cmp always use machine word length, So load to register first.
    pwart_EmitStackValueLoadReg(m, sv);
    jump = sljit_emit_cmp(m->jitc, SLJIT_EQUAL, sv->val.op, sv->val.opw,
                          SLJIT_IMM, 0);
  }
  block->else_jump = jump;
  m->sp--;
}

static void opgen_GenElse(Module *m) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  block = dynarr_get(m->blocks, Block, m->blocks->len - 1);
  *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
      sljit_emit_jump(m->jitc, SLJIT_JUMP);
  sljit_set_label(block->else_jump, sljit_emit_label(m->jitc));
  block->else_jump = NULL;
}

static void opgen_GenEnd(Module *m) {
  struct sljit_jump *sj;
  Block *block = dynarr_pop_type(&m->blocks, Block);
  int a;
  if (block->block_type == 0x2 || block->block_type == 0x4) {
    pwart_EmitSaveStackAll(m);
    // if block
    block->br_label = sljit_emit_label(m->jitc);
    for (a = 0; a < block->br_jump->len; a++) {
      sj = *dynarr_get(block->br_jump, struct sljit_jump *, a);
      sljit_set_label(sj, block->br_label);
    }
    // if block, ensure else_jump have target.
    if (block->else_jump != NULL) {
      sljit_set_label(block->else_jump, block->br_label);
    }

  } else if (block->block_type == 0x00) {
    // function
    //Check if there is already a redundant return instruction, if so, skip pwart_EmitFuncReturn.
    //XXX:May we need better way to avoid access m->bytes directly.
    if(m->bytes[m->pc-2]!=0x0f){
      pwart_EmitFuncReturn(m);
    }
    wa_assert(m->blocks->len == 0,"block stack not empty");
    m->eof = 1;
  }
  if (block->br_jump != NULL) {
    dynarr_free(&block->br_jump);
  }
}

static void opgen_GenBr(Module *m, int32_t depth) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
  if (block->br_label != NULL) {
    sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_JUMP), block->br_label);
  } else {
    *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
        sljit_emit_jump(m->jitc, SLJIT_JUMP);
  }
}

static void opgen_GenBrIf(Module *m, int32_t depth) {
  StackValue *sv;
  Block *block;
  StackValue *stack = m->stack;
  sv = &stack[m->sp];
  // stack top will be remove soon. Don't save it here.
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
    // sljit_emit_cmp always use machine word length, So load to register first.
    pwart_EmitStackValueLoadReg(m, sv);
    if (block->br_label != NULL) {
      sljit_set_label(sljit_emit_cmp(m->jitc, SLJIT_NOT_EQUAL, sv->val.op,
                                     sv->val.opw, SLJIT_IMM, 0),
                      block->br_label);
    } else {
      *dynarr_push_type(&block->br_jump, struct sljit_jump *) = sljit_emit_cmp(
          m->jitc, SLJIT_NOT_EQUAL, sv->val.op, sv->val.opw, SLJIT_IMM, 0);
    }
  } else {
    SLJIT_UNREACHABLE();
  }
  m->sp--;
}

static void opgen_GenBrTable(Module *m) {
  int32_t depth;
  int a, count;
  Block *block;
  StackValue *sv;
  sv=m->stack+m->sp;
  count = m->br_table->len-1;
  for (int i = 0; i < m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  pwart_EmitStackValueLoadReg(m, sv);
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
      sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_ZERO), block->br_label);
    } else {
      *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
          sljit_emit_jump(m->jitc, SLJIT_ZERO);
    }
  }

  // default
  depth = *dynarr_get(m->br_table, uint32_t, count);
  block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
  if (block->br_label != NULL) {
    sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_JUMP), block->br_label);
  } else {
    *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
        sljit_emit_jump(m->jitc, SLJIT_JUMP);
  }
  m->sp--;
}

static void opgen_GenReturn(Module *m) {
  pwart_EmitFuncReturn(m);
  // pop all value in stack, to avoid unnecessary value saved.
  m->sp = -1;
}

static void opgen_GenCall(Module *m, int32_t fidx) {
  WasmFunction *fn;
  int a;
  StackValue *sv;
  Type *type;
  fn = dynarr_get(m->functions, WasmFunction, fidx);
  if(!pwart_CheckAndGenStubFunction(m,fn->func_ptr)){
    a = pwart_GetFreeReg(m, RT_INTEGER, 0);
    sv = dynarr_get(m->locals, StackValue, m->functions_base_local);
    sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                  sizeof(void *) * fidx);
    type = dynarr_get(m->types, Type, fn->tidx);
    pwart_EmitCallFunc(m, type, SLJIT_MEM1(a), 0);
  }
}

static void opgen_GenCallIndirect(Module *m, int32_t typeidx,
                                  int32_t tableidx) {
  Type *type;
  StackValue *sv;
  int a;
  StackValue *stack = m->stack;
  type = dynarr_get(m->types, Type, typeidx);
  sv = &stack[m->sp];
  a = pwart_GetFreeReg(m, RT_INTEGER, 1);
  sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                 sizeof(void *) == 4 ? 2 : 3);

  sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
  sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv->val.op, sv->val.opw);
  m->sp--;

  pwart_EmitCallFunc(m, type, SLJIT_MEM1(a), 0);
}

static void opgen_GenDrop(Module *m) { m->sp--; }

static void opgen_GenSelect(Module *m) {
  // must save to stack.
  StackValue *sv, *sv2;
  StackValue *stack = m->stack;
  struct sljit_jump *jump;
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
}

static void opgen_GenCtlOp(Module *m, int opcode) {
  uint8_t *bytes = m->bytes;
  int32_t blktype, depth, count, *i32p, fidx, tidx;

  switch (opcode) {
  case 0x00: // unreachable
    opgen_GenUnreachable(m);
    break;
  case 0x01: // nop
    break;
  case 0x02: // block
    blktype = read_LEB_signed(bytes, &m->pc, 33);
    opgen_GenBlock(m);
    break;
  case 0x03: // loop
    blktype = read_LEB_signed(bytes, &m->pc, 33);
    opgen_GenLoop(m);
    break;
  case 0x04: // if
    blktype = read_LEB_signed(bytes, &m->pc, 33);
    opgen_GenIf(m);
    break;
  case 0x05: // else
    opgen_GenElse(m);
    break;
  case 0x0b: // end
    opgen_GenEnd(m);
    break;
  case 0x0c: // br
    depth = read_LEB(bytes, &m->pc, 32);
    opgen_GenBr(m, depth);
    break;
  case 0x0d: // br_if
    depth = read_LEB(bytes, &m->pc, 32);
    opgen_GenBrIf(m, depth);
    break;
  case 0x0e: // br_table
    count = read_LEB(bytes, &m->pc, 32);
    // reset br_table
    m->br_table->len = 0;
    for (uint32_t i = 0; i <= count; i++) {
      i32p = dynarr_push_type(&m->br_table, uint32_t);
      *i32p = read_LEB(bytes, &m->pc, 32);
    }
    opgen_GenBrTable(m);
    break;
  case 0x0f: // return
    opgen_GenReturn(m);
    break;
  case 0x10: // call
    fidx = read_LEB(bytes, &m->pc, 32);
    opgen_GenCall(m, fidx);
    break;
  case 0x11: // call_indirect
    tidx = read_LEB(bytes, &m->pc, 32);
    read_LEB(bytes, &m->pc, 1); // tableidx. only support 1 table now.
    opgen_GenCallIndirect(m, tidx, 0);
    break;
  //
  // Parametric operators
  //
  case 0x1a: // drop
    opgen_GenDrop(m);
    break;
  case 0x1c:                     // select t
    read_LEB(bytes, &m->pc, 32); // ignore type
  case 0x1b:                     // select
    opgen_GenSelect(m);
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    exit(1);
    break;
  }
}

#endif