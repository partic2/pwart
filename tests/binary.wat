(module
  ;; i32
  (func (export "i32_add") (result i32)
    i32.const 1
    i32.const 2 
    i32.add)
  (func (export "i32_sub") (result i32) 
    i32.const 20
    i32.const 4
    i32.sub)
  (func (export "i32_mul") (result i32)
    i32.const 3
    i32.const 7
    i32.mul)
  (func (export "i32_div_s") (result i32)
    i32.const -4
    i32.const 2 
    i32.div_s)
  (func (export "i32_div_u") (result i32) 
    i32.const -4
    i32.const 2
    i32.div_u)
  (func (export "i32_rem_s") (result i32) 
    i32.const -5
    i32.const 2
    i32.rem_s)
  (func (export "i32_rem_u") (result i32)
    i32.const -5
    i32.const 2 
    i32.rem_u)
  (func (export "i32_and") (result i32)
    i32.const 11
    i32.const 5 
    i32.and)
  (func (export "i32_or") (result i32)
    i32.const 11
    i32.const 5 
    i32.or)
  (func (export "i32_xor") (result i32)
    i32.const 11
    i32.const 5 
    i32.xor)
  (func (export "i32_shl") (result i32)
    i32.const -100
    i32.const 3 
    i32.shl)
  (func (export "i32_shr_u") (result i32)
    i32.const -100
    i32.const 3 
    i32.shr_u)
  (func (export "i32_shr_s") (result i32)
    i32.const -100
    i32.const 3 
    i32.shr_s)
  (func (export "i32_rotl") (result i32)
    i32.const -100
    i32.const 3 
    i32.rotl)
  (func (export "i32_rotr") (result i32)
    i32.const -100 
    i32.const 3
    i32.rotr)

  ;; i64
  (func (export "i64_add") (result i64)
    i64.const 1
    i64.const 2 
    i64.add)
  (func (export "i64_sub") (result i64)
    i64.const 20
    i64.const 4 
    i64.sub)
  (func (export "i64_mul") (result i64)
    i64.const 3
    i64.const 7 
    i64.mul)
  (func (export "i64_div_s") (result i64)
    i64.const -4 
    i64.const 2
    i64.div_s)
  (func (export "i64_div_u") (result i64)
    i64.const -4
    i64.const 2 
    i64.div_u)
  (func (export "i64_rem_s") (result i64)
    i64.const -5
    i64.const 2 
    i64.rem_s)
  (func (export "i64_rem_u") (result i64)
    i64.const -5
    i64.const 2 
    i64.rem_u)
  (func (export "i64_and") (result i64)
    i64.const 11
    i64.const 5 
    i64.and)
  (func (export "i64_or") (result i64)
    i64.const 11
    i64.const 5 
    i64.or)
  (func (export "i64_xor") (result i64)
    i64.const 11
    i64.const 5 
    i64.xor)
  (func (export "i64_shl") (result i64) 
    i64.const -100
    i64.const 3
    i64.shl)
  (func (export "i64_shr_u") (result i64)
    i64.const -100
    i64.const 3 
    i64.shr_u)
  (func (export "i64_shr_s") (result i64)
    i64.const -100
    i64.const 3 
    i64.shr_s)
  (func (export "i64_rotl") (result i64)
    i64.const -100
    i64.const 3 
    i64.rotl)
  (func (export "i64_rotr") (result i64)
    i64.const -100
    i64.const 3 
    i64.rotr)

  ;; f32
  (func (export "f32_add") (result f32)
    f32.const 1.25
    f32.const 3.75
    f32.add)
  (func (export "f32_sub") (result f32)
    f32.const 4.5
    f32.const 1e4
    f32.sub)
  (func (export "f32_mul") (result f32)
    f32.const 1234.5
    f32.const -6.875
    f32.mul)
  (func (export "f32_div") (result f32)
    f32.const 1e14
    f32.const -2e5
    f32.div)
  (func (export "f32_min") (result f32)
    f32.const 0
    f32.const 0
    f32.min)
  (func (export "f32_max") (result f32)
    f32.const 0
    f32.const 0
    f32.max)
  (func (export "f32_copysign") (result f32)
    f32.const 0
    f32.const 0
    f32.copysign)

  ;; f64
  (func (export "f64_add") (result f64)
    f64.const 987654321
    f64.const 123456789
    f64.add)
  (func (export "f64_sub") (result f64)
    f64.const 1234e56
    f64.const 5.5e23
    f64.sub)
  (func (export "f64_mul") (result f64)
    f64.const -123e4
    f64.const 12341234
    f64.mul)
  (func (export "f64_div") (result f64)
    f64.const 1e200
    f64.const 1e50
    f64.div)
  (func (export "f64_min") (result f64)
    f64.const 0
    f64.const 0
    f64.min)
  (func (export "f64_max") (result f64)
    f64.const 0
    f64.const 0
    f64.max)
  (func (export "f64_copysign") (result f64)
    f64.const 0
    f64.const 0
    f64.copysign)
)