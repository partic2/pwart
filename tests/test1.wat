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
      local.get 0
      local.get 1
      i64.add
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
    local.get 0
    local.get 1
    i32.const 1
    call_indirect (type $typeAddTwo)
    i64.const 140
    call $addTwo
  )
  (export "test1" (func $test1))
  (export "test2" (func $test2))
)