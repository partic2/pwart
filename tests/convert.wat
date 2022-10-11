(module
  ;; i32
  (func (export "i32_wrap_i64") (result i32) 
    i64.const -1
    i32.wrap_i64)
  (func (export "i32_trunc_s_f32") (result i32)
    f32.const -100.12345 
    i32.trunc_f32_s)
  (func (export "i32_trunc_u_f32") (result i32)
    f32.const 3e9 
    i32.trunc_f32_u)
  (func (export "i32_trunc_s_f64") (result i32)
    f64.const -100.12345 
    i32.trunc_f64_s)
  (func (export "i32_trunc_u_f64") (result i32)
    f64.const 3e9 
    i32.trunc_f64_u)

  ;; i64
  (func (export "i64_extend_u_i32") (result i64) 
    i32.const -1
    i64.extend_i32_u)
  (func (export "i64_extend_s_i32") (result i64)
    i32.const -1 
    i64.extend_i32_s)

  (func (export "i64_trunc_s_f32") (result i32)
     f32.const -100.12345
     i64.trunc_f32_s 
     i64.const -100
     i64.eq)
  (func (export "i64_trunc_u_f32") (result i32)
     f32.const 3e9
     i64.trunc_f32_u
     i64.const 3000000000
     i64.eq)
  (func (export "i64_trunc_s_f64") (result i32)
     f64.const -100.12345
     i64.trunc_f64_s
     i64.const -100
     i64.eq)
  (func (export "i64_trunc_u_f64") (result i32)
     f64.const 3e9
     i64.trunc_f64_u
     i64.const 3000000000
     i64.eq)

  ;; f32
  (func (export "f32_convert_s_i32") (result f32)
    i32.const -1 
    f32.convert_i32_s)
  (func (export "f32_convert_u_i32") (result f32) 
    i32.const -1
    f32.convert_i32_u)
  (func (export "f32_demote_f64") (result f32)
    f64.const 12345678.9 
    f32.demote_f64)
  (func (export "f32_convert_s_i64") (result f32) 
    i64.const 0
    f32.convert_i64_s)
  (func (export "f32_convert_u_i64") (result f32) 
    i64.const 0
    f32.convert_i64_u)

  ;; f64
  (func (export "f64_convert_s_i32") (result f64)
    i32.const -1 
    f64.convert_i32_s)
  (func (export "f64_convert_u_i32") (result f64) 
    i32.const -1
    f64.convert_i32_u)
  (func (export "f64_demote_f32") (result f64)
    f32.const 12345678.9 
    f64.promote_f32)
  (func (export "f64_convert_s_i64") (result f64) 
    i64.const 0
    f64.convert_i64_s)
  (func (export "f64_convert_u_i64") (result f64) 
    i64.const 0
    f64.convert_i64_u)

  (func (export "f32_reinterpret_i32") (result f32)
    i32.const 0x40900000
    f32.reinterpret_i32)

  (func (export "i32_reinterpret_f32") (result i32)
    f32.const -3.5
    i32.reinterpret_f32)

  (func (export "f64_reinterpret_i64") (result f64)
    i64.const 0x405f480000000000
    f64.reinterpret_i64)

  (func (export "i64_reinterpret_f64") (result i64)
    f64.const 1.375e10
    i64.reinterpret_f64)
)