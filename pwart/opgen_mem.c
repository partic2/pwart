#ifndef _PWART_OPGEN_MEM_C
#define _PWART_OPGEN_MEM_C

// Variable access,Memory-related operators,Constants

#include "def.h"
#include "extfunc.c"
#include "opgen_utils.c"



static void opgen_GenLocalGet(ModuleCompiler *m, uint32_t idx) {
  stackvalue_PushStackValueLike(m, dynarr_get(m->locals, StackValue, idx));
}

static void opgen_GenLocalSet(ModuleCompiler *m, uint32_t idx) {
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

static void opgen_GenLocalTee(ModuleCompiler *m, uint32_t idx) {
  StackValue *sv, *sv2;
  StackValue *stack = m->stack;
  sv = dynarr_get(m->locals, StackValue, idx);
  sv2 = &stack[m->sp];
  pwart_EmitStoreStackValue(m, sv2, sv->val.op, sv->val.opw);
}

static void opgen_GenGlobalGet(ModuleCompiler *m, uint32_t idx) {
  int32_t a;
  StackValue *sv;
  a = pwart_GetFreeReg(m, RT_BASE, 0);
  sv = dynarr_get(m->globals, StackValue, idx);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_IMM, sv->val.opw);
  sv = stackvalue_PushStackValueLike(m, dynarr_get(m->globals, StackValue, idx));
  sv->val.op = SLJIT_MEM1(a);
  sv->val.opw=0;
  sv->jit_type = SVT_GENERAL;
  pwart_EmitStackValueLoadReg(m, sv);
}

static void opgen_GenGlobalSet(ModuleCompiler *m, uint32_t idx) {
  int32_t a;
  StackValue *sv, *sv2;
  a = pwart_GetFreeReg(m, RT_BASE, 0);
  sv = dynarr_get(m->globals, StackValue, idx);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_IMM, sv->val.opw);
  sv2 = &m->stack[m->sp];
  pwart_EmitStoreStackValue(m, sv2, SLJIT_MEM1(a), 0);
  m->sp--;
}

//pop stack top as table entry index, and convert to base address stored in register, return the register.
static int opgen_GenBaseAddressRegForTable(ModuleCompiler *m,uint32_t tabidx){
  int a,b;
  StackValue *sv, *sv2;
  sv = &m->stack[m->sp];
  a = pwart_GetFreeReg(m, RT_INTEGER, 1);
  sljit_emit_op2(m->jitc, SLJIT_SHL32, a, 0, sv->val.op, sv->val.opw, SLJIT_IMM,
                sizeof(void *) == 4 ? 2 : 3);
  if(tabidx==0){
    sv2 = dynarr_get(m->locals, StackValue, m->table_entries_local);
    sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, sv2->val.op, sv2->val.opw);
  }else{
    b=pwart_GetFreeRegExcept(m,RT_BASE,a,1);
    Table *tab=*dynarr_get(m->context->tables,Table *,tabidx);
    sljit_emit_op1(m->jitc,SLJIT_MOV,b,0,SLJIT_IMM,(sljit_uw)&tab->entries);
    sljit_emit_op1(m->jitc,SLJIT_MOV,b,0,SLJIT_MEM1(b),0);
    sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, a, 0, b, 0);
  }
  m->sp--;
  return a;
}

static void opgen_GenTableGet(ModuleCompiler *m, uint32_t idx) {
  int a;
  StackValue *sv;
  a=opgen_GenBaseAddressRegForTable(m,idx);
  sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_MEM1(a), 0);
  sv = stackvalue_Push(m,WVT_FUNC);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = a;
  sv->val.opw = 0;
}

static void opgen_GenTableSet(ModuleCompiler *m, uint32_t idx) {
  int32_t a;
  StackValue *sv, *sv2;
  stackvalue_EmitSwapTopTwoValue(m);
  a=opgen_GenBaseAddressRegForTable(m,idx);
  sv = &m->stack[m->sp];
  pwart_EmitStoreStackValue(m, sv, SLJIT_MEM1(a), 0);
  m->sp--;
}

