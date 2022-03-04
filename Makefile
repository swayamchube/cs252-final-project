client-phase1: client-phase1.o
	g++ client-phase1.o -o client-phase1 -lpthread

client-phase1.o: client-phase1.cpp
	g++ -c -std=c++17 -DDEBUG client-phase1.cpp
