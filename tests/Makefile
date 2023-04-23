

# CFLAGS+= -ggdb3 -DDEBUG_BUILD=1

all:test test2

include $(PWART_SOURCE_ROOT)/pwart/make_config.mk


test:build_pwart
	- rm testexe || rm testexe.exe
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o testexe testmain.c $(PWART_LDFLAGS) $(LDFLAGS)
	$(WAT2WASM) --enable-multi-memory test1.wat
	$(WAT2WASM) unary.wat
	$(WAT2WASM) binary.wat
	$(WAT2WASM) control.wat
	$(WAT2WASM) convert.wat
	$(WAT2WASM) compare.wat
	./testexe

test2:build_pwart
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -o testexe2 testmain2.c $(PWART_LDFLAGS) $(LDFLAGS)
	$(WAT2WASM) --enable-multi-memory extension1.wat
	./testexe2