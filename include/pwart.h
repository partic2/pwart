#ifndef _PWART_H
#define _PWART_H


#include <stdint.h>

typedef  void *pwart_module;

typedef void *pwart_runtime_context;

//webassembly function definition (for 1.0 version)
typedef void (*pwart_wasmfunction)(void *stack_frame,pwart_runtime_context context);

//currently only have 1.0 version
#define PWART_VERSION_1 1

extern int pwart_get_version();

extern pwart_module pwart_new_module();

//free module and relative runtime context , if existed.
//equal to call pwart_free_runtime and pwart_free_module separately.
//return error message if any, or NULL if succeded.
extern char *pwart_delete_module(pwart_module m);

//free compile infomation, have no effect to pwart_runtime_context and generated code.
//return error message if any, or NULL if succeded.
extern char *pwart_free_module(pwart_module mod);

//free runtime context and generated code.
//return error message if any, or NULL if succeded.
extern char *pwart_free_runtime(pwart_runtime_context rc);

//load module and generate code.
//return error message if any, or NULL if succeded.
extern char *pwart_load(pwart_module m,char *data,int len);

//get wasmfunction exported by wasm module.
//you can only get exported symbol before module is free.
extern pwart_wasmfunction pwart_get_export_function(pwart_module module,char *name);

// pwart_module load config, must set before pwart_load.
//return error message if any, or NULL if succeded.
//currently only implement function import.
extern char *pwart_set_symbol_resolver(pwart_module m,void (*resolver)(char *import_module,char *import_field,void *result));
// set max runtime stack size, default is 63KB.
// return error message if any, or NULL if succeded.
extern char *pwart_set_stack_size(pwart_module m,int stack_size);

struct pwart_load_config{
    //To indicate the api version, should be set before get_config or set_config.
    int size_of_this;
    void (*import_resolver)(char *import_module,char *import_field,void *result);
    int stack_size;
    // PWART_MEMORY_MODEL_xxx flags
    char memory_model;
    // PWART_STACK_FLAGS_xxx flags
    char stack_flags;
};
//Default memory model, memory is initialize to the max size, memory.grow will always return -1
#define PWART_MEMORY_MODEL_FIXED_SIZE 0

//memory.grow will try realloc memory. 
//NOTICE: This option will slow the generated code due to memory base reloading.
#define PWART_MEMORY_MODEL_GROW_ENABLED 1

//EXPERIMENTAL
//This option make PWART to use direct memory access instead of offset + base, which can make generated code faster.
//But If set this flag, you can only use i32 to access memory on 32bit arch, and i64 on 64bit arch.
//Memory must be allocated by calling external function "pwart_builtin.malloc32(type:[i32]->[i32])" or "pwart_builtin.malloc64(type:[i64]->[i64])" (for memory64)
#define PWART_MEMORY_MODEL_RAW 2

//If set, elements on stack will align to it's size, and function frame base will align to 8. 
//Some arch require this flag to avoid align error.
#define PWART_STACK_FLAGS_AUTO_ALIGN 1

// return error message if any, or NULL if succeded.
extern char *pwart_set_load_config(pwart_module m,struct pwart_load_config *config);
extern char *pwart_get_load_config(pwart_module m,struct pwart_load_config *config);



extern pwart_runtime_context pwart_get_runtime_context(pwart_module m);

//runtime_context inspect result
struct pwart_inspect_result1{
    //default runtime stack bottom.
    void *runtime_stack;
    //memory 0 size , in byte
    int memory_size;
    void *memory;
    //table 0 entries count, in void*
    int table_entries_count;
    void **table_entries;
    //global buffer size, in byte
    int globals_buffer_size;
    void *globals_buffer;
};
// return error message if any, or NULL if succeded.
extern char *pwart_inspect_runtime_context(pwart_runtime_context c,struct pwart_inspect_result1 *result);

extern void pwart_set_runtime_user_data(pwart_runtime_context c,void *ud);
extern void *pwart_get_runtime_user_data(pwart_runtime_context c);

// return error message if any, or NULL if succeded.
extern char *pwart_free_module(pwart_module mod);


//pwart invoke and runtime stack helper

//For version 1.0, arguments and results are all stored on stack.
//PWART_STACK_FLAGS_AUTO_ALIGN currently has no effect to these function, user need add padding by self.

//put value to *sp, move *sp to next entries.
extern void pwart_rstack_put_i32(void **sp,int val);
extern void pwart_rstack_put_i64(void **sp,long long val);
extern void pwart_rstack_put_f32(void **sp,float val);
extern void pwart_rstack_put_f64(void **sp,double val);
//get value on *sp, move *sp to next entries.
extern int pwart_rstack_get_i32(void **sp);
extern long long pwart_rstack_get_i64(void **sp);
extern float pwart_rstack_get_f32(void **sp);
extern double pwart_rstack_get_f64(void **sp);


//pwart builtin WasmFunction, can be call by import pwart_builtin module in wasm.
//Unless explicitly mark as Overwriteable,  Modifing the value may take no effect and should be avoided.
struct pwart_builtin_functable{
    //get pwart version, []->[i32]
    pwart_wasmfunction version;

    //allocate memory, see also PWART_MEMORY_MODEL_RAW flag.

    //function type [i32]->[i32]
    pwart_wasmfunction malloc32;
    //function  type [i64]->[i64]
    pwart_wasmfunction malloc64;

    //convert i32/i64 to ref type.

    //function type [i32]->[ref]
    pwart_wasmfunction ref_from_i32;
    //function type [i64]->[ref]
    pwart_wasmfunction ref_from_i64;
};

extern struct pwart_builtin_functable *pwart_get_builtin_functable();
#endif