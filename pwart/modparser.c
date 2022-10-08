#ifndef _PWART_MODPARSER_C
#define _PWART_MODPARSER_C

#include "def.h"
#include "wagen.c"


static void parse_table_type(Module *m, uint32_t *pos) {
  m->context->table.elem_type = read_LEB(m->bytes, pos, 7);
  uint32_t flags = read_LEB(m->bytes, pos, 32);
  uint32_t tsize = read_LEB(m->bytes, pos, 32); // Initial size
  m->context->table.initial = tsize;
  m->context->table.size = tsize;
  // Limit maximum to 64K
  if (flags & 0x1) {
    tsize = read_LEB(m->bytes, pos, 32); // Max size
    m->context->table.maximum = tsize;
  } else {
    m->context->table.maximum = tsize;
  }
  wa_debug("  table size: %d\n", tsize);
}

static void parse_memory_type(Module *m, uint32_t *pos) {
  uint32_t flags = read_LEB(m->bytes, pos, 32);
  uint32_t pages = read_LEB(m->bytes, pos, 32); // Initial size
  m->context->memory.initial = pages;
  m->context->memory.pages = pages;
  // Limit the maximum to 2GB
  if (flags & 0x1) {
    pages = read_LEB(m->bytes, pos, 32); // Max size
    m->context->memory.maximum = pages;
  } else {
    m->context->memory.maximum = pages;
  }
}

static Export *get_export(Module *m, char *name, uint32_t kind) {
    // Find export index by name and return the value
    for (uint32_t e=0; e<m->exports->len; e++) {
        Export *exp=dynarr_get(m->exports,Export,e);
        char *ename = exp->export_name;
        if (!ename) { continue; }
        if (strncmp(name, ename, 1024) == 0) {
            return exp;
        }
    }
    return NULL;
}


static Memory *get_export_memory(Module *m, char *name) {
  if (strncmp(name, m->context->memory.export_name, 1024) == 0) {
    return &m->context->memory;
  }
  return NULL;
}


