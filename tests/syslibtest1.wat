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
(import "libuv" "uv_mutex_init" (func $uv_mutex_init (result funcref)))
(import "libuv" "uv_mutex_destroy" (func $uv_mutex_destroy (result funcref)))
(import "libuv" "uv_dlopen" (func $uv_dlopen (param funcref)(result funcref i32)))
(import "libuv" "uv_dlsym" (func $uv_dlsym (param funcref funcref)(result funcref)))
(import "libuv" "uv_dlclose" (func $uv_dlcose (param funcref)))
(import "libffi" "ffix_new_cif" (func $ffix_new_cif (param funcref)(result funcref i32)))
(import "libffi" "ffi_call" (func $ffi_call (param funcref funcref funcref funcref)(result i32)))
(import "libffi" "ffix_call" (func $ffix_call (param funcref funcref funcref funcref)(result i32)))
(import "libffi" "ffix_new_c_callback" (func $ffix_new_c_callback (param funcref funcref funcref)(result funcref funcref)))
(import "pwart_builtin" "native_memory" (memory i64 10000))

(import "testaid" "printi64" (func $printi64 (param i64) (result i64) ))
(table $tb0 16 funcref)
(elem $tb0 (i32.const 0) $cbadd)

(global $crt:sp$ (mut i32) (i32.const 65528))
(global $crt:fp$ (mut i32) (i32.const 65528))
(memory $mem0 1 1)
(data (memory $mem0) (i32.const 0) "!\n\00\n\\0")
(data (memory $mem0) (i32.const 64) "ffitest.dll\00")
  (data (memory $mem0) (i32.const 288) "add\00")
  (data (memory $mem0) (i32.const 296) "iip>i\00")
  (data (memory $mem0) (i32.const 312) "\03\00\00\00\00\00\00\00\04\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00")
  (data (memory $mem0) (i32.const 428) "\00\00\00\00")

  (func $cbadd (param $cif funcref)(param $ret funcref)(param $args funcref)(param $userdata funcref)
  (local $argsbase i64) (local $result i32)
  (drop (call $printi64 (i64.const 700)))
  (local.set $argsbase (call $i64_from_ref (local.get $args)))
  (drop (call $printi64 (i64.const 800)))
  (i32.load (local.get $argsbase))
  
  (drop (call $printi64 (local.get $argsbase)))
  (drop (call $printi64 (i64.add (local.get $argsbase) (i64.const 8))))
  (i32.load (i64.add (local.get $argsbase) (i64.const 8)))
  (drop (call $printi64 (i64.const 1000)))
  i32.add
  local.set $result
  (drop (call $printi64 (i64.const 1100)))
  (i32.store (call $i64_from_ref (local.get $ret))(local.get $result))
  (drop (call $printi64 (i64.const 1200)))
  )

  (func $fficalltest (export "fficalltest")
    (result i32) 
    (local $dll funcref) (local $fn funcref) (local $cif funcref) (local $ccb funcref)
    (local $bp$ i32) (local $fp$ i32) (local $i1$ i32) 
    global.get $crt:sp$
    local.set $bp$
    global.get $crt:sp$
    i32.const 16
    i32.sub
    global.set $crt:sp$
    global.get $crt:sp$
    local.set $fp$
    i32.const 1
    i32.const 64
    call $ref_from_index 

    (drop (call $printi64 (i64.const 100)))

    call $uv_dlopen
    drop
    local.tee $dll
    i32.const 1
    i32.const 288
    call $ref_from_index
    (drop (call $printi64 (i64.const 200)))
    call $uv_dlsym
    local.set $fn
    i32.const 1
    i32.const 296
    call $ref_from_index
    (drop (call $printi64 (i64.const 300)))
    call $ffix_new_cif
    drop
    local.set $cif


    local.get $cif
    local.get $fn
    (call $ref_from_index (i32.const 1)(i32.const 428))
    (call $ref_from_index (i32.const 1)(i32.const 312))
    (drop (call $printi64 (i64.const 500)))
    call $ffix_call
    drop

    (call $ffix_new_c_callback (local.get $cif)(ref.func $cbadd) (ref.null func))
    local.set $ccb
    drop
    (i64.store $mem0 (i32.const 328)(call $i64_from_ref (local.get $ccb)))
    local.get $cif
    local.get $fn
    (call $ref_from_index (i32.const 1)(i32.const 432))
    (call $ref_from_index (i32.const 1)(i32.const 312))
    (drop (call $printi64 (i64.const 600)))
    call $ffix_call
    drop

    i32.const 0
    return
  )
  (export "mem0" (memory $mem0))
)