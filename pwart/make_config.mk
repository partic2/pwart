include $(PWART_SOURCE_ROOT)/makescript/sljit_make_config.mk

PWART_CFLAGS=$(SLJIT_CFLAGS) -I$(PWART_SOURCE_ROOT)/include

PWART_LDFLAGS=pwart.o $(SLJIT_LDFLAGS)

build_pwart:
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -c $(PWART_SOURCE_ROOT)/pwart/pwart.c $(SLJIT_LDFLAGS) $(LDFLAGS)

clean:
	- rm $(PWART_SOURCE_ROOT)/pwart/pwart.o

