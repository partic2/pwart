#ifndef _PWART_WASM_DEF_H
#define _PWART_WASM_DEF_H

#include <pwart.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "util.c"

#include <stdint.h>

#include "../sljit/sljit_src/sljitLir.h"



#define WA_MAGIC   0x6d736100
#define WA_VERSION 0x01

#define PAGE_SIZE       0x10000  // 65536

#define WVT_I32       0x7f  // -0x01
#define WVT_I64       0x7e  // -0x02
#define WVT_F32       0x7d  // -0x03
#define WVT_F64       0x7c  // -0x04
#define WVT_FUNC      0x70
#define WVT_REF       0x6f




//register type
#define RT_INTEGER 1
#define RT_FLOAT 2
//base register, can be use as address base [R0+imm]
#define RT_BASE 3



//register status
#define RS_RESERVED 1   //reserved register will not return by get_free_reg


typedef struct Type {
    uint32_t form;
    uint8_t *params;     // type array
    uint8_t *results;    // type array
} Type;

typedef union FuncPtr {
    void   (*void_void)  ();
    void   (*void_i32)   (uint32_t);
    void   (*void_i64)   (uint64_t);
    void   (*void_f32)   (float);
    void   (*void_f64)   (double);
    double (*f64_f64)    (double);
} FuncPtr;


// A block or function
typedef struct Block {
    uint8_t    block_type;    // 0x00: function, 0x01: init_exp
                              // 0x02: block, 0x03: loop, 0x04: if
    //Type      *type;           params/results type
    struct sljit_jump *else_jump;     // if block only
    struct sljit_label *br_label;       // blocks only
    struct dynarr *br_jump;         //type struct jump *
} Block;

//fp:stack frame pointer
typedef void (*WasmFunctionEntry)(void *fp);
typedef struct WasmFunction {
    WasmFunctionEntry func_ptr; // function only
    uint32_t tidx;  // type index
    uint32_t fidx;
    uint8_t *locals_type;  //type array
} WasmFunction;



// General type, use val.sljitop.
#define SVT_GENERAL 1

//Indicate the value is a compare result, for optimizing branch instruction, use val.cmp. 
#define SVT_CMP 2

//pointer type, not use yet
#define SVT_POINTER 3

// To represent WVT_I64 in two 32 bit register, use val.tworeg. 32bit arch only. 
//DO NOT use SAVED_REGISTER as val for this type, until i64 operate write SCRATCH_REGISTER properly.
#define SVT_TWO_REG 4

// 64bit constant.  use const64. 32bit arch only. 
#define SVT_I64CONST 5

// dummy stack value, May also be set if we want stack value to be ignored in free register scanning.
#define SVT_DUMMY 6 


typedef struct StackValue {
    uint8_t    wasm_type;
    uint8_t    jit_type;
    //indicate the offset of this StackValue, relative to runtime frame pointer.
    uint16_t   frame_offset;
    union {
        struct{
            uint8_t flag;
        } cmp;
        //SVT_GENERAL only, represent the arguments for sljit_emit_opx(op,opw)
        struct{
            sljit_s32 op;
            sljit_sw  opw;
        } ;
        //SVT_TWO_REG only
        struct{
            sljit_s32 opr1;
            sljit_s32 opr2;
        } tworeg;
        uint64_t const64;
    }val;
} StackValue;


typedef struct pwart_wasm_table Table;

typedef struct pwart_wasm_memory Memory;

typedef struct Export {
    uint8_t     external_kind;
    char       *export_name;
    void       *value;
} Export;


typedef struct RuntimeContext{
    uint8_t stack_flags;    //PWART_STACK_FLAGS_xxx
    uint8_t is_in_namespace;//if this context attached to namespace.
    struct pool strings_pool;   //string pools, type char
    uint32_t start_function;    // start function index,0xffffffff if none.
    WasmFunctionEntry *funcentries;  // imported and locally defined functions, type WasmFunctionEntry
    uint32_t funcentries_count;
    uint32_t import_funcentries_count;
    uint16_t import_tables_count;
    uint16_t own_tables_count;
    uint16_t own_memories_count;
    Table *own_tables; //tables owned by module self, not imported. (can't move)
    Memory *own_memories; //memories owned by module self, not imported. (can't move)
    //XXX: maybe use fixed size array is better
    struct dynarr *own_globals; // globals variable buffer, type uint8_t
    struct dynarr *tables; // tables, type Table *
    struct dynarr *memories; // memories, type Memory *
    struct dynarr *exports;// exorts, type Export
    struct pwart_symbol_resolver *resolver; //symbol resolver.
    void *userdata;  //user data, pwart don't use it.
}RuntimeContext;

