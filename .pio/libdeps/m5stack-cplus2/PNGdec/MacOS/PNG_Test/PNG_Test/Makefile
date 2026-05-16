CFLAGS=-Wall -O2 -g -fsanitize=address,undefined
LIBS = 

all: png_test

png_test: main.o
	$(CC) main.o -fsanitize=address,undefined -g $(LIBS) -o png_test 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o png_test
