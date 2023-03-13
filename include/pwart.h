#ifndef _PWART_H
#define _PWART_H


#include <stdint.h>

typedef  void *pwart_module_compiler;

typedef void *pwart_module_state;

//webassembly function definition (for 1.0 version)

typedef void *pwart_wasm_function;

typedef void (*pwart_host_function_c)(void *stack_frame);

//currently only have 1.0 version
#define PWART_VERSION_1 1



//pwart symbol type
#define PWART_KIND_FUNCTION 0
#define PWART_KIND_TABLE    1
#define PWART_KIND_MEMORY   2
#define PWART_KIND_GLOBAL   3

// module load config

// These config are related to ABI of the generated code, so keep same config to load module at same time in one process.
struct pwart_global_compile_config{
    // PWART_STACK_FLAGS_xxx flags, indicate how operand stored in stack.
    char stack_flags;
    // PWART_MEMORY_MODEL_xxx flags, indicate how code access memory, this flag can be overwrite by pwart_compile_config.memory_model.
    char memory_model;
};

//If set, elements on stack will align to it's size, and function frame base will align to 8. 
//Some arch require this flag to avoid align error.
#define PWART_STACK_FLAGS_AUTO_ALIGN 1

struct pwart_compile_config{
    void (*import_resolver)(char *import_module,char *import_field,uint32_t kind,void *result);
    // PWART_MEMORY_MODEL_xxx flags
    char memory_model;
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



extern int pwart_get_version();

/*  
    pwart_load_module load a module from data, if succeeded, return compiled module, if failed, return NULL and set err_msg, if err_msg is not NULL.
    It invoke "pwart_new_module_compiler","pwart_compile","pwart_get_module_state","pwart_free_module_compiler" internally.
*/
extern pwart_module_state *pwart_load_module(char *data,int len,char **err_msg);

extern pwart_module_compiler pwart_new_module_compiler();

//free compile infomation, have no effect to pwart_module_state and generated code.
//return error message if any, or NULL if succeded.
extern char *pwart_free_module_compiler(pwart_module_compiler mod);



//compile module and generate code. if cfg is NULL, use default compile config.
//return error message if any, or NULL if succeded.
extern char *pwart_compile(pwart_module_compiler m,char *data,int len);

//return wasm start function, or NULL if no start function specified.
//you should call the start function to init module, according to WebAssembly spec.
//Though it's may not necessary for PWART.
extern pwart_wasm_function pwart_get_start_function(pwart_module_compiler m);




//wrap host function to wasm function, must be free by pwart_free_wrapped_function.
extern pwart_wasm_function pwart_wrap_host_function_c(pwart_host_function_c host_func);

extern void pwart_free_wrapped_function(pwart_wasm_function wrapped);

extern void pwart_call_wasm_function(pwart_wasm_function fn,void *stack_pointer);


// return error message if any, or NULL if succeded.
extern char *pwart_set_global_compile_config(struct pwart_global_compile_config *config);
extern char *pwart_get_global_compile_config(struct pwart_global_compile_config *config);
extern char *pwart_set_load_config(pwart_module_compiler m,struct pwart_compile_config *config);
extern char *pwart_get_load_config(pwart_module_compiler m,struct pwart_compile_config *config);

//return error message if any, or NULL if succeded.
extern char *pwart_set_symbol_resolver(pwart_module_compiler m,void (*resolver)(char *import_module,char *import_field,uint32_t kind,void *result));


extern pwart_module_state pwart_get_module_state(pwart_module_compiler m);

//free module state and generated code.
//return error message if any, or NULL if succeded.
//only need free if pwart_compile succeeded.
extern char *pwart_free_module_state(pwart_module_state rc);

//get wasm function exported by wasm module runtime.
extern pwart_wasm_function pwart_get_export_function(pwart_module_state rc,char *name);

//pwart_module_state inspect result
struct pwart_inspect_result1{
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
extern char *pwart_inspect_module_state(pwart_module_state c,struct pwart_inspect_result1 *result);

extern void pwart_module_state_set_user_data(pwart_module_state c,void *ud);
extern void *pwart_module_state_get_user_data(pwart_module_state c);

// return error message if any, or NULL if succeded.
extern char *pwart_free_module_compiler(pwart_module_compiler mod);


//pwart invoke and runtime stack helper

//allocate a stack buffer, with align 16. can only be free by pwart_free_stack.
extern void *pwart_allocate_stack(int size);
extern void pwart_free_stack(void *stack);

//For version 1.0, arguments and results are all stored on stack.
//stack_flags has effect on these function.

//put value to *sp, move *sp to next entries.
extern void pwart_rstack_put_i32(void **sp,int val);
extern void pwart_rstack_put_i64(void **sp,long long val);
extern void pwart_rstack_put_f32(void **sp,float val);
extern void pwart_rstack_put_f64(void **sp,double val);
extern void pwart_rstack_put_ref(void **sp,void *val);
//get value on *sp, move *sp to next entries.
extern int pwart_rstack_get_i32(void **sp);
extern long long pwart_rstack_get_i64(void **sp);
extern float pwart_rstack_get_f32(void **sp);
extern double pwart_rstack_get_f64(void **sp);
extern void *pwart_rstack_get_ref(void **sp);



//pwart builtin WasmFunction, can be call by import pwart_builtin module in wasm.
//Unless explicitly mark as Overwriteable,  Modifing the value may take no effect and should be avoided.
struct pwart_builtin_functable{
    //get pwart version, []->[i32]
    pwart_wasm_function version;

    //allocate memory, see also PWART_MEMORY_MODEL_RAW flag.

    //function type [i32]->[i32]
    pwart_wasm_function malloc32;
    //function  type [i64]->[i64]
    pwart_wasm_function malloc64;

    //Stub function, can only be called DIRECTLY by wasm.
    //PWART will compile these 'call' instruction into native code, instead of function call.

    //get pwart_runtime_context attached to current function.
    //function type []->[ref]
    pwart_wasm_function get_self_runtime_context;

};

extern struct pwart_builtin_functable *pwart_get_builtin_functable();



//WIP: namespace(linker)
struct pwart_import_request{
    char kind;
    char *module_name;
    char *field_name;
    void **result;
};
struct pwart_export_request{
    char kind;
    char *module_name;
    char *field_name;
    void *to_export;
};
struct pwart_create_request{
    char kind;
    void **result;
};
struct pwart_symbol_resolver{
    char *(*create)(struct pwart_symbol_resolver *_this,struct pwart_create_request *req);
    char *(*import)(struct pwart_symbol_resolver *_this,struct pwart_import_request *req);
    char *(*export)(struct pwart_symbol_resolver *_this,struct pwart_export_request *req);
};

struct pwart_wasm_table {
    uint8_t     elem_type;   // type of entries (only FUNC in MVP)
    uint8_t     is_import;   // is this table imported, if it is, pwart_free_module_state will not free the entries
    uint32_t    initial;     // initial table size
    uint32_t    maximum;     // maximum table size
    uint32_t    size;        // current table size
    void        **entries;
};
struct pwart_wasm_memory {
    uint32_t    initial;     // initial size (64K pages)
    uint32_t    maximum;     // maximum size (64K pages)
    uint32_t    pages;       // current size (64K pages)
    uint8_t    *bytes;       // memory area, if NULL, indicate this is memory is mapped to native host memory directly.
    uint8_t     fixed;       // fixed memory, memory base will never change, and memory.grow to this memory always return -1. 
};

#endif