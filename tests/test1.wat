(module
(memory 4)
  (table 16 funcref )
  (elem 0 (i32.const 0) $addTwo )
  (type $typeAddTwo (func (param i32 i32) (result i32)))

  (func $nop
  )

  (func $addTwo (param f64 f64) (result i64) 
      local.get 0
      local.get 1
      f64.add
      i64.trunc_f64_s(f64)
  )
    
  (func $test1 (param i32 i32) (result i32) (local i32)
    i32.const 1
    i32.const 0
    table.get 0
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
    block $exit
      local.get 0
      local.get 1
      i32.add
      local.tee 2
      i32.const 100
      i32.ge_u
      br_if $exit
      local.get 2
      i32.const 2
      i32.mul
      return
    end
    local.get 2
    return
  )

  (func $test2 (param f64 f64) (result i64) 
    call_indirect (type $typeAddTwo)
  )
  (export "test1" (func $test1))
  (export "test2" (func $test1))
)