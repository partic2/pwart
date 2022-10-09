
#include <stdio.h>
#include <stdint.h>
#include <pwart.h>
#include <stdlib.h>
#include <string.h>

static void test_wasmfn(uint64_t *fp,pwart_runtime_context c){
    printf("test_wasmfn stack at %p\n",fp);
    *fp=0;
}

//base test
int test1(){
    FILE *f=fopen("test1.wasm","rb");
    // 8MB buffer;
    uint8_t *data=malloc(1024*1024*8);
    int len=fread(data,1,1024*1024*8,f);
    pwart_module m=pwart_new_module();
    struct pwart_inspect_result1 ctxinfo;
    pwart_load(m,data,len);
    pwart_runtime_context ctx=pwart_get_runtime_context(m);
    pwart_inspect_runtime_context(ctx,&ctxinfo);

    printf("runtime stack at %p\n",ctxinfo.runtime_stack);

    pwart_wasmfunction test1=pwart_get_export_function(m,"test1");

    pwart_wasmfunction test2=pwart_get_export_function(m,"test2");

    pwart_free_module(m);

    *((uint32_t *)ctxinfo.runtime_stack)=22;
    *((uint32_t *)ctxinfo.runtime_stack+1)=33;
    test1(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %d\n",(22+33)*2,*(uint32_t *)ctxinfo.runtime_stack);
    if((22+33)*2!=*(uint32_t *)ctxinfo.runtime_stack){
        return 0;
    }
    *((uint32_t *)ctxinfo.runtime_stack)=100;
    *((uint32_t *)ctxinfo.runtime_stack+1)=11;
    test1(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %d\n",100+11,*(uint32_t *)ctxinfo.runtime_stack);
    if(100+11!=*(uint32_t *)ctxinfo.runtime_stack){
        return 0;
    }

    char *memstr=(uint8_t *)ctxinfo.memory;
    *(memstr+7)=0;
    printf("expect Hello, got %s\n",memstr+1);
    if(strcmp(memstr+1,"Hello")!=0){
        return 0;
    }
    printf("expect %p, got %p , %p\n",ctxinfo.table_entries[0],ctxinfo.table_entries[1],ctxinfo.table_entries[2]);
    if(ctxinfo.table_entries[0]!=ctxinfo.table_entries[1]){
        return 0;
    }
    if(ctxinfo.table_entries[0]!=ctxinfo.table_entries[2]){
        return 0;
    }

    *((uint64_t *)ctxinfo.runtime_stack)=22;
    *((uint64_t *)ctxinfo.runtime_stack+1)=33;
    test2(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %lld\n",(22+33+140),*(uint64_t *)ctxinfo.runtime_stack);
    if(22+33+140!=*(uint64_t *)ctxinfo.runtime_stack){
        return 0;
    }

    ctxinfo.table_entries[1]=&test_wasmfn;
    *((uint64_t *)ctxinfo.runtime_stack)=22;
    *((uint64_t *)ctxinfo.runtime_stack+1)=33;
    test2(ctxinfo.runtime_stack,ctx);
    printf("expect %d, got %lld\n",140,*(uint64_t *)ctxinfo.runtime_stack);
    if(140!=*(uint64_t *)ctxinfo.runtime_stack){
        return 0;
    }
    pwart_free_runtime(ctx);
    return 1;
}

int main(int argc,char *argv[]){
    if(test1()){
        printf("test1 pass\n");
    }else{
        printf("test1 failed\n");
    }
    return 0;
}