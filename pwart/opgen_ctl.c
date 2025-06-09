#ifndef _PWART_OPGEN_CTL_C
#define _PWART_OPGEN_CTL_C

// Control flow operators,Call operators ,Parametric operators

#include "def.h"
#include "opgen_utils.c"

static void opgen_GenUnreachable(ModuleCompiler *m) {
  //just do nothing now.
}

static void opgen_GenBlock(ModuleCompiler *m,int stackOffset) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x2;
  dynarr_init(&block->br_jump, sizeof(struct sljit_jump *));
  block->else_jump = NULL;
  block->stack_offset=stackOffset;
  block->saved_sp=m->sp;
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
}
/* blktype 0x40 indicating the empty type. */
static void opgen_GenLoop(ModuleCompiler *m,int stackOffset) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  block = dynarr_push_type(&m->blocks, Block);
  block->block_type = 0x3;
  block->br_label = sljit_emit_label(m->jitc);
  block->stack_offset=stackOffset;
  block->saved_sp=m->sp;
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
}
/* blktype 0x40 indicating the empty type. */
static void opgen_GenIf(ModuleCompiler *m,int stackOffset) {
  Block *block;
  StackValue *sv;
  struct sljit_jump *jump;
  StackValue *stack = m->stack;
  // stack top will be remove soon. Don't save it here.
  for (int i = 0; i < m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  #if PWART_DEBUG_RUNTIME_PROBE
  pwart_EmitSaveStackAll(m);
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
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
  block->stack_offset=stackOffset;
  block->saved_sp=m->sp;
  
}

static void opgen_GenElse(ModuleCompiler *m) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  block = dynarr_get(m->blocks, Block, m->blocks->len - 1);
  *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
      sljit_emit_jump(m->jitc, SLJIT_JUMP);
  sljit_set_label(block->else_jump, sljit_emit_label(m->jitc));
  block->else_jump = NULL;
  m->sp=block->saved_sp;
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
}

static void opgen_GenEnd(ModuleCompiler *m) {
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
    if(!m->block_returned){
      pwart_EmitFuncReturn(m);
    }
    m->eof = 1;
  } 
  m->sp=block->saved_sp+block->stack_offset;
  if (block->br_jump != NULL) {
    dynarr_free(&block->br_jump);
  }
  m->block_returned=0;
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
}

static void opgen_GenBr(ModuleCompiler *m, int32_t depth) {
  Block *block;
  pwart_EmitSaveStackAll(m);
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
  block = dynarr_get(m->blocks, Block, m->blocks->len - 1 - depth);
  if (block->br_label != NULL) {
    sljit_set_label(sljit_emit_jump(m->jitc, SLJIT_JUMP), block->br_label);
  } else {
    *dynarr_push_type(&block->br_jump, struct sljit_jump *) =
        sljit_emit_jump(m->jitc, SLJIT_JUMP);
  }
  if(block->block_type==0x2 || block->block_type==0x4){
    m->block_returned=1;
  }
}

