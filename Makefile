CC        = gcc
CFLAGS    = -ansi -Wall -O2
LIBS      = -lX11 -lXtst
PREFIX    = /usr/local
BINPREFIX = $(PREFIX)/bin

SRC = keydouble.c

all: options keydouble

options:
	@echo "keydouble build options:"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "PREFIX  = $(PREFIX)"

keydouble: $(SRC) Makefile
	$(CC) -o "$@" $(SRC) $(CFLAGS) $(LIBS)

clean:
	@echo "cleaning"
	rm -f $@

install: all
	@echo "installing executable files to $(DESTDIR)$(BINPREFIX)"
	@install -D -m 755 $@ $(DESTDIR)$(BINPREFIX)/$@
	@install -D -m 755 kdlaunch $(DESTDIR)$(BINPREFIX)/kdlaunch
	@install -D -m 755 kdkill $(DESTDIR)$(BINPREFIX)/kdkill

uninstall:
	@echo "removing executable files from $(DESTDIR)$(BINPREFIX)"
	@rm -f $(DESTDIR)$(BINPREFIX)/{$@,kdlaunch,kdkill}

.PHONY: all options clean install uninstall