static void opgen_GenCurrentMemory(ModuleCompiler *m,uint32_t index) {
  int32_t a;
  StackValue *sv;
  a = pwart_GetFreeReg(m, RT_INTEGER, 0);
  sljit_emit_op1(m->jitc,SLJIT_MOV,a,0,SLJIT_IMM,(sljit_uw)(&(*dynarr_get(m->context->memories,Memory *,index))->pages));
  sv = stackvalue_Push(m, WVT_I32);
  sv->jit_type = SVT_GENERAL;
  sv->val.op = SLJIT_MEM1(a);
  sv->val.opw = 0;
}
static void opgen_GenMemoryGrow(ModuleCompiler *m,uint32_t index) {
  opgen_GenI32Const(m,index);
  opgen_GenRefConst(m,m->context);
  pwart_EmitCallFunc(m, &func_type_i32_i32_ref_ret_i32, SLJIT_IMM,
                     (sljit_sw)&insn_memorygrow);
}
// a:memory access base register, can be result register. result StackValue will
// be pushed to stack.
static void opgen_GenI32Load(ModuleCompiler *m, int32_t opcode, int32_t a,
                             sljit_sw offset) {
  StackValue *sv = stackvalue_Push(m, WVT_I32);
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
  sv->val.op = a;
  sv->val.opw = 0;
}
static void opgen_GenI64Load(ModuleCompiler *m, int32_t opcode, int32_t a,
                             sljit_sw offset) {
  int32_t b;
  b = pwart_GetFreeRegExcept(m, RT_INTEGER, a, 0);
  StackValue *sv = stackvalue_Push(m, WVT_I64);
  if (m->target_ptr_size == 32) {
    // i64.load64 on 32bit arch
    if (opcode == 0x29) {
      sv->jit_type = SVT_GENERAL;
      sv->val.op = SLJIT_MEM1(a);
      sv->val.opw = offset;
      pwart_EmitStackValueLoadReg(m, sv);
      return;
    }else{
      sv->jit_type = SVT_TWO_REG;
      sv->val.tworeg.opr1 = a;
      sv->val.tworeg.opr2 = b;
      sljit_emit_op1(m->jitc,SLJIT_MOV,b,0,SLJIT_IMM,0);
    }
  } else {
    sv->jit_type = SVT_GENERAL;
    sv->val.op = a;
    sv->val.opw = 0;
  }
  switch (opcode) {
  case 0x29: // i64.load64
    sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_MEM1(a), offset);
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
  
}

static void opgen_GenFloatLoad(ModuleCompiler *m, int32_t opcode, int32_t a,
                               uint32_t offset) {
  StackValue *sv;
  switch (opcode) {
  case 0x2a: // f32.load32
    sv=stackvalue_Push(m, WVT_F32);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(a);
    sv->val.opw = offset;
    pwart_EmitStackValueLoadReg(m, sv);
    break;
  case 0x2b: // f64.load64
    sv=stackvalue_Push(m, WVT_F64);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = SLJIT_MEM1(a);
    sv->val.opw = offset;
    pwart_EmitStackValueLoadReg(m, sv);
    break;
  }
}
//pop stack top as memory index, and convert to base address stored in register, return the register.
static int opgen_GenBaseAddressReg(ModuleCompiler *m,uint32_t midx){
  StackValue *sv,*sv2;
  int a;
  sljit_s32 op;
  sljit_sw opw;
  sv2 = &m->stack[m->sp];                                    // addr
  int8_t spAdd=-1;

  if((*dynarr_get(m->context->memories,Memory *,midx))->bytes==NULL){
    //raw memory
    pwart_EmitStackValueLoadReg(m,sv2);
    m->sp--;
    return sv2->val.op;
  }
  if(midx==m->cached_midx){
    sv = dynarr_get(m->locals, StackValue, m->mem_base_local); // memory base
    //here we don't need reserve top stack value
    a = pwart_GetFreeReg(m, RT_BASE, 1);
  }else{
    Memory *mem=*dynarr_get(m->context->memories,Memory *,midx);
    a = pwart_GetFreeReg(m, RT_BASE, 0);
    sljit_emit_op1(m->jitc,SLJIT_MOV,a,0,SLJIT_IMM,(sljit_uw)&mem->bytes);
    sljit_emit_op1(m->jitc,SLJIT_MOV,a,0,SLJIT_MEM1(a),0);
    sv=stackvalue_Push(m,WVT_REF);
    spAdd--;
    sv->jit_type=SVT_GENERAL;
    sv->val.op=a;
    sv->val.opw=0;
  }
  if (sv2->wasm_type == WVT_I32) {
    sljit_emit_op2(m->jitc, SLJIT_ADD32, a, 0, sv->val.op, sv->val.opw,
                  sv2->val.op, sv2->val.opw);
  } else {
    if (m->target_ptr_size == 64) {
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                    sv2->val.op, sv2->val.opw);
    } else {
      // For 32bit arch, memory64 only use the least significant word. 
      stackvalue_LowWord(m,sv2,&op,&opw);
      sljit_emit_op2(m->jitc, SLJIT_ADD, a, 0, sv->val.op, sv->val.opw,
                    op, opw);
    }
  }
  m->sp+=spAdd;
  return a;
}


