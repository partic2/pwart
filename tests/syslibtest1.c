

#include <pwart.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwart_syslib.h>

#define puti32 pwart_rstack_put_i32
#define puti64 pwart_rstack_put_i64
#define putf32 pwart_rstack_put_f32
#define putf64 pwart_rstack_put_f64
#define putref pwart_rstack_put_ref

#define geti32 pwart_rstack_get_i32
#define geti64 pwart_rstack_get_i64
#define getf32 pwart_rstack_get_f32
#define getf64 pwart_rstack_get_f64
#define getref pwart_rstack_get_ref



static void test_printf64(void *fp) {
  void *sp = fp;
  printf("testaid:%lf\n", getf64(&sp));
  sp = fp;
  printf("testaid:%lld\n", geti64(&sp));
}

static void test_printi64(void *fp) {
  void *sp = fp;
  printf("testaid:%lld\n", geti64(&sp));
}

static void do_resolve(struct pwart_host_module *_this,struct pwart_symbol_resolve_request *req) {
  char *mod=req->import_module;
  char *name=req->import_field;
  void **val=&req->result;
  if (!strcmp("testaid", mod)) {
    if (!strcmp("printi64", name)) {
      *val = pwart_wrap_host_function_c(test_printi64);
    } else if (!strcmp("printf64", name)) {
      *val = pwart_wrap_host_function_c(test_printf64);
    }
  }
}

struct pwart_host_module testaid={.resolve=&do_resolve,.on_attached=NULL,.on_detached=NULL};


int syslib_test(){
  struct pwart_global_compile_config gcfg;
  void *stackbase = pwart_allocate_stack(64 * 1024);
  char *err=NULL;
  void *sp;
  FILE *f;
  int len;
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);

  pwart_get_global_compile_config(&gcfg);
  pwart_namespace *ns=pwart_namespace_new();

  pwart_syslib_load(ns);
  struct pwart_named_module nmod;
  nmod.name="testaid";
  nmod.type=PWART_MODULE_TYPE_HOST_MODULE;
  nmod.val.host=&testaid;
  pwart_namespace_define_module(ns,&nmod);

  f = fopen("test1.wasm", "rb");
  len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_namespace_define_wasm_module(ns,"test1",data,len,&err);
  if(err!=NULL){
    printf("error occur:%s\n",err);
    return 0;
  }

  f = fopen("extension1.wasm", "rb");
  len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_namespace_define_wasm_module(ns,"extension1",data,len,&err);
  if(err!=NULL){
    printf("error occur:%s\n",err);
    return 0;
  }
  struct pwart_symbol_resolve_request req;
  req.import_field="test1";
  req.import_module="extension1";
  req.kind=PWART_KIND_FUNCTION;
  req.result=NULL;
  pwart_namespace_resolver(ns)->resolve(pwart_namespace_resolver(ns),&req);
  if(req.result==NULL){
    printf("error occur:%s\n","import extension1.test1 failed");
    return 0;
  }
  pwart_wasm_function test1=(pwart_wasm_function)req.result;
  pwart_call_wasm_function(test1,stackbase);
  sp=stackbase;
  uint64_t *pmem=(uint64_t *)(size_t)geti64(&sp);
  printf("allocated memory at %p, write value %llu, expect %llu\n",pmem,*pmem,123456ll);
  return 1;
}

int main(int argc, char *argv[]) {
  
  if (namespace_test()) {
    printf("namespace_test pass\n");
  } else {
    printf("namespace_test failed\n");
    return 1;
  }
  printf("all test pass.\n");
  return 0;
}