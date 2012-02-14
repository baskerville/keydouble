CC      = gcc
CFLAGS  = -std=c99 -Wall -pedantic -O2
LIBS    = -lX11 -lXtst
PREFIX  = /usr/local

SRC = keydouble.c

all: options keydouble

options:
	@echo "keydouble build options:"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "PREFIX  = $(PREFIX)"

keydouble: $(SRC)
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LIBS)
