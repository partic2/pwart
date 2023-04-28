#ifndef _PWART_H
#define _PWART_H


#include <stdint.h>




/* Low Level API , about loading and compiling wasm module.*/

/* return error message if any, or NULL if succeded. */
struct pwart_symbol_resolve_request{
    char *import_module;
    char *import_field;
    uint32_t kind;
    void *result;
};
struct pwart_symbol_resolver{
    void (*resolve)(struct pwart_symbol_resolver *_this,struct pwart_symbol_resolve_request *req);
};


typedef void *pwart_module_compiler;

typedef void *pwart_module_state;

/* webassembly function definition (for 1.0 version) */

typedef void *pwart_wasm_function;

typedef void (*pwart_host_function_c)(void *stack_frame);

/* currently only have 1.0 version */
#define PWART_VERSION_1 1


/* pwart symbol type */
#define PWART_KIND_FUNCTION 0
#define PWART_KIND_TABLE    1
#define PWART_KIND_MEMORY   2
#define PWART_KIND_GLOBAL   3

/* module load config */ 

/* These config are related to ABI of the generated code, so keep same config to load module at same time in one process. */
struct pwart_global_compile_config{
    /* PWART_STACK_FLAGS_xxx flags, indicate how operand stored in stack. */
    char stack_flags;
};

/* If set, elements on stack will align to it's size, and function frame base will align to 8. 
Some arch require this flag to avoid align error. */
#define PWART_STACK_FLAGS_AUTO_ALIGN 1



extern int pwart_get_version();



struct pwart_wasm_table {
    uint8_t     elem_type;   /* type of entries (only FUNC in MVP) */
    uint32_t    initial;     /* initial table size */
    uint32_t    maximum;     /* maximum table size */
    uint32_t    size;        /* current table size */
    void        **entries;
};
struct pwart_wasm_memory {
    uint32_t    initial;     /* initial size (64K pages) */
    uint32_t    maximum;     /* maximum size (64K pages) */
    uint32_t    pages;       /* current size (64K pages) */

    /*  memory area, if 0, indicate this memory is mapped to native host memory directly, and the pwart will generated faster code to access this memory. 
        In this case, memory can only be allocated by call pwart_builtin.memory_alloc, and index type should match the machine word size.
        wasm module can use this memory by import memory pwart_builtin.native_memory, see pwart_builtin_symbols for detail.
    */
    uint8_t    *bytes;

    /*  fixed memory, if set, memory base will never change, and memory.grow to this memory always return -1. 
        set this flags make generated code faster, by avoid memory base reloading. 
        by default, pwart set fixed to true if wasm memory maximum size is set.(allocate to maximum),
        and false if no maximum provided */
    uint8_t     fixed;
};


/*  pwart_load_module load a module from data, if succeeded, return compiled module, if failed, return NULL and set err_msg, if err_msg is not NULL.
    It invoke "pwart_new_module_compiler","pwart_compile","pwart_get_module_state","pwart_free_module_compiler" internally. */
extern pwart_module_state *pwart_load_module(char *data,int len,char **err_msg);

extern pwart_module_compiler pwart_new_module_compiler();

/*  free compile infomation, have no effect to pwart_module_state and generated code. 
    return error message if any, or NULL if succeded. */
extern char *pwart_free_module_compiler(pwart_module_compiler mod);

extern char *pwart_set_symbol_resolver(pwart_module_compiler m,struct pwart_symbol_resolver *resolver);


/*  compile module and generate code. if cfg is NULL, use default compile config. 
    return error message if any, or NULL if succeded. */
extern char *pwart_compile(pwart_module_compiler m,char *data,int len);

/*  return wasm start function, or NULL if no start function specified. 
    you should call the start function to init module, according to WebAssembly spec. 
    Though it's may not necessary for PWART. */
extern pwart_wasm_function pwart_get_start_function(pwart_module_compiler m);

/*  return error message if any, or NULL if succeded. */
extern char *pwart_set_global_compile_config(struct pwart_global_compile_config *config);
extern char *pwart_get_global_compile_config(struct pwart_global_compile_config *config);

