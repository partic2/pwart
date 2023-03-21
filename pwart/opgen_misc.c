#ifndef _PWART_OPGEN_MISC_C
#define _PWART_OPGEN_MISC_C

#include "def.h"
#include "extfunc.c"
#include "opgen_num.c"
#include "opgen_mem.c"
#include "opgen_utils.c"

static void opgen_GenRefNull(ModuleCompiler *m, int32_t tidx) {
  StackValue *sv = stackvalue_Push(m, tidx);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_IMM;
  sv->val.opw = 0;
}
static void opgen_GenIsNull(ModuleCompiler *m) {
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

static void opgen_GenRefFunc(ModuleCompiler *m, int32_t fidx) {
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

static void opgen_GenTableSize(ModuleCompiler *m,int tabidx){
  int32_t r;
  StackValue *sv;
  r = pwart_GetFreeReg(m, RT_INTEGER, 0);
  sv=dynarr_get(m->locals,StackValue,m->runtime_ptr_local);
  sljit_emit_op2(m->jitc,SLJIT_ADD,r,0,sv->val.op,sv->val.opw,SLJIT_IMM,offsetof(RuntimeContext,tables));
  sljit_emit_op2(m->jitc,SLJIT_ADD,r,0,SLJIT_MEM1(r),0,SLJIT_IMM,offsetof(struct dynarr,data)+sizeof(Table)*tabidx+offsetof(Table,size));
  sljit_emit_op1(m->jitc,SLJIT_MOV,r,0,SLJIT_MEM1(r),0);
  sv = stackvalue_Push(m, WVT_I32);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = r;
  sv->val.opw = 0;
}

static char *opgen_GenMiscOp_FC(ModuleCompiler *m,int opcode){
  StackValue *sv;
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
    read_LEB_signed(m->bytes,&m->pc,32); //destination memory index
    read_LEB_signed(m->bytes,&m->pc,32); //source memory index
    opgen_GenMemoryCopy(m);
    break;
    case 0x0b: //memory.fill
    read_LEB_signed(m->bytes,&m->pc,32); //destination memory index
    opgen_GenMemoryFill(m);
    break;
    case 0x0e: //table.copy
    {
      int32_t dtab=read_LEB_signed(m->bytes,&m->pc,32); //destination table index
      int32_t stab=read_LEB_signed(m->bytes,&m->pc,32); //source table index
      opgen_GenTableCopy(m,dtab,stab);
    }
    break;
    case 0x0f: //table.grow
    //table are not allowed to grow
    read_LEB_signed(m->bytes,&m->pc,32); //table index
    m->sp-=2;
    sv=stackvalue_Push(m,WVT_I32);
    sv->val.op=SLJIT_IMM;
    sv->val.opw=-1;
    break;
    case 0x10: //table.size
    {
      uint32_t tabidx=read_LEB_signed(m->bytes,&m->pc,32);
      opgen_GenTableSize(m,tabidx);
    }
    break;
    case 0x11:  //table.fill
    {
      uint32_t tabidx=read_LEB_signed(m->bytes,&m->pc,32);
      opgen_GenTableFill(m,tabidx);
    }
    break;
    default:
    wa_debug("unrecognized misc opcode 0xfc 0x%x at %d", opcode, m->pc);
    return "unrecognized opcode";
  }
  return NULL;
}

static char *opgen_GenMiscOp(ModuleCompiler *m, int opcode) {
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
    ReturnIfErr(opgen_GenMiscOp_FC(m,opcode2));
    m->pc++;
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    return "unrecognized opcode";
  }
  return NULL;
}

//if fn is stub/inline function, generate native code and return 1, else return 0.
static int pwart_CheckAndGenStubFunction(ModuleCompiler *m,WasmFunctionEntry fn){
  struct pwart_builtin_symbols *functbl=pwart_get_builtin_symbols();
  if(fn==functbl->get_self_runtime_context){
    opgen_GenRefConst(m,m->context);
    return 1;
  }else if(fn==functbl->ref_from_i64){
    if(m->target_ptr_size==32){
      opgen_GenConvertOp(m,0xa7);
    }
    m->stack[m->sp].wasm_type=WVT_REF;
    return 1;
  }else if(fn==functbl->i64_from_ref){
    if(m->target_ptr_size==32){
      m->stack[m->sp].wasm_type=WVT_I32;
      opgen_GenConvertOp(m,0xad);
    }
    m->stack[m->sp].wasm_type=WVT_I64;
    return 1;
  }else if(fn==functbl->ref_from_index){
    int r=opgen_GenBaseAddressReg(m,0);
    StackValue *sv=stackvalue_Push(m,WVT_REF);
    sv->val.op=r;
    sv->val.opw=0;
    sv->jit_type=SVT_GENERAL;
    return 1;
  }
  return 0;
}
#endif