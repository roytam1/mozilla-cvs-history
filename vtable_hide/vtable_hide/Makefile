# vim:set noet ts=4 sw=4:

all: test
shim.o: shim.cc shim.h inner.h Makefile
	gcc -c -g -fno-exceptions $< -o $@
test.o: test.cc shim.h inner.h Makefile
	gcc -c -g -fno-exceptions $< -o $@
test: test.o shim.o
	gcc $^ -o $@ -lsupc++
clean:
	rm -f *.o test
