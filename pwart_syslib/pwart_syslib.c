

#include <pwart_syslib.h>
#include <stddef.h>
#include <stdint.h>


#include "./libffi.c"
#include "./libuv.c"
#include "./libwasi.c"


static char *enabled_module[]={
    "libuv",
    "libffi",
    NULL
};

extern char **pwart_syslib_get_enabled_list(){
    return enabled_module;
}

extern char *pwart_syslib_load(pwart_namespace ns){
    struct pwart_named_module mod;
    mod.name="libuv";
    mod.type=PWART_MODULE_TYPE_HOST_MODULE;
    mod.val.host=pwart_libuv_module_new();
    pwart_namespace_define_module(ns,&mod);
    mod.name="libffi";
    mod.val.host=pwart_libffi_module_new();
    pwart_namespace_define_module(ns,&mod);
    mod.name="wasi_snapshot_preview1";
    mod.val.host=pwart_wasi_module_new();
    pwart_namespace_define_module(ns,&mod);
    return NULL;
}

extern char *pwart_syslib_unload(pwart_namespace ns){
    pwart_libuv_module_delete(pwart_namespace_find_module(ns,"libuv")->val.host);
    pwart_libffi_module_delete(pwart_namespace_find_module(ns,"libffi")->val.host);
    pwart_wasi_module_delete(pwart_namespace_find_module(ns,"wasi_snapshot_preview1")->val.host);
    return NULL;
}