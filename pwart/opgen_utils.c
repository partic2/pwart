#ifndef _PWART_OPGEN_UTILS_C
#define _PWART_OPGEN_UTILS_C

#include "def.h"
#include <stddef.h>

#define SLJIT_GET_REG_PART(r) (r&0x7f)
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
static uint32_t stackvalue_GetAlignedOffset(StackValue *sv,int offset,int *nextOffset){
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
static void stackvalue_HighWord(ModuleCompiler *m, StackValue *sv, sljit_s32 *op,
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
static void stackvalue_LowWord(ModuleCompiler *m, StackValue *sv, sljit_s32 *op,
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
static StackValue *stackvalue_FindSvalueUseReg(ModuleCompiler *m, sljit_s32 r,
                                               sljit_s32 regtype, int upstack) {
  StackValue *sv;
  int i, used = 0;
  for (i = 0; i <= m->sp - upstack; i++) {
    sv = &m->stack[i];
    if ((regtype == RT_FLOAT) != stackvalue_IsFloat(sv)) {
      continue;
    }
    if (sv->jit_type == SVT_GENERAL) {
      wa_assert((sv->val.op & (~(int)0xff))==0,"two reg operand not supported");
      if (SLJIT_GET_REG_PART(sv->val.op) == r || SLJIT_GET_REG_PART(sv->val.op >> 8) == r) {
        used = 1;
        break;
      }
    } else if (sv->jit_type == SVT_TWO_REG) {
      if (sv->val.tworeg.opr1 == r || sv->val.tworeg.opr2 == r) {
        used = 1;
        break;
      }
    }else if(sv->jit_type == SVT_I64CONST || sv->jit_type==SVT_CMP ||sv->jit_type==SVT_DUMMY){
      //pass
    }else{
      SLJIT_UNREACHABLE();
    }
  }
  if (used) {
    return sv;
  } else {
    return NULL;
  }
}

static char *pwart_EmitStoreStackValue(ModuleCompiler *m, StackValue *sv, int memreg,
                                     int offset) {
  if (sv->jit_type == SVT_GENERAL) {
    if(memreg==sv->val.op && offset==sv->val.opw)return 0;
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
    wa_assert(sv->wasm_type==WVT_I32,"SVT_CMP must be i32");
    sljit_emit_op_flags(m->jitc, SLJIT_MOV32, memreg, offset, sv->val.cmp.flag);
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
  } else if(sv->jit_type==SVT_DUMMY){
    //do nothing.
  }else{
    SLJIT_UNREACHABLE();
  }
  return NULL;
}
// store value into [S0+IMM]
static char* pwart_EmitSaveStack(ModuleCompiler *m, StackValue *sv) {
  if (sv->val.op == SLJIT_MEM1(SLJIT_S0) && sv->val.opw == sv->frame_offset) {
    return NULL;
  }
  pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0), sv->frame_offset);
  sv->val.op = SLJIT_MEM1(SLJIT_S0);
  sv->val.opw = sv->frame_offset;
  sv->jit_type = SVT_GENERAL;
  return NULL;
}

static char* pwart_EmitSaveStackAll(ModuleCompiler *m) {
  int i;
  for (i = 0; i <= m->sp; i++) {
    pwart_EmitSaveStack(m, &m->stack[i]);
  }
  return NULL;
}

static char* pwart_EmitSaveScratchRegisterAll(ModuleCompiler *m,int upstack) {
  int i,usereg=0;
  for (i = 0; i <= m->sp-upstack; i++) {
    if(m->stack[i].jit_type==SVT_GENERAL){
      usereg=SLJIT_GET_REG_PART(m->stack[i].val.op);
      if(usereg!=0 && usereg< SLJIT_FIRST_SAVED_REG){
        pwart_EmitSaveStack(m,m->stack+i);
      }
    }else if(m->stack[i].jit_type==SVT_TWO_REG){
      pwart_EmitSaveStack(m, &m->stack[i]);
    }
  }
  return NULL;
}

static char* pwart_EmitLoadReg(ModuleCompiler *m, StackValue *sv, int reg) {

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
    sljit_emit_op_flags(m->jitc, SLJIT_MOV32, reg, 0, sv->val.cmp.flag);
  } else if (sv->jit_type == SVT_POINTER) {
    SLJIT_UNREACHABLE();
  } else if (sv->jit_type == SVT_TWO_REG) {
    // 32bit arch only
    SLJIT_UNREACHABLE();
  } else if (sv->jit_type == SVT_I64CONST) {
    // 32bit arch only
    SLJIT_UNREACHABLE();
  }
  return NULL;
}

