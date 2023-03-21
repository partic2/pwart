#ifndef _PWART_OPGEN_NUM_C
#define _PWART_OPGEN_NUM_C

#include "def.h"
#include "extfunc.c"
#include "opgen_utils.c"

static void opgen_GenCompareOp(ModuleCompiler *m,int opcode){
  StackValue *sv,*sv2;
  int32_t a,b;
  struct sljit_jump *jump;
  switch(opcode){
  // Comparison operators
  // unary
  case 0x45: // i32.eqz
    sv=m->stack+m->sp;
    sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z, sv->val.op, sv->val.opw,
                    SLJIT_IMM, 0);
    m->sp--;
    sv=stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_CMP;
    sv->val.cmp.flag = SLJIT_EQUAL;
    break;
  case 0x50: // i64.eqz
    sv=m->stack+m->sp;
    if (m->target_ptr_size == 32) {
      pwart_EmitCallFunc(m, &func_type_i64_ret_i32, SLJIT_IMM,
                         (sljit_sw)&insn_i64eqz);
    } else {
      sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op, sv->val.opw,
                      SLJIT_IMM, 0);
      m->sp--;
      stackvalue_Push(m,WVT_I32);
      sv->jit_type = SVT_CMP;
      sv->val.cmp.flag = SLJIT_EQUAL;
    }
    break;

  // i32 binary
  case 0x46 ... 0x4f:
    sv2 = m->stack+m->sp;
    sv = m->stack+m->sp-1;

    switch (opcode) {
    case 0x46:
      sljit_emit_op2u(m->jitc,
                      SLJIT_SUB32 | SLJIT_SET_Z ,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_EQUAL;
      break; // i32.eq
    case 0x47:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_Z, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_NOT_EQUAL;
      break; // i32.ne
    case 0x48:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_LESS, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_SIG_LESS;
      break; // i32.lt_s
    case 0x49:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_LESS, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_LESS;
      break; // i32.lt_u
    case 0x4a:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_GREATER, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_SIG_GREATER;
      break; // i32.gt_s
    case 0x4b:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_GREATER, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_GREATER;
      break; // i32.gt_u
    case 0x4c:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_LESS_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_SIG_LESS_EQUAL;
      break; // i32.le_s
    case 0x4d:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_LESS_EQUAL, sv->val.op,
                      sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_LESS_EQUAL;
      break; // i32.le_u
    case 0x4e:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_SIG_GREATER_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_SIG_GREATER_EQUAL;
      break; // i32.ge_s
    case 0x4f:
      sljit_emit_op2u(m->jitc, SLJIT_SUB32 | SLJIT_SET_GREATER_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      a = SLJIT_GREATER_EQUAL;
      break; // i32.ge_u
    }
    m->sp-=2;
    stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_CMP;
    sv->val.cmp.flag=a;
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
      break;
    } else {
      sv2 = m->stack+m->sp;
      sv = m->stack+m->sp-1;

      switch (opcode) {
      case 0x51:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_EQUAL;
        break; // i64.eq
      case 0x52:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_Z, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_NOT_EQUAL;
        break; // i64.ne
      case 0x53:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_LESS, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_SIG_LESS;
        break; // i64.lt_s
      case 0x54:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_LESS, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_LESS;
        break; // i64.lt_u
      case 0x55:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_GREATER, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_SIG_GREATER;
        break; // i64.gt_s
      case 0x56:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_GREATER, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_GREATER;
        break; // i64.gt_u
      case 0x57:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_SIG_LESS_EQUAL;
        break; // i64.le_s
      case 0x58:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_LESS_EQUAL, sv->val.op,
                        sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_LESS_EQUAL;
        break; // i64.le_u
      case 0x59:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_SIG_GREATER_EQUAL;
        break; // i64.ge_s
      case 0x5a:
        sljit_emit_op2u(m->jitc, SLJIT_SUB | SLJIT_SET_GREATER_EQUAL,
                        sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
        a = SLJIT_GREATER_EQUAL;
        break; // i64.ge_u
      }
    }
    sv=stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_CMP;
    sv->val.cmp.flag=a;
    break;
  case 0x5b ... 0x60:
    sv2 = m->stack+m->sp;
    sv = m->stack+m->sp-1;
    b = pwart_GetFreeReg(m, RT_INTEGER, 2);
    jump = sljit_emit_fcmp(m->jitc, SLJIT_32 | SLJIT_UNORDERED, sv->val.op,
                           sv->val.opw, sv2->val.op, sv2->val.opw);
    switch (opcode) {
    case 0x5b: // f32.eq
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_EQUAL);
      break;
    case 0x5c: // f32.ne
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_NOT_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_NOT_EQUAL);
      break;
    case 0x5d: // f32.lt
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_LESS,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_LESS);
      break;
    case 0x5e: // f32.gt
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_GREATER,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_GREATER);
      break;
    case 0x5f: // f32.le
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_LESS_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_LESS_EQUAL);
      break;
    case 0x60: // f32.ge
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F32 | SLJIT_SET_F_GREATER_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_GREATER_EQUAL);
      break;
    }
    {
      struct sljit_jump *jump2 = sljit_emit_jump(m->jitc, SLJIT_JUMP);
      sljit_set_label(jump, sljit_emit_label(m->jitc));
      // f32.ne return true if NaN
      sljit_emit_op1(m->jitc, SLJIT_MOV, b, 0, SLJIT_IMM, opcode == 0x5c);
      sljit_set_label(jump2, sljit_emit_label(m->jitc));
    }
    m->sp -= 2;
    sv = stackvalue_Push(m, WVT_I32);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = b;
    sv->val.opw = 0;
    break;
  case 0x61 ... 0x66:
    sv2 = &m->stack[m->sp];
    sv = &m->stack[m->sp]-1;
    b = pwart_GetFreeReg(m, RT_INTEGER, 2);
    jump = sljit_emit_fcmp(m->jitc, SLJIT_UNORDERED, sv->val.op, sv->val.opw,
                           sv2->val.op, sv2->val.opw);
    switch (opcode) {
    case 0x61: // f64.eq
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_EQUAL);
      break;
    case 0x62: // f64.ne
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_NOT_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_NOT_EQUAL);
      break;
    case 0x63: // f64.lt
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_LESS,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_LESS);
      break;
    case 0x64: // f64.gt
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_GREATER,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_GREATER);
      break;
    case 0x65: // f64.le
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_LESS_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_LESS_EQUAL);
      break;
    case 0x66: // f64.ge
      sljit_emit_fop1(m->jitc, SLJIT_CMP_F64 | SLJIT_SET_F_GREATER_EQUAL,
                      sv->val.op, sv->val.opw, sv2->val.op, sv2->val.opw);
      sljit_emit_op_flags(m->jitc, SLJIT_MOV, b, 0, SLJIT_F_GREATER_EQUAL);
      break;
    }
    {
      struct sljit_jump *jump2 = sljit_emit_jump(m->jitc, SLJIT_JUMP);
      sljit_set_label(jump, sljit_emit_label(m->jitc));
      // f64.ne return true if NaN
      sljit_emit_op1(m->jitc, SLJIT_MOV, b, 0, SLJIT_IMM, opcode == 0x62);
      sljit_set_label(jump2, sljit_emit_label(m->jitc));
    }
    m->sp -= 2;
    sv = stackvalue_Push(m, WVT_I32);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = b;
    sv->val.opw = 0;
    break;
  }
}

