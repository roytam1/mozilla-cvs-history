# vim:set ts=8 sw=8 noet:

all: libfoo.so libfoo++.so test test++
libfoo.so: libfoo.c Makefile
	gcc -g -fPIC -shared $< -o $@
libfoo++.so: libfoo++.cc Makefile
	gcc -g -fPIC -fno-exceptions -shared $< -o $@ -lsupc++
test: test.cc libfoo.h Makefile
	gcc -g -fno-exceptions $< -o $@ -L. -lfoo -lsupc++
test++: test.cc libfoo.h Makefile
	gcc -g -fno-exceptions $< -o $@ -L. -lfoo++ -lsupc++
clean:
	rm -f test libfoo.so
