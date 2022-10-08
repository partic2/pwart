

SLJIT_SOURCE_ROOT=$(PWART_SOURCE_ROOT)/sljit

SLJIT_CFLAGS=

SLJIT_LDFLAGS=sljitLir.o -lm -lpthread

build_sljit:
	$(CC) $(SLJIT_CFLAGS) $(CFLAGS) -c $(SLJIT_SOURCE_ROOT)/sljit_src/sljitLir.c