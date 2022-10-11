(module
  (func $brif0 (param i32) (result i32)
    (block $exit (result i32)
      (i32.sub
        (br_if $exit (i32.const 42) (local.get 0))
        (i32.const 13))))

  (func (export "brif1") (result i32)
    (call $brif0 (i32.const 0)))

  (func (export "brif2") (result i32)
    (call $brif0 (i32.const 1)))


  (func $loop0 (param i32) (result i32)
    (local i32)
    i32.const 0
    local.set 1
    loop $cont
    block $nest1
      local.get 1
      i32.const 1
      i32.add
      local.set 1
    block $nest2
      local.get 1
      local.get 0
      i32.lt_s
      br_if $cont
      br $nest2
    end
    end
    end
    local.get 1
    return)

  (func (export "loop1") (result i32)
    i32.const 3
    call $loop0)

  (func (export "loop2") (result i32)
    i32.const 10
    call $loop0)

  (func (export "if1") (result i32) (local i32)
    i32.const 0
    local.set 0
    i32.const 1
    if
      local.get 0
      i32.const 1
      i32.add
      local.set 0 
    end
    i32.const 0
    if
      local.get 0
      i32.const 1
      i32.add
      local.set 0 
    end
    local.get 0
    return)

  (func (export "if2") (result i32) (local i32 i32)

    i32.const 1
    if
      i32.const 1 
      local.set 0
    else
      i32.const 2
      local.set 0
    end
    i32.const 0
    if
      i32.const 4
      local.set 1
    else
      i32.const 8
      local.set 1
    end
    local.get 0
    local.get 1
    i32.add
    return)

  (func $brtable0 (param i32) (result i32)
    block $default
      block $2
        block $1
          block $0
            local.get 0
            br_table $0 $1 $2 $default
          end
          ;; 0
          i32.const 0
          return
        end
        ;; 1
        i32.const 1
        return 
       end
      end
      ;; 2, fallthrough
      
    ;; default
    i32.const 2
    return)

  (func (export "brtable1") (result i32)
    i32.const 0 
    call $brtable0)
  (func (export "brtable2") (result i32) 
    i32.const 1
    call $brtable0)
  (func (export "brtable3") (result i32)
    i32.const 2 
    call $brtable0)
)