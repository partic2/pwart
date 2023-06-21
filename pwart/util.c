#ifndef _PWART_UTIL_C

#define _PWART_UTIL_C

#ifndef DEBUG_BUILD
#define DEBUG_BUILD 0
#endif

#define PWART_DEBUG_RUNTIME_PROBE 0

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <limits.h>
#include <string.h>

#include <stdarg.h>

#include "../sljit/sljit_src/sljitLir.h"


#define ReturnIfErr(expr) {char *err=expr;if(err!=NULL){wa_debug("error occure:%s\n",err);return err;}}



static void *wa_malloc(size_t size) {
    void *res = malloc(size);
    return res;
}

static void *wa_realloc(void *p,size_t nsize){
    void *res=realloc(p,nsize);
    return res;
}

static void wa_free(void *p){
    free(p);
}

static void *wa_calloc(size_t size){
    return calloc(size,1);
}


static void wa_debug(char *fmt,...){
    #if DEBUG_BUILD
    va_list args;
    va_start(args,fmt);
    vprintf(fmt,args);
    va_end(args);
    fflush(stdout);
    #endif
}

static void wa_abort(){
    printf("aborted...\n");
    exit(1);
}

static void wa_assert(int cond,char *failInfo){
    if(!cond){
        wa_debug("assert fail due to %s\n",failInfo);
        wa_abort();
    }
}

// type readers

static uint64_t read_LEB_(uint8_t *bytes, uint32_t *pos, uint32_t maxbits, int sign) {
    uint64_t result = 0;
    uint32_t shift = 0;
    uint32_t bcnt = 0;
    uint32_t startpos = *pos;
    uint64_t byte;

    while (1) {
        byte = bytes[*pos];
        *pos += 1;
        result |= ((byte & 0x7f)<<shift);
        shift += 7;
        if ((byte & 0x80) == 0) {
            break;
        }
        bcnt += 1;
        if (bcnt > (maxbits + 7 - 1) / 7) {
            wa_debug("Unsigned LEB at byte %d overflow", startpos);
        }
    }
    if (sign && (shift < maxbits) && (byte & 0x40)) {
        // Sign extend
        result |= - (1 << shift);
    }
    return result;
}

static uint64_t read_LEB(uint8_t *bytes, uint32_t *pos, uint32_t maxbits) {
    return read_LEB_(bytes, pos, maxbits, 0);
}

static uint64_t read_LEB_signed(uint8_t *bytes, uint32_t *pos, uint32_t maxbits) {
    return read_LEB_(bytes, pos, maxbits, 1);
}

static uint32_t read_uint32(uint8_t *bytes, uint32_t *pos) {
    *pos += 4;
    return ((uint32_t *) (bytes+*pos-4))[0];
}


struct dynarr{
    uint32_t len;
    uint32_t cap;
    uint32_t elemSize;
    uint8_t data[1];
};

static void dynarr_init(struct dynarr **arr,uint32_t elemSize){
    if(*arr!=NULL){
        wa_free(*arr);
    }
    *arr=wa_malloc(sizeof(struct dynarr)+elemSize);
    (*arr)->cap=1;
    (*arr)->len=0;
    (*arr)->elemSize=elemSize;
}
static void *dynarr_push(struct dynarr **buf,int count){
    struct dynarr *arr=*buf;
    uint32_t cap=arr->cap;
    uint32_t elemSize=arr->elemSize;
    void *newelem=NULL;
    while(cap<=arr->len+count){
        cap=cap*2;
    }
    if(arr->cap!=cap){
        arr=wa_realloc(*buf,sizeof(struct dynarr)+elemSize*cap);
        *buf=arr;
        arr->cap=cap;
    }
    newelem=&arr->data[arr->len*elemSize];
    memset(newelem,0,elemSize*count);
    arr->len+=count;
    return newelem;
}
#if DEBUG_BUILD

static void *dynarr_push_sizecheck(struct dynarr **buf,int count,int size){
    struct dynarr *arr=*buf;
    if(arr->elemSize!=size){
        wa_debug("size check failed.");
        wa_abort(1);
    }
    return dynarr_push(buf,count);
}
#define dynarr_push_type(dynarr,typename) (typename *)dynarr_push_sizecheck(dynarr,1,sizeof(typename))
#else
#define dynarr_push_type(dynarr,typename) (typename *)dynarr_push(dynarr,1)
#endif

static void *dynarr_pop(struct dynarr **buf,int count){
    struct dynarr *arr=*buf;
    uint32_t elemSize=arr->elemSize;
    arr->len-=count;
    return arr->data+arr->len*elemSize;
}
#define dynarr_pop_type(dynarr,typename) (typename *)dynarr_pop(dynarr,1)

#if DEBUG_BUILD
static void *dynarr_get_boundcheck(struct dynarr *dynarr,int index){
    if(index>dynarr->len || index<0){
        wa_debug("bound check failed.");
        wa_abort(1);
    }
    return dynarr->data+index*dynarr->elemSize;
}
#define dynarr_get(dynarr,typename,index) ((typename *)dynarr_get_boundcheck(dynarr,index))
#else
#define dynarr_get(dynarr,typename,index) (((typename *)dynarr->data)+index)
#endif


static void dynarr_free(struct dynarr **arr){
    if(*arr!=NULL){
        wa_free(*arr);
    }
    *arr=NULL;
}

//pool can allocate small memory many times and free all in one call.
struct pool{
    struct dynarr *chunks; // chunks ,type (char *)
    int cur;
    int used;
    int chunk_size;
}

static pool_init(struct pool *p,int chunk_size){
    dynarr_init(&p->chunks,sizeof(char *));
    *dynarr_push_type(&p->chunks,char *)=wa_calloc(chunk_size);
    p->cur=0;
    p->used=0;
    p->chunk_size=chunk_size;
}

//alloc size must less than chunk_size
static char *pool_alloc(struct pool *p,int size){
    wa_assert(size<=p->chunk_size,"pool_alloc require size<chunk_size");
    char *r;
    if(p->used+size<=p->chunk_size){
        r=*dynarr_get(p->chunks,char *,p->cur)+p->used;
        p->used+=size;
    }else{
        r=wa_calloc(p->chunk_size);
        *dynarr_push_type(&p->chunks,char *)=r;
        p->cur=p->chunks->len-1;
        p->used=size;
    }
    return r;
}

static void pool_free(struct pool *p){
    for(int i=0;i<p->chunks->len;i++){
        wa_free(*dynarr_get(p->chunks,char *,i));
    }
    dynarr_free(&p->chunks);
}




#endif