
#include <pwart.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define puti32 pwart_rstack_put_i32
#define puti64 pwart_rstack_put_i64
#define putf32 pwart_rstack_put_f32
#define putf64 pwart_rstack_put_f64

#define geti32 pwart_rstack_get_i32
#define geti64 pwart_rstack_get_i64
#define getf32 pwart_rstack_get_f32
#define getf64 pwart_rstack_get_f64

static void test_wasmfn(void *fp, pwart_runtime_context c) {
  void *sp = fp;
  uint64_t r = geti64(&sp) + geti64(&sp) + 2;
  sp = fp;
  puti64(&sp, r);
}

// base test
int test1() {
  struct pwart_load_config cfg;
  FILE *f = fopen("test1.wasm", "rb");
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  cfg.size_of_this=sizeof(cfg);

  for(int i1=0;i1<2;i1++){
    pwart_module m = pwart_new_module();
    //different config
    switch(i1){
      case 0:
      printf("test config:default\n");
      break;
      case 1:
      printf("test config:memory_model=PWART_MEMORY_MODEL_GROW_ENABLED\n");
      pwart_get_load_config(m,&cfg);
      cfg.memory_model=PWART_MEMORY_MODEL_GROW_ENABLED;
      pwart_set_load_config(m,&cfg);
      break;
    }

    struct pwart_inspect_result1 ctxinfo;
    pwart_load(m, data, len);
    pwart_runtime_context ctx = pwart_get_runtime_context(m);
    pwart_inspect_runtime_context(ctx, &ctxinfo);

    printf("runtime stack at %p\n", ctxinfo.runtime_stack);

    pwart_wasmfunction test1 = pwart_get_export_function(m, "test1");
    pwart_wasmfunction test2 = pwart_get_export_function(m, "test2");
    pwart_wasmfunction test3 = pwart_get_export_function(m, "test3");
    pwart_wasmfunction fib_main=pwart_get_export_function(m,"fib_main");

    pwart_free_module(m);

    void *sp;

    int32_t ri32;
    uint64_t ri64;

    sp = ctxinfo.runtime_stack;
    puti32(&sp, 22);
    puti32(&sp, 33);

    sp = ctxinfo.runtime_stack;
    test1(sp, ctx);
    ri32 = geti32(&sp);
    printf("expect %u, got %u\n", (22 + 33) * 2, ri32);

    if ((22 + 33) * 2 != ri32) {
      return 0;
    }

    sp = ctxinfo.runtime_stack;
    puti32(&sp, 100);
    puti32(&sp, 11);
    sp = ctxinfo.runtime_stack;
    test1(sp, ctx);
    ri32 = geti32(&sp);
    printf("expect %u, got %u\n", 100 + 11, ri32);
    if (100 + 11 != ri32) {
      return 0;
    }

    char *memstr = (uint8_t *)ctxinfo.memory;
    *(memstr + 100) = 0;
    printf("expect HelloHello!!!!, got %s\n", memstr + 1);
    if (strcmp(memstr + 1, "HelloHello!!!!") != 0) {
      return 0;
    }
    printf("expect %p, got %p , %p\n", ctxinfo.table_entries[0],
          ctxinfo.table_entries[1], ctxinfo.table_entries[2]);
    if (ctxinfo.table_entries[0] != ctxinfo.table_entries[1]) {
      return 0;
    }
    if (ctxinfo.table_entries[0] != ctxinfo.table_entries[2]) {
      return 0;
    }

    sp = ctxinfo.runtime_stack;
    puti64(&sp, 22);
    puti64(&sp, 33);
    sp = ctxinfo.runtime_stack;
    test2(sp, ctx);
    ri64 = geti64(&sp);
    printf("expect %u, got %llu\n", (22 + 33 + 140), ri64);
    if (22 + 33 + 140 != ri64) {
      return 0;
    }

    ctxinfo.table_entries[1] = &test_wasmfn;
    sp = ctxinfo.runtime_stack;
    puti64(&sp, 22);
    puti64(&sp, 33);
    sp = ctxinfo.runtime_stack;
    test2(sp, ctx);
    ri64 = geti64(&sp);
    printf("expect %u, got %llu\n", 22 + 33 + 140 + 2, ri64);
    if (22 + 33 + 140 + 2 != ri64) {
      return 0;
    }

    sp = ctxinfo.runtime_stack;
    putf64(&sp, 3.25);
    putf64(&sp, 4.75);
    sp = ctxinfo.runtime_stack;
    test3(sp, ctx);
    ri64 = geti64(&sp);
    printf("expect %llu, got %llu\n", (int64_t)(3.25+4.75), ri64);
    if ((int64_t)(3.25+4.75) != ri64) {
      return 0;
    }
    
    //fibnacci sequence
    sp=ctxinfo.runtime_stack;
    puti32(&sp,2);
    puti32(&sp,8);
    *(int *)((char *)ctxinfo.memory+8)=0;
    *(int *)((char *)ctxinfo.memory+12)=16;
    memcpy((char *)ctxinfo.memory+16,"30",3);
    sp = ctxinfo.runtime_stack;
    fib_main(sp,ctx);
    ri32 = geti32(&sp);
    printf("fib test, expect %d, got %d\n",832040,ri32);
    if (832040 != ri32) {
      return 0;
    }

    pwart_free_runtime(ctx);
    
  }
  free(data);
  return 1;
}

