(module
(import "pwart_builtin" "version" (func $getPwartVersion (result i32) ))
(import "pwart_builtin" "native_index_size" (func $native_index_size (result i32) ))
(;; we use funcref to replace anyref ;;)
(import "pwart_builtin" "get_self_runtime_context" (func $get_self_runtime_context (result funcref) ))
(import "pwart_builtin" "ref_from_index" (func $ref_from_index (param i32 i32) (result funcref) ))
(import "pwart_builtin" "ref_from_i64" (func $ref_from_i64 (param i64) (result funcref) ))
(import "pwart_builtin" "i64_from_ref" (func $i64_from_ref (param funcref) (result i64) ))
(import "pwart_builtin" "ref_string_length" (func $ref_string_length (param funcref) (result i32) ))
(import "pwart_builtin" "memory_alloc" (func $malloc (param i32) (result i64) ))
(import "pwart_builtin" "memory_free" (func $mfree (param i64)))
(import "pwart_builtin" "native_memory" (memory $mem1 i64 16384))
(import "pwart_builtin" "ref_copy_bytes" (func $refmcopy (param funcref funcref i32)))
(import "test1" "addTwo" (func $addTwo (param i64 i64) (result i64) ))
(import "pwart_builtin" "stdio" (func $stdio (result funcref funcref funcref)))
(import "pwart_builtin" "fwrite" (func $fwrite (param funcref i32 i32 funcref)))
(import "pwart_builtin" "host_definition" (func $host_definition (param i32)(result funcref)))


(memory $mem2 1 1)
(data (memory $mem2) (i32.const 0) "fwrite test ok!\n\00\n\\0")

(global $mbase (mut i64) (i64.const 0))
(table $stdiols 3 funcref)


  (func $test1 (result i64) (local $mem2 i64)(local $stdout funcref)
    (local.set $mem2 (call $malloc (i32.const 128)))
    (i64.store $mem1 (local.get $mem2) (call $addTwo (i64.const 123000) (i64.const 456)) )
    (global.set $mbase (local.get $mem2))

    (call $stdio)
    drop
    (local.set $stdout)
    (table.set $stdiols (i32.const 0) (local.get $stdout))
    drop

    (call $ref_from_i64 (i64.add (local.get $mem2) (i64.const 24)))
    (call $ref_from_index (i32.const 1) (i32.const 0))
    i32.const 16
    call $refmcopy

    (call $ref_from_i64 (i64.add (local.get $mem2) (i64.const 24)))
    i32.const 16
    i32.const 1
    local.get $stdout
    call $fwrite

    global.get $mbase
  )
  (func $printstring (param $str funcref)
    (call $fwrite 
    (local.get $str)
    (call $ref_string_length (local.get $str))
    (i32.const 1)
    (table.get $stdiols (i32.const 0))
    )
    (call $fwrite 
    (call $ref_from_index (i32.const 1) (i32.const 17))
    (i32.const 1)
    (i32.const 1)
    (table.get $stdiols (i32.const 0))
    )
  )
  (func $test2 (local $index i32)(local $defstr funcref)
  (local.set $index (i32.const 0))
  loop $break1
    (local.set $defstr (call $host_definition (local.get $index)))
    (i32.eqz (ref.is_null (local.get $defstr)))
    if
      (call $printstring (local.get $defstr))
      (local.set $index (i32.add (local.get $index)(i32.const 1)))
      br $break1
    end
  end
    (call $printstring (call $ref_from_index (i32.const 1) (i32.const 0)))
    (call $mfree (global.get $mbase))
  )
  (export "test1" (func $test1))
  (export "test2" (func $test2))
)