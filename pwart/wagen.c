#ifndef _PWART_WAGEN_C
#define _PWART_WAGEN_C

#include "def.h"

#include "extfunc.c"
#include "opgen_utils.c"
#include "opgen_ctl.c"
#include "opgen_mem.c"
#include "opgen_num.c"
#include "opgen_misc.c"


static void skip_immediates(uint8_t *bytes, uint32_t *pos) {
  uint32_t count, opcode = bytes[*pos];
  *pos = *pos + 1;
  switch (opcode) {
  // varuint1
  case 0x3f ... 0x40: // current_memory, memory.grow
    read_LEB(bytes, pos, 1);
    break;
  // varuint32, varint32
  case 0x0c ... 0x0d: // br, br_if
  case 0x10:          // call
    read_LEB(bytes, pos, 32);
    break;
  case 0x20: //local.get
    read_LEB(bytes, pos, 32);
    break;
  case 0x21: //local.set
    read_LEB(bytes, pos, 32);
    break;
  case 0x22: //local.tee
    read_LEB(bytes, pos, 32);
    break;
  case 0x23: //global.get
  case 0x24: //global.set
    read_LEB(bytes, pos, 32);
    break;
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
  {
    uint32_t align=read_LEB(bytes, pos, 32);
    if(align&0x40){read_LEB(bytes,pos,32);}//memory index
    read_LEB(bytes, pos, 32);
    break;
  }
  // br_table
  case 0x0e:                          // br_table
    count = read_LEB(bytes, pos, 32); // target count
    for (uint32_t i = 0; i < count; i++) {
      read_LEB(bytes, pos, 32);
    }
    read_LEB(bytes, pos, 32); // default target
    break;
  case 0xfc: //misc op.
    switch(bytes[*pos]){
      case 0xa:
      case 0xe:
      read_LEB(bytes,pos,32); 
      read_LEB(bytes,pos,32); 
      break;
      case 0x0b:
      case 0x0f ... 0x11:
      read_LEB(bytes,pos,32); 
      break;
    }
    *pos++;
    break;
  default: // no immediates
    break;
  }
}



static char *pwart_PrepareFunc(ModuleCompiler *m) {
  uint8_t *bytes = m->bytes;
  StackValue *stack = m->stack;
  StackValue *sv=NULL;

  uint32_t cur_pc;

  uint32_t arg, val, fidx, tidx, depth, count,tabidx,midx;
  uint32_t flags, offset, addr;
  uint8_t opcode, eof=0;
  uint32_t len = 0;
  Block *blk = NULL;
  m->locals = NULL;
  dynarr_init(&m->locals, sizeof(StackValue));
  m->blocks=NULL;
  dynarr_init(&m->blocks,sizeof(Block));
  m->cached_midx=-1;
  int i;

  int paramCnt=0;
  for (i = 0; m->function_type->params[i] != 0; i++) {
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = m->function_type->params[i];
  }
  paramCnt=i;

  for (i = 0; m->function_locals_type[i] != 0; i++) {
    sv = dynarr_push_type(&m->locals, StackValue);
    sv->wasm_type = m->function_locals_type[i];
    //XXX: use sv->jit_type as locals flags. 0 - Unknown 1-Need Zero Init 2-No Need Zero Init
    sv->jit_type=0;
  }
  // initialize to -1
  // if local variable required, set to -2 and set to real value(>=0) after code
  // scan.
  m->mem_base_local = -1;
  m->table_entries_local = -1;
  while (!eof && m->pc < m->byte_count) {
    opcode = bytes[m->pc];
    cur_pc = m->pc;
    m->pc += 1;

    switch (opcode) {
    //
    // Control flow operators
    //
    case 0x02: // block
      blk=dynarr_push_type(&m->blocks,Block);
      blk->block_type=0x02;
      break;
    case 0x03: // loop
      blk=dynarr_push_type(&m->blocks,Block);
      blk->block_type=0x03;
      break;
    case 0x04: // if
      blk=dynarr_push_type(&m->blocks,Block);
      blk->block_type=0x04;
      break;
    case WASMOPC_br:
    case WASMOPC_br_if: 
      read_LEB(bytes, &m->pc, 32);
      break;
    case 0x0b: // end
      if (m->blocks->len <= 0) {
        eof = 1;
      }else{
        blk=dynarr_pop_type(&m->blocks,Block);
      }
      break;
    case 0x10: // call
      fidx = read_LEB(bytes, &m->pc, 32);
      break;
    case 0x11: // call_indirect
      tidx = read_LEB(bytes, &m->pc, 32); //type
      tabidx=read_LEB(bytes, &m->pc, 1); //table index
      if(tabidx==0)m->table_entries_local = -2;
      break;
    case 0x23: // global.get
      arg = read_LEB(bytes, &m->pc, 32);
      break;
    case 0x24: // global.set
      arg = read_LEB(bytes, &m->pc, 32);
      break;
    case 0x25:                            // table.get
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      if(tidx==0)m->table_entries_local = -2;
      break;
    case 0x26:                            // table.set
      tidx = read_LEB(bytes, &m->pc, 32); // table index
      if(tidx==0)m->table_entries_local = -2;
      break;
    // Memory load operators
    case 0x28 ... 0x35:
    // Memory store operators
    case 0x36 ... 0x3e:
      midx=0;
      arg = read_LEB(m->bytes, &m->pc, 32);
      if(arg&0x40)midx=read_LEB(m->bytes, &m->pc, 32);
      offset = read_LEB(m->bytes, &m->pc, 32);
      if(m->cached_midx<0 && (*dynarr_get(m->context->memories,Memory *,midx))->bytes!=NULL){
        m->mem_base_local = -2;
        m->cached_midx=midx;
      }
      break;
    case 0xd2: // ref.func
      fidx = read_LEB(bytes, &m->pc, 32);
      break;
    case 0x20: //local.get
      arg=read_LEB(bytes, &m->pc, 32);
      sv=dynarr_get(m->locals,StackValue,arg);
      if((pwart_gcfg.misc_flags&PWART_MISC_FLAGS_LOCALS_ZERO_INIT)){
        if(sv->jit_type==0)sv->jit_type=1;
      }
      break;
    case 0x21: //local.set
      arg=read_LEB(bytes, &m->pc, 32);
      sv=dynarr_get(m->locals,StackValue,arg);
      if((pwart_gcfg.misc_flags&PWART_MISC_FLAGS_LOCALS_ZERO_INIT)&&(m->blocks->len==0)){
        if(sv->jit_type==0)sv->jit_type=2;
      }
      break;
    case 0x22: //local.tee
      arg=read_LEB(bytes, &m->pc, 32);
      sv=dynarr_get(m->locals,StackValue,arg);
      if((pwart_gcfg.misc_flags&PWART_MISC_FLAGS_LOCALS_ZERO_INIT)){
        if(sv->jit_type==0)sv->jit_type=1;
      }
      break;
    default:
      m->pc--;
      skip_immediates(m->bytes, &m->pc);
      break;
    }
  }
  SLJIT_ASSERT(m->bytes[m->pc - 1] == 0xb);
  m->locals_need_zero=NULL;
  if(pwart_gcfg.misc_flags&PWART_MISC_FLAGS_LOCALS_ZERO_INIT){
    dynarr_init(&m->locals_need_zero,sizeof(int16_t));
    for(int i1=paramCnt;i1<m->locals->len;i1++){
      sv=dynarr_get(m->locals,StackValue,i1);
      if(sv->jit_type==1){
        *dynarr_push_type(&m->locals_need_zero,int16_t)=(int16_t)i1;
      }
    }
  }
  
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
  dynarr_free(&m->blocks);
  return NULL;
}