static int load_module(Module *m,uint8_t *bytes, uint32_t byte_count) {
    uint8_t   vt;
    uint32_t  pos = 0, word;

    // Allocate the module

    if(sizeof(void *)==4){
        m->target_ptr_size=32;
    }else{
        m->target_ptr_size=64;
    }
    

    dynarr_init(&m->types,sizeof(Type));
    dynarr_init(&m->types_pool,sizeof(uint8_t));
    dynarr_init(&m->globals,sizeof(uint8_t));
    dynarr_init(&m->exports,sizeof(Export));

    m->bytes = bytes;
    m->byte_count = byte_count;
    m->context=wa_calloc(sizeof(RuntimeContext));
    if(m->default_stack_size==0){
        m->default_stack_size=PAGE_SIZE-1024;
    }
    m->context->stack_buffer=wa_malloc(m->default_stack_size+16);
    m->context->stack_start_offset=
        (((sljit_sw)m->context->stack_buffer)+15) & (~(sljit_sw)(0xf))-
        (sljit_sw)m->context->stack_buffer;
    
    dynarr_init(&m->context->globals,sizeof(StackValue));
    dynarr_init(&m->functions,sizeof(WasmFunction));
    m->start_function = -1;

    // Check the module
    pos = 0;
    word = read_uint32(bytes, &pos);
    wa_assert(word == WA_MAGIC, "Wrong module magic\n");
    word = read_uint32(bytes, &pos);
    wa_assert(word == WA_VERSION, "Wrong module version\n");

    // Read the sections
    while (pos < byte_count) {
        uint32_t id = read_LEB(bytes, &pos, 7);
        uint32_t slen = read_LEB(bytes, &pos, 32);
        int start_pos = pos;
        wa_debug("Reading section %d at 0x%x, length %d\n", id, pos, slen);
        switch (id) {
        case 0:
            wa_debug("Parsing Custom(0) section (length: 0x%x)\n", slen);
            uint32_t end_pos = pos+slen;
            char *name = read_string(bytes, &pos, NULL);
            wa_debug("  Section name '%s'\n", name);
            if (strncmp(name, "dylink", 7) == 0) {
                // https://github.com/WebAssembly/tool-conventions/blob/master/DynamicLinking.md
                // TODO: make use of these
                uint32_t memorysize = read_LEB(bytes, &pos, 32);
                uint32_t tablesize = read_LEB(bytes, &pos, 32);
                (void)memorysize; (void)tablesize;
            } else {
                wa_debug("Ignoring unknown custom section '%s'\n", name);
            }
            pos = end_pos;
            break;
        case 1:
        {
            int count,count2;
            wa_debug("Parsing Type(1) section (length: 0x%x)\n", slen);
            count = read_LEB(bytes, &pos, 32);
            m->types=NULL;
            dynarr_init(&m->types,sizeof(Type));
            for (uint32_t c=0; c<count; c++) {
                Type *type = dynarr_push_type(&m->types,Type);
                type->form = read_LEB(bytes, &pos, 7);
                count2 = read_LEB(bytes, &pos, 32);
                type->params=dynarr_push(&m->types_pool,count2+1);
                for (uint32_t p=0; p<count2; p++) {
                    type->params[p]=read_LEB(bytes, &pos, 32);
                }
                type->params[count2]=0;

                count2 = read_LEB(bytes, &pos, 32);
                type->results=dynarr_push(&m->types_pool,count2+1);
                for (uint32_t r=0; r<count2; r++) {
                    type->results[r] = read_LEB(bytes, &pos, 32);
                }
                type->results[count2]=0;
                wa_debug("type %d:\n",c+1);
                debug_printfunctype(type);
            }
        }break;
        case 2:
            wa_debug("Parsing Import(2) section (length: 0x%x)\n", slen);
            uint32_t import_count = read_LEB(bytes, &pos, 32);
            for (uint32_t gidx=0; gidx<import_count; gidx++) {
                uint32_t module_len, field_len;
                char *import_module = read_string(bytes, &pos, &module_len);
                char *import_field = read_string(bytes, &pos, &field_len);

                uint32_t external_kind = bytes[pos++];

                wa_debug("  import: %d/%d, external_kind: %d, %s.%s\n",
                      gidx, import_count, external_kind, import_module,
                      import_field);

                uint32_t type_index = 0, fidx;
                uint8_t content_type = 0, mutability;

                switch (external_kind) {
                case KIND_FUNCTION:
                    type_index = read_LEB(bytes, &pos, 32); break;
                case KIND_TABLE:
                    parse_table_type(m, &pos); break;
                case KIND_MEMORY:
                    parse_memory_type(m, &pos); break;
                case KIND_GLOBAL:
                    content_type = read_LEB(bytes, &pos, 7);
                    // TODO: use mutability
                    mutability = read_LEB(bytes, &pos, 1);
                    (void)mutability; break;
                }

                void *val;
                char *err, *sym = wa_malloc(module_len + field_len + 5);

                if(m->symbol_resolver!=NULL){
                    m->symbol_resolver(import_module,import_field,&val);
                }

                wa_debug("  found '%s.%s' as symbol '%s' at address %p\n",
                      import_module, import_field, sym, val);
                wa_free(sym);

                // Store in the right place
                switch (external_kind) {
                case KIND_FUNCTION:
                    fidx = m->functions->len;
                    m->import_count += 1;
                    WasmFunction *func = dynarr_push_type(&m->functions,WasmFunction);
                    func->type = dynarr_get(m->types,Type,type_index);
                    func->func_ptr = val;
                    break;
                case KIND_TABLE:
                    wa_assert(!m->context->table.entries,
                           "More than 1 table not supported\n");
                    Table *tval = val;
                    m->context->table.entries = val;
                    wa_assert(m->context->table.initial <= tval->maximum,
                        "Imported table is not large enough\n");
                    m->context->table.entries = val;
                    m->context->table.size = tval->size;
                    m->context->table.maximum = tval->maximum;
                    m->context->table.entries = tval->entries;
                    break;
                case KIND_MEMORY:
                    wa_assert(!m->context->memory.bytes,
                           "More than 1 memory not supported\n");
                    Memory *mval = val;
                    wa_assert(m->context->memory.initial <= mval->maximum,
                        "Imported memory is not large enough\n");
                    wa_debug("  setting memory pages: %d, max: %d, bytes: %p\n",
                         mval->pages, mval->maximum, mval->bytes);
                    m->context->memory.pages = mval->pages;
                    m->context->memory.maximum = mval->maximum;
                    m->context->memory.bytes = mval->bytes;
                    break;
                case KIND_GLOBAL:
                {
                    StackValue *sv;
                    sv=dynarr_push_type(&m->globals,StackValue);
                    sv->wasm_type=content_type;
                    sv->val.opw=m->context->globals->len;
                    switch (content_type) {
                    case WVT_I32:
                    case WVT_F32:
                     *(uint32_t *)dynarr_push(&m->context->globals,4)=*(uint32_t *)val;
                      break;
                    case WVT_I64: 
                    case WVT_F64: 
                    *(uint64_t *)dynarr_push(&m->context->globals,8)=*(uint64_t *)val;
                      break;
                    default:
                    *(sljit_sw *)dynarr_push(&m->context->globals,sizeof(sljit_sw))=*(sljit_sw *)val;
                      break;
                    }
                }break;
                default:
                    wa_debug("Import of kind %d not supported\n", external_kind);
                    break;
                }
            }
            break;
        case 3:{
            WasmFunction *wasmfunc;
            uint32_t localFuncCount = read_LEB(bytes, &pos, 32);
            wa_debug("Parsing Function(3) section (length: 0x%x)\n", slen);

            for (uint32_t f=0; f<localFuncCount; f++) {
                wasmfunc=dynarr_push_type(&m->functions,WasmFunction);
                uint32_t tidx = read_LEB(bytes, &pos, 32);
                wasmfunc->fidx = f;
                wasmfunc->type = dynarr_get(m->types,Type,tidx);
                wa_debug("  function fidx: 0x%x, tidx: 0x%x\n",
                      f, tidx);
            }
        }break;
        case 4:
            wa_debug("Parsing Table(4) section\n");
            uint32_t table_count = read_LEB(bytes, &pos, 32);
            wa_debug("  table count: 0x%x\n", table_count);
            wa_assert(table_count == 1, "More than 1 table not supported");

            parse_table_type(m, &pos);
            m->context->table.entries = wa_calloc(m->context->table.size*sizeof(void *));
            break;
        case 5:
            wa_debug("Parsing Memory(5) section\n");
            uint32_t memory_count = read_LEB(bytes, &pos, 32);
            wa_debug("  memory count: 0x%x\n", memory_count);
            wa_assert(memory_count == 1, "More than 1 memory not supported\n");

            parse_memory_type(m, &pos);
            //we allocate memory only once, due to jit not allow to change the memory base. For performance reason.
            m->context->memory.bytes = wa_calloc(m->context->memory.maximum*PAGE_SIZE);
            break;
        case 6:
            wa_debug("Parsing Global(6) section\n");
            uint32_t global_count = read_LEB(bytes, &pos, 32);
            for (uint32_t g=0; g<global_count; g++) {
                StackValue *sv;
                // Same allocation Import of global above
                uint8_t type = read_LEB(bytes, &pos, 7);
                // TODO: use mutability
                uint8_t mutability = read_LEB(bytes, &pos, 1);
                (void)mutability;
                uint64_t result;
                uint32_t gidx = m->globals->len;
                sv=dynarr_push_type(&m->globals,StackValue);
                sv->wasm_type=type;
                sv->val.opw=m->context->globals->len;

                // Run the init_expr to get global value
                m->pc=pos;
                waexpr_run_const(m,&result);
                pos=m->pc;
                switch (type) {
                    case WVT_I32:
                    case WVT_F32:
                     *(uint32_t *)dynarr_push(&m->context->globals,4)=(uint32_t)result;
                      break;
                    case WVT_I64: 
                    case WVT_F64: 
                    *(uint64_t *)dynarr_push(&m->context->globals,8)=result;
                      break;
                    default:
                    *(sljit_sw *)dynarr_push(&m->context->globals,sizeof(sljit_sw))=(sljit_sw)result;
                      break;
                    }
            }
            pos = start_pos+slen;
            break;
        case 7:
            wa_debug("Parsing Export(7) section (length: 0x%x)\n", slen);
            uint32_t export_count = read_LEB(bytes, &pos, 32);
            for (uint32_t e=0; e<export_count; e++) {
                char *name = read_string(bytes, &pos, NULL);

                uint32_t external_kind = bytes[pos++];
                uint32_t index = read_LEB(bytes, &pos, 32);
                uint32_t eidx = m->exports->len;
                {
                    Export *exp=dynarr_push_type(&m->exports,Export);
                    exp->external_kind=external_kind;
                    exp->export_name=name;
                    switch (external_kind) {
                    case KIND_FUNCTION:
                        exp->value = dynarr_get(m->functions,WasmFunction,index);
                        break;
                    case KIND_TABLE:
                        wa_assert(index == 0, "Only 1 table in MVP");
                        exp->value = &m->context->table;
                        break;
                    case KIND_MEMORY:
                        wa_assert(index == 0, "Only 1 memory in MVP");
                        exp->value = &m->context->memory;
                        break;
                    case KIND_GLOBAL:
                        exp->value = &m->context->globals->data+dynarr_get(m->globals,StackValue,index)->val.opw;
                        break;
                    }
                }
                wa_debug("  export: %s (0x%x)\n", name, index);
            }
            break;
        case 8:
            wa_debug("Parsing Start(8) section (length: 0x%x)\n", slen);
            m->start_function = read_LEB(bytes, &pos, 32);
            break;
        case 9:
            wa_debug("Parsing Element(9) section (length: 0x%x)\n", slen);
            uint32_t element_count = read_LEB(bytes, &pos, 32);

            for(uint32_t c=0; c<element_count; c++) {
                uint32_t index = read_LEB(bytes, &pos, 32);
                uint32_t offset;
                wa_assert(index == 0, "Only 1 default table in MVP");

                // Run the init_expr to get offset
                m->pc=pos;
                waexpr_run_const(m,&offset);
                pos=m->pc;
                
                uint32_t num_elem = read_LEB(bytes, &pos, 32);
                wa_debug("  table.entries: %p, offset: 0x%x\n", m->context->table.entries, offset);

                for (uint32_t n=0; n<num_elem; n++) {
                    wa_debug("  write table entries %p, offset: 0x%x, n: 0x%x, addr: %p\n",
                            m->context->table.entries, offset, n, &m->context->table.entries[offset+n]);
                    m->context->table.entries[offset+n] = dynarr_get(m->functions,WasmFunction,read_LEB(bytes, &pos, 32));
                }
            }
            pos = start_pos+slen;
            break;
        // 9 and 11 are similar so keep them together, 10 is below 11
        case 11:
            wa_debug("Parsing Data(11) section (length: 0x%x)\n", slen);
            uint32_t seg_count = read_LEB(bytes, &pos, 32);
            for (uint32_t s=0; s<seg_count; s++) {
                uint32_t midx = read_LEB(bytes, &pos, 32);
                uint32_t offset;
                wa_assert(midx == 0, "Only 1 default memory in MVP");

                // Run the init_expr to get the offset
                m->pc=pos;
                waexpr_run_const(m,&offset);
                pos=m->pc;

                

                // Copy the data to the memory offset
                uint32_t size = read_LEB(bytes, &pos, 32);
                
                wa_debug("  setting 0x%x bytes of memory at 0x%x + offset 0x%x\n",
                     size, m->context->memory.bytes, offset);
                memcpy(m->context->memory.bytes+offset, bytes+pos, size);
                pos += size;
            }

            break;
        case 10:
            wa_debug("Parsing Code(10) section (length: 0x%x)\n", slen);
            uint32_t body_count = read_LEB(bytes, &pos, 32);
            for (uint32_t b=0; b<body_count; b++) {
                WasmFunction *function = dynarr_get(m->functions,WasmFunction,m->import_count+b);
                uint32_t body_size = read_LEB(bytes, &pos, 32);
                uint32_t payload_start = pos;
                uint32_t local_count=0;
                uint32_t local_type_count= read_LEB(bytes, &pos, 32);
                uint32_t save_pos, tidx, lidx, lecount;

                // Local variable handling
                // Get number of locals for alloc
                save_pos = pos;
                local_count = 0;
                for (uint32_t l=0; l<local_type_count; l++) {
                    lecount = read_LEB(bytes, &pos, 32);
                    local_count += lecount;
                    tidx =  read_LEB(bytes, &pos, 7);
                    (void)tidx; // TODO: use tidx?
                }
                function->locals_type = dynarr_push(&m->types_pool,local_count+1);
                //Read the locals
                pos = save_pos; 
                lidx = 0;
                for (uint32_t l=0; l<local_count; l++) {
                    lecount = read_LEB(bytes, &pos, 32);
                    vt = read_LEB(bytes, &pos, 7);
                    for (uint32_t l=0; l<lecount; l++) {
                        function->locals_type[lidx]=vt;
                        lidx++;
                    }
                }
                m->pc=pos;
                pwart_EmitFunction(m,function);
                pos=m->pc;
                wa_assert(bytes[pos-1] == 0x0b,
                       "Code section did not end with 0x0b\n");
            }
            break;
        default:
            wa_debug("Section %d unimplemented\n", id);
            pos += slen;
            break;
        }

    }

    //set functionentry
    m->context->funcentries=wa_calloc(m->functions->len*sizeof(WasmFunctionEntry));
    m->context->funcentries_count=m->functions->len;
    for(int i=0;i<m->functions->len;i++){
        m->context->funcentries[i]=dynarr_get(m->functions,WasmFunction,i)->func_ptr;
    }

    //update table entries to real function entries.
    for(int i=0;i<m->context->table.size;i++){
        if(m->context->table.entries[i]!=NULL){
            m->context->table.entries[i]=((WasmFunction *)m->context->table.entries[i])->func_ptr;
        }
    }

    if (m->start_function != -1) {
        uint32_t fidx = m->start_function;
        WasmFunction *entf=dynarr_get(m->functions,WasmFunction,fidx);
        entf->func_ptr(m->context->stack_buffer+m->context->stack_start_offset,m->context);
    }

    return 0;
}

