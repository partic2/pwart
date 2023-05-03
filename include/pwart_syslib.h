#ifndef _PWART_SYSLIB_H
#define _PWART_SYSLIB_H

#include <pwart.h>


extern char **pwart_syslib_get_enabled_list();
extern char *pwart_syslib_load(pwart_namespace ns);
extern char *pwart_syslib_unload(pwart_namespace ns);

extern char *pwart_libffi_module_delete(struct pwart_host_module *mod);
extern struct pwart_host_module *pwart_libffi_module_new();

extern struct pwart_host_module *pwart_libuv_module_new();
extern char *pwart_libuv_module_delete(struct pwart_host_module *mod);



#endif