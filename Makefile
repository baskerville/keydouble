CC      = gcc
CFLAGS  = -ansi -Wall -O2
LIBS    = -lX11 -lXtst
PREFIX  = /usr/local

SRC = keydouble.c

all: options keydouble

options:
	@echo "keydouble build options:"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "PREFIX  = $(PREFIX)"

keydouble: $(SRC) Makefile
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LIBS)
