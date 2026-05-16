CFLAGS=-c -Wall -O2 -ggdb -D__LINUX__ -I../src
LIBS = -lm -lpthread

all: libAnimatedGIF.a

libAnimatedGIF.a: AnimatedGIF.o
	ar -rc libAnimatedGIF.a AnimatedGIF.o ;\
	sudo cp libAnimatedGIF.a /usr/local/lib ;\
	sudo cp ../src/AnimatedGIF.h /usr/local/include

AnimatedGIF.o: ../src/AnimatedGIF.cpp ../src/AnimatedGIF.h ../src/gif.inl
	$(CXX) $(CFLAGS) ../src/AnimatedGIF.cpp

clean:
	rm *.o libAnimatedGIF.a
