#ifndef _PWART_SYSLIB_H
#define _PWART_SYSLIB_H

#include <pwart.h>


extern char **pwart_syslib_get_enabled_list();
/* wasi module require extra initialize before invoking exports. see pwart_wasi_module_init */
extern char *pwart_syslib_load(pwart_namespace ns);
extern char *pwart_syslib_unload(pwart_namespace ns);

/* 
    libffi is optional.
    There is also "ffi_is_enabled" function in this module. 
    If libffi isn't enabled, all function in this module just do nothing except "ffi_is_enabled".
*/
extern int pwart_libffi_module_ffi_is_enabled();
extern char *pwart_libffi_module_delete(struct pwart_host_module *mod);
extern struct pwart_host_module *pwart_libffi_module_new();

extern struct pwart_host_module *pwart_libuv_module_new();
extern char *pwart_libuv_module_delete(struct pwart_host_module *mod);

/* module wasi_snapshot_preview1 */
extern void pwart_wasi_module_set_wasimemory(struct pwart_wasm_memory *m);
extern void pwart_wasi_module_set_wasiargs(uint32_t argc,char **argv);
/* Initialize wasi environment, the wasi 'memory' and 'args' should be set properly */
extern char *pwart_wasi_module_init();
/* Currently we only support to create and initialize ONE wasi module in one PROCESS in same time,
 If you want another one, delete the previous one.*/
extern struct pwart_host_module *pwart_wasi_module_new();
extern char *pwart_wasi_module_delete(struct pwart_host_module *mod);


#endif