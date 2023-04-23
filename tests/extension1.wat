(module
(import "pwart_builtin" "version" (func $getPwartVersion (result i32) ))
(import "pwart_builtin" "native_index_size" (func $native_index_size (result i32) ))
(;; we use funcref to replace anyref ;;)
(import "pwart_builtin" "get_self_runtime_context" (func $get_self_runtime_context (result funcref) ))
(import "pwart_builtin" "ref_from_index" (func $ref_from_index (param i32) (result funcref) ))
(import "pwart_builtin" "ref_from_i64" (func $ref_from_i64 (param i64) (result funcref) ))
(import "pwart_builtin" "i64_from_ref" (func $i64_from_ref (param funcref) (result i64) ))
(import "pwart_builtin" "ref_string_length" (func $ref_string_length (param funcref) (result i32) ))
(import "pwart_builtin" "memory_alloc" (func $malloc (param i32) (result i64) ))
(import "pwart_builtin" "memory_free" (func $mfree (param i64)))
(import "pwart_builtin" "native_memory" (memory $mem1 i64 16384))
(import "test1" "addTwo" (func $addTwo (param i64 i64) (result i64) ))
(global $mbase (mut i64) (i64.const 0))


  (func $test1 (result i64) (local $mem2 i64)
    (local.set $mem2 (call $malloc (i32.const 128)))
    (i64.store $mem1 (local.get $mem2) (call $addTwo (i64.const 123000) (i64.const 456)) )
    (global.set $mbase (local.get $mem2))
    global.get $mbase
  )
  (func $test2 (local $mem2 i64)
    (call $mfree (global.get $mbase))
  )
  (export "test1" (func $test1))
  (export "test2" (func $test2))
)