static void opgen_GenLoad(ModuleCompiler *m, int32_t opcode, sljit_sw offset,
                          sljit_sw align,sljit_uw memindex) {
  int32_t a;
  a=opgen_GenBaseAddressReg(m,memindex);
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

static void opgen_GenStore(ModuleCompiler *m, int32_t opcode, sljit_sw offset,
                           sljit_sw align,sljit_uw memindex) {
  int32_t a;
  StackValue *sv, *sv2;
  stackvalue_EmitSwapTopTwoValue(m);
  a=opgen_GenBaseAddressReg(m,memindex);
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

static void opgen_GenMemoryCopy(ModuleCompiler *m,int dmidx,int smidx){
  StackValue *sv,*sv2;
  int r;
  sv2=m->stack+m->sp-2;//dst index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressReg(m,dmidx);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  sv2=m->stack+m->sp-2;//src index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressReg(m,smidx);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  sv2 = sv=m->stack+m->sp-2;    //n;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  if(sv->wasm_type==WVT_I64){
    //XXX: we limit to use 32bit count now, which may be change in future
    opgen_GenNumOp(m,0xa7);//i32.wrap_i64
  }
  pwart_EmitCallFunc(m,&func_type_ref_ref_i32_void,SLJIT_IMM,(sljit_sw)&insn_ref_copy_bytes);
  // recover stack
  m->sp-=3;
}

static void opgen_GenTableCopy(ModuleCompiler *m,int32_t dtab,int32_t stab){
  StackValue *sv,*sv2;
  int r;
  sv2=m->stack+m->sp-2;//dst index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressRegForTable(m,dtab);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  sv2=m->stack+m->sp-2;//src index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressRegForTable(m,stab);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  sv2 = sv=m->stack+m->sp-2;    //n;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  opgen_GenI32Const(m,(m->target_ptr_size==32)?2:3);
  opgen_GenNumOp(m,0x74);
  pwart_EmitCallFunc(m,&func_type_ref_ref_i32_void,SLJIT_IMM,(sljit_sw)&insn_ref_copy_bytes);
  // recover stack
  m->sp-=3;
}


static void opgen_GenMemoryFill(ModuleCompiler *m,int midx){
  StackValue *sv,*sv2;
  int r;
  sv2=m->stack+m->sp-2;//dst index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressReg(m,midx);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  sv2 = sv=m->stack+m->sp-2;    //val;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;

  sv2 = sv=m->stack+m->sp-2;    //n;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;

  opgen_GenRefNull(m,0);
  opgen_GenI32Const(m,0);

  pwart_EmitCallFunc(m,&func_type_memoryfill,SLJIT_IMM,(sljit_sw)&insn_memoryfill);
  m->sp-=3;
}

static void opgen_GenTableFill(ModuleCompiler *m,int32_t tabidx){
  StackValue *sv,*sv2;
  int r;
  sv2=m->stack+m->sp-2;//dst index;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;
  r=opgen_GenBaseAddressRegForTable(m,tabidx);
  sv2 = stackvalue_Push(m,WVT_REF);
  sv2->wasm_type=WVT_REF;sv2->jit_type=SVT_GENERAL;
  sv2->val.op=r;sv2->val.opw=0;

  opgen_GenI32Const(m,0);

  sv2 = sv=m->stack+m->sp-2;    //n;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;

  sv2 = sv=m->stack+m->sp-4;    //val;
  sv = stackvalue_PushStackValueLike(m, sv2);
  sv2->jit_type=SVT_DUMMY;

  opgen_GenI32Const(m,1);

  pwart_EmitCallFunc(m,&func_type_memoryfill,SLJIT_IMM,(sljit_sw)&insn_memoryfill);
  m->sp-=3;
}

static char *opgen_GenMemOp(ModuleCompiler *m, int opcode) {
  int32_t arg, tidx,midx;
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
    midx=read_LEB(m->bytes, &m->pc, 32); // memory index
    opgen_GenCurrentMemory(m,midx);
    break;
  case 0x40:                        // memory.grow
    midx=read_LEB(m->bytes, &m->pc, 32); // memory index
    opgen_GenMemoryGrow(m,midx);
    break;
  // Memory load operators
  case 0x28 ... 0x35:
    midx=0;
    align = read_LEB(m->bytes, &m->pc, 32);
    if(align&0x40)midx=read_LEB(m->bytes, &m->pc, 32);
    offset = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenLoad(m, opcode, offset, align,midx);
    break;
  case 0x36 ... 0x3e:
    midx=0;
    align = read_LEB(m->bytes, &m->pc, 32);
    if(align&0x40)midx=read_LEB(m->bytes, &m->pc, 32);
    offset = read_LEB(m->bytes, &m->pc, 32);
    opgen_GenStore(m, opcode, offset, align,midx);
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
    opgen_GenF32Const(m, m->bytes + m->pc);
    m->pc += 4;
    break;
  case 0x44: // f64.const
    opgen_GenF64Const(m, m->bytes + m->pc);
    m->pc += 8;
    break;
  default:
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    return "unrecognized opcode";
  }
  return NULL;
}

#endif