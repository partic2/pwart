#ifndef _PWART_SYSLIB_LIBSIMPLEFFI_C
#define _PWART_SYSLIB_LIBSIMPLEFFI_C


#include <simpleffi.h>
#include <pwart.h>
#include "./util.c"

static void *wasm__simpleffi_cb_handler_slot[32]={NULL};

static void wasm__simpleffi_call(void *fp){
    void *sp=fp;
    _wargref(fn)
    _wargi32(argc)
    _wargref(argv)
    sp=fp;
    pwart_rstack_put_ref(&sp, simpleffi_call(fn, argc, (void **)argv));
}

static void wasm__simpleffi_get_callback(void *fp){
    void *sp=fp;
    _wargi32(index)
    sp=fp;
    pwart_rstack_put_ref(&sp, simpleffi_get_callback(index));
}

static void *wasm__simpleffi_wasm_adapter_cb(void *arg0,void *arg1,void *arg2,void *arg3,int cb_index){
    if(wasm__simpleffi_cb_handler_slot[cb_index]!=NULL){

    }
    void *stack=pwart_allocate_stack(4*1024);
    void *sp=stack;
    pwart_rstack_put_ref(&sp,arg0);
    pwart_rstack_put_ref(&sp,arg1);
    pwart_rstack_put_ref(&sp,arg2);
    pwart_rstack_put_ref(&sp,arg3);
    pwart_rstack_put_i32(&sp,cb_index);
    pwart_call_wasm_function(wasm__simpleffi_cb_handler_slot[cb_index],stack);
    void *ret=*(void **)stack;
    pwart_free_stack(stack);
    return ret;
}

static void wasm__simpleffi_set_callback_handler(void *fp){
    void *sp=fp;
    _wargi32(index);
    _wargref(cb);
    sp=fp;
    wasm__simpleffi_cb_handler_slot[index]=cb;
    if(cb==NULL){
        simpleffi_set_callback_handler(index,NULL);
    }else{
        simpleffi_set_callback_handler(index,wasm__simpleffi_wasm_adapter_cb);
    }
}

static void wasm__simpleffi_get_callback_handler(void *fp){
    void *sp=fp;
    _wargi32(index);
    sp=fp;
    if(wasm__simpleffi_cb_handler_slot[index]==NULL){
        pwart_rstack_put_ref(&sp,(void *)simpleffi_get_callback_handler(index));
    }else{
        pwart_rstack_put_ref(&sp, wasm__simpleffi_cb_handler_slot[index]);
    }
}

static void wasm__simpleffi_call_win32std(void *fp){
    void *sp=fp;
    _wargref(fn)
    _wargi32(argc)
    _wargref(argv)
    sp=fp;
    pwart_rstack_put_ref(&sp, simpleffi_call_win32std(fn, argc, argv));
}

static const char *simpleffi_funcname_list[]={
    "simpleffi_call","simpleffi_get_callback",
    "simpleffi_set_callback_handler","simpleffi_get_callback_handler",
    "simpleffi_call_win32std"
};
static pwart_host_function_c simpleffi_funcsymbols_list[]={
    &wasm__simpleffi_call,&wasm__simpleffi_get_callback,
    &wasm__simpleffi_set_callback_handler,&wasm__simpleffi_get_callback_handler,
    &wasm__simpleffi_call_win32std
};


extern struct pwart_host_module *pwart_simpleffi_module_new(){
    return pwart_namespace_new_host_module(simpleffi_funcname_list,simpleffi_funcsymbols_list,5);
}

extern char *pwart_simpleffi_module_delete(struct pwart_host_module *mod){
    wa_free(mod);
    return NULL;
}

#endif