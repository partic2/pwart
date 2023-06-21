#ifndef _PWART_SYSLIB_LIBUV_C
#define _PWART_SYSLIB_LIBUV_C


#include <uv.h>
#include <pwart.h>

#include "./util.c"


static void wasm__uv_version(void *fp){
    void *sp=fp;
    pwart_rstack_put_i32(&sp,uv_version());
}

static void wasm__uv_dlopen(void *fp){
    void *sp=fp;
    _wargref(filename)
    sp=fp;
    void *lib=wa_malloc(sizeof(uv_lib_t));
    int result=uv_dlopen(filename,lib);
    pwart_rstack_put_ref(&sp,lib);
    pwart_rstack_put_i32(&sp,result);
}

static void wasm__uv_dlsym(void *fp){
    void *sp=fp;
    _wargref(lib)
    _wargref(name)
    sp=fp;
    void *ptr=NULL;
    int result=uv_dlsym(lib,name,&ptr);
    pwart_rstack_put_ref(&sp,ptr);
    pwart_rstack_put_i32(&sp,result);
}

static void wasm__uv_dlclose(void *fp){
    void *sp=fp;
    _wargref(lib)
    sp=fp;
    uv_dlclose(lib);
    wa_free(lib);
}

struct libuv_wasmthread{
    uv_thread_t uvt;
    pwart_wasm_function wasmentry;
    uint64_t arg;
    void *stack;
};
static void wasm__uv_wasmthreadentry(void *awt){
    struct libuv_wasmthread *wt=awt;
    wt->stack=pwart_allocate_stack(64*1024);
    void *sp=wt->stack;
    pwart_rstack_put_i64(&sp,wt->arg);
    pwart_call_wasm_function(wt->wasmentry,wt);
    pwart_free_stack(wt->stack);
    wa_free(wt);
}
static void wasm__uv_thread_create(void *fp){
    void *sp=fp;
    _wargref(entry)
    _wargi64(arg)
    struct libuv_wasmthread *info=wa_malloc(sizeof(struct libuv_wasmthread));
    info->wasmentry=entry;
    info->arg=arg;
    uv_thread_create(&info->uvt,wasm__uv_wasmthreadentry,info);
    sp=fp;
    pwart_rstack_put_ref(&sp,info);
}


static void wasm__uv_mutex_init(void *fp){
    void *sp=fp;
    void *mut=wa_malloc(sizeof(uv_mutex_t));
    uv_mutex_init(mut);
    pwart_rstack_put_ref(&sp,mut);
}

static void wasm__uv_mutex_destroy(void *fp){
    void *sp=fp;
    _wargref(mut)
    uv_mutex_destroy(mut);
    wa_free(mut);
}

static void wasm__uv_mutex_lock(void *fp){
    void *sp=fp;
    _wargref(mut)
    uv_mutex_lock(mut);
}

static void wasm__uv_mutex_trylock(void *fp){
    void *sp=fp;
    _wargref(mut)
    int err=uv_mutex_trylock(mut);
    pwart_rstack_put_i32(&sp,err);
}

static void wasm__uv_mutex_unlock(void *fp){
    void *sp=fp;
    _wargref(mut)
    uv_mutex_unlock(mut);
}


static void wasm__uv_cond_init(void *fp){
    void *sp=fp;
    void *cond=wa_malloc(sizeof(uv_cond_t));
    uv_cond_init(cond);
    pwart_rstack_put_ref(&sp,cond);
}

static void wasm__uv_cond_destroy(void *fp){
    void *sp=fp;
    _wargref(cond)
    uv_cond_destroy(cond);
    wa_free(cond);
}

static void wasm__uv_cond_signal(void *fp){
    void *sp=fp;
    _wargref(cond)
    uv_cond_signal(cond);
}

static void wasm__uv_cond_wait(void *fp){
    void *sp=fp;
    _wargref(cond)
    _wargref(mut)
    uv_cond_wait(cond,mut);
}


static void wasm__uv_cond_timedwait(void *fp){
    void *sp=fp;
    _wargref(cond)
    _wargref(mut)
    _wargi64(timeout)
    int r=uv_cond_timedwait(cond,mut,timeout);
    pwart_rstack_put_i32(&sp,r);
}

static struct dynarr *uvsyms=NULL;  //type pwart_named_symbol





extern struct pwart_host_module *pwart_libuv_module_new(){
    if(uvsyms==NULL){
        struct pwart_named_symbol *sym;
        struct dynarr *syms=NULL;
        dynarr_init(&syms,sizeof(struct pwart_named_symbol));
        
        _ADD_BUILTIN_FN(uv_version)

        _ADD_BUILTIN_FN(uv_dlopen)
        _ADD_BUILTIN_FN(uv_dlsym)
        _ADD_BUILTIN_FN(uv_dlclose)

        _ADD_BUILTIN_FN(uv_thread_create)
        _ADD_BUILTIN_FN(uv_mutex_init)
        _ADD_BUILTIN_FN(uv_mutex_destroy)
        _ADD_BUILTIN_FN(uv_mutex_lock)
        _ADD_BUILTIN_FN(uv_mutex_unlock)
        _ADD_BUILTIN_FN(uv_mutex_trylock)
        _ADD_BUILTIN_FN(uv_cond_init)
        _ADD_BUILTIN_FN(uv_cond_destroy)
        _ADD_BUILTIN_FN(uv_cond_signal)
        _ADD_BUILTIN_FN(uv_cond_wait)

        uvsyms=syms;
    }
    struct ModuleDef *md=wa_malloc(sizeof(struct ModuleDef));
    md->syms=uvsyms;
    md->mod.resolve=(void *)&ModuleResolver;
    md->mod.on_attached=NULL;
    md->mod.on_detached=NULL;
    return (struct pwart_host_module *)md;
}

extern char *pwart_libuv_module_delete(struct pwart_host_module *mod){
    wa_free(mod);
    return NULL;
}

#endif