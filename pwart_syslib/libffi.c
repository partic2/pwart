#ifndef _PWART_SYSLIB_LIBFFI_C
#define _PWART_SYSLIB_LIBFFI_C

#include <ffi.h>
#include <pwart.h>

#include "./util.c"


#ifdef PWART_SYSLIB_LIBFFI_ENABLED

/* Check if available before call any other ffi function. */
static void wasm__ffi_is_enabled(void *fp){
    void *sp=fp;
    pwart_rstack_put_i32(&sp,1);
}

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
    ffi_type **typearr=(ffi_type **)((char *)cif+sizeof(ffi_cif));
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


static void wasm__ffi_closure_alloc(void *fp){
    void *sp=fp;
    void *code=NULL;
    sp=fp;
    void *closure=ffi_closure_alloc(sizeof(FFI_CLOSURES),&code);
    pwart_rstack_put_ref(&sp,closure);
    pwart_rstack_put_ref(&sp,code);
}
static void wasm__ffi_closure_free(void *fp){
    void *sp=fp;
    _wargref(closure)
    sp=fp;
    ffi_closure_free(closure);
}
static void wasmcb__ffi_closurecb(ffi_cif* cif,void *ret, void* args[],void **userdata2){
    void *fn2=userdata2[0];
    void *user_data=userdata2[1];
    wa_free(userdata2);
    void *stackbase=pwart_allocate_stack(4096);
    void *sp=stackbase;
    pwart_rstack_put_ref(&sp,cif);
    pwart_rstack_put_ref(&sp,ret);
    pwart_rstack_put_ref(&sp,args);
    pwart_rstack_put_ref(&sp,user_data);
    pwart_call_wasm_function(fn2,stackbase);
    pwart_free_stack(stackbase);
}
static void wasm__ffi_prep_closure_loc(void *fp){
    void *sp=fp;
    _wargref(closure)
    _wargref(cif)
    _wargref(fn)
    _wargref(user_data)
    _wargref(codeloc);
    sp=fp;
    void **userdata2=wa_malloc(sizeof(void *)*2);
    userdata2[0]=fn;
    userdata2[1]=user_data;
    ffi_status stat=ffi_prep_closure_loc(closure,cif,(void *)&wasmcb__ffi_closurecb,userdata2,codeloc);
    pwart_rstack_put_i32(&sp,stat);
}

static void wasmcb__c2wasm_adapter(ffi_cif* cif,void *ret, void* args[],void **userdata2){
    void *wasmfn=userdata2[0];
    void *user_data=userdata2[1];
    wa_free(userdata2);
    void *stackbase=pwart_allocate_stack(4096);
    void *sp=stackbase;
    for(int i=0;i<cif->nargs;i++){
        int size=cif->arg_types[i]->size;
        memcpy(sp,args[i],size);
        sp=sp+8;
    }
    void *fp=sp;
    pwart_rstack_put_ref(&sp,cif);
    pwart_rstack_put_ref(&sp,ret);
    pwart_rstack_put_ref(&sp,stackbase);
    pwart_rstack_put_ref(&sp,user_data);
    pwart_call_wasm_function(wasmfn,fp);
    pwart_free_stack(stackbase);
}
static void wasm__ffix_new_c_callback(void *fp){
    void *sp=fp;
    _wargref(cif)
    _wargref(fn)
    _wargref(user_data)
    sp=fp;
    void *code=NULL;
    void **userdata2=wa_malloc(sizeof(void *)*2);
    userdata2[0]=fn;
    userdata2[1]=user_data;
    void *closure=ffi_closure_alloc(sizeof(FFI_CLOSURES),&code);
    ffi_status stat=ffi_prep_closure_loc(closure,cif,(void *)&wasmcb__c2wasm_adapter,userdata2,code);
    pwart_rstack_put_ref(&sp,closure); 
    pwart_rstack_put_ref(&sp,code); 
    pwart_rstack_put_i32(&sp,stat);
}
static void wasm__ffix_del_c_callback(void *fp){
    void *sp=fp;
    _wargref(closure)
    sp=fp;
    ffi_closure_free(closure);
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

static void *libffi_funcsyms_list[]={wasm__ffi_is_enabled,
    &wasm__ffi_prep_cif,&wasm__ffi_prep_cif_var,&wasm__ffi_call,&wasm__ffi_default_abi,&wasm__ffi_built_in_types,
    &wasm__ffix_new_cif,&wasm__ffix_del_cif,&wasm__ffix_call,&wasm__ffi_closure_alloc,&wasm__ffi_closure_free,
    &wasm__ffi_prep_closure_loc,&wasm__ffix_new_c_callback,&wasm__ffix_del_c_callback};

#else

static void wasm__ffi_is_enabled(void *fp){
    void *sp=fp;
    pwart_rstack_put_i32(&sp,0);
}

static void wasm__ffi_noop_stub(void *fp){
}

static void *libffi_funcsyms_list[]={wasm__ffi_is_enabled,
    &wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,
    &wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub,
    &wasm__ffi_noop_stub,&wasm__ffi_noop_stub,&wasm__ffi_noop_stub};

#endif



static struct dynarr *ffisyms=NULL;  //type pwart_named_symbol

static char *libffi_funcname_list[]={"ffi_is_enabled",
    "ffi_prep_cif","ffi_prep_cif_var","ffi_call","ffi_default_abi","ffi_built_in_types",
    "ffix_new_cif","ffix_del_cif","ffix_call","ffi_closure_alloc","ffi_closure_free",
    "ffi_prep_closure_loc","ffix_new_c_callback","ffix_del_c_callback"};

extern int pwart_libffi_module_ffi_is_enabled(){
    int is_enabled;
    wasm__ffi_is_enabled(&is_enabled);
    return is_enabled;
}

extern struct pwart_host_module *pwart_libffi_module_new(){
    return pwart_namespace_new_host_module(libffi_funcname_list,libffi_funcsyms_list,14);
}

extern char *pwart_libffi_module_delete(struct pwart_host_module *mod){
    pwart_namespace_delete_host_module(mod);
    return NULL;
}



#endif
