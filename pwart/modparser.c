#ifndef _PWART_MODPARSER_C
#define _PWART_MODPARSER_C

#include "def.h"
#include "wagen.c"

// Reads a string from the bytes array at pos that starts with a LEB length
// the length of string(include tailing 0) must less than maxlen, or the function read nothing.
// return the real length of the string(include tailing 0), no matter with 'maxlen'.
static uint32_t read_string(uint8_t *bytes, uint32_t *pos, uint32_t maxlen,char *buf) {
    uint32_t savedpos=*pos;
    uint32_t str_len = read_LEB(bytes, pos, 32);
    uint32_t copy_len=str_len;
    if(str_len+1>maxlen){
        *pos=savedpos;
        return str_len+1;
    }
    memcpy(buf,bytes+*pos,str_len);
    buf[str_len] = '\0';
    *pos += str_len;
    return str_len+1;
}


static void parse_table_type(Module *m, uint32_t *pos,Table *table) {
  uint32_t flags,tsize;
  table->elem_type = read_LEB(m->bytes, pos, 7);
  flags = read_LEB(m->bytes, pos, 32);
  tsize = read_LEB(m->bytes, pos, 32); // Initial size
  table->initial = tsize;
  table->size = tsize;
  // Limit maximum to 64K
  if (flags & 0x1) {
    tsize = read_LEB(m->bytes, pos, 32); // Max size
    table->maximum = tsize;
  } else {
    table->maximum = tsize;
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


static char *load_module(Module *m,uint8_t *bytes, uint32_t byte_count) {
    uint8_t   vt;
    uint32_t  pos = 0, word;
    struct pwart_builtin_functable *builtinfunc=pwart_get_builtin_functable();

    // Allocate the module

    if(sizeof(void *)==4){
        m->target_ptr_size=32;
    }else{
        m->target_ptr_size=64;
    }
    

    dynarr_init(&m->types,sizeof(Type));
    pool_init(&m->types_pool,256);
    pool_init(&m->string_pool,512);
    dynarr_init(&m->globals,sizeof(StackValue));
    dynarr_init(&m->exports,sizeof(Export));

    m->bytes = bytes;
    m->byte_count = byte_count;
    m->context=wa_calloc(sizeof(RuntimeContext));
    m->context->memory_model=m->cfg.memory_model;
    
    dynarr_init(&m->context->globals,sizeof(uint8_t));
    dynarr_init(&m->context->tables,sizeof(Table));
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
            char *name = m->import_name_buffer;
            if(read_string(bytes, &pos, 512,name)>500){
                return "section name too long,must less than 500.";
            }
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
                if(count2>255)return "type group too long.(should less than 256)";
                type->params=pool_alloc(&m->types_pool,count2+1);
                for (uint32_t p=0; p<count2; p++) {
                    type->params[p]=read_LEB(bytes, &pos, 32);
                }
                type->params[count2]=0;

                count2 = read_LEB(bytes, &pos, 32);
                if(count2>255)return "type group too long.(should less than 256)";
                type->results=pool_alloc(&m->types_pool,count2+1);
                for (uint32_t r=0; r<count2; r++) {
                    type->results[r] = read_LEB(bytes, &pos, 32);
                }
                type->results[count2]=0;
                wa_debug("type %d:\n",c);
                debug_printfunctype(type);
            }
        }break;
        case 2:
            wa_debug("Parsing Import(2) section (length: 0x%x)\n", slen);
            uint32_t import_count = read_LEB(bytes, &pos, 32);
            for (uint32_t gidx=0; gidx<import_count; gidx++) {
                void *val;
                uint32_t module_len, field_len;
                char *import_module,*import_field;
                import_module=m->import_name_buffer;
                module_len = read_string(bytes, &pos, 512,import_module);
                if(module_len>500){return "import name too long, module+field should less than 500 byte.";};
                import_field=import_module+module_len;
                field_len  = read_string(bytes, &pos, 512-module_len,import_field);
                if(module_len+field_len>500){return "import name too long, module+field should less than 500 byte.";};
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
                  parse_table_type(m, &pos,dynarr_push_type(&m->context->tables,Table)); break;
                case KIND_MEMORY:
                    parse_memory_type(m, &pos); break;
                case KIND_GLOBAL:
                    content_type = read_LEB(bytes, &pos, 7);
                    // TODO: use mutability
                    mutability = read_LEB(bytes, &pos, 1);
                    (void)mutability; break;
                }

                
                val=NULL;
                if(m->cfg.import_resolver!=NULL){
                    m->cfg.import_resolver(import_module,import_field,&val);
                }

                if(external_kind == KIND_FUNCTION && val == NULL && !strcmp("pwart_builtin",import_module)){
                    if(!strcmp("version",import_field)){
                        val=builtinfunc->version;
                    }else if(!strcmp("malloc32",import_field)){
                        val=builtinfunc->malloc32;
                    }else if(!strcmp("malloc64",import_field)){
                        val=builtinfunc->malloc64;
                    }else if(!strcmp("get_self_runtime_context",import_field)){
                        val=builtinfunc->get_self_runtime_context;
                    }
                }
                
                if(val==NULL){
                    return "import function not found";
                }

                wa_debug("  found '%s.%s' at address %p\n",
                      import_module, import_field, val);

                // Store in the right place
                switch (external_kind) {
                case KIND_FUNCTION:
                    fidx = m->functions->len;
                    m->import_count += 1;
                    WasmFunction *func = dynarr_push_type(&m->functions,WasmFunction);
                    func->tidx = type_index;
                    func->func_ptr = val;
                    break;
                case KIND_TABLE:
                {
                    Table *tval = val;
                    Table *dtab=dynarr_get(m->context->tables,Table,m->context->tables->len);
                    wa_assert(dtab->initial <= tval->maximum,
                        "Imported table is not large enough\n");
                    dtab->entries = val;
                    dtab->size = tval->size;
                    dtab->maximum = tval->maximum;
                    dtab->entries = tval->entries;
                    dtab->is_import=1;
                }
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
                wasmfunc->tidx = tidx;
                wa_debug("  function fidx: 0x%x, tidx: 0x%x\n",
                      f, tidx);
            }
        }break;
        case 4:
            wa_debug("Parsing Table(4) section\n");
            uint32_t table_count = read_LEB(bytes, &pos, 32);
            wa_debug("  table count: 0x%x\n", table_count);
            for(int i=0;i<table_count;i++){
                Table *table=dynarr_push_type(&m->context->tables,Table);
                parse_table_type(m, &pos,table);
                table->entries = wa_calloc(table->size*sizeof(void *));
                table->is_import=0;
            }
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
                wa_debug("globals %d ,type %d, value %lld\n",g,type,result);
            }
            pos = start_pos+slen;
            break;
        case 7:
            wa_debug("Parsing Export(7) section (length: 0x%x)\n", slen);
            uint32_t export_count = read_LEB(bytes, &pos, 32);
            for (uint32_t e=0; e<export_count; e++) {
                uint32_t strlen=read_string(bytes, &pos, 0,NULL);
                char *name;
                if(strlen>500){return "export name too long, should less than 500 byte.";}
                name=pool_alloc(&m->string_pool,strlen);
                read_string(bytes, &pos, 512,name);

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
                        exp->value = dynarr_get(m->context->tables,Table,index);
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
                uint32_t flags = read_LEB(bytes, &pos, 32);
                uint32_t index=0;
                uint32_t offset;
                void *elem_expr;
                switch(flags){
                    case 0:case 4:
                    index=0;
                    // Run the init_expr to get offset
                    m->pc=pos;waexpr_run_const(m,&offset);pos=m->pc;
                    break;
                    case 1:case 3:
                    index=0;
                    pos++; //extern_kind, ignored.
                    break;
                    case 2:
                    index=read_LEB(bytes, &pos, 32);
                    m->pc=pos;waexpr_run_const(m,&offset);pos=m->pc;
                    pos++;
                    break;
                    case 6:
                    index=read_LEB(bytes, &pos, 32);
                    m->pc=pos;waexpr_run_const(m,&elem_expr);pos=m->pc;
                    case 5:case 7:
                    m->pc=pos;waexpr_run_const(m,&elem_expr);pos=m->pc;
                }
                
                uint32_t num_elem = read_LEB(bytes, &pos, 32);
                Table *dtab=dynarr_get(m->context->tables,Table,index);
                wa_debug("  table.entries: %p, offset: 0x%x\n", dtab->entries, offset);

                for (uint32_t n=0; n<num_elem; n++) {
                    wa_debug("  write table entries %p, offset: 0x%x, n: 0x%x, addr: %p\n",
                            dtab->entries, offset, n, &dtab->entries[offset+n]);
                    dtab->entries[offset+n] = dynarr_get(m->functions,WasmFunction,read_LEB(bytes, &pos, 32));
                }
            }
            pos = start_pos+slen;
            break;
        // 9 and 11 are similar so keep them together, 10 is below 11
        case 11:
            wa_debug("Parsing Data(11) section (length: 0x%x)\n", slen);
            uint32_t seg_count = read_LEB(bytes, &pos, 32);
            for (uint32_t s=0; s<seg_count; s++) {
                uint32_t flags = read_LEB(bytes, &pos, 32);
                uint32_t midx;
                uint32_t offset;
                switch(flags){
                    case 0:  //active
                    midx=0;
                    break;
                    case 1:  //passive
                    return "passive segment not support yet";
                    break;
                    case 2:  //active with memory index
                    midx=read_LEB(bytes, &pos, 32);
                    break;
                    default:
                    return "Illegal segment flags.";
                }
                wa_assert(midx==0,"multimemory not support yet.");
                // Run the init_expr to get the offset
                m->pc=pos;
                waexpr_run_const(m,&offset);
                pos=m->pc;

                // Copy the data to the memory offset
                uint32_t size = read_LEB(bytes, &pos, 32);
                
                wa_debug("  setting %d bytes of memory at %p + offset %d\n",
                    size, m->context->memory.bytes, offset);
                memcpy(m->context->memory.bytes+offset, bytes+pos, size);
                pos += size;
                
            }

            break;
        case 10:
            wa_debug("Parsing Code(10) section (length: 0x%x)\n", slen);
            uint32_t body_count = read_LEB(bytes, &pos, 32);
            for (uint32_t b=0; b<body_count; b++) {
                wa_debug("generate code for function %d\n",b);
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
                function->locals_type = pool_alloc(&m->types_pool,local_count+1);
                //Read the locals
                pos = save_pos; 
                lidx = 0;
                for (uint32_t l=0; l<local_type_count; l++) {
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
    m->context->import_funcentries_count=m->import_count;
    for(int i=0;i<m->functions->len;i++){
        m->context->funcentries[i]=dynarr_get(m->functions,WasmFunction,i)->func_ptr;
    }
    
    //update table entries to real function entries.
    for(int i1=0;i1<m->context->tables->len;i1++){
        Table *tab=dynarr_get(m->context->tables,Table,i1);
        if(tab->elem_type==WVT_FUNC && tab->is_import==0){
            for(int i=0;i<tab->size;i++){
                if(tab->entries[i]!=NULL){
                    tab->entries[i]=((WasmFunction *)tab->entries[i])->func_ptr;
                }
            }
        }
    }

    return NULL;
}

static int free_runtimectx(RuntimeContext *rc){
    int i;
    if(rc->globals!=NULL){
        dynarr_free(&rc->globals);
    }
    if(rc->memory.bytes!=NULL){
        if(rc->memory.export_name!=NULL){
            wa_free(rc->memory.export_name);
            rc->memory.export_name=NULL;
        }
        wa_free(rc->memory.bytes);
        rc->memory.bytes=NULL;
    }
    if(rc->tables!=NULL){
        for(i=0;i<rc->tables->len;i++){
            Table *tab=dynarr_get(rc->tables,Table,i);
            if(tab->is_import==0 && tab->entries!=NULL){
                wa_free(tab->entries);
                tab->entries=NULL;
            }
        }
        dynarr_free(&rc->tables);
    }
    
    
    //only free code generate by module self.
    for(i=rc->import_funcentries_count;i<rc->funcentries_count;i++){
        pwart_FreeFunction(rc->funcentries[i]);
    }
    if(rc->funcentries!=NULL){
        wa_free(rc->funcentries);
        rc->funcentries=NULL;
    }
    
    
    wa_free(rc);
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
        dynarr_free(&m->exports);
    }
    pool_free(&m->string_pool);
    if(m->functions!=NULL){
        dynarr_free(&m->functions);
    }
    if(m->types_pool.chunks!=NULL){
        pool_free(&m->types_pool);
    }
    wa_free(m);
    return 0;
}



#endif