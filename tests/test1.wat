(module
(memory 4)
  (table 16 funcref )
  (elem 0 (i32.const 0) $addTwo )
  (type $typeAddTwo (func (param i64 i64) (result i64)))
  (type $longtype (func (param f64 f64 i32 i32 f32 f32 i64 i64) (result i64)))
  (global $one i32 (i32.const 1))
  (global $two i32 (i32.const 2))
  (func $nop
  )

  (func $addTwo (param i64 i64) (result i64) 
      (i64.store (i32.const 0) (local.get 0))
      (i64.load (i32.const 0))
      local.get 1
      i64.add
  )

  (func $addTwoF (param f64 f64) (result f64)
      (f64.store (i32.const 0) (local.get 0))
      (f64.load (i32.const 0))
      local.get 1
      f64.add
  )
    
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
    i32.const 0x6f6c6c65
    i32.store
    i32.const 6
    i32.const 1
    i32.const 5
    memory.copy
    i32.const 11
    i32.const 33
    i32.const 4
    memory.fill
    i32.const 15
    i32.const 0
    i32.store8
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

  (func $test2 (param i64 i64) (result i64) 
    i32.const 0
    local.get 0
    i64.store
    i32.const 0
    i64.load
    local.get 1
    i32.const 1
    call_indirect (type $typeAddTwo)
    i64.const 140
    call $addTwo
  )

  (func $test3 (param f64 f64) (result i64) 
    i32.const 0
    local.get 0
    f64.store
    i32.const 0
    f64.load
    local.get 1
    call $addTwoF
    i64.trunc_sat_f64_s
  )

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

  (export "test1" (func $test1))
  (export "test2" (func $test2))
  (export "test3" (func $test3))
)