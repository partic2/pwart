
#include <stdio.h>

#include "def.h"
#include "util.c"
#include "extfunc.c"
#include "wagen.c"
#include "modparser.c"

#include <pwart.h>

extern int pwart_get_version(){
    return PWART_VERSION_1;
}

extern pwart_module pwart_new_module(){
    Module *m=wa_calloc(sizeof(Module));
    m->cfg.stack_size=63*1024;
    m->cfg.import_resolver=NULL;
    return m;
}

extern char *pwart_delete_module(pwart_module mod){
    Module *m=mod;
    RuntimeContext *rc=m->context;
    free_module(m);
    if(rc!=NULL){
        free_runtimectx(rc);
    }
    return NULL;
}

extern char *pwart_free_module(pwart_module mod){
    Module *m=mod;
    free_module(m);
    return NULL;
}

extern char *pwart_free_runtime(pwart_runtime_context rc2){
    RuntimeContext *rc=rc2;
    free_runtimectx(rc);
    return NULL;
}


extern char *pwart_load(pwart_module m,char *data,int len){
    return load_module(m,data,len);

}

extern pwart_wasmfunction pwart_get_export_function(pwart_module module,char *name){
    Module *m=module;
    Export *exp=get_export(m,name,KIND_FUNCTION);
    if(exp!=NULL){
        return ((WasmFunction *)exp->value)->func_ptr;
    }else{
        return NULL;
    }
    
}

extern char *pwart_set_symbol_resolver(pwart_module m2,void (*resolver)(char *import_module,char *import_field,void *result)){
    Module *m=m2;
    m->cfg.import_resolver=resolver;
    return NULL;
}
extern char *pwart_set_stack_size(pwart_module m2,int stack_size){
    Module *m=m2;
    m->cfg.stack_size=stack_size;
    return NULL;
}

extern char *pwart_set_load_config(pwart_module m2,struct pwart_load_config *config){
    Module *m=m2;
    m->cfg.import_resolver=config->import_resolver;
    m->cfg.stack_size=config->stack_size;
    m->cfg.memory_model=config->memory_model;
    return NULL;
}
extern char *pwart_get_load_config(pwart_module m2,struct pwart_load_config *config){
    Module *m=m2;
    config->import_resolver=m->cfg.import_resolver;
    config->stack_size=m->cfg.stack_size;
    config->memory_model=m->cfg.memory_model;
    return NULL;
}

extern pwart_runtime_context pwart_get_runtime_context(pwart_module mod){
    Module *m=mod;
    return m->context;
}

extern char *pwart_inspect_runtime_context(pwart_runtime_context c,struct pwart_inspect_result1 *result){
    RuntimeContext *rc=c;
    result->memory_size=rc->memory.pages*PAGE_SIZE;
    result->memory=rc->memory.bytes;
    result->runtime_stack=rc->stack_buffer+rc->stack_start_offset;
    result->table_entries_count=rc->table.size;
    result->table_entries=rc->table.entries;
    result->globals_buffer_size=rc->globals->len;
    result->globals_buffer=&rc->globals->data;
    return NULL;
}


extern void pwart_set_runtime_user_data(pwart_runtime_context c,void *ud){
    RuntimeContext *rc=c;
    rc->userdata=ud;
}

extern void *pwart_get_runtime_user_data(pwart_runtime_context c){
    RuntimeContext *rc=c;
    return rc->userdata;
}

extern void pwart_rstack_put_i32(void **sp,int val){
    *(int32_t *)(*sp)=val;
    *sp+=4;
}
extern void pwart_rstack_put_i64(void **sp,long long val){
    *(int64_t *)(*sp)=val;
    *sp+=8;
}
extern void pwart_rstack_put_f32(void **sp,float val){
    *(float *)(*sp)=val;
    *sp+=4;
}
extern void pwart_rstack_put_f64(void **sp,double val){
    *(double *)(*sp)=val;
    *sp+=8;
}

extern int pwart_rstack_get_i32(void **sp){
    int32_t *sp2=*sp;
    *sp+=4;
    return *sp2;
}
extern long long pwart_rstack_get_i64(void **sp){
    int64_t *sp2=*sp;
    *sp+=8;
    return *sp2;
}
extern float pwart_rstack_get_f32(void **sp){
    float *sp2=*sp;
    *sp+=4;
    return *sp2;
}
extern double pwart_rstack_get_f64(void **sp){
    double *sp2=*sp;
    *sp+=8;
    return *sp2;
}

static struct pwart_builtin_functable default_builtin_functable={
    .version=(void *)&insn_version,
    .malloc32=(void *)&insn_malloc32,
    .malloc64=(void *)&insn_malloc64,
    .ref_from_i32=(void *)&insn_reffromi32,
    .ref_from_i64=(void *)&insn_reffromi64
};
extern struct pwart_builtin_functable *pwart_get_builtin_functable(){
    return &default_builtin_functable;
}