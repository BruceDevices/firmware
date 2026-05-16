CXX=c++
LIBS=$(shell pkg-config sdl2 --libs) -lAnimatedGIF
CXXFLAGS= -D__LINUX__ -Wall $(shell pkg-config sdl2 --cflags)

all: sdl2_player

sdl2_player: main.o
	${CXX} main.o $(LIBS) -o sdl2_player

main: main.cpp
	${CXX} ${CXXFLAGS} main.cpp

clean:
	rm -f main *.o
