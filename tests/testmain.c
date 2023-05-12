
#include <pwart.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

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


static void test_wasmfn(void *fp) {
  void *sp = fp;
  uint64_t r = geti64(&sp) + geti64(&sp) + 2;
  sp = fp;
  puti64(&sp, r);
}

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

static void do_resolve(struct pwart_symbol_resolver *_this,struct pwart_symbol_resolve_request *req) {
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
struct pwart_symbol_resolver symbol_resolver={.resolve=do_resolve};

// base test
int test1() {
  struct pwart_global_compile_config gcfg;
  void *stackbase = pwart_allocate_stack(64 * 1024);

  FILE *f = fopen("test1.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);

  pwart_get_global_compile_config(&gcfg);
  for (int i1 = 0; i1 < 2; i1++) {
    pwart_module_compiler m = pwart_new_module_compiler();
    pwart_set_symbol_resolver(m, &symbol_resolver);
    // different config
    switch (i1) {
    case 0:
      printf("test config:default\n");
      break;
    case 1:
      printf("test config:stack_flags=PWART_STACK_FLAGS_AUTO_ALIGN\n");
      gcfg.stack_flags|=PWART_STACK_FLAGS_AUTO_ALIGN;
      pwart_set_global_compile_config(&gcfg);
      break;
    }
    struct pwart_inspect_result1 ctxinfo;
    char *loadresult = pwart_compile(m, data, len);
    if (loadresult != NULL) {
      printf("error:%s\n", loadresult);
      return 0;
    }
    pwart_module_state ctx = pwart_get_module_state(m);
    pwart_inspect_module_state(ctx, &ctxinfo);

    printf("runtime stack at %p\n", stackbase);
    printf("context at %p\n", ctx);

    pwart_wasm_function addTwo = pwart_get_export_function(ctx, "addTwo");
    pwart_wasm_function test1 = pwart_get_export_function(ctx, "test1");
    pwart_wasm_function test2 = pwart_get_export_function(ctx, "test2");
    pwart_wasm_function test3 = pwart_get_export_function(ctx, "test3");
    pwart_wasm_function fib_main = pwart_get_export_function(ctx, "fib_main");
    pwart_wasm_function builtinFuncTest =
        pwart_get_export_function(ctx, "builtinFuncTest");

    pwart_free_module_compiler(m);

    void *sp;

    int32_t ri32;
    int32_t ri32_2;
    int32_t ri32_3;
    uint64_t ri64;
    void *rref;

    sp = stackbase;
    puti64(&sp, 123);
    puti64(&sp, 654);

    sp = stackbase;
    pwart_call_wasm_function(addTwo, sp);
    ri64 = geti64(&sp);
    printf("1.expect %"PRIu64", got %"PRIu64"\n", (uint64_t)777ll, ri64);

    if ((uint64_t)777ll != ri64) {
      return 0;
    }

    sp = stackbase;
    puti32(&sp, 22);
    puti32(&sp, 33);

    sp = stackbase;
    pwart_call_wasm_function(test1, sp);
    ri32 = geti32(&sp);
    printf("2.expect %u, got %u\n", (22 + 33) * 2, ri32);

    if ((22 + 33) * 2 != ri32) {
      return 0;
    }

    sp = stackbase;
    puti32(&sp, 100);
    puti32(&sp, 11);
    sp = stackbase;
    pwart_call_wasm_function(test1, sp);
    ri32 = geti32(&sp);
    printf("3.expect %u, got %u\n", 100 + 11, ri32);
    if (100 + 11 != ri32) {
      return 0;
    }

    char *memstr = (uint8_t *)ctxinfo.memory;
    *(memstr + 50) = 0;
    printf("4.expect HelloHello!!!!, got %s\n", memstr + 1);
    if (strcmp(memstr + 1, "HelloHello!!!!") != 0) {
      return 0;
    }
    printf("5.expect %p, got %p , %p\n", ctxinfo.table_entries[0],
           ctxinfo.table_entries[1], ctxinfo.table_entries[2]);
    if (ctxinfo.table_entries[0] != ctxinfo.table_entries[1]) {
      return 0;
    }
    if (ctxinfo.table_entries[0] != ctxinfo.table_entries[2]) {
      return 0;
    }

    sp = stackbase;
    puti64(&sp, 22);
    puti64(&sp, 33);
    sp = stackbase;
    pwart_call_wasm_function(test2, sp);
    ri64 = geti64(&sp);
    printf("6.expect %u, got %"PRIu64"\n", (22 + 33 + 140), ri64);
    if (22 + 33 + 140 != ri64) {
      return 0;
    }

    ctxinfo.table_entries[1] = &test_wasmfn;
    sp = stackbase;
    puti64(&sp, 22);
    puti64(&sp, 33);
    sp = stackbase;
    pwart_call_wasm_function(test2, sp);
    ri64 = geti64(&sp);
    printf("7.expect %u, got %"PRIu64"\n", 22 + 33 + 140 + 2, ri64);
    if (22 + 33 + 140 + 2 != ri64) {
      return 0;
    }

    sp = stackbase;
    // align and float add test
    puti32(&sp, 3844);
    putf64(&sp, 3.25);
    putf64(&sp, 4.75);
    sp = stackbase;
    pwart_call_wasm_function(test3, sp);
    ri32 = geti32(&sp);
    ri64 = geti64(&sp);
    printf("8.expect %d,%"PRIu64", got %d,%"PRIu64"\n", 3844, (int64_t)(3.25 + 4.75), ri32,
           ri64);
    if ((int64_t)(3.25 + 4.75) != ri64) {
      return 0;
    }
    if (ri32 != 3844) {
      return 0;
    }

    // fibnacci sequence
    sp = stackbase;
    puti32(&sp, 2);
    puti32(&sp, 100);
    *(int *)((char *)ctxinfo.memory + 100) = 0;
    *(int *)((char *)ctxinfo.memory + 104) = 116;
    memcpy((char *)ctxinfo.memory + 116, "30", 3);
    sp = stackbase;
    pwart_call_wasm_function(fib_main, sp);
    ri32 = geti32(&sp);
    printf("9.fib test, expect %d, got %d\n", 832040, ri32);
    if (832040 != ri32) {
      return 0;
    }

    // builtinFuncTest test
    sp = stackbase;
    sp = stackbase;
    pwart_call_wasm_function(builtinFuncTest, sp);
    ri32 = geti32(&sp);
    ri32_2=geti32(&sp);
    rref = getref(&sp);
    ri32_3=geti32(&sp);
    printf("10.builtinFuncTest test, expect %x,%x,%p,%x, got %x,%x,%p,%x\n",
           pwart_get_version(),(uint32_t)sizeof(void *) ,ctx,(uint32_t)strlen(memstr+1), ri32, ri32_2,rref,ri32_3);
    if (pwart_get_version() != ri32 || sizeof(void *)!=ri32_2 || ctx != rref || strlen(memstr+1)!=ri32_3) {
      return 0;
    }

    pwart_free_module_state(ctx);
  }
  free(data);
  pwart_free_stack(stackbase);
  return 1;
}

int unary_test() {
  FILE *f = fopen("unary.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  void *stackbase = pwart_allocate_stack(64 * 1024);
  fclose(f);
  struct pwart_inspect_result1 ctxinfo;
  char *err=NULL;
  pwart_module_state ctx=pwart_load_module(data,len,&err);
  if(err!=NULL){printf("load module failed:%s\n",err);return 0;};
  pwart_inspect_module_state(ctx, &ctxinfo);

  printf("runtime stack at %p\n", stackbase);

  pwart_wasm_function fn;
  void *sp;

  fn = pwart_get_export_function(ctx, "i32_eqz_100");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_eqz_100 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_eqz_0");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_eqz_0 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_clz");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_clz check...");
  if (*(uint32_t *)sp != 24) {
    printf("failed, expect %u, got %u\n", 24, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_ctz");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ctz check...");
  if (*(uint32_t *)sp != 7) {
    printf("failed, expect %u, got %u\n", 7, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_popcnt");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_popcnt check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_eqz_100");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_eqz_100 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_eqz_0");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_eqz_0 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_clz");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_clz check...");
  if (*(uint64_t *)sp != 56) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)56ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_ctz");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ctz check...");
  if (*(uint64_t *)sp != 7) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)7ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_popcnt");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_popcnt check...");
  if (*(uint64_t *)sp != 1) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)1ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_neg");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_neg check...");
  if (*(float *)sp != -100.000000) {
    printf("failed, expect %f, got %f\n", -100.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_abs");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_abs check...");
  if (*(float *)sp != 100.000000) {
    printf("failed, expect %f, got %f\n", 100.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_sqrt_neg_is_nan");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_sqrt_neg_is_nan check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_sqrt_100");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_sqrt_100 check...");
  if (*(float *)sp != 10.000000) {
    printf("failed, expect %f, got %f\n", 10.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_ceil");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ceil check...");
  if (*(float *)sp != -0.000000) {
    printf("failed, expect %f, got %f\n", -0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_floor");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_floor check...");
  if (*(float *)sp != -1.000000) {
    printf("failed, expect %f, got %f\n", -1.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_trunc");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_trunc check...");
  if (*(float *)sp != -0.000000) {
    printf("failed, expect %f, got %f\n", -0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_nearest_lo");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_nearest_lo check...");
  if (*(float *)sp != 1.000000) {
    printf("failed, expect %f, got %f\n", 1.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_nearest_hi");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_nearest_hi check...");
  if (*(float *)sp != 2.000000) {
    printf("failed, expect %f, got %f\n", 2.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_neg");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_neg check...");
  if (*(double *)sp != -100.000000) {
    printf("failed, expect %lf, got %lf\n", -100.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_abs");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_abs check...");
  if (*(double *)sp != 100.000000) {
    printf("failed, expect %lf, got %lf\n", 100.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_sqrt_neg_is_nan");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_sqrt_neg_is_nan check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_sqrt_100");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_sqrt_100 check...");
  if (*(double *)sp != 10.000000) {
    printf("failed, expect %lf, got %lf\n", 10.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_ceil");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ceil check...");
  if (*(double *)sp != -0.000000) {
    printf("failed, expect %lf, got %lf\n", -0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_floor");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_floor check...");
  if (*(double *)sp != -1.000000) {
    printf("failed, expect %lf, got %lf\n", -1.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_trunc");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_trunc check...");
  if (*(double *)sp != -0.000000) {
    printf("failed, expect %lf, got %lf\n", -0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_nearest_lo");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_nearest_lo check...");
  if (*(double *)sp != 1.000000) {
    printf("failed, expect %lf, got %lf\n", 1.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_nearest_hi");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_nearest_hi check...");
  if (*(double *)sp != 2.000000) {
    printf("failed, expect %lf, got %lf\n", 2.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");
  pwart_free_module_state(ctx);
  pwart_free_stack(stackbase);
  return 1;
}

int binary_test() {
  FILE *f = fopen("binary.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  void *stackbase = pwart_allocate_stack(64 * 1024);
  fclose(f);
  struct pwart_inspect_result1 ctxinfo;
  char *err=NULL;
  pwart_module_state ctx=pwart_load_module(data,len,&err);
  pwart_inspect_module_state(ctx, &ctxinfo);

  printf("runtime stack at %p\n", stackbase);

  pwart_wasm_function fn;
  void *sp;
  fn = pwart_get_export_function(ctx, "i32_add");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_add check...");
  if (*(uint32_t *)sp != 3) {
    printf("failed, expect %u, got %u\n", 3, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_sub");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_sub check...");
  if (*(uint32_t *)sp != 16) {
    printf("failed, expect %u, got %u\n", 16, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_mul");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_mul check...");
  if (*(uint32_t *)sp != 21) {
    printf("failed, expect %u, got %u\n", 21, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_div_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_div_s check...");
  if (*(uint32_t *)sp != 4294967294) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967294,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_div_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_div_u check...");
  if (*(uint32_t *)sp != 2147483646) {
    printf("failed, expect %u, got %u\n", 2147483646, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_rem_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_rem_s check...");
  if (*(uint32_t *)sp != 4294967295) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967295,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_rem_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_rem_u check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_and");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_and check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_or");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_or check...");
  if (*(uint32_t *)sp != 15) {
    printf("failed, expect %u, got %u\n", 15, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_xor");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_xor check...");
  if (*(uint32_t *)sp != 14) {
    printf("failed, expect %u, got %u\n", 14, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_shl");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_shl check...");
  if (*(uint32_t *)sp != 4294966496) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294966496,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_shr_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_shr_u check...");
  if (*(uint32_t *)sp != 536870899) {
    printf("failed, expect %u, got %u\n", (uint32_t)536870899, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_shr_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_shr_s check...");
  if (*(uint32_t *)sp != 4294967283) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967283,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_rotl");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_rotl check...");
  if (*(uint32_t *)sp != 4294966503) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294966503,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_rotr");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_rotr check...");
  if (*(uint32_t *)sp != 2684354547) {
    printf("failed, expect %u, got %u\n", (uint32_t)2684354547,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_add");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_add check...");
  if (*(uint64_t *)sp != 3L) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)3ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_sub");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_sub check...");
  if (*(uint64_t *)sp != (uint64_t)16ll) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)16ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_mul");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_mul check...");
  if (*(uint64_t *)sp != (uint64_t)21ll) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)21ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_div_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_div_s check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709551614ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709551614ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_div_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_div_u check...");
  if (*(uint64_t *)sp != (uint64_t)9223372036854775806ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)9223372036854775806ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_rem_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_rem_s check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709551615ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709551615ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_rem_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_rem_u check...");
  if (*(uint64_t *)sp != 1L) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)1ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_and");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_and check...");
  if (*(uint64_t *)sp != 1L) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)1ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_or");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_or check...");
  if (*(uint64_t *)sp != 15L) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)15ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_xor");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_xor check...");
  if (*(uint64_t *)sp != 14L) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)14ll, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_shl");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_shl check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709550816ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709550816ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_shr_u");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_shr_u check...");
  if (*(uint64_t *)sp != (uint64_t)2305843009213693939ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)2305843009213693939ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_shr_s");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_shr_s check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709551603ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709551603ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_rotl");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_rotl check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709550823ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709550823ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_rotr");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_rotr check...");
  if (*(uint64_t *)sp != (uint64_t)11529215046068469747ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)11529215046068469747ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_add");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_add check...");
  if (*(float *)sp != 5.000000) {
    printf("failed, expect %f, got %f\n", 5.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_sub");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_sub check...");
  if (*(float *)sp != -9995.500000) {
    printf("failed, expect %f, got %f\n", -9995.500000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_mul");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_mul check...");
  if (*(float *)sp != -8487.187500) {
    printf("failed, expect %f, got %f\n", -8487.187500, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_div");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_div check...");
  if (*(float *)sp != -500000000.000000) {
    printf("failed, expect %f, got %f\n", -500000000.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_min");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_min check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_max");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_max check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_copysign");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_copysign check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_add");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_add check...");
  if (*(double *)sp != 1111111110.000000) {
    printf("failed, expect %lf, got %lf\n", 1111111110.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_sub");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_sub check...");
  if (*(double *)sp !=
      123400000000000007812762268812638756607430593436581896388608.000000) {
    printf("failed, expect %lf, got %lf\n",
           123400000000000007812762268812638756607430593436581896388608.000000,
           *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_mul");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_mul check...");
  if (*(double *)sp != -15179717820000.000000) {
    printf("failed, expect %lf, got %lf\n", -15179717820000.000000,
           *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_div");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_div check...");
  if (*(double *)sp !=
      999999999999999980835596172437374590573120014030318793091164810154100112203678582976298268616221151962702060266176005440567032331208403948233373515776.000000) {
    printf(
        "failed, expect %lf, got %lf\n",
        999999999999999980835596172437374590573120014030318793091164810154100112203678582976298268616221151962702060266176005440567032331208403948233373515776.000000,
        *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_min");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_min check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_max");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_max check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_copysign");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_copysign check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");
  pwart_free_module_state(ctx);
  pwart_free_stack(stackbase);
  return 1;
}

int control_test() {
  FILE *f = fopen("control.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  void *stackbase = pwart_allocate_stack(64 * 1024);
  fclose(f);
  struct pwart_inspect_result1 ctxinfo;
  char *err=NULL;
  pwart_module_state ctx=pwart_load_module(data,len,&err);
  pwart_inspect_module_state(ctx, &ctxinfo);

  printf("runtime stack at %p\n", stackbase);

  pwart_wasm_function fn;
  void *sp;

  fn = pwart_get_export_function(ctx, "brif1");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("brif1 check...");
  if (*(uint32_t *)sp != 29) {
    printf("failed, expect %u, got %u\n", 29, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "brif2");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("brif2 check...");
  if (*(uint32_t *)sp != 42) {
    printf("failed, expect %u, got %u\n", 42, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "loop1");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("loop1 check...");
  if (*(uint32_t *)sp != 3) {
    printf("failed, expect %u, got %u\n", 3, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "loop2");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("loop2 check...");
  if (*(uint32_t *)sp != 10) {
    printf("failed, expect %u, got %u\n", 10, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "if1");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("if1 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "if2");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("if2 check...");
  if (*(uint32_t *)sp != 9) {
    printf("failed, expect %u, got %u\n", 9, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "brtable1");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("brtable1 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "brtable2");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("brtable2 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "brtable3");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("brtable3 check...");
  if (*(uint32_t *)sp != 2) {
    printf("failed, expect %u, got %u\n", 2, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(ctx, "expr_block");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("expr_block check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "expr_brif");
  sp = stackbase;
  puti32(&sp,0);
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("expr_brif 1 check...");
  if (*(uint32_t *)sp != 29) {
    printf("failed, expect %u, got %u\n", 29, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "expr_brif");
  sp = stackbase;
  puti32(&sp,1);
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("expr_brif 2 check...");
  if (*(uint32_t *)sp != 42) {
    printf("failed, expect %u, got %u\n", 42, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  pwart_free_module_state(ctx);
  pwart_free_stack(stackbase);
  return 1;
}

int convert_test() {
  FILE *f = fopen("convert.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  void *stackbase = pwart_allocate_stack(64 * 1024);
  fclose(f);
  struct pwart_inspect_result1 ctxinfo;
  char *err=NULL;
  pwart_module_state ctx=pwart_load_module(data,len,&err);
  pwart_inspect_module_state(ctx, &ctxinfo);

  printf("runtime stack at %p\n", stackbase);

  pwart_wasm_function fn;
  void *sp;

  fn = pwart_get_export_function(ctx, "i32_wrap_i64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_wrap_i64 check...");
  if (*(uint32_t *)sp != 4294967295) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967295,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_trunc_s_f32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_trunc_s_f32 check...");
  if (*(uint32_t *)sp != 4294967196) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967196,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_trunc_u_f32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_trunc_u_f32 check...");
  if (*(uint32_t *)sp != 3000000000) {
    printf("failed, expect %u, got %u\n", (uint32_t)3000000000,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_trunc_s_f64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_trunc_s_f64 check...");
  if (*(uint32_t *)sp != 4294967196) {
    printf("failed, expect %u, got %u\n", (uint32_t)4294967196,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i32_trunc_u_f64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_trunc_u_f64 check...");
  if (*(uint32_t *)sp != 3000000000) {
    printf("failed, expect %u, got %u\n", (uint32_t)3000000000,
           *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_extend_u_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_extend_u_i32 check...");
  if (*(uint64_t *)sp != (uint64_t)4294967295ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)4294967295ull, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_extend_s_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_extend_s_i32 check...");
  if (*(uint64_t *)sp != (uint64_t)18446744073709551615ull) {
    printf("failed, expect %"PRIu64", got %"PRIu64"\n", (uint64_t)18446744073709551615ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_trunc_s_f32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_trunc_s_f32 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_trunc_u_f32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_trunc_u_f32 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_trunc_s_f64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_trunc_s_f64 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "i64_trunc_u_f64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_trunc_u_f64 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_convert_s_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_convert_s_i32 check...");
  if (*(float *)sp != -1.000000) {
    printf("failed, expect %f, got %f\n", -1.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_convert_u_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_convert_u_i32 check...");
  if (*(float *)sp != 4294967296.000000) {
    printf("failed, expect %f, got %f\n", 4294967296.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_demote_f64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_demote_f64 check...");
  if (*(float *)sp != 12345679.000000) {
    printf("failed, expect %f, got %f\n", 12345679.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_convert_s_i64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_convert_s_i64 check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f32_convert_u_i64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_convert_u_i64 check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_convert_s_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_convert_s_i32 check...");
  if (*(double *)sp != -1.000000) {
    printf("failed, expect %lf, got %lf\n", -1.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_convert_u_i32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_convert_u_i32 check...");
  if (*(double *)sp != 4294967295.000000) {
    printf("failed, expect %lf, got %lf\n", 4294967295.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_demote_f32");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_demote_f32 check...");
  if (*(double *)sp != 12345679.000000) {
    printf("failed, expect %lf, got %lf\n", 12345679.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_convert_s_i64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_convert_s_i64 check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(ctx, "f64_convert_u_i64");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_convert_u_i64 check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");
  pwart_free_module_state(ctx);
  pwart_free_stack(stackbase);
  return 1;
}

int compare_test() {
  FILE *f = fopen("compare.wasm", "rb");
  if(f==NULL){printf("open wasm file failed.\n");return 0;}
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  void *stackbase = pwart_allocate_stack(64 * 1024);
  fclose(f);
  struct pwart_inspect_result1 ctxinfo;
  char *err=NULL;
  pwart_module_state ctx=pwart_load_module(data,len,&err);
  pwart_inspect_module_state(ctx, &ctxinfo);

  printf("runtime stack at %p\n", stackbase);

  pwart_wasm_function fn;
  void *sp;

  fn = pwart_get_export_function(ctx, "i32_eq_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_eq_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_eq_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_eq_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ne_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ne_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ne_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ne_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_s_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_s_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_s_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_u_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_u_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_lt_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_lt_u_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_s_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_s_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_s_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_u_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_u_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_le_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_le_u_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_s_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_s_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_s_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_u_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_u_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_gt_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_gt_u_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_s_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_s_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_s_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_u_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_u_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i32_ge_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i32_ge_u_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_eq_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_eq_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_eq_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_eq_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ne_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ne_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ne_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ne_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_s_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_s_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_s_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_u_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_u_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_lt_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_lt_u_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_s_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_s_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_s_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_u_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_u_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_le_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_le_u_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_s_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_s_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_s_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_u_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_u_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_gt_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_gt_u_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_s_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_s_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_s_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_s_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_s_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_s_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_u_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_u_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_u_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_u_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "i64_ge_u_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("i64_ge_u_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_eq_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_eq_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_eq_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_eq_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_ne_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ne_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_ne_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ne_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_lt_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_lt_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_lt_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_lt_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_lt_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_lt_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_le_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_le_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_le_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_le_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_le_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_le_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_gt_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_gt_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_gt_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_gt_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_gt_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_gt_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_ge_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ge_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_ge_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ge_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f32_ge_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f32_ge_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_eq_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_eq_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_eq_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_eq_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_ne_true");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ne_true check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_ne_false");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ne_false check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_lt_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_lt_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_lt_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_lt_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_lt_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_lt_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_le_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_le_less check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_le_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_le_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_le_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_le_greater check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_gt_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_gt_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_gt_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_gt_equal check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_gt_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_gt_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_ge_less");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ge_less check...");
  if (*(int *)sp != 0) {
    printf("failed, expect %x, got %x\n", 0, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_ge_equal");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ge_equal check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  fn = pwart_get_export_function(ctx, "f64_ge_greater");
  sp = stackbase;
  pwart_call_wasm_function(fn, sp);
  printf("f64_ge_greater check...");
  if (*(int *)sp != 1) {
    printf("failed, expect %x, got %x\n", 1, *(int *)sp);
    return 0;
  }
  printf("pass\n");
  sp = stackbase;

  pwart_free_module_state(ctx);
  pwart_free_stack(stackbase);
  return 1;

}

int main(int argc, char *argv[]) {
  
  if (unary_test()) {
    printf("unary_test pass\n");
  } else {
    printf("unary_test failed\n");
    return 1;
  }
  if (binary_test()) {
    printf("binary_test pass\n");
  } else {
    printf("binary_test failed\n");
    return 1;
  }
  if (control_test()) {
    printf("control_test pass\n");
  } else {
    printf("control_test failed\n");
    return 1;
  }
  if (convert_test()) {
    printf("convert_test pass\n");
  } else {
    printf("convert_test failed\n");
    return 1;
  }
  if(compare_test()){
    printf("compare_test pass\n");
  } else {
    printf("convert_test failed\n");
    return 1;
  }
  
  
  if (test1()) {
    printf("test1 pass\n");
  } else {
    printf("test1 failed\n");
    return 1;
  }
  printf("all test pass.\n");
  return 0;
}