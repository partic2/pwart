
#include <stdio.h>
#include <stdint.h>
#include <pwart.h>
#include <stdlib.h>


int main(int argc,char *argv[]){
    FILE *f=fopen("test1.wasm","rb");
    // 8MB buffer;
    uint8_t *data=malloc(1024*1024*8);
    int len=fread(data,1,1024*1024*8,f);
    pwart_module m=pwart_new_module();
    struct pwart_inspect_result1 ctxinfo;
    pwart_load(m,data,len);
    pwart_runtime_context ctx=pwart_get_runtime_context(m);
    pwart_inspect_runtime_context(ctx,&ctxinfo);
    pwart_wasmfunction fn=pwart_get_export_function(m,"test1");

    *((uint32_t *)ctxinfo.runtime_stack)=22;
    *((uint32_t *)ctxinfo.runtime_stack+1)=33;
    fn(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %d\n",(22+33)*2,*(uint32_t *)ctxinfo.runtime_stack);
    *((uint32_t *)ctxinfo.runtime_stack)=100;
    *((uint32_t *)ctxinfo.runtime_stack+1)=11;
    fn(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %d\n",100+11,*(uint32_t *)ctxinfo.runtime_stack);
    char *memstr=(uint8_t *)ctxinfo.memory;
    *(memstr+7)=0;
    printf("expect Hello, got %s\n",memstr+1);
    printf("expect %p, got %p\n",ctxinfo.table_entries[0],ctxinfo.table_entries[1]);

    fn=pwart_get_export_function(m,"test2");
    *((double *)ctxinfo.runtime_stack)=22;
    *((double *)ctxinfo.runtime_stack+1)=33;
    fn(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %lld\n",(22+33)*2,*(uint64_t *)ctxinfo.runtime_stack);

    return 0;
}