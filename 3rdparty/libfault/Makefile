PREFIX = /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
LIBDIR = $(DESTDIR)$(PREFIX)/lib
INCDIR = $(DESTDIR)$(PREFIX)/include

CC = $(shell echo $${CC:-cc})
CFLAGS += -std=c11 -D_GNU_SOURCE -O2 -Wall -Wextra

all: lib/libfault.so lib/libfaultpreload.so lib/libfault.a
lib/libfault.so: lib/libfault.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<
lib/libfaultpreload.so: lib/libfault.c
	$(CC) $(CFLAGS) -fPIC -DLIBFAULT_PRELOAD_SHARED_LIBRARY -shared -o $@ $<
lib/libfault.a: lib/libfault.o
	ar -rc $@ $<
	ranlib $@
lib/libfault.o: lib/libfault.c
	$(CC) $(CFLAGS) -c -o $@ $<

check: t/test1 t/test2 t/test3 t/test4
	-for x in t/test1 t/test2 t/test3 t/test4; do ./$$x; done
test: t/test1 t/test2 t/test3 t/test4

t/test1: t/test1.c lib/libfault.c
	$(CC) $(CFLAGS) -Ilib -rdynamic -o $@ t/test1.c lib/libfault.c
t/test2: t/test2.c lib/libfault.c
	$(CC) $(CFLAGS) -Ilib -rdynamic -o $@ t/test2.c lib/libfault.c
t/test3: t/test3.c
	$(CC) $(CFLAGS) -Ilib -rdynamic -o $@ $<
t/test4: t/test4.c lib/libfault.c
	$(CC) $(CFLAGS) -Ilib -rdynamic -o $@ t/test4.c lib/libfault.c

clean:
	rm -f ./t/test1 ./t/test2 ./t/test3 ./t/test4
	rm -f *~ lib/*~ lib/*.so lib/*.o lib/*.a

install: lib/libfault.so lib/libfault.a lib/libfaultpreload.so bin/libfault
	install -Dm755 bin/libfault    $(BINDIR)/libfault
	install -Dm755 lib/libfault.a  $(LIBDIR)/libfault.a
	install -Dm755 lib/libfault.so $(LIBDIR)/libfault.so
	install -Dm755 lib/libfaultpreload.so $(LIBDIR)/libfaultpreload.so
	install -Dm755 lib/libfault.h  $(INCDIR)/libfault.h
	install -Dm755 lib/libfault++.h  $(INCDIR)/libfault++.h
	perl -pi -e 's@LIBFAULT_BASEDIR=@LIBFAULT_BASEDIR=$(PREFIX)/lib@g' \
	  $(BINDIR)/libfault
