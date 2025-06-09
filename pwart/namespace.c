
#ifndef _PWART_NAMESPACE_C
#define _PWART_NAMESPACE_C

#include <pwart.h>

#include "def.h"

#include "util.c"

static Namespace *namespace_GetNamespaceFormResolver(struct pwart_symbol_resolver *_this){
    return (Namespace *)_this;
}

extern struct pwart_symbol_resolver *pwart_namespace_resolver(pwart_namespace ns){
    return &((Namespace *)ns)->resolver;
}

static void namespace_SymbolResolve(struct pwart_symbol_resolver *_this,struct pwart_symbol_resolve_request *req){
    Namespace *n=namespace_GetNamespaceFormResolver(_this);
    struct pwart_named_module *m=pwart_namespace_find_module(n,req->import_module);
    req->result=NULL;
    if(m==NULL){
        return;
    }
    switch(m->type){
        case PWART_MODULE_TYPE_HOST_MODULE:
            m->val.host->resolve(m->val.host,req);
            break;
        case PWART_MODULE_TYPE_WASM_MODULE:
            switch(req->kind){
                case PWART_KIND_FUNCTION:
                req->result=pwart_get_export_function(m->val.wasm,req->import_field);
                break;
                case PWART_KIND_GLOBAL:
                req->result=pwart_get_export_global(m->val.wasm,req->import_field);
                break;
                case PWART_KIND_TABLE:
                req->result=pwart_get_export_table(m->val.wasm,req->import_field);
                break;
                case PWART_KIND_MEMORY:
                req->result=pwart_get_export_memory(m->val.wasm,req->import_field);
                break;
            }
            break;
    }
    return;
}

static const char *nsmetaSymbolNames[]={"this"};

extern pwart_namespace pwart_namespace_new(){
    Namespace *ns=(Namespace *)wa_calloc(sizeof(Namespace));
    dynarr_init(&ns->mods,sizeof(struct pwart_named_module));
    ns->resolver.resolve=&namespace_SymbolResolve;
    ns->nsmetasyms[0]=ns;
    ns->nsmeta=pwart_namespace_new_host_module2(nsmetaSymbolNames, &ns->nsmetasyms[0], 1);
    pwart_namespace_define_host_module(ns,"pwart_namespace_meta", ns->nsmeta);
    return ns;
}

extern char *pwart_namespace_delete(pwart_namespace ns){
    Namespace *n=(Namespace *)ns;
    int i=0;
    for(i=0;i<n->mods->len;i++){
        struct pwart_named_module *m=dynarr_get(n->mods,struct pwart_named_module,i);
        switch(m->type){
            case PWART_MODULE_TYPE_HOST_MODULE:
                if(m->val.host->on_detached!=NULL){
                    m->val.host->on_detached(m->val.host);
                }
                break;
            case PWART_MODULE_TYPE_WASM_MODULE:
                ReturnIfErr(pwart_free_module_state(m->val.wasm));
                break;
        }
    }
    if(n->nsmeta!=NULL){
        wa_free(n->nsmeta);
    }
    wa_free(n);
    return NULL;
}

extern struct pwart_named_module *pwart_namespace_find_module(pwart_namespace ns,const char *name){
    Namespace *n=(Namespace *)ns;
    int i=0;
    for(i=0;i<n->mods->len;i++){
        struct pwart_named_module *m=dynarr_get(n->mods,struct pwart_named_module,i);
        if(strcmp(m->name,name)==0){
            return m;
        }
    }
    return NULL;
}

extern char *pwart_namespace_remove_module(pwart_namespace ns,const char *name){
    struct pwart_named_module *m=pwart_namespace_find_module(ns,name);
    if(m!=NULL){
        switch(m->type){
            case PWART_MODULE_TYPE_HOST_MODULE:
                if(m->val.host->on_detached!=NULL){
                    m->val.host->on_detached(m->val.host);
                }
                break;
            case PWART_MODULE_TYPE_WASM_MODULE:
                ReturnIfErr(pwart_free_module_state(m->val.wasm));
                break;
        }
        m->type=PWART_MODULE_TYPE_NULL;
    }
    return NULL;
}


