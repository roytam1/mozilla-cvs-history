CXXFLAGS = -g -Os -Wall -fno-exceptions
#CXXFLAGS = -g -Wall -fno-exceptions
test: test.o nsStrRef.o nsCharTraits.o
nsStrRef.o: nsStrRef.cpp nsStrRef.h nsCharTraits.h types.h Makefile
nsCharTraits: nsCharTraits.cpp nsCharTraits.h types.h Makefile
test.o: test.cpp nsStrRef.h nsCharTraits.h types.h Makefile
clean:
	rm -f *.o test