//compile module info
typedef struct ModuleCompiler {

    //runtime variable, must initialized before execute.
    RuntimeContext *context;
    
    char       *path;           // file path of the wasm module

    uint32_t    byte_count;     // number of bytes in the module
    uint8_t    *bytes;          // module content/bytes

    struct dynarr  *types;          // function types, type Type
    struct pool  types_pool;     // types pool


    uint32_t    import_count;   // number of leading imports in functions

    WasmFunctionEntry trap_handler;

    struct dynarr *globals;        // globals, type StackValue

    struct dynarr *functions;      //functions, type WasmFunction

    struct pwart_symbol_resolver *import_resolver;

    uint8_t compile_succeeded;

    //jit state, used in compile time.
    struct sljit_compiler *jitc;
    //in bits, 32 or 64
    int32_t target_ptr_size;
    int         pc;        // current parser pos
    int         sp;         // operand stack pointer, stack[sp] is valid stack value.
    StackValue  stack[32]; // main operand stack
    struct dynarr *blocks; // block stacks, block type
    struct dynarr *br_table; // br_table branch indexes, uint32_t type
    uint8_t eof;      // end of function

    //prepare info, used in compile time
    Type *function_type;   // function type current processing.
    uint8_t *function_locals_type; //function locals type current processing.
    struct dynarr *locals;      // function only, allocate after pwart_PrepareFunc, StackValue type
    int16_t mem_base_local;   // index of local variable which store memory 0 base.
    int16_t table_entries_local;  // index of local variable which store the table 0 base.
    int16_t first_stackvalue_offset;
    int16_t cached_midx;     //the index of memory we cache memory base to register.
    struct dynarr *locals_need_zero;  //the array of index of locals we need zero init, type int16_t
    uint32_t registers_status[SLJIT_NUMBER_OF_SCRATCH_REGISTERS]; // register status [Rx-R0]
    uint32_t float_registers_status[SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS];
    union{
        //temporary buffer used in importing
        char import_name_buffer[512];
        //buffer to format error message.
        char err_message[512];
    };
    
} ModuleCompiler;



typedef struct {
    struct pwart_symbol_resolver resolver;
    struct dynarr *mods;   //modules in this namespace, type pwart_named_module.
} Namespace;


#if DEBUG_BUILD

