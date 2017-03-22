all: main

grace: main_g

main_g: main_g.o
	g++ main_g.o -o main_g -lcurl

main_g.o: main.cpp
	g++ -Wall -c main.cpp -o main_g.o

main: main.o
	/usr/bin/g++ main.o -o main -lcurl

main.o: main.cpp
	/usr/bin/g++ -Wall -c main.cpp -o main.o

clean: 
	rm -f *.o main main_g