static int free_runtimectx(RuntimeContext *rc){
    if(rc->globals!=NULL){
        dynarr_free(&rc->globals);
    }
    if(rc->stack_buffer!=NULL){
        wa_free(rc->stack_buffer);
    }
    if(rc->memory.bytes!=NULL){
        if(rc->memory.export_name!=NULL){
            wa_free(rc->memory.export_name);
            rc->memory.export_name=NULL;
        }
        wa_free(rc->memory.bytes);
    }
    if(rc->table.entries!=NULL){
        wa_free(rc->table.entries);
        rc->table.entries=NULL;
    }
    for(int i=0;i<rc->funcentries_count;i++){
        pwart_FreeFunction(rc->funcentries[i]);
    }
    return 0;
}

static int free_module(Module *m){
    if(m->types!=NULL){
        dynarr_free(&m->types);
    }
    if(m->globals!=NULL){
        dynarr_free(&m->globals);
    }
    if(m->exports!=NULL){
        for(int i=0;i<m->exports->len;i++){
            Export *exp=dynarr_get(m->exports,Export,i);
            if(exp->export_name!=NULL){
                wa_free(exp->export_name);
                exp->export_name=NULL;
            }
        }
        dynarr_free(&m->exports);
    }
    if(m->functions!=NULL){
        dynarr_free(&m->functions);
    }
    return 0;
}



#endif