static inline size_t get_funcarr_offset(ModuleCompiler *m) {
  return offsetof(RuntimeContext, funcentries);
}

// Don't emit any code here.
// Don't write other stackvalue (due to stackvalue_EmitSwapTopTwoValue
// function).
static StackValue *stackvalue_Push(ModuleCompiler *m, int32_t wasm_type) {
  StackValue *sv2,*sv;
  sv2 = &m->stack[++m->sp];
  sv2->jit_type = SVT_GENERAL;
  sv2->wasm_type=wasm_type;
  if (sv2 - m->stack == 0) {
    // first stack value
    sv2->frame_offset = m->first_stackvalue_offset;
  } else {
    sv = sv2 - 1;
    sv2->frame_offset = sv->frame_offset + stackvalue_GetSizeAndAlign(sv, NULL);
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
      sv2->frame_offset=stackvalue_GetAlignedOffset(sv2,sv2->frame_offset,NULL);
    }
  }
  wa_assert(m->sp<OPERAND_STACK_COUNT-1,"operand stack overflow");
  return sv2;
}

static StackValue *stackvalue_PushStackValueLike(ModuleCompiler *m,StackValue *refsv){
  StackValue *sv;
  sv=stackvalue_Push(m,refsv->wasm_type);
  sv->jit_type=refsv->jit_type;
  sv->val=refsv->val;
  return sv;
}

static void stackvalue_EmitSwapTopTwoValue(ModuleCompiler *m) {
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


static char *pwart_EmitCallFunc2(ModuleCompiler *m, Type *type, sljit_s32 memreg,
                              sljit_sw offset,int callReturn) {
  StackValue *sv;
  int a, b, len,func_frame_offset;
  int i;
  //Caution: Do not use scratch register before sljit_emit_icall to avoid overwrite [memreg,offset]
  //XXX: Maybe we need use RS_RESERVED to avoid unexpected overwrite.
  len = strlen((const char *)type->params);
  pwart_EmitSaveScratchRegisterAll(m,0);
  if ((memreg & (SLJIT_MEM - 1)) == SLJIT_R0 ) {
    sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R2, 0, memreg, offset);
    memreg = SLJIT_R2;
    offset = 0;
  }
  if(len == 0){
    //no argument, push dummy stackvalue
    stackvalue_Push(m,WVT_I32);
    m->sp--;
  }else{
    m->sp -= len;
  }
  sv = m->stack+m->sp + 1; // first argument

  if((pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN) && ((sv->frame_offset&7)>0)){
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
  }else if(callReturn){
    //tail call
    sv->frame_offset=0;
    if(len>0){
      pwart_EmitSaveStack(m,sv);
      b=sv->frame_offset+stackvalue_GetSizeAndAlign(sv,NULL);
      m->sp+=len;
      for(a=1;a<len;a++){
        sv++;
        sv->frame_offset = stackvalue_GetAlignedOffset(sv,b,&b);
        pwart_EmitSaveStack(m,sv);
      }
      //move sp back
      m->sp-=len;
      sv=m->stack+m->sp+1; // first argument
    }
  }else{
    for(i=len;i>0;i--){
      m->sp++;
      pwart_EmitSaveStack(m,&m->stack[m->sp]);
    }
    //move sp back
    m->sp-=len;
    sv=m->stack+m->sp+1; // first argument
  }

  func_frame_offset=sv->frame_offset;
  
  sljit_emit_op2(m->jitc, SLJIT_ADD, SLJIT_R0, 0, SLJIT_S0, 0, SLJIT_IMM,func_frame_offset);

  a=SLJIT_CALL;
  if(callReturn)a|=SLJIT_CALL_RETURN;
  sljit_emit_icall(m->jitc, a, SLJIT_ARGS1V(W), memreg,
                   offset);

  if(callReturn){
    m->block_returned=1;
  }
  len = strlen((const char *)type->results);
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
  
  if((m->mem_base_local >= 0)){
    Memory *mem0=*dynarr_get(m->context->memories,Memory *,m->cached_midx);
    if(!mem0->fixed){
      //reload memory base
      uint32_t tr;
      StackValue *sv;
      tr=pwart_GetFreeReg(m,RT_BASE,0);
      sljit_emit_op1(m->jitc, SLJIT_MOV, tr, 0, SLJIT_IMM,(sljit_uw)&mem0->bytes);
      sv=dynarr_get(m->locals,StackValue,m->mem_base_local);
      sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw, SLJIT_MEM1(tr),0);
    }
  }
  return NULL;
}


