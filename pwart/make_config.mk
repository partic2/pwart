include $(PWART_SOURCE_DIR)/makescript/sljit_make_config.mk

PWART_CFLAGS=$(SLJIT_CFLAGS) -I$(PWART_SOURCE_DIR)/include

PWART_LDFLAGS=pwart.o $(SLJIT_LDFLAGS) -lm

build_pwart:build_sljit
	$(CC) $(PWART_CFLAGS) $(CFLAGS) -c $(PWART_SOURCE_DIR)/pwart/pwart.c

clean:
	- rm $(PWART_SOURCE_DIR)/pwart/pwart.o

