
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
    pwart_wasmfunction fn=pwart_get_export_function(m,"addTwo");
    *((uint32_t *)ctxinfo.runtime_stack)=22;
    *((uint32_t *)ctxinfo.runtime_stack+1)=33;
    fn(ctxinfo.runtime_stack,ctx);
    printf("expected %d, get %d\n",22+33,*(uint32_t *)ctxinfo.runtime_stack);
    return 0;
}