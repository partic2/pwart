#ifndef _PWART_H
#define _PWART_H

#include <stdio.h>

typedef  void *pwart_module;

typedef void *pwart_runtime_context;

typedef void (*pwart_wasmfunction)(void *stack_frame,pwart_runtime_context context);

extern pwart_module pwart_new_module();

extern void pwart_delete_module(pwart_module m);

//this function free module but code should be executable until runtime context is free.
extern void pwart_free_module(pwart_module mod);

extern int pwart_load(pwart_module m,char *data,int len);

extern pwart_wasmfunction pwart_get_export_function(pwart_module module,char *name);

extern int pwart_set_symbol_resolver(pwart_module m,void (*resolver)(char *import_module,char *import_field,void *result));

extern pwart_runtime_context pwart_get_runtime_context(pwart_module m);
//runtime_context inspect result
struct pwart_inspect_result1{
    void *runtime_stack;
    void *memory;
    void *table_entries;
};
extern int pwart_inspect_runtime_context(pwart_runtime_context c,struct pwart_inspect_result1 *result);

extern int pwart_set_runtime_user_data(pwart_runtime_context c,void *ud);
extern void *pwart_get_runtime_user_data(pwart_runtime_context c);

extern void pwart_free_module(pwart_module mod);


#endif