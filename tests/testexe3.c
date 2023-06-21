#include <pwart.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>



#define puti32 pwart_rstack_put_i32
#define puti64 pwart_rstack_put_i64
#define putf32 pwart_rstack_put_f32
#define putf64 pwart_rstack_put_f64
#define putref pwart_rstack_put_ref

#define geti32 pwart_rstack_get_i32
#define geti64 pwart_rstack_get_i64
#define getf32 pwart_rstack_get_f32
#define getf64 pwart_rstack_get_f64
#define getref pwart_rstack_get_ref


FILE *cmdf=NULL;
char word[255];
pwart_module_state wmod=NULL;
void *stackbase = NULL;
pwart_wasm_function wfn=NULL;
void *sp;
int assertFail=0;
int shifted=0;

int next(){
    if(shifted){
        shifted=0;
        return 1;
    }
    if(feof(cmdf)){
        return 0;
    }else{
        fscanf(cmdf,"%s",word);
        return 1;
    }
}
int ifcmd(char *expect){
    return strcmp(expect,word)==0;
}

void cmdarg(){
    sp=stackbase;
    while(next()){
        if(ifcmd("i32")){
            next();
            puti32(&sp,atoi(word));
        }else if(ifcmd("i64")){
            next();
            puti64(&sp,atoll(word));
        }else if(ifcmd("f32")){
            next();
            putf32(&sp,strtof(word,NULL));
        }else if(ifcmd("f64")){
            next();
            putf64(&sp,strtod(word,NULL));
        }else if(ifcmd("end")){
            break;
        }else{
            printf("Not support type %s\n",word);
            abort();
        }
    }
}
void cmdresult(){
    sp=stackbase;
    printf("invoking...");
    pwart_call_wasm_function(wfn,sp);
    sp=stackbase;
    while(next()){
        if(ifcmd("i32")){
            next();
            int t=geti32(&sp);
            if(t!=atoi(word)){
                assertFail=1;
                printf("expect %s, got %d\n",word,t);
            }
        }else if(ifcmd("i64")){
            next();
            long long t=geti64(&sp);
            if(t!=atoll(word)){
                assertFail=1;
                printf("expect %s, got %lld\n",word,t);
            }
        }else if(ifcmd("f32")){
            next();
            int t=geti32(&sp);
            if(t!=atoi(word)){
                assertFail=1;
                printf("expect %s, got %d\n",word,t);
            }
        }else if(ifcmd("f64")){
            next();
            long long t=geti64(&sp);
            if(t!=atoll(word)){
                assertFail=1;
                printf("expect %s, got %lld\n",word,t);
            }
        }else if(ifcmd("end")){
            break;
        }else{
            printf("Not support type %s\n",word);
            abort();
        }
    }
}

char *cmdinvoke(){
    next();
    printf("function %s :",word);
    wfn=pwart_get_export_function(wmod,word);
    if(wfn==NULL){
        printf("function not found...\n");
        return "err";
    }
    sp=stackbase;
    assertFail=0;
    while(next()){
        if(ifcmd("arg")){
            cmdarg();
        }else if(ifcmd("result")){
            cmdresult();
            break;
        }else{
            shifted=1;
            break;
        }
    }
    if(assertFail){
        printf("failed...\n");
    }else{
        printf("pass...\n");
    }
}

char *cmdfile(){
    char filename[256]="test3/";
    next();
    strcat(filename,word);
    next();
    if(ifcmd("file")){
        printf("empty test, skip...\n");
        shifted=1;
        return NULL;
    }else{
        shifted=1;
    }
    printf("open %s ...\n",filename);
    FILE *f = fopen(filename, "rb");
    if(f==NULL){
        printf("file open failed.\n");
        return "err";
    }

    fseek(f,0,SEEK_END);
    int filesize=ftell(f);

    if(filesize>1024*1024*1024){
        printf(".wasm file too large(>1GB)\n");
        return "err";
    }
    fseek(f,0,SEEK_SET);

    uint8_t *data = malloc(filesize);

    pwart_namespace *ns=pwart_namespace_new();
    
    int len = fread(data, 1, filesize, f);
    fclose(f);
    char *err=NULL;
    pwart_module_state stat=pwart_namespace_define_wasm_module(ns,"__main__",data,len,&err);
    free(data);
    wmod=stat;
    if(err!=NULL){
        printf("error occur:%s\n",err);
        return "err";
    }
    if(err!=NULL){
        return err;
    }
    
    pwart_wasm_function fn=pwart_get_start_function(stat);
    if(fn!=NULL){
        pwart_call_wasm_function(fn,stackbase);
    }
    while(next()){
        if(ifcmd("invoke")){
            cmdinvoke();
        }else{
            shifted=1;
            break;
        }
    }
    pwart_namespace_delete(ns);
    return NULL;
}


char *parseCommandFile(){
    while(next()){
        if(ifcmd("file")){
            cmdfile();
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    
  stackbase=pwart_allocate_stack(64 * 1024);
  cmdf=fopen("test3/testcommand.txt","rb");
  parseCommandFile();
  pwart_free_stack(stackbase);
  fclose(cmdf);
  return 0;
}