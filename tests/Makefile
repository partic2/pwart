

# CFLAGS+= -ggdb3 -DDEBUG_BUILD=1

all:test

include $(PWART_SOURCE_ROOT)/pwart/make_config.mk

test:build_pwart
	- rm testexe || rm testexe.exe
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o testexe testmain.c $(PWART_LDFLAGS) $(LDFLAGS)
	$(WAT2WASM) test1.wat
	$(WAT2WASM) unary.wat
	$(WAT2WASM) binary.wat
	$(WAT2WASM) control.wat
	$(WAT2WASM) convert.wat
	./testexe