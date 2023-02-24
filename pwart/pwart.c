
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
    m->cfg.import_resolver=NULL;
    m->cfg.memory_model=pwart_gcfg.memory_model;
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

extern pwart_wasm_function pwart_get_export_function(pwart_module module,char *name){
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

extern char *pwart_set_global_compile_config(struct pwart_global_compile_config *cfg){
    memcpy(&pwart_gcfg,cfg,sizeof(struct pwart_global_compile_config));
};
extern char *pwart_get_global_compile_config(struct pwart_global_compile_config *cfg){
    memcpy(cfg,&pwart_gcfg,sizeof(struct pwart_global_compile_config));
}
extern char *pwart_set_load_config(pwart_module m2,struct pwart_load_config *config){
    Module *m=m2;
    m->cfg.import_resolver=config->import_resolver;
    m->cfg.memory_model=config->memory_model;
    return NULL;
}
extern char *pwart_get_load_config(pwart_module m2,struct pwart_load_config *config){
    Module *m=m2;
    config->import_resolver=m->cfg.import_resolver;
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

extern void *pwart_allocate_stack(int size){
    uint8_t *mem=wa_malloc(size+16);
    uint8_t *stack_buffer=(uint8_t *)((((size_t)mem)+16) & (~(size_t)(0xf)));
    stack_buffer[-1]=stack_buffer-mem;
    return stack_buffer;
}

extern void pwart_free_stack(void *stack){
    uint8_t *stack_buffer=stack;
    int off=stack_buffer[-1];
    stack_buffer-=off;
    wa_free(stack_buffer);
}


extern void pwart_rstack_put_i32(void **sp,int val){
    *(int32_t *)(*sp)=val;
    *sp+=4;
}
extern void pwart_rstack_put_i64(void **sp,long long val){
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
    *(int64_t *)(*sp)=val;
    *sp+=8;
}
extern void pwart_rstack_put_f32(void **sp,float val){
    *(float *)(*sp)=val;
    *sp+=4;
}
extern void pwart_rstack_put_f64(void **sp,double val){
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
    *(double *)(*sp)=val;
    *sp+=8;
}
extern void pwart_rstack_put_ref(void **sp,void *val){
    if((pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN) && (sizeof(void *)==8)){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
    *(void **)(*sp)=val;
    *sp+=sizeof(void *);
}

extern int pwart_rstack_get_i32(void **sp){
    int32_t *sp2=*sp;
    *sp+=4;
    return *sp2;
}
extern long long pwart_rstack_get_i64(void **sp){
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
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
    if(pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
    double *sp2=*sp;
    *sp+=8;
    return *sp2;
}
extern void *pwart_rstack_get_ref(void **sp){
    if((pwart_gcfg.stack_flags&PWART_STACK_FLAGS_AUTO_ALIGN) && (sizeof(void *)==8)){
        int t=(size_t)sp&7;
        if(t>0){
            sp=(void *)sp+8-t;
        }
    }
    void **sp2=*sp;
    *sp+=sizeof(void *);
    return *sp2;
}

extern pwart_wasm_function pwart_wrap_host_function_c(pwart_host_function_c host_func){
    //currently, we don't need wrap host function.
    return host_func;
}

extern void pwart_free_wrapped_function(pwart_wasm_function wrapped){
}

extern void pwart_call_wasm_function(pwart_wasm_function fn,void *stack_pointer){
    WasmFunctionEntry fn2=fn;
    (*fn2)(stack_pointer);
}

static struct pwart_builtin_functable default_builtin_functable={NULL};
extern struct pwart_builtin_functable *pwart_get_builtin_functable(){
    if(default_builtin_functable.version==NULL){
        default_builtin_functable.version=pwart_wrap_host_function_c((void *)insn_version);
        default_builtin_functable.malloc32=pwart_wrap_host_function_c((void *)insn_malloc32);
        default_builtin_functable.malloc64=pwart_wrap_host_function_c((void *)insn_malloc64);
        default_builtin_functable.get_self_runtime_context=pwart_wrap_host_function_c((void *)insn_get_self_runtime_context);
    }
    return &default_builtin_functable;
}