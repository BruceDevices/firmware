CFLAGS=-D__LINUX__ -Wall -O2 
LIBS = 

all: png_demo

png_demo: main.o PNGdec.o adler32.o crc32.o infback.o inffast.o inflate.o inftrees.o zutil.o
	$(CC) main.o PNGdec.o adler32.o crc32.o infback.o inffast.o inflate.o inftrees.o zutil.o $(LIBS) -o png_demo 

main.o: main.cpp
	$(CXX) $(CFLAGS) -c main.cpp

PNGdec.o: ../src/PNGdec.cpp ../src/png.inl ../src/PNGdec.h
	$(CXX) $(CFLAGS) -c ../src/PNGdec.cpp

adler32.o: ../src/adler32.c
	$(CC) $(CFLAGS) -c ../src/adler32.c

crc32.o: ../src/crc32.c
	$(CC) $(CFLAGS) -c ../src/crc32.c

infback.o: ../src/infback.c
	$(CC) $(CFLAGS) -c ../src/infback.c

inffast.o: ../src/inffast.c
	$(CC) $(CFLAGS) -c ../src/inffast.c

inflate.o: ../src/inflate.c
	$(CC) $(CFLAGS) -c ../src/inflate.c

inftrees.o: ../src/inftrees.c
	$(CC) $(CFLAGS) -c ../src/inftrees.c

zutil.o: ../src/zutil.c
	$(CC) $(CFLAGS) -c ../src/zutil.c

clean:
	rm -rf *.o png_demo