extern pwart_module_state pwart_get_module_state(pwart_module_compiler m);

/*  free module state and generated code. 
    return error message if any, or NULL if succeded. 
    only need free if pwart_compile succeeded. */
extern char *pwart_free_module_state(pwart_module_state rc);

/*  The symbol resolver of pwart_module_state will be set to the same as compiler use by default, 
    and can be changed by this function */
extern char *pwart_set_state_symbol_resolver(pwart_module_state rc,struct pwart_symbol_resolver *resolver);

/* get wasm function exported by wasm module runtime. */
extern pwart_wasm_function pwart_get_export_function(pwart_module_state rc,char *name);

/* get wasm memory exported by wasm module runtime. */
extern struct pwart_wasm_memory *pwart_get_export_memory(pwart_module_state rc,char *name);

/* get wasm table exported by wasm module runtime. */
extern struct pwart_wasm_table *pwart_get_export_table(pwart_module_state rc,char *name);

/* get wasm globals exported by wasm module runtime. */
extern void *pwart_get_export_global(pwart_module_state rc,char *name);

/* pwart_module_state inspect result */
struct pwart_inspect_result1{
    /* memory 0 size , in byte */
    int memory_size;
    void *memory;
    /* table 0 entries count, in void* */
    int table_entries_count;
    void **table_entries;
    /* global buffer size, in byte */
    int globals_buffer_size;
    void *globals_buffer;
    struct pwart_symbol_resolver *symbol_resolver;
};
/*  return error message if any, or NULL if succeded. */
extern char *pwart_inspect_module_state(pwart_module_state c,struct pwart_inspect_result1 *result);

extern void pwart_module_state_set_user_data(pwart_module_state c,void *ud);
extern void *pwart_module_state_get_user_data(pwart_module_state c);

/*  return error message if any, or NULL if succeded. */
extern char *pwart_free_module_compiler(pwart_module_compiler mod);

/* pwart invoke and runtime stack helper */

/* allocate a stack buffer, with align 16. can only be free by pwart_free_stack. */
extern void *pwart_allocate_stack(int size);
extern void pwart_free_stack(void *stack);

/* wrap host function to wasm function, must be free by pwart_free_wrapped_function. */
extern pwart_wasm_function pwart_wrap_host_function_c(pwart_host_function_c host_func);

extern void pwart_free_wrapped_function(pwart_wasm_function wrapped);

extern void pwart_call_wasm_function(pwart_wasm_function fn,void *stack_pointer);


/* For version 1.0, arguments and results are all stored on stack. */
/* stack_flags has effect on these function. */

/* put value to *sp, move *sp to next entries. */
extern void pwart_rstack_put_i32(void **sp,int val);
extern void pwart_rstack_put_i64(void **sp,long long val);
extern void pwart_rstack_put_f32(void **sp,float val);
extern void pwart_rstack_put_f64(void **sp,double val);
extern void pwart_rstack_put_ref(void **sp,void *val);
/* get value on *sp, move *sp to next entries. */
extern int pwart_rstack_get_i32(void **sp);
extern long long pwart_rstack_get_i64(void **sp);
extern float pwart_rstack_get_f32(void **sp);
extern double pwart_rstack_get_f64(void **sp);
extern void *pwart_rstack_get_ref(void **sp);



/* pwart builtin symbol, can be use by import "pwart_builtin" module in wasm. */
/* Unless explicitly mark as Overwriteable,  Modifing the value may take no effect and should be avoided. */
struct pwart_builtin_symbols{
    /* get pwart version, []->[i32] */
    pwart_wasm_function version;

    /* allocate memory, see also pwart_wasm_memory.bytes */
    /* allocate memory with 'size', in byte, return index on native_memory, for 32bit native_memory, the most significant 32bit word are 0. */
    /* function type [i32 size]->[i64] */
    pwart_wasm_function memory_alloc;
    /* function type [i64]->[] */
    pwart_wasm_function memory_free;


    /* get the index size to access native memory, in byte (sizeof(void *)). */
    /* function type []->[i32] */
    pwart_wasm_function native_index_size;

    /* module function */

