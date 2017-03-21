all: main

main: main.o
	/usr/bin/g++ main.o -o main

main.o: main.cpp
	/usr/bin/g++ -Wall -c main.cpp -o main.o

clean: 
	rm -f *.o main