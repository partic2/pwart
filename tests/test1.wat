(module
(import "pwart_builtin" "version" (func $getPwartVersion (result i32) ))
(import "pwart_builtin" "native_index_size" (func $native_index_size (result i32) ))
(;; we use funcref to replace anyref ;;)
(import "pwart_builtin" "get_self_runtime_context" (func $get_self_runtime_context (result funcref) ))
(import "pwart_builtin" "ref_from_index" (func $ref_from_index (param i32) (result funcref) ))
(import "pwart_builtin" "ref_from_i64" (func $ref_from_i64 (param i64) (result funcref) ))
(import "pwart_builtin" "i64_from_ref" (func $i64_from_ref (param funcref) (result i64) ))
(import "pwart_builtin" "ref_string_length" (func $ref_string_length (param funcref) (result i32) ))
(import "testaid" "printi64" (func $printi64 (param i64) (result i64) ))
(memory 4)
(memory $mem1 i64 4 4)
  (type $typeAddTwo (func (param i64 i64) (result i64)))
  (type $longtype (func (param f64 f64 i32 i32 f32 f32 i64 i64) (result i64)))
  
  (table 16 funcref )
  (table 16 funcref )
  (elem 0 (i32.const 0) $addTwo )
  (global $one i32 (i32.const 1))
  (global $two i32 (i32.const 2))

  ;; function 0
  (func $nop
  )

  ;; function 1
  (func $addTwo (param i64 i64) (result i64) 
      i64.const 10000
      call $printi64
      drop
      (i64.store $mem1 offset=0 (i64.const 200) (local.get 0) )
      (i64.load $mem1 (i64.const 200))
      local.get 1
      i64.add
  )

  ;; function 2
  (func $addTwoF (param i32 f64 f64) (result i32 f64)
      local.get 0
      local.get 1
      local.get 2
      f64.add
  )
  
  ;; function 3
  (func $test1 (param i32 i32) (result i32) (local i32 i64)
    i32.const 2
    i32.const 1
    i32.const 0
    table.get 0
    table.set 0
    ref.func $addTwo
    table.set 0

    i32.const 1
    i32.const 0x48
    i32.store8
    i32.const 2
    i32.const 0x6c65
    i32.store16
    i32.const 4
    i32.const 0x6f6c
    i32.store

    i64.const 300
    call $printi64
    drop

    i32.const 6
    i32.const 1
    i32.const 5
    memory.copy

    i64.const 400
    call $printi64
    drop

    i32.const 11
    i32.const 33
    i32.const 4
    memory.fill

    i64.const 500
    call $printi64
    drop

    i32.const 15
    i32.const 0
    i32.store8

    i64.const 600
    call $printi64
    drop

    block $exit (result i32)
	i32.const 1
      local.get 0
      local.get 1
      i32.add
      local.tee 2
      i32.const 100
      i32.ge_u
      br_if $exit
      local.get 2
      global.get $two
      i32.mul
      local.set 2
    end
	drop
    local.get 2
  )

  ;; function 4
  (func $test2 (param i64 i64) (result i64) 
    i64.const 100
    call $printi64
    drop
    i32.const 100
    local.get 0
    i64.store
    i32.const 100
    i64.load
    local.get 1
      
    i32.const 2
    i32.const 1
    table.get 0
    table.set 1
      
    i64.const 200
    call $printi64
    drop
      
    i32.const 2
    call_indirect 1 (type $typeAddTwo)
    
    i64.const 300
    call $printi64
    drop
    
    i64.const 140
    call $addTwo
  )

  ;; function 5
  (func $test3 (param i32 f64 f64) (result i32 i64) (local i32)
    i64.const 100
    call $printi64
    drop
      
    i32.const 1
    local.get 0
    i32.const 160
    local.get 1
    f64.store
    i32.const 160
    f64.load
    local.get 2
    call $addTwoF
    local.set 2
    local.set 3
    drop
      
    i64.const 200
    call $printi64
    drop
      
    local.get 3
    local.get 2
    i64.trunc_sat_f64_s
  )

  ;; function 6
  (func $fib:fib
    (param $n i32) (result i32) 
    local.get $n
    i32.const 2
    i32.lt_u
    if
    local.get $n
    return
    end
    local.get $n
    i32.const 1
    i32.sub
    call $fib:fib
    local.get $n
    i32.const 2
    i32.sub
    call $fib:fib
    i32.add
    return
  )

  ;; function 7
  (func $fib:parseInt
    (param $str i32) (result i32) 
    (local $res i32) (local $i i32) 
    i32.const 0
    local.set $res
    block $2$
    i32.const 0
    local.set $i
    loop $1$
    local.get $str
    local.get $i
    i32.add
    i32.load8_s offset=0 align=1
    i32.const 0
    i32.ne
    i32.eqz
    br_if $2$
    block $3$
    local.get $res
    i32.const 10
    i32.mul
    local.get $str
    local.get $i
    i32.add
    i32.load8_s offset=0 align=1
    i32.add
    i32.const 48
    i32.sub
    local.set $res
    end $3$
    local.get $i
    i32.const 1
    i32.add
    local.set $i
    br $1$
    end $1$
    end $2$
    local.get $res
    return
  )

  ;; function 8
  (func $fib:main (export "fib_main")
    (param $args i32) (param $argv i32) (result i32) 
    (local $n i32) 
    local.get $argv
    i32.load offset=4 align=4
    call $fib:parseInt
    local.tee $n
    call $fib:fib
    return
  )

  ;; function 9
  (func $builtinFuncTest (result i32 i32 funcref i32)
  call $getPwartVersion
  call $native_index_size
  call $get_self_runtime_context
  i32.const 1
  call $ref_from_index
  call $i64_from_ref
  call $ref_from_i64
  call $ref_string_length
  )

  (export "addTwo" (func $addTwo))
  (export "test1" (func $test1))
  (export "test2" (func $test2))
  (export "test3" (func $test3))
  (export "builtinFuncTest" (func $builtinFuncTest))
)