    /* load module into same namespace, the caller module must been attached to a namespace. */
    /* function type [ref self_context,ref name,ref wasm_bytes,i32 length]->[ref err_msg] */
    pwart_wasm_function load_module;

    /* function type [ref self_context,ref name]->[ref err_msg] */
    pwart_wasm_function unload_module;

    /* Only support PWART_KIND_FUNCTION now */
    /* function type [ref self_context,ref module_name,ref field_name,i32 kind] -> [ref] */
    pwart_wasm_function import;

    /* Stub function, can only be called DIRECTLY by wasm. */
    /* PWART will compile these 'call' instruction into native code, instead of function call. */

    /* get pwart_module_state attached to current function. */
    /* function type []->[ref self_context] */
    pwart_wasm_function get_self_runtime_context;

    /* get "pointer" reference that represent the address specified by the index at memory 0, should only be called directly. */
    /* function type [i32|i64 index]->[ref] */
    pwart_wasm_function ref_from_index;

    /* copy bytes from 'src' to 'dst' for 'n' bytes, use "pointer" reference.  */
    /* function type [ref dst,ref src,i32 n]->[] */
    pwart_wasm_function ref_copy_bytes;

    /* get the length of a string, specified by "pointer" reference. */
    /* function type [ref str]->[i32 n] */
    pwart_wasm_function ref_string_length;

    /* cast i64 to "pointer" reference. */
    /* function type [i64]->[ref]  */
    pwart_wasm_function ref_from_i64;

    /* cast "pointer" reference to i64. */
    /* function type [ref]->[i64]  */
    pwart_wasm_function i64_from_ref;


    /* WIP */
    /* stdio function */
    /* function type [ref path,ref mode]->[ref file,i32 err] */
    pwart_wasm_function fopen;

    /* function type [ref buf,i32 size,i32 count,ref file]->[i32 nread] */
    pwart_wasm_function fread;

    /* function type [ref buf,i32 size,i32 count,ref file]->[i32 nwrite] */
    pwart_wasm_function fwrite;

    /* function type [ref file,i64 offset,i32 fromwhere]->[i32 err] */
    pwart_wasm_function fseek;

    /* function type [ref file]->[i32 err] */
    pwart_wasm_function fclose;

    /* special pwart_wasm_memory that map to the native host memory directly, memory->bytes is 0;  */
    struct pwart_wasm_memory native_memory;

};

extern struct pwart_builtin_symbols *pwart_get_builtin_symbols();



/* High level API, Namespace and Module */

typedef void *pwart_namespace;

struct pwart_host_module{
     void (*resolve)(struct pwart_host_module *_this,struct pwart_symbol_resolve_request *req);
     /*Will be set after added to namespace */
     pwart_namespace namespace;
     /*If not null, call after added to namespace */
     void (*on_attached)(struct pwart_host_module *_this);
     /*If not null, call after remove from namespace, or namespace is deleted */
     void (*on_detached)(struct pwart_host_module *_this);
};

#define PWART_MODULE_TYPE_HOST_MODULE 1
#define PWART_MODULE_TYPE_WASM_MODULE 2
/* Indicate module has been free. */
#define PWART_MODULE_TYPE_NULL 0;


struct pwart_named_module{
    char *name;
    int type;   /* PWART_MODULE_TYPE_xxxx */
    union{
        struct pwart_host_module *host;
        pwart_module_state wasm;
    } val;
};

extern pwart_namespace pwart_namespace_new();

extern char *pwart_namespace_delete(pwart_namespace ns);

extern char *pwart_namespace_remove_module(pwart_namespace ns,char *name);

/* module structure will be copied into internal buffer, No need for caller to hold the memory */
extern char *pwart_namespace_define_module(pwart_namespace ns,struct pwart_named_module *mod);

extern pwart_module_state *pwart_namespace_define_wasm_module(pwart_namespace ns,char *name,char *wasm_bytes,int length,char **err_msg);

extern struct pwart_named_module *pwart_namespace_find_module(pwart_namespace ns,char *name);

extern struct pwart_symbol_resolver *pwart_namespace_resolver(pwart_namespace ns);


#endif