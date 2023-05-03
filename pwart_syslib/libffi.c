#ifndef _PWART_SYSLIB_LIBFFI_C
#define _PWART_SYSLIB_LIBFFI_C

#include <ffi.h>
#include <pwart.h>

#include "./util.c"



static void wasm__ffi_prep_cif(void *fp){
    void *sp=fp;
    _wargref(cif)
    _wargi32(abi)
    _wargi32(narg)
    _wargref(rtype)
    _wargref(atypes)
    sp=fp;
    uint32_t ffi_status=ffi_prep_cif(cif,abi,narg,rtype,atypes);
    pwart_rstack_put_i32(&sp,ffi_status);
}

static void wasm__ffi_prep_cif_var(void *fp){
    void *sp=fp;
    _wargref(cif)
    _wargi32(abi)
    _wargi32(nfixedargs)
    _wargi32(narg)
    _wargref(rtype)
    _wargref(atypes)
    sp=fp;
    uint32_t ffi_status=ffi_prep_cif_var(cif,abi,nfixedargs,narg,rtype,atypes);
    pwart_rstack_put_i32(&sp,ffi_status);
}

static void wasm__ffi_call(void *fp){
    void *sp=fp;
    _wargref(cif)
    _wargref(fn)
    _wargref(rvalue)
    _wargref(avalue)
    sp=fp;
    ffi_call(cif,fn,rvalue,avalue);
}

static void wasm__ffi_default_abi(void *fp){
    void *sp=fp;
    pwart_rstack_put_i32(&sp,FFI_DEFAULT_ABI);
}

static void wasm__ffi_built_in_types(void *fp){
    void *sp=fp;
    pwart_rstack_put_ref(&sp,&ffi_type_void);
    pwart_rstack_put_ref(&sp,&ffi_type_uint32);
    pwart_rstack_put_ref(&sp,&ffi_type_sint32);
    pwart_rstack_put_ref(&sp,&ffi_type_uint64);
    pwart_rstack_put_ref(&sp,&ffi_type_sint64);
    pwart_rstack_put_ref(&sp,&ffi_type_float);
    pwart_rstack_put_ref(&sp,&ffi_type_double);
    pwart_rstack_put_ref(&sp,&ffi_type_pointer);
}

static struct dynarr *ffisyms=NULL;  //type pwart_named_symbol


extern struct pwart_host_module *pwart_libffi_module_new(){
    if(ffisyms==NULL){
        struct pwart_named_symbol *sym;
        dynarr_init(&ffisyms,sizeof(struct pwart_named_symbol));
        struct dynarr *syms=ffisyms;
        _ADD_BUILTIN_FN(ffi_prep_cif)
        _ADD_BUILTIN_FN(ffi_prep_cif_var)
        _ADD_BUILTIN_FN(ffi_call)
        _ADD_BUILTIN_FN(ffi_default_abi)
        _ADD_BUILTIN_FN(ffi_built_in_types)
    }
    struct ModuleDef *md=wa_malloc(sizeof(struct ModuleDef));
    md->syms=ffisyms;
    md->mod.resolve=(void *)&ModuleResolver;
    md->mod.on_attached=NULL;
    md->mod.on_detached=NULL;
}

extern char *pwart_libffi_module_delete(struct pwart_host_module *mod){
    wa_free(mod);
    return NULL;
}


#endif
