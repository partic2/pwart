#ifndef _PWART_OPGEN_UTILS_C
#define _PWART_OPGEN_UTILS_C

#include "def.h"
#include <stddef.h>

//
// Stack machine (byte code related functions)
//
static uint32_t wasmtype_GetSizeAndAlign(int wasm_type,uint32_t *align){
  switch (wasm_type) {
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
static int stackvalue_IsFloat(StackValue *sv) {
  return sv->wasm_type == WVT_F32 || sv->wasm_type == WVT_F64;
}
static uint32_t stackvalue_GetSizeAndAlign(StackValue *sv, uint32_t *align) {
  return wasmtype_GetSizeAndAlign(sv->wasm_type,align);
}
//return offset where can place the StackValue 'sv' behind the free 'offset', set the next free offset to 'nextOffset', if not NULL.
static uint32_t stackvalue_GetAlignedOffset(StackValue *sv,uint32_t offset,uint32_t *nextOffset){
  uint32_t align,size,t;
  size=stackvalue_GetSizeAndAlign(sv,&align);
  t=offset&(align-1);
  if(t>0){
    offset=offset+align-t;
  }
  if(nextOffset!=NULL){
    *nextOffset=offset+size;
  }
  return offset;
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
static StackValue *stackvalue_Push(Module *m, int32_t wasm_type) {
  StackValue *sv2,*sv;
  uint32_t align,a;
  sv2 = &m->stack[++m->sp];
  sv2->jit_type = SVT_GENERAL;
  sv2->wasm_type=wasm_type;
  if (sv2 - m->stack == 0) {
    // first stack value
    sv2->frame_offset = m->first_stackvalue_offset;
  } else {
    sv = sv2 - 1;
    sv2->frame_offset = sv->frame_offset + stackvalue_GetSizeAndAlign(sv, NULL);
    if(m->cfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
      sv2->frame_offset=stackvalue_GetAlignedOffset(sv2,sv2->frame_offset,NULL);
    }
  }
  return sv2;
}

static StackValue *stackvalue_PushStackValueLike(Module *m,StackValue *refsv){
  StackValue *sv;
  sv=stackvalue_Push(m,refsv->wasm_type);
  sv->jit_type=refsv->jit_type;
  sv->val=refsv->val;
  return sv;
}

static void stackvalue_EmitSwapTopTwoValue(Module *m) {
  StackValue *sv;
  // copy stack[sp-1]
  sv = stackvalue_PushStackValueLike(m, &m->stack[m->sp - 1]);
  if (sv->jit_type == SVT_GENERAL && sv->val.op == SLJIT_MEM1(SLJIT_S0) &&
      sv->val.opw >= m->first_stackvalue_offset) {
    // value are on stack, we need save it otherwhere.
    pwart_EmitStackValueLoadReg(m, sv);
  }
  m->sp -= 3;
  stackvalue_PushStackValueLike(m, &m->stack[m->sp + 2]);
  stackvalue_PushStackValueLike(m, &m->stack[m->sp + 2]);
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
  StackValue *sv,sv2;
  uint32_t a, b, len,func_frame_offset;
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
  len = strlen(type->params);
  if(len == 0){
    //no argument, push dummy stackvalue
    stackvalue_Push(m,WVT_I32);
    m->sp--;
  }else{
    m->sp -= len;
  }
  sv = m->stack+m->sp + 1; // first argument

  if((m->cfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN) && ((sv->frame_offset&7)>0)){
    //XXX: move stack value to align frame to 8 ,we should avoid this happen.
    sv->frame_offset+=4;
    if(len>0){
      b=sv->frame_offset+stackvalue_GetSizeAndAlign(sv,NULL);
      m->sp+=len;
      for(a=1;a<len;a++){
        sv++;
        sv->frame_offset = stackvalue_GetAlignedOffset(sv,b,&b);
      }
      //re-save stackvalue from last to head, to avoid stack overwriting
      sv=m->stack+m->sp;
      for(a=0;a<len;a++){
        pwart_EmitSaveStack(m,sv);
        sv--;
      }
      //move sp back
      m->sp-=len;
      sv=m->stack+m->sp+1; // first argument
    }
  }

  func_frame_offset=sv->frame_offset;
  
  sljit_emit_op2(m->jitc, SLJIT_ADD, SLJIT_R0, 0, SLJIT_S0, 0, SLJIT_IMM,func_frame_offset);

  sljit_emit_icall(m->jitc, SLJIT_CALL, SLJIT_ARGS2(VOID, W, W), memreg,
                   offset);
  len = strlen(type->results);
  for (a = 0; a < len; a++) {
    b = type->results[a];
    sv = stackvalue_Push(m, b);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(SLJIT_S0);
    if(a==0){
      sv->frame_offset=func_frame_offset;
    }
    sv->val.opw = sv->frame_offset;
  }
  if(m->cfg.memory_model==PWART_MEMORY_MODEL_GROW_ENABLED){
    //reload memory base
    if (m->mem_base_local >= 0) {
      uint32_t tr;
      StackValue *sv2;
      tr=pwart_GetFreeReg(m,RT_BASE,0);
      sv = dynarr_get(m->locals, StackValue, m->mem_base_local);
      sv2= dynarr_get(m->locals, StackValue, m->runtime_ptr_local);
      sljit_emit_op1(m->jitc, SLJIT_MOV, tr, 0, sv2->val.op, sv2->val.opw);
      sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw, SLJIT_MEM1(tr),
                    get_memorybytes_offset(m));
    }
  }
}

static int pwart_EmitFuncReturn(Module *m) {
  int idx, len, off;
  StackValue *sv;
  off = 0;
  len = strlen(m->function_type->results);
  for (idx = 0; idx < len; idx++) {
    sv = &m->stack[m->sp - len + idx + 1];
    if(m->cfg.stack_flags & PWART_STACK_FLAGS_AUTO_ALIGN){
      pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0),
        stackvalue_GetAlignedOffset(sv,off,&off));
    }else{
      pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0), off);
      off += stackvalue_GetSizeAndAlign(sv, NULL);
    }
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
    if(sljit_get_register_index(SLJIT_R3)<0){
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
    }else{
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
    }
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
    if (sv->jit_type==SVT_GENERAL && (sv->val.op & SLJIT_MEM) == 0 && sv->val.op != SLJIT_IMM) {
      return;
    }
    if (stackvalue_IsFloat(sv)) {
      r1 = pwart_GetFreeReg(m, RT_FLOAT, 0);
    } else {
      //XXX: if sv is SLJIT_MEM1 type, we can reuse this register.
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
    if(m->cfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
      sv->val.opw = stackvalue_GetAlignedOffset(sv,nextLoc,&nextLoc);
    }else{
      sv->val.opw=nextLoc;
      nextLoc+=stackvalue_GetSizeAndAlign(sv,NULL);
    }
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
      //XXX: we force local variable aligned, no mater PWART_STACK_FLAGS_AUTO_ALIGN flags is set.
      sv->val.opw = stackvalue_GetAlignedOffset(sv,nextLoc,&nextLoc);
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
      sv->val.opw = sv->val.opw = stackvalue_GetAlignedOffset(sv,nextLoc,&nextLoc);
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
    m->runtime_ptr_local = t;
  }
}


#endif