/*
r0,r1,r2 is scratch registers(at least three registers are required.).
s0(arg0) contains stack frame pointer.
*/
static char *pwart_GenCode(ModuleCompiler *m) {
  Block *block;
  int opcode,cur_pc;

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
  m->eof=0;

  while (!m->eof && m->pc < m->byte_count) {
    opcode = m->bytes[m->pc];
    cur_pc = m->pc;
    m->pc += 1;

#if DEBUG_BUILD
    if(opcode<=0xd2){
      wa_debug("op %x:%s\n", m->pc, OPERATOR_INFO[opcode]);
    }
#endif

    // XXX: save flag if next op is not if, br_if or i32.eqz(not).
    if (m->sp>=0 && m->stack[m->sp].jit_type == SVT_CMP && 
    opcode != WASMOPC_if && opcode != WASMOPC_br_if && opcode != WASMOPC_i32_eqz) {
      pwart_EmitStackValueLoadReg(m, &m->stack[m->sp]);
    }

    if (opcode >= 0 && opcode <= 0x1b) {
      ReturnIfErr(opgen_GenCtlOp(m, opcode));
    } else if (opcode <= 0x44) {
      ReturnIfErr(opgen_GenMemOp(m, opcode));
    } else if (opcode <= 0xc4) {
      ReturnIfErr(opgen_GenNumOp(m, opcode));
    } else {
      ReturnIfErr(opgen_GenMiscOp(m, opcode));
    }
  }

  dynarr_free(&m->br_table);
  dynarr_free(&m->blocks);
  return NULL;
}

static char *pwart_EmitFunction(ModuleCompiler *m, WasmFunction *func) {
  WasmFunctionEntry code;
  uint32_t savepos = m->pc;

  // Empty stacks
  m->sp = -1;

  m->function_type = dynarr_get(m->types, Type, func->tidx);
  m->function_locals_type = func->locals_type;

  m->locals = NULL;
  dynarr_init(&m->locals, sizeof(StackValue));

  ReturnIfErr(pwart_PrepareFunc(m));
  m->pc = savepos;
  ReturnIfErr(pwart_GenCode(m));
  code = (WasmFunctionEntry)sljit_generate_code(m->jitc);
  sljit_free_compiler(m->jitc);
  m->jitc=NULL;
  dynarr_free(&m->locals);
  func->func_ptr = code;
  return NULL;
}

static void pwart_FreeFunction(WasmFunctionEntry code) {
  sljit_free_code(code, NULL);
}

#endif