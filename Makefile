SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o

PROGRAM = memccli

CC      = gcc
CFLAGS  = -std=gnu99 -D_GNU_SOURCE=1 -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wall -Werror -Wextra
LDFLAGS = -lmemcached

INSTALL = install
INSTALL_PROGRAM = $(INSTALL) -m 755

all: $(PROGRAM)

$(PROGRAM): memccli.o
	$(CC) $(LDFLAGS) -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(PROGRAM)
	$(INSTALL_PROGRAM) $< $(DESTDIR)$(PREFIX)/bin/$<

clean:
	rm -fv *.o $(PROGRAM)

.PHONY: all install clean