static char OPERATOR_INFO[][20] = {
    // Control flow operators
    "unreachable",           // 0x00
    "nop",                   // 0x01
    "block",                 // 0x02
    "loop",                  // 0x03
    "if",                    // 0x04
    "else",                  // 0x05
    "RESERVED",              // 0x06
    "RESERVED",              // 0x07
    "RESERVED",              // 0x08
    "RESERVED",              // 0x09
    "RESERVED",              // 0x0a
    "end",                   // 0x0b
    "br",                    // 0x0c
    "br_if",                 // 0x0d
    "br_table",              // 0x0e
    "return",                // 0x0f

    // Call operators
    "call",                  // 0x10
    "call_indirect",         // 0x11

    "RESERVED",              // 0x12
    "RESERVED",              // 0x13
    "RESERVED",              // 0x14
    "RESERVED",              // 0x15
    "RESERVED",              // 0x16
    "RESERVED",              // 0x17
    "RESERVED",              // 0x18
    "RESERVED",              // 0x19

    // Parametric operators
    "drop",                  // 0x1a
    "select",                // 0x1b
    "select t",               // 0x1c

    "RESERVED",              // 0x1d
    "RESERVED",              // 0x1e
    "RESERVED",              // 0x1f

    // Variable access
    "local.get",             // 0x20
    "local.set",             // 0x21
    "local.tee",             // 0x22
    "global.get",            // 0x23
    "global.set",            // 0x24

    "table.get",              // 0x25
    "table.set",              // 0x26
    "RESERVED",               // 0x27

    // Memory-related operator
    "i32.load",              // 0x28
    "i64.load",              // 0x29
    "f32.load",              // 0x2a
    "f64.load",              // 0x2b
    "i32.load8_s",           // 0x2c
    "i32.load8_u",           // 0x2d
    "i32.load16_s",          // 0x2e
    "i32.load16_u",          // 0x2f
    "i64.load8_s",           // 0x30
    "i64.load8_u",           // 0x31
    "i64.load16_s",          // 0x32
    "i64.load16_u",          // 0x33
    "i64.load32_s",          // 0x34
    "i64.load32_u",          // 0x35
    "i32.store",             // 0x36
    "i64.store",             // 0x37
    "f32.store",             // 0x38
    "f64.store",             // 0x39
    "i32.store8",            // 0x3a
    "i32.store16",           // 0x3b
    "i64.store8",            // 0x3c
    "i64.store16",           // 0x3d
    "i64.store32",           // 0x3e
    "current_memory",        // 0x3f
    "grow_memory",           // 0x40

    // Constants
    "i32.const",             // 0x41
    "i64.const",             // 0x42
    "f32.const",             // 0x43
    "f64.const",             // 0x44

    // Comparison operators
    "i32.eqz",               // 0x45
    "i32.eq",                // 0x46
    "i32.ne",                // 0x47
    "i32.lt_s",              // 0x48
    "i32.lt_u",              // 0x49
    "i32.gt_s",              // 0x4a
    "i32.gt_u",              // 0x4b
    "i32.le_s",              // 0x4c
    "i32.le_u",              // 0x4d
    "i32.ge_s",              // 0x4e
    "i32.ge_u",              // 0x4f

    "i64.eqz",               // 0x50
    "i64.eq",                // 0x51
    "i64.ne",                // 0x52
    "i64.lt_s",              // 0x53
    "i64.lt_u",              // 0x54
    "i64.gt_s",              // 0x55
    "i64.gt_u",              // 0x56
    "i64.le_s",              // 0x57
    "i64.le_u",              // 0x58
    "i64.ge_s",              // 0x59
    "i64.ge_u",              // 0x5a

    "f32.eq",                // 0x5b
    "f32.ne",                // 0x5c
    "f32.lt",                // 0x5d
    "f32.gt",                // 0x5e
    "f32.le",                // 0x5f
    "f32.ge",                // 0x60

    "f64.eq",                // 0x61
    "f64.ne",                // 0x62
    "f64.lt",                // 0x63
    "f64.gt",                // 0x64
    "f64.le",                // 0x65
    "f64.ge",                // 0x66

    // Numeric operators
    "i32.clz",               // 0x67
    "i32.ctz",               // 0x68
    "i32.popcnt",            // 0x69
    "i32.add",               // 0x6a
    "i32.sub",               // 0x6b
    "i32.mul",               // 0x6c
    "i32.div_s",             // 0x6d
    "i32.div_u",             // 0x6e
    "i32.rem_s",             // 0x6f
    "i32.rem_u",             // 0x70
    "i32.and",               // 0x71
    "i32.or",                // 0x72
    "i32.xor",               // 0x73
    "i32.shl",               // 0x74
    "i32.shr_s",             // 0x75
    "i32.shr_u",             // 0x76
    "i32.rotl",              // 0x77
    "i32.rotr",              // 0x78

    "i64.clz",               // 0x79
    "i64.ctz",               // 0x7a
    "i64.popcnt",            // 0x7b
    "i64.add",               // 0x7c
    "i64.sub",               // 0x7d
    "i64.mul",               // 0x7e
    "i64.div_s",             // 0x7f
    "i64.div_u",             // 0x80
    "i64.rem_s",             // 0x81
    "i64.rem_u",             // 0x82
    "i64.and",               // 0x83
    "i64.or",                // 0x84
    "i64.xor",               // 0x85
    "i64.shl",               // 0x86
    "i64.shr_s",             // 0x87
    "i64.shr_u",             // 0x88
    "i64.rotl",              // 0x89
    "i64.rotr",              // 0x8a

    "f32.abs",               // 0x8b
    "f32.neg",               // 0x8c
    "f32.ceil",              // 0x8d
    "f32.floor",             // 0x8e
    "f32.trunc",             // 0x8f
    "f32.nearest",           // 0x90
    "f32.sqrt",              // 0x91
    "f32.add",               // 0x92
    "f32.sub",               // 0x93
    "f32.mul",               // 0x94
    "f32.div",               // 0x95
    "f32.min",               // 0x96
    "f32.max",               // 0x97
    "f32.copysign",          // 0x98

    "f64.abs",               // 0x99
    "f64.neg",               // 0x9a
    "f64.ceil",              // 0x9b
    "f64.floor",             // 0x9c
    "f64.trunc",             // 0x9d
    "f64.nearest",           // 0x9e
    "f64.sqrt",              // 0x9f
    "f64.add",               // 0xa0
    "f64.sub",               // 0xa1
    "f64.mul",               // 0xa2
    "f64.div",               // 0xa3
    "f64.min",               // 0xa4
    "f64.max",               // 0xa5
    "f64.copysign",          // 0xa6

    // Conversions
    "i32.wrap_i64",          // 0xa7
    "i32.trunc_f32_s",       // 0xa8
    "i32.trunc_f32_u",       // 0xa9
    "i32.trunc_f64_s",       // 0xaa
    "i32.trunc_f64_u",       // 0xab

    "i64.extend_i32_s",      // 0xac
    "i64.extend_i32_u",      // 0xad
    "i64.trunc_f32_s",       // 0xae
    "i64.trunc_f32_u",       // 0xaf
    "i64.trunc_f64_s",       // 0xb0
    "i64.trunc_f64_u",       // 0xb1

    "f32.convert_i32_s",     // 0xb2
    "f32.convert_i32_u",     // 0xb3
    "f32.convert_i64_s",     // 0xb4
    "f32.convert_i64_u",     // 0xb5
    "f32.demote_f64",        // 0xb6

    "f64.convert_i32_s",     // 0xb7
    "f64.convert_i32_u",     // 0xb8
    "f64.convert_i64_s",     // 0xb9
    "f64.convert_i64_u",     // 0xba
    "f64.promote_f32",       // 0xbb

    // Reinterpretations
    "i32.reinterpret_f32",   // 0xbc
    "i64.reinterpret_f64",   // 0xbd
    "f32.reinterpret_i32",   // 0xbe
    "f64.reinterpret_i64",   // 0xbf

    "i32.extend8_s",         // 0xc0
    "i32.extend16_s",        // 0xc1
    "i64.extend8_s",         // 0xc2
    "i64.extend16_s",        // 0xc3
    "i64.extend32_s",        // 0xc4
    "RESERVED",              // 0xc5
    "RESERVED",              // 0xc6
    "RESERVED",              // 0xc7
    "RESERVED",              // 0xc8
    "RESERVED",              // 0xc9
    "RESERVED",              // 0xca
    "RESERVED",              // 0xcb
    "RESERVED",              // 0xcc
    "RESERVED",              // 0xcd
    "RESERVED",              // 0xce
    "RESERVED",              // 0xcf
    "ref.null",              // 0xd0
    "ref.isnull",            // 0xd1
    "ref.func"               // 0xd2

};

