#ifndef _PWART_H
#define _PWART_H

#include <stdio.h>

typedef  void *pwart_module;

typedef void *pwart_runtime_context;

typedef void (*pwart_wasmfunction)(void *stack_frame,pwart_runtime_context context);

extern pwart_module pwart_new_module();

//free module and runtime context, if created.
extern void pwart_delete_module(pwart_module m);

//free compile infomation, have no effect to pwart_runtime_context and generated code.
extern void pwart_free_module(pwart_module mod);

//free runtime context and generated code.
extern void pwart_free_runtime(pwart_runtime_context rc);

extern int pwart_load(pwart_module m,char *data,int len);

//this function can only called before module is free.
extern pwart_wasmfunction pwart_get_export_function(pwart_module module,char *name);

// pwart_module config, must set before pwart_load.

extern int pwart_set_symbol_resolver(pwart_module m,void (*resolver)(char *import_module,char *import_field,void *result));
// set max runtime stack size, default is 63KB.
extern int pwart_set_stack_size(pwart_module m,int stack_size);



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
extern int pwart_inspect_runtime_context(pwart_runtime_context c,struct pwart_inspect_result1 *result);

extern int pwart_set_runtime_user_data(pwart_runtime_context c,void *ud);
extern void *pwart_get_runtime_user_data(pwart_runtime_context c);

extern void pwart_free_module(pwart_module mod);


#endif