static void opgen_GenArithmeticOp(ModuleCompiler *m,int opcode){
  StackValue *sv, *sv2;
  int32_t a, b;
  sljit_s32 op1, op2;
  sljit_sw opw1, opw2;
  struct sljit_jump *jump;
  StackValue *stack = m->stack;
  switch (opcode) {
  
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
    sv = stackvalue_Push(m, WVT_F64);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = a;
    sv->val.opw = 0;
    break;
  case 0x9a: // f64.neg
    sv = &stack[m->sp];
    a = pwart_GetFreeReg(m, RT_FLOAT, 1); // free register
    sljit_emit_fop1(m->jitc, SLJIT_NEG_F64, a, 0, sv->val.op, sv->val.opw);
    sv = stackvalue_Push(m, WVT_F64);
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
      sv = stackvalue_Push(m, WVT_I32);
      sv->jit_type = SVT_GENERAL;
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
        sv = stackvalue_Push(m, WVT_I64);
        sv->jit_type = SVT_GENERAL;
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
      sv = stackvalue_Push(m, WVT_F64);
      sv->jit_type = SVT_GENERAL;
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
      sv = stackvalue_Push(m, WVT_F64);
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
    }
    break;
  }
}

static void opgen_GenConvertOp(ModuleCompiler *m,int opcode){
  StackValue *sv, *sv2;
  int32_t a, b;
  sljit_s32 op1, op2;
  sljit_sw opw1, opw2;
  struct sljit_jump *jump;
  StackValue *stack = m->stack;

  switch(opcode){

  // conversion operations
  // case 0xa7 ... 0xbb:
  case 0xa7: // i32.wrap_i64
    sv = &stack[m->sp];
    a = pwart_GetFreeReg(m, RT_INTEGER, 1);
    if (m->target_ptr_size == 32 && sv->wasm_type == WVT_I64) {
      if (sv->jit_type == SVT_TWO_REG) {
        sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, sv->val.tworeg.opr1, 0);
      } else if (sv->jit_type == SVT_I64CONST) {
        sljit_emit_op1(m->jitc, SLJIT_MOV, a, 0, SLJIT_IMM,
                       (sljit_sw)sv->val.const64 & 0xffffffff);
      }
    } else {
      sljit_emit_op2(m->jitc, SLJIT_AND, a, 0, sv->val.op, sv->val.opw,
                     SLJIT_IMM, 0xffffffff);
    }
    m->sp--;
    sv=stackvalue_Push(m,WVT_I32);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = a;
    sv->val.opw = 0;
    break;
  case 0xa8: // i32.trunc_f32_s
    sv = &stack[m->sp];
    a = pwart_GetFreeReg(m, RT_INTEGER, 1);
    sljit_emit_fop1(m->jitc, SLJIT_CONV_S32_FROM_F32, a, 0, sv->val.op,
                    sv->val.opw);
    m->sp--;
    sv=stackvalue_Push(m,WVT_I32);
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
    m->sp--;
    sv=stackvalue_Push(m,WVT_I32);
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
    m->sp--;
    sv=stackvalue_Push(m,WVT_I64);
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
    m->sp--;
    sv=stackvalue_Push(m,WVT_I64);
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
      m->sp--;
      sv=stackvalue_Push(m,WVT_I64);
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
      pwart_EmitCallFunc(m, &func_type_f64_ret_i64, SLJIT_IMM,
                         (sljit_sw)&insn_i64truncf64s);
    } else {
      sv = &stack[m->sp];
      a = pwart_GetFreeReg(m, RT_INTEGER, 1);
      sljit_emit_fop1(m->jitc, SLJIT_CONV_SW_FROM_F64, a, 0, sv->val.op,
                      sv->val.opw);
      m->sp--;
      sv=stackvalue_Push(m,WVT_I64);
      sv->jit_type = SVT_GENERAL;
      sv->val.op = a;
      sv->val.opw = 0;
    }
    break;
  case 0xb1: // i64.trunc_f64_u
    pwart_EmitCallFunc(m, &func_type_f64_ret_i64, SLJIT_IMM,
                       (sljit_sw)&insn_i64truncf64u);
    break;
  case 0xb2: // f32.convert_i32_s
    sv = &stack[m->sp];
    a = pwart_GetFreeReg(m, RT_FLOAT, 1);
    sljit_emit_fop1(m->jitc, SLJIT_CONV_F32_FROM_S32, a, 0, sv->val.op,
                    sv->val.opw);
    m->sp--;
    sv=stackvalue_Push(m,WVT_F32);
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
      m->sp--;
      sv=stackvalue_Push(m,WVT_F32);
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
    m->sp--;
    sv=stackvalue_Push(m,WVT_F32);
    sv->jit_type = SVT_GENERAL;
    sv->val.op = a;
    sv->val.opw = 0;
    break;
  case 0xb7: // f64.convert_i32_s
    sv = &stack[m->sp];
    a = pwart_GetFreeReg(m, RT_FLOAT, 1);
    sljit_emit_fop1(m->jitc, SLJIT_CONV_F64_FROM_S32, a, 0, sv->val.op,
                    sv->val.opw);
    m->sp--;
    sv=stackvalue_Push(m,WVT_F64);
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
      m->sp--;
      sv=stackvalue_Push(m,WVT_F64);
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
    m->sp--;
    sv=stackvalue_Push(m,WVT_F64);
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
    sv->wasm_type = WVT_I64;
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
      sljit_emit_op1(m->jitc, SLJIT_MOV_S8, sv->val.tworeg.opr1, 0, sv->val.op,
                     sv->val.opw);
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
      sljit_emit_op1(m->jitc, SLJIT_MOV_S16, sv->val.tworeg.opr1, 0, sv->val.op,
                     sv->val.opw);
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
  }
}

static char *opgen_GenNumOp(ModuleCompiler *m, int opcode) {
  if(opcode>=0x45 && opcode <=0x66){
    opgen_GenCompareOp(m,opcode);
  }else if(opcode>=0x67 && opcode <=0xa6){
    opgen_GenArithmeticOp(m,opcode);
  }else if(opcode>=0xa7 && opcode <=0xc4){
    opgen_GenConvertOp(m,opcode);
  }else{
    wa_debug("unrecognized opcode 0x%x at %d", opcode, m->pc);
    return "unrecognized opcode";
  }
  return NULL;
}

#endif