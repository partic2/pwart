

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pwart_syslib.h>

int main(int argc,char **argv){
    pwart_wasi_module_set_wasiargs(argc-1,argv+1);
    if(argc>=2){
        char *modpath=argv[1];
        void *stackbase = pwart_allocate_stack(64 * 1024);
        char *err=NULL;
        void *sp;
        long filesize;
        FILE *f;
        int len;
        int returncode;

        f = fopen(modpath, "rb");

        fseek(f,0,SEEK_END);
        filesize=ftell(f);

        if(filesize>1024*1024*1024){
            printf(".wasm file too large(>1GB)\n");
            return 1;
        }
        fseek(f,0,SEEK_SET);

        uint8_t *data = malloc(filesize);

        pwart_namespace *ns=pwart_namespace_new();
        
        err=pwart_wasi_module_init();
        if(err!=NULL){
            printf("warning:%s uvwasi will not load",err);
        }
        pwart_syslib_load(ns);
        len = fread(data, 1, filesize, f);
        fclose(f);
        pwart_module_state stat=pwart_namespace_define_wasm_module(ns,"__main__",data,len,&err);
        free(data);
        if(err!=NULL){
            printf("error occur:%s\n",err);
            return 1;
        }
        struct pwart_wasm_memory *mem=pwart_get_export_memory(stat,"memory");
        pwart_wasi_module_set_wasimemory(mem);
        pwart_wasm_function fn=pwart_get_start_function(stat);
        if(fn!=NULL){
            pwart_call_wasm_function(fn,stackbase);
        }
        fn=pwart_get_export_function(stat,"_start");
        if(fn!=NULL){
            pwart_call_wasm_function(fn,stackbase);
        }else{
            printf("%s\n","'_start' function not found. ");
        }
        pwart_free_stack(stackbase);
        return 0;
    }else{
        printf("No input .wasm file.\n");
        return 1;
    }
}