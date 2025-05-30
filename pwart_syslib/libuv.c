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

static char *libuv_funcname_list[]={
    "uv_version","uv_dlopen","uv_dlsym","uv_dlclose","uv_thread_create",
    "uv_mutex_init","uv_mutex_destroy","uv_mutex_lock","uv_mutex_unlock","uv_mutex_trylock",
    "uv_cond_init","uv_cond_destroy","uv_cond_signal","uv_cond_wait"
};
static pwart_host_function_c libuv_funcsymbols_list[]={
    &wasm__uv_version,&wasm__uv_dlopen,&wasm__uv_dlsym,&wasm__uv_dlclose,&wasm__uv_thread_create,
    &wasm__uv_mutex_init,&wasm__uv_mutex_destroy,&wasm__uv_mutex_lock,&wasm__uv_mutex_unlock,&wasm__uv_mutex_trylock,
    &wasm__uv_cond_init,&wasm__uv_cond_destroy,&wasm__uv_cond_signal,&wasm__uv_cond_wait
};


extern struct pwart_host_module *pwart_libuv_module_new(){
    return pwart_namespace_new_host_module(libuv_funcname_list,libuv_funcsymbols_list,14);
}

extern char *pwart_libuv_module_delete(struct pwart_host_module *mod){
    wa_free(mod);
    return NULL;
}

#endif