int unary_test() {
  FILE *f = fopen("unary.wasm", "rb");
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_module m = pwart_new_module();
  struct pwart_inspect_result1 ctxinfo;
  pwart_load(m, data, len);
  pwart_runtime_context ctx = pwart_get_runtime_context(m);
  pwart_inspect_runtime_context(ctx, &ctxinfo);

  printf("runtime stack at %p\n", ctxinfo.runtime_stack);

  pwart_wasmfunction fn;
  void *sp;

  fn = pwart_get_export_function(m, "i32_eqz_100");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_eqz_100 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_eqz_0");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_eqz_0 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_clz");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_clz check...");
  if (*(uint32_t *)sp != 24) {
    printf("failed, expect %u, got %u\n", 24, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_ctz");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_ctz check...");
  if (*(uint32_t *)sp != 7) {
    printf("failed, expect %u, got %u\n", 7, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_popcnt");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_popcnt check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_eqz_100");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_eqz_100 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_eqz_0");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_eqz_0 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_clz");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_clz check...");
  if (*(uint64_t *)sp != 56) {
    printf("failed, expect %llu, got %llu\n", 56, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_ctz");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_ctz check...");
  if (*(uint64_t *)sp != 7) {
    printf("failed, expect %llu, got %llu\n", 7, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_popcnt");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_popcnt check...");
  if (*(uint64_t *)sp != 1) {
    printf("failed, expect %llu, got %llu\n", 1, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_neg");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_neg check...");
  if (*(float *)sp != -100.000000) {
    printf("failed, expect %f, got %f\n", -100.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_abs");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_abs check...");
  if (*(float *)sp != 100.000000) {
    printf("failed, expect %f, got %f\n", 100.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  
  fn=pwart_get_export_function(m,"f32_sqrt_neg_is_nan");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_sqrt_neg_is_nan check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");
  

  fn = pwart_get_export_function(m, "f32_sqrt_100");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_sqrt_100 check...");
  if (*(float *)sp != 10.000000) {
    printf("failed, expect %f, got %f\n", 10.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_ceil");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_ceil check...");
  if (*(float *)sp != -0.000000) {
    printf("failed, expect %f, got %f\n", -0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_floor");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_floor check...");
  if (*(float *)sp != -1.000000) {
    printf("failed, expect %f, got %f\n", -1.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_trunc");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_trunc check...");
  if (*(float *)sp != -0.000000) {
    printf("failed, expect %f, got %f\n", -0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_nearest_lo");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_nearest_lo check...");
  if (*(float *)sp != 1.000000) {
    printf("failed, expect %f, got %f\n", 1.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_nearest_hi");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_nearest_hi check...");
  if (*(float *)sp != 2.000000) {
    printf("failed, expect %f, got %f\n", 2.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_neg");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_neg check...");
  if (*(double *)sp != -100.000000) {
    printf("failed, expect %lf, got %lf\n", -100.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_abs");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_abs check...");
  if (*(double *)sp != 100.000000) {
    printf("failed, expect %lf, got %lf\n", 100.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_sqrt_neg_is_nan");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_sqrt_neg_is_nan check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");
  

  fn = pwart_get_export_function(m, "f64_sqrt_100");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_sqrt_100 check...");
  if (*(double *)sp != 10.000000) {
    printf("failed, expect %lf, got %lf\n", 10.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_ceil");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_ceil check...");
  if (*(double *)sp != -0.000000) {
    printf("failed, expect %lf, got %lf\n", -0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_floor");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_floor check...");
  if (*(double *)sp != -1.000000) {
    printf("failed, expect %lf, got %lf\n", -1.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_trunc");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_trunc check...");
  if (*(double *)sp != -0.000000) {
    printf("failed, expect %lf, got %lf\n", -0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_nearest_lo");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_nearest_lo check...");
  if (*(double *)sp != 1.000000) {
    printf("failed, expect %lf, got %lf\n", 1.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_nearest_hi");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_nearest_hi check...");
  if (*(double *)sp != 2.000000) {
    printf("failed, expect %lf, got %lf\n", 2.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");
  pwart_delete_module(m);
  return 1;
}

int binary_test() {
  FILE *f = fopen("binary.wasm", "rb");
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_module m = pwart_new_module();
  struct pwart_inspect_result1 ctxinfo;
  pwart_load(m, data, len);
  pwart_runtime_context ctx = pwart_get_runtime_context(m);
  pwart_inspect_runtime_context(ctx, &ctxinfo);

  printf("runtime stack at %p\n", ctxinfo.runtime_stack);

  pwart_wasmfunction fn;
  void *sp;
  fn = pwart_get_export_function(m, "i32_add");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_add check...");
  if (*(uint32_t *)sp != 3) {
    printf("failed, expect %u, got %u\n", 3, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_sub");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_sub check...");
  if (*(uint32_t *)sp != 16) {
    printf("failed, expect %u, got %u\n", 16, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_mul");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_mul check...");
  if (*(uint32_t *)sp != 21) {
    printf("failed, expect %u, got %u\n", 21, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_div_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_div_s check...");
  if (*(uint32_t *)sp != 4294967294) {
    printf("failed, expect %u, got %u\n", 4294967294, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_div_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_div_u check...");
  if (*(uint32_t *)sp != 2147483646) {
    printf("failed, expect %u, got %u\n", 2147483646, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_rem_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_rem_s check...");
  if (*(uint32_t *)sp != 4294967295) {
    printf("failed, expect %u, got %u\n", 4294967295, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_rem_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_rem_u check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_and");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_and check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_or");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_or check...");
  if (*(uint32_t *)sp != 15) {
    printf("failed, expect %u, got %u\n", 15, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_xor");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_xor check...");
  if (*(uint32_t *)sp != 14) {
    printf("failed, expect %u, got %u\n", 14, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_shl");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_shl check...");
  if (*(uint32_t *)sp != 4294966496) {
    printf("failed, expect %u, got %u\n", 4294966496, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_shr_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_shr_u check...");
  if (*(uint32_t *)sp != 536870899) {
    printf("failed, expect %u, got %u\n", 536870899, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_shr_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_shr_s check...");
  if (*(uint32_t *)sp != 4294967283) {
    printf("failed, expect %u, got %u\n", 4294967283, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_rotl");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_rotl check...");
  if (*(uint32_t *)sp != 4294966503) {
    printf("failed, expect %u, got %u\n", 4294966503, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i32_rotr");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i32_rotr check...");
  if (*(uint32_t *)sp != 2684354547) {
    printf("failed, expect %u, got %u\n", 2684354547, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_add");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_add check...");
  if (*(uint64_t *)sp != 3L) {
    printf("failed, expect %llu, got %llu\n", 3L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_sub");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_sub check...");
  if (*(uint64_t *)sp != 16L) {
    printf("failed, expect %llu, got %llu\n", 16L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_mul");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_mul check...");
  if (*(uint64_t *)sp != 21L) {
    printf("failed, expect %llu, got %llu\n", 21L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_div_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_div_s check...");
  if (*(uint64_t *)sp != 18446744073709551614ull) {
    printf("failed, expect %llu, got %llu\n", 18446744073709551614ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_div_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_div_u check...");
  if (*(uint64_t *)sp != 9223372036854775806ull) {
    printf("failed, expect %llu, got %llu\n", 9223372036854775806ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_rem_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_rem_s check...");
  if (*(uint64_t *)sp != 18446744073709551615ull) {
    printf("failed, expect %llu, got %llu\n", 18446744073709551615ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_rem_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_rem_u check...");
  if (*(uint64_t *)sp != 1L) {
    printf("failed, expect %llu, got %llu\n", 1L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_and");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_and check...");
  if (*(uint64_t *)sp != 1L) {
    printf("failed, expect %llu, got %llu\n", 1L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_or");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_or check...");
  if (*(uint64_t *)sp != 15L) {
    printf("failed, expect %llu, got %llu\n", 15L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_xor");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_xor check...");
  if (*(uint64_t *)sp != 14L) {
    printf("failed, expect %llu, got %llu\n", 14L, *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_shl");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_shl check...");
  if (*(uint64_t *)sp != 18446744073709550816ull) {
    printf("failed, expect %llu, got %llu\n", 18446744073709550816ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_shr_u");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_shr_u check...");
  if (*(uint64_t *)sp != 2305843009213693939ull) {
    printf("failed, expect %llu, got %llu\n", 2305843009213693939ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_shr_s");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_shr_s check...");
  if (*(uint64_t *)sp != 18446744073709551603ull) {
    printf("failed, expect %llu, got %llu\n", 18446744073709551603ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_rotl");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_rotl check...");
  if (*(uint64_t *)sp != 18446744073709550823ull) {
    printf("failed, expect %llu, got %llu\n", 18446744073709550823ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "i64_rotr");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("i64_rotr check...");
  if (*(uint64_t *)sp != 11529215046068469747ull) {
    printf("failed, expect %llu, got %llu\n", 11529215046068469747ull,
           *(uint64_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_add");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_add check...");
  if (*(float *)sp != 5.000000) {
    printf("failed, expect %f, got %f\n", 5.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_sub");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_sub check...");
  if (*(float *)sp != -9995.500000) {
    printf("failed, expect %f, got %f\n", -9995.500000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_mul");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_mul check...");
  if (*(float *)sp != -8487.187500) {
    printf("failed, expect %f, got %f\n", -8487.187500, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_div");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_div check...");
  if (*(float *)sp != -500000000.000000) {
    printf("failed, expect %f, got %f\n", -500000000.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_min");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_min check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_max");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_max check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f32_copysign");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f32_copysign check...");
  if (*(float *)sp != 0.000000) {
    printf("failed, expect %f, got %f\n", 0.000000, *(float *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_add");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_add check...");
  if (*(double *)sp != 1111111110.000000) {
    printf("failed, expect %lf, got %lf\n", 1111111110.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_sub");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_sub check...");
  if (*(double *)sp !=
      123400000000000007812762268812638756607430593436581896388608.000000) {
    printf("failed, expect %lf, got %lf\n",
           123400000000000007812762268812638756607430593436581896388608.000000,
           *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_mul");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_mul check...");
  if (*(double *)sp != -15179717820000.000000) {
    printf("failed, expect %lf, got %lf\n", -15179717820000.000000,
           *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_div");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
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

  fn = pwart_get_export_function(m, "f64_min");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_min check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_max");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_max check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "f64_copysign");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("f64_copysign check...");
  if (*(double *)sp != 0.000000) {
    printf("failed, expect %lf, got %lf\n", 0.000000, *(double *)sp);
    return 0;
  }
  printf("pass\n");
  pwart_delete_module(m);
  return 1;
}

int control_test(){
  FILE *f = fopen("control.wasm", "rb");
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_module m = pwart_new_module();
  struct pwart_inspect_result1 ctxinfo;
  pwart_load(m, data, len);
  pwart_runtime_context ctx = pwart_get_runtime_context(m);
  pwart_inspect_runtime_context(ctx, &ctxinfo);

  printf("runtime stack at %p\n", ctxinfo.runtime_stack);

  pwart_wasmfunction fn;
  void *sp;


  fn = pwart_get_export_function(m, "brif1");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("brif1 check...");
  if (*(uint32_t *)sp != 29) {
    printf("failed, expect %u, got %u\n", 29, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(m, "brif2");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("brif2 check...");
  if (*(uint32_t *)sp != 42) {
    printf("failed, expect %u, got %u\n", 42, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "loop1");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("loop1 check...");
  if (*(uint32_t *)sp != 3) {
    printf("failed, expect %u, got %u\n", 3, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(m, "loop2");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("loop2 check...");
  if (*(uint32_t *)sp != 10) {
    printf("failed, expect %u, got %u\n", 10, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  fn = pwart_get_export_function(m, "if1");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("if1 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(m, "if2");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("if2 check...");
  if (*(uint32_t *)sp != 9) {
    printf("failed, expect %u, got %u\n", 9, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");


  fn = pwart_get_export_function(m, "brtable1");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("brtable1 check...");
  if (*(uint32_t *)sp != 0) {
    printf("failed, expect %u, got %u\n", 0, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(m, "brtable2");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("brtable2 check...");
  if (*(uint32_t *)sp != 1) {
    printf("failed, expect %u, got %u\n", 1, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");
  fn = pwart_get_export_function(m, "brtable3");
  sp = ctxinfo.runtime_stack;
  fn(sp, ctx);
  printf("brtable3 check...");
  if (*(uint32_t *)sp != 2) {
    printf("failed, expect %u, got %u\n", 2, *(uint32_t *)sp);
    return 0;
  }
  printf("pass\n");

  pwart_delete_module(m);
  return 1;
}

int convert_test(){
  FILE *f = fopen("convert.wasm", "rb");
  // 8MB buffer;
  uint8_t *data = malloc(1024 * 1024 * 8);
  int len = fread(data, 1, 1024 * 1024 * 8, f);
  fclose(f);
  pwart_module m = pwart_new_module();
  struct pwart_inspect_result1 ctxinfo;
  pwart_load(m, data, len);
  pwart_runtime_context ctx = pwart_get_runtime_context(m);
  pwart_inspect_runtime_context(ctx, &ctxinfo);

  printf("runtime stack at %p\n", ctxinfo.runtime_stack);

  pwart_wasmfunction fn;
  void *sp;

  fn=pwart_get_export_function(m,"i32_wrap_i64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i32_wrap_i64 check...");
  if(*(uint32_t *)sp!=4294967295){
      printf("failed, expect %u, got %u\n",4294967295,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i32_trunc_s_f32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i32_trunc_s_f32 check...");
  if(*(uint32_t *)sp!=4294967196){
      printf("failed, expect %u, got %u\n",4294967196,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i32_trunc_u_f32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i32_trunc_u_f32 check...");
  if(*(uint32_t *)sp!=3000000000){
      printf("failed, expect %u, got %u\n",3000000000,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i32_trunc_s_f64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i32_trunc_s_f64 check...");
  if(*(uint32_t *)sp!=4294967196){
      printf("failed, expect %u, got %u\n",4294967196,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i32_trunc_u_f64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i32_trunc_u_f64 check...");
  if(*(uint32_t *)sp!=3000000000){
      printf("failed, expect %u, got %u\n",3000000000,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_extend_u_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_extend_u_i32 check...");
  if(*(uint64_t *)sp!=4294967295ull){
      printf("failed, expect %llu, got %llu\n",4294967295ull,*(uint64_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_extend_s_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_extend_s_i32 check...");
  if(*(uint64_t *)sp!=18446744073709551615ull){
      printf("failed, expect %llu, got %llu\n",18446744073709551615ull,*(uint64_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_trunc_s_f32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_trunc_s_f32 check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_trunc_u_f32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_trunc_u_f32 check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_trunc_s_f64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_trunc_s_f64 check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"i64_trunc_u_f64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("i64_trunc_u_f64 check...");
  if(*(uint32_t *)sp!=1){
      printf("failed, expect %u, got %u\n",1,*(uint32_t *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f32_convert_s_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_convert_s_i32 check...");
  if(*(float *)sp!=-1.000000){
      printf("failed, expect %f, got %f\n",-1.000000,*(float *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f32_convert_u_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_convert_u_i32 check...");
  if(*(float *)sp!=4294967296.000000){
      printf("failed, expect %f, got %f\n",4294967296.000000,*(float *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f32_demote_f64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_demote_f64 check...");
  if(*(float *)sp!=12345679.000000){
      printf("failed, expect %f, got %f\n",12345679.000000,*(float *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f32_convert_s_i64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_convert_s_i64 check...");
  if(*(float *)sp!=0.000000){
      printf("failed, expect %f, got %f\n",0.000000,*(float *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f32_convert_u_i64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f32_convert_u_i64 check...");
  if(*(float *)sp!=0.000000){
      printf("failed, expect %f, got %f\n",0.000000,*(float *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_convert_s_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_convert_s_i32 check...");
  if(*(double *)sp!=-1.000000){
      printf("failed, expect %lf, got %lf\n",-1.000000,*(double *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_convert_u_i32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_convert_u_i32 check...");
  if(*(double *)sp!=4294967295.000000){
      printf("failed, expect %lf, got %lf\n",4294967295.000000,*(double *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_demote_f32");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_demote_f32 check...");
  if(*(double *)sp!=12345679.000000){
      printf("failed, expect %lf, got %lf\n",12345679.000000,*(double *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_convert_s_i64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_convert_s_i64 check...");
  if(*(double *)sp!=0.000000){
      printf("failed, expect %lf, got %lf\n",0.000000,*(double *)sp);
      return 0;
  }
  printf("pass\n");

  fn=pwart_get_export_function(m,"f64_convert_u_i64");
  sp=ctxinfo.runtime_stack;
  fn(sp,ctx);
  printf("f64_convert_u_i64 check...");
  if(*(double *)sp!=0.000000){
      printf("failed, expect %lf, got %lf\n",0.000000,*(double *)sp);
      return 0;
  }
  printf("pass\n");
  pwart_delete_module(m);
  return 1;
}

int main(int argc, char *argv[]) {
  if (test1()) {
    printf("test1 pass\n");
  } else {
    printf("test1 failed\n");
    return 1;
  }

  if ( unary_test()) {
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
  if(control_test()){
    printf("control_test pass\n");
  } else {
    printf("control_test failed\n");
    return 1;
  }
  if(convert_test()){
    printf("convert_test pass\n");
  } else {
    printf("convert_test failed\n");
    return 1;
  }
  printf("all test pass.\n");
  return 0;
}