#endif

static struct{
  pwart_wasm_function get_self_runtime_context;
  pwart_wasm_function ref_from_index;
  pwart_wasm_function ref_from_i64;
  pwart_wasm_function i64_from_ref;
} pwart_InlineFuncList;

#define WASMOPC_i32_wrap_i64 0xa7
#define WASMOPC_i64_extend_i32_u 0xad
#define WASMOPC_i32_eqz 0x45
#define WASMOPC_if 0x04
#define WASMOPC_br_if 0x0d
#define WASMOPC_br 0x0c
#define WASMOPC_i32_shl 0x74
#define WASMOPC_i64_shl 0x86

static struct pwart_global_compile_config pwart_gcfg={
    .stack_flags=0,
    .misc_flags=PWART_MISC_FLAGS_LOCALS_ZERO_INIT
};

static sljit_s32 pwart_GetFreeReg(ModuleCompiler *m, sljit_s32 regtype,int upstack);
static uint32_t stackvalue_GetSizeAndAlign(StackValue *sv,uint32_t *align);
static void pwart_EmitStackValueLoadReg(ModuleCompiler *m, StackValue *sv);
static int opgen_GenBaseAddressRegForTable(ModuleCompiler *m,uint32_t tabidx);
static int opgen_GenBaseAddressReg(ModuleCompiler *m,uint32_t midx);
static char *opgen_GenNumOp(ModuleCompiler *m, int opcode);
//if fn is stub/inline function, generate native code and return 1, else return 0.
static int pwart_CheckAndGenStubFunction(ModuleCompiler *m,WasmFunctionEntry fn);
static void opgen_GenRefNull(ModuleCompiler *m, int32_t typeidx);
static void opgen_GenI32Const(ModuleCompiler *m, uint32_t c);
static void opgen_GenI64Const(ModuleCompiler *m, uint64_t c);
static void opgen_GenF32Const(ModuleCompiler *m, uint8_t *c);
static void opgen_GenF64Const(ModuleCompiler *m, uint8_t *c);
static void opgen_GenRefConst(ModuleCompiler *m,void *c);
static void opgen_GenLocalSet(ModuleCompiler *m, uint32_t idx);
static void opgen_GenLocalTee(ModuleCompiler *m, uint32_t idx);
static void opgen_GenGlobalGet(ModuleCompiler *m, uint32_t idx);

#endif