extern char *pwart_namespace_define_module(pwart_namespace ns,struct pwart_named_module *mod){
    Namespace *n=(Namespace *)ns;
    struct pwart_named_module *m;
    m=pwart_namespace_find_module(ns,mod->name);
    if(m!=NULL){
        pwart_namespace_remove_module(ns,m->name);
    }else{
        m=dynarr_push_type(&n->mods,struct pwart_named_module);
    }
    memmove(m,mod,sizeof(struct pwart_named_module));
    switch(m->type){
        case PWART_MODULE_TYPE_HOST_MODULE:
        m->val.host->namespace2=ns;
        if(m->val.host->on_attached!=NULL){
            m->val.host->on_attached(m->val.host);
        }
        break;
        case PWART_MODULE_TYPE_WASM_MODULE:
        pwart_set_state_symbol_resolver(m->val.wasm,&n->resolver);
        ((RuntimeContext *)m->val.wasm)->is_in_namespace=1;
        break;
    }
    return NULL;
}

extern pwart_module_state *pwart_namespace_define_wasm_module(pwart_namespace ns,const char *name,const char *wasm_bytes,int length,char **err_msg){
    Namespace *n=(Namespace *)ns;
    struct pwart_named_module mod;
    char *err=NULL;
    pwart_module_state state=NULL;
    pwart_module_compiler m=pwart_new_module_compiler();
    pwart_set_symbol_resolver(m,&n->resolver);
    err=pwart_compile(m,wasm_bytes,length);
    if(err==NULL){
        state=pwart_get_module_state(m);
    }else{
        if(err_msg!=NULL)*err_msg=err;
        return NULL;
    }
    pwart_free_module_compiler(m);
    mod.name=name;
    mod.type=PWART_MODULE_TYPE_WASM_MODULE;
    mod.val.wasm=state;
    pwart_namespace_define_module(ns,&mod);
    return state;
}

extern void pwart_namespace_define_host_module(pwart_namespace ns,const char *name,struct pwart_host_module *host_mod){
    struct pwart_named_module mod;
    mod.name=name;
    mod.type=PWART_MODULE_TYPE_HOST_MODULE;
    mod.val.host=host_mod;
    pwart_namespace_define_module(ns, &mod);
}

struct InternalHostModule{
    struct pwart_host_module base;
    const char **names;
    void **symbols;
    int length;
};

static void namespace_InternalHostModuleSymbolResolver(struct pwart_host_module *_this2,struct pwart_symbol_resolve_request *req){
    struct InternalHostModule *_this=(struct InternalHostModule *)_this2;
    int i=0;
    req->result=NULL;
    for(i=0;i<_this->length;i++){
        if(strcmp(_this->names[i],req->import_field)==0){
            req->result=_this->symbols[i];
            break;
        }
    }
}

extern struct pwart_host_module *pwart_namespace_new_host_module(const char **names,pwart_host_function_c *funcs,int length){
    struct InternalHostModule *hostmod=wa_calloc(sizeof(struct InternalHostModule));
    hostmod->length=length;
    hostmod->names=names;
    /*XXX: Once pwart_wasm_function changed, Fix here by using pwart_wrap_host_function_c. */
    hostmod->symbols=(void *)funcs;
    hostmod->base.resolve=&namespace_InternalHostModuleSymbolResolver;
    return (struct pwart_host_module *)hostmod;
}

extern struct pwart_host_module *pwart_namespace_new_host_module2(const char **names,void **symbols,int length){
    struct InternalHostModule *hostmod=wa_calloc(sizeof(struct InternalHostModule));
    hostmod->length=length;
    hostmod->names=names;
    hostmod->symbols=symbols;
    hostmod->base.resolve=&namespace_InternalHostModuleSymbolResolver;
    return (struct pwart_host_module *)hostmod;
}

extern void pwart_namespace_delete_host_module(struct pwart_host_module *hostmod){
    wa_free(hostmod);
}


#endif