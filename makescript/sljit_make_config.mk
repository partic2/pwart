

SLJIT_SOURCE_ROOT=$(PWART_SOURCE_DIR)/sljit

SLJIT_CFLAGS=
 
# Allow empty PTHREAD_LDFLAGS ?
ifndef PTHREAD_LDFLAGS
PTHREAD_LDFLAGS=-lpthread
endif

SLJIT_LDFLAGS=sljitLir.o -lm $(PTHREAD_LDFLAGS)

build_sljit:
	$(CC) $(SLJIT_CFLAGS) $(CFLAGS) -c $(SLJIT_SOURCE_ROOT)/sljit_src/sljitLir.c