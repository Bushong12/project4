all: main

main: main.o
	g++ main.o -o main

main.o: main.cpp
	g++ -Wall -c main.cpp -o main.o

clean: 
	rm -f *.o main