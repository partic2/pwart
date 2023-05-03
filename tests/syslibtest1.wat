(module
(import "pwart_builtin" "version" (func $getPwartVersion (result i32) ))
(import "pwart_builtin" "native_index_size" (func $native_index_size (result i32) ))
(;; we use funcref to replace anyref ;;)
(import "pwart_builtin" "get_self_runtime_context" (func $get_self_runtime_context (result funcref) ))
(import "pwart_builtin" "ref_from_index" (func $ref_from_index (param i32 i32) (result funcref) ))
(import "pwart_builtin" "ref_from_i64" (func $ref_from_i64 (param i64) (result funcref) ))
(import "pwart_builtin" "i64_from_ref" (func $i64_from_ref (param funcref) (result i64) ))
(import "pwart_builtin" "ref_string_length" (func $ref_string_length (param funcref) (result i32) ))
(import "libuv" "uv_thread_create" (func $uv_thread_create(param i64)(result funcref)))
(import "libuv" "uv_mutex_init" (func $uv_thread_create (result funcref)))
(import "libuv" "uv_mutex_destroy" (func $uv_mutex_destroy (result funcref)))

(import "testaid" "printi64" (func $printi64 (param i64) (result i64) ))

(memory $mem0 1 1)

(data (memory $mem2) (i32.const 0) "fwrite test ok!\n")

  (func $thread1(param $arg i64)(result i32)
    local.get $arg
    i32.wrap_i64
    
  )
  (func $main (export "main")
    (param $args i32) (param $argv i32) (result i32) 
    (local $n i32) 
    local.get $argv
    i32.load offset=4 align=4
    call $fib:parseInt
    local.tee $n
    call $fib:fib
    return
  )

)