static void opgen_GenBrIf(ModuleCompiler *m, int32_t depth) {
  StackValue *sv;
  Block *block;
  StackValue *stack = m->stack;
  sv = &stack[m->sp];
  // stack top will be remove soon. Don't save it here.
  for (int i = 0; i < m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  #if PWART_DEBUG_RUNTIME_PROBE
  pwart_EmitSaveStackAll(m);
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
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

static void opgen_GenBrTable(ModuleCompiler *m) {
  int32_t depth;
  int a, count;
  Block *block;
  StackValue *sv;
  sv=m->stack+m->sp;
  count = m->br_table->len-1;
  for (int i = 0; i < m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  #if PWART_DEBUG_RUNTIME_PROBE
  pwart_EmitSaveStackAll(m);
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc-1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintBlockInstr);
  #endif
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

static void opgen_GenReturn(ModuleCompiler *m) {
  #if DEBUG_BUILD
  debug_checkreturnstack(m);
  #endif
  pwart_EmitFuncReturn(m);
  // pop all value in stack, to avoid unnecessary value saved.
  m->sp = -1;
  m->block_returned=1;
}

static void opgen_GenCall(ModuleCompiler *m, int32_t fidx, int callReturn) {
  WasmFunction *fn;
  int a;
  StackValue *sv;
  Type *type;
  fn = dynarr_get(m->functions, WasmFunction, fidx);
  if(!pwart_CheckAndGenStubFunction(m,fn->func_ptr)){
    a = pwart_GetFreeReg(m, RT_BASE, 0);
    sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_IMM,(sljit_uw)(m->context->funcentries+fidx));
    type = dynarr_get(m->types, Type, fn->tidx);
    pwart_EmitCallFunc2(m, type, SLJIT_MEM1(a), 0,callReturn);
  }
}

static void opgen_GenCallIndirect(ModuleCompiler *m, int32_t typeidx,
                                  int32_t tableidx, int callReturn) {
  Type *type;
  int a=0;
  type = dynarr_get(m->types, Type, typeidx);
  opgen_GenBaseAddressRegForTable(m,tableidx);
  a=m->stack[m->sp].val.op;
  m->sp--;
  pwart_EmitCallFunc2(m, type, SLJIT_MEM1(a), 0, callReturn);
}

static void opgen_GenDrop(ModuleCompiler *m) { m->sp--; }

static void opgen_GenSelect(ModuleCompiler *m) {
  // must save to stack.
  StackValue *sv, *sv2;
  StackValue *stack = m->stack;
  struct sljit_jump *jump;
  pwart_EmitSaveStack(m, &stack[m->sp - 2]);
  sv = &stack[m->sp];
  m->sp--;

  if (sv->jit_type == SVT_CMP) {
    jump = sljit_emit_jump(m->jitc, sv->val.cmp.flag);
  } else if (sv->jit_type == SVT_GENERAL) {
    // sljit_emit_cmp always use machine word length, So load to register first.
    pwart_EmitStackValueLoadReg(m,sv);
    jump = sljit_emit_cmp(m->jitc, SLJIT_NOT_EQUAL, sv->val.op, sv->val.opw,
                          SLJIT_IMM, 0);
  } else {
    SLJIT_UNREACHABLE();
  }
  sv2 = &stack[m->sp];
  m->sp--;
  sv = &stack[m->sp];
  sv->val = sv2->val;
  sv->jit_type = sv2->jit_type;
  pwart_EmitSaveStack(m, sv);
  sljit_set_label(jump, sljit_emit_label(m->jitc));
}

static char *opgen_ParseBlockStackOffset(ModuleCompiler *m,int *stackOffset){
  if(*(m->bytes+m->pc)==0x40){
    *stackOffset=0;
    m->pc++;
  }else if(((*(m->bytes+m->pc))&0xC0)==0x40){
    *stackOffset=1;
    m->pc++;
  }else{
    return "Not support type index for block type";
  }
  return NULL;
}

static char *opgen_GenCtlOp(ModuleCompiler *m, int opcode) {
  uint8_t *bytes = m->bytes;
  int32_t blktype, depth, count, *i32p, fidx, tidx,tabidx;

  switch (opcode) {
  case 0x00: // unreachable
    opgen_GenUnreachable(m);
    break;
  case 0x01: // nop
    break;
  case 0x02: // block
    ReturnIfErr(opgen_ParseBlockStackOffset(m,&count));
    opgen_GenBlock(m,count);
    break;
  case 0x03: // loop
    ReturnIfErr(opgen_ParseBlockStackOffset(m,&count));
    opgen_GenLoop(m,count);
    break;
  case 0x04: // if
    ReturnIfErr(opgen_ParseBlockStackOffset(m,&count));
    opgen_GenIf(m,count);
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
      i32p = (int32_t *)dynarr_push_type(&m->br_table, uint32_t);
      *i32p = read_LEB(bytes, &m->pc, 32);
    }
    opgen_GenBrTable(m);
    break;
  case 0x0f: // return
    opgen_GenReturn(m);
    break;
  case WASMOPC_call:
  case WASMOPC_return_call: 
    fidx = read_LEB(bytes, &m->pc, 32);
    opgen_GenCall(m, fidx,opcode==WASMOPC_return_call);
    break;
  case WASMOPC_call_indirect:
  case WASMOPC_return_call_indirect:
    tidx = read_LEB(bytes, &m->pc, 32);
    tabidx=read_LEB(bytes, &m->pc, 1); // tableidx. only support 1 table now.
    opgen_GenCallIndirect(m, tidx, tabidx,opcode==WASMOPC_return_call_indirect);
    break;
  //
  // Parametric operators
  //
  case 0x1a: // drop
    opgen_GenDrop(m);
    break;
  case WASMOPC_select_t:                     // select t
    m->pc++;
  case WASMOPC_select:                     // select
    opgen_GenSelect(m);
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    return "unrecognized opcode";
  }
  return NULL;
}

#endif