static char *pwart_EmitCallFunc(ModuleCompiler *m, Type *type, sljit_s32 memreg,
                              sljit_sw offset){
  return pwart_EmitCallFunc2(m,type,memreg,offset,0);
}

static char *pwart_EmitFuncReturn(ModuleCompiler *m) {
  int idx, len, off;
  StackValue *sv;
  off = 0;
  len = strlen((const char *)m->function_type->results);
  if(m->sp+1<len){
    /* maybe unreachable code, Do nothing */
    return NULL;
  }
  for (idx = 0; idx < len; idx++) {
    sv = &m->stack[m->sp - len + idx + 1];
    if(pwart_gcfg.stack_flags & PWART_STACK_FLAGS_AUTO_ALIGN){
      pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0),
        stackvalue_GetAlignedOffset(sv,off,&off));
    }else{
      pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(SLJIT_S0), off);
      off += stackvalue_GetSizeAndAlign(sv, NULL);
    }
  }
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintFuncReturn);
  #endif
  sljit_emit_return_void(m->jitc);
  return NULL;
}

static int stackvalue_AnyRegUsedBySvalue(StackValue *sv){
  int r=0;
  if(sv->jit_type==SVT_GENERAL){
    r=sv->val.op&0xf;
    SLJIT_ASSERT(r>0);
    return r;
  }else if(sv->jit_type==SVT_TWO_REG){
    SLJIT_ASSERT(sv->val.tworeg.opr1>0);
    return sv->val.tworeg.opr1;
  }else{
    SLJIT_UNREACHABLE();
  }
}

// get free register. check up to upstack.
static sljit_s32 pwart_GetFreeReg(ModuleCompiler *m, sljit_s32 regtype, int upstack) {
  sljit_s32 fr;
  StackValue *sv;

  if (regtype != RT_FLOAT) {
// XXX: on x86-32, R3 - R6 (same as S3 - S6) are emulated, and cannot be used
// for memory addressing
    if(sljit_get_register_index(SLJIT_GP_REGISTER,SLJIT_R3)<0){
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
      fr=stackvalue_AnyRegUsedBySvalue(sv);
      pwart_EmitSaveStack(m, sv);
      return fr;
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
      fr=stackvalue_AnyRegUsedBySvalue(sv);
      pwart_EmitSaveStack(m, sv);
      return fr;
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
    fr=stackvalue_AnyRegUsedBySvalue(sv);
    pwart_EmitSaveStack(m, sv);
    return fr;
  }
}

static sljit_s32 pwart_GetFreeRegExcept(ModuleCompiler *m, sljit_s32 regtype,
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
static void pwart_EmitStackValueLoadReg(ModuleCompiler *m, StackValue *sv) {
  sljit_s32 r1, r2;
  if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
    if (sv->jit_type == SVT_TWO_REG) {
      return;
    }
    r1 = pwart_GetFreeReg(m, RT_INTEGER, 0);
    r2 = pwart_GetFreeRegExcept(m, RT_INTEGER, r1, 0);
    if (sv->jit_type == SVT_GENERAL) {
      wa_assert(sv->val.op & SLJIT_MEM,"assert failed.");
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

static char* pwart_EmitFuncEnter(ModuleCompiler *m) {
  int nextSr = SLJIT_S1;
  int nextSfr = SLJIT_FS0;
  int i, len;
  int nextLoc; 
  StackValue *sv=NULL;

  // temporarily use nextLoc to represent offset of argument, base on SLJIT_S0
  nextLoc = 0;
  len = strlen((const char *)m->function_type->params);
  for (i = 0; i < len; i++) {
    sv = dynarr_get(m->locals, StackValue, i);
    sv->wasm_type = m->function_type->params[i];
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(SLJIT_S0);
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
      sv->val.opw = stackvalue_GetAlignedOffset(sv,nextLoc,&nextLoc);
    }else{
      sv->val.opw=nextLoc;
      nextLoc+=stackvalue_GetSizeAndAlign(sv,NULL);
    }
  }
  m->first_stackvalue_offset = nextLoc;

  //nextLoc: next local offset to store locals variable, base on SLJIT_SP;
  nextLoc=0;
  //ensure memory base stored into register.
  
  if (m->mem_base_local >= 0) {
    sv = dynarr_get(m->locals, StackValue, m->mem_base_local);
    sv->jit_type=SVT_GENERAL;
    sv->val.op=nextSr;
    sv->val.opw=0;
    nextSr--;
  }
  
  for (i = len; i < m->locals->len; i++) {
    if(i==m->mem_base_local)continue;
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
      sv->val.opw = stackvalue_GetAlignedOffset(sv,nextLoc,&nextLoc);
    }
  }
  sljit_emit_enter(m->jitc, 0, SLJIT_ARGS1V(W),
                   SLJIT_NUMBER_OF_SCRATCH_REGISTERS | SLJIT_ENTER_FLOAT(SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS),
                   (SLJIT_S0 - nextSr) | SLJIT_ENTER_FLOAT(SLJIT_FS0 - nextSfr),
                   nextLoc);
  
  if (m->mem_base_local >= 0) {
    Memory *mem=*dynarr_get(m->context->memories,Memory *,m->cached_midx);
    sv = dynarr_get(m->locals, StackValue, m->mem_base_local);
    if(mem->bytes==NULL){
      //raw memory, so we don't need save memory base.
    }else if(mem->fixed){
      sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw,
                   SLJIT_IMM, (sljit_uw)mem->bytes);
    }else{
      sljit_emit_op1(m->jitc, SLJIT_MOV, SLJIT_R0, 0,
                   SLJIT_IMM, (sljit_uw)&mem->bytes);
      sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw,
                   SLJIT_MEM1(SLJIT_R0), 0);
    }
  }
  if (m->table_entries_local >= 0) {
    Table *tab=*dynarr_get(m->context->tables,Table *,0);
    sv = dynarr_get(m->locals, StackValue, m->table_entries_local);
    sljit_emit_op1(m->jitc, SLJIT_MOV, sv->val.op, sv->val.opw,
                   SLJIT_IMM, (sljit_uw)tab->entries);
  }

  if(m->locals_need_zero!=NULL){
    float f32zero=0;
    double f64zero=0;
    for(i=0;i<m->locals_need_zero->len;i++){
      int idx=*dynarr_get(m->locals_need_zero,int16_t,i);
      sv=dynarr_get(m->locals,StackValue,idx);
      switch(sv->wasm_type){
        case WVT_F32:
        opgen_GenF32Const(m,(uint8_t *)&f32zero);
        break;
        case WVT_F64:
        opgen_GenF32Const(m,(uint8_t *)&f64zero);
        break;
        case WVT_I32:
        opgen_GenI32Const(m,0);
        break;
        case WVT_I64:
        opgen_GenI64Const(m,0);
        break;
        default :
        opgen_GenRefNull(m,WVT_REF);
        break;
      }
      opgen_GenLocalSet(m,idx);
    }
    dynarr_free(&m->locals_need_zero);
  }
  #if PWART_DEBUG_RUNTIME_PROBE
  sljit_emit_op1(m->jitc,SLJIT_MOV,SLJIT_R0,0,SLJIT_IMM,m->pc+1);
  sljit_emit_icall(m->jitc,SLJIT_CALL,SLJIT_ARGS1(VOID,W),SLJIT_IMM,(sljit_sw)&debug_PrintFuncEnter);
  #endif
  return NULL;
}


