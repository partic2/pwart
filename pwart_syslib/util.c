#ifndef _PWART_SYSLIB_UTILS_C
#define _PWART_SYSLIB_UTILS_C

#define _wargref(vname) void *vname=pwart_rstack_get_ref(&sp);
#define _wargi32(vname) uint32_t vname=pwart_rstack_get_i32(&sp);
#define _wargi64(vname) uint64_t vname=pwart_rstack_get_i64(&sp);

//XXX: Can we use pwart internal code?
#include "../pwart/util.c"


struct ModuleDef{
    struct pwart_host_module mod;
    struct dynarr *syms;
};

static void ModuleResolver(struct ModuleDef *_this,struct pwart_symbol_resolve_request *req){
    int i1;
    req->result=NULL;
    for(i1=0;i1<_this->syms->len;i1++){
        struct pwart_named_symbol *sym=dynarr_get(_this->syms,struct pwart_named_symbol,i1);
        if(strcmp(sym->name,req->import_field)==0){
            req->result=sym->val.fn;
            break;
        }
    }
}

#define _ADD_BUILTIN_FN(fname) sym=dynarr_push_type(&syms,struct pwart_named_symbol); \
sym->name=#fname; \
sym->kind=PWART_KIND_FUNCTION;\
sym->val.fn=pwart_wrap_host_function_c((void *)wasm__##fname);

#endif