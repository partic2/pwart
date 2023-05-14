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

static ffi_type *ffix_CharToType(int typeid){
    switch(typeid){
        case 'i':
        return &ffi_type_uint32;
        case 'l':
        return &ffi_type_uint64;
        case 'f':
        return &ffi_type_float;
        case 'd':
        return &ffi_type_double;
        case 'p':
        return &ffi_type_pointer;
        case 'v':
        return &ffi_type_void;
        default:
        return &ffi_type_pointer;
    }
}

static void wasm__ffix_new_cif(void *fp){
    void *sp=fp;
    _wargref(fndef);
    char *typeid=fndef;
    int i=0,alen=0;
    ffi_status ffistat;
    ffi_type *rtype;
    sp=fp;

    for(i=0;i<256;i++){
        if(typeid[i]==0 || typeid[i]=='>'){
            alen=i;
            break;
        }
    }
    if(i==256&&alen==0){
        pwart_rstack_put_ref(&sp,NULL);
        pwart_rstack_put_i32(&sp,1);
        return;
    }
    ffi_cif *cif=wa_malloc(sizeof(ffi_cif)+sizeof(ffi_type *)*alen);
    ffi_type **typearr=(ffi_type *)((char *)cif+sizeof(ffi_cif));
    for(i=0;i<alen;i++){
        typearr[i]=ffix_CharToType(typeid[i]);
    }
    if(typeid[alen]=='>'){
        rtype=ffix_CharToType(typeid[alen+1]);
    }else{
        rtype=&ffi_type_void;
    }
    ffistat=ffi_prep_cif(cif,FFI_DEFAULT_ABI,alen,rtype,typearr);
    pwart_rstack_put_ref(&sp,cif);
    pwart_rstack_put_i32(&sp,ffistat);
}

static void wasm__ffix_del_cif(void *fp){
    void *sp=fp;
    _wargref(cif);
    wa_free(cif);
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

static void wasm__ffix_call(void *fp){
    void *sp=fp;
    ffi_cif *cif=pwart_rstack_get_ref(&sp);
    _wargref(fn)
    void *rvalue=pwart_rstack_get_ref(&sp);
    int64_t *i64arg=pwart_rstack_get_ref(&sp);
    void **avalue=wa_malloc(sizeof(void *)*cif->nargs);
    int i1;
    for(i1=0;i1<cif->nargs;i1++){
        avalue[i1]=i64arg+i1;
    }
    ffi_call(cif,fn,rvalue,avalue);
    wa_free(avalue);
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
        _ADD_BUILTIN_FN(ffix_new_cif)
        _ADD_BUILTIN_FN(ffix_del_cif)
        _ADD_BUILTIN_FN(ffix_call)
        ffisyms=syms;
    }
    struct ModuleDef *md=wa_malloc(sizeof(struct ModuleDef));
    md->syms=ffisyms;
    md->mod.resolve=(void *)&ModuleResolver;
    md->mod.on_attached=NULL;
    md->mod.on_detached=NULL;
    return md;
}

extern char *pwart_libffi_module_delete(struct pwart_host_module *mod){
    wa_free(mod);
    return NULL;
}


#endif