static void opgen_GenI32Const(ModuleCompiler *m, uint32_t c) {
  StackValue *sv = stackvalue_Push(m, WVT_I32);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = c;
}
static void opgen_GenI64Const(ModuleCompiler *m, uint64_t c) {
  StackValue *sv = stackvalue_Push(m, WVT_I64);
  if (m->target_ptr_size == 32) {
    sv->jit_type = SVT_I64CONST;
    sv->val.const64 = c;
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_IMM;
    sv->val.opw = c;
  }
}
static void opgen_GenF32Const(ModuleCompiler *m, uint8_t *c) {
  StackValue *sv = stackvalue_Push(m, WVT_F32);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  //use memmove to avoid align error.
  memmove(&sv->val.opw,c,4);
  sv->val.opw = *(sljit_s32 *)c;
  // sljit only allow load float from memory
  pwart_EmitSaveStack(m, sv);
}

static void opgen_GenF64Const(ModuleCompiler *m, uint8_t *c) {
  StackValue *sv = stackvalue_Push(m, WVT_F64);
  if (m->target_ptr_size == 32) {
    sv->jit_type = SVT_I64CONST;
    memmove(&sv->val.const64,c,8);
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_IMM;
    memmove(&sv->val.opw,c,8);
  }
  // sljit only allow load float from memory
  pwart_EmitSaveStack(m, sv);
}
static void opgen_GenRefConst(ModuleCompiler *m,void *c) {
  StackValue *sv=stackvalue_Push(m,WVT_REF);
  sv->jit_type=SVT_GENERAL;
  sv->val.op=SLJIT_IMM;
  sv->val.opw=(sljit_uw)c;
}



#endif