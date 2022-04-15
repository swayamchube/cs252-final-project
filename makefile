all: client-phase1 client-phase2 client-phase3 client-phase4 client-phase5

client-phase1: client-phase1.o
	g++ client-phase1.o -o client-phase1 -lpthread

client-phase1.o: client-phase1.cpp
	g++ -c -std=c++17 client-phase1.cpp -o client-phase1.o

client-phase2: client-phase2.o
	g++ client-phase2.o -o client-phase2 -lpthread

client-phase2.o: client-phase2.cpp
	g++ -c -std=c++17 client-phase2.cpp -o client-phase2.o

client-phase3: client-phase3.o
	g++ client-phase3.o -o client-phase3 -lpthread

client-phase3.o: client-phase3.cpp
	g++ -c -std=c++17 client-phase3.cpp -o client-phase3.o

client-phase4: client-phase4.o
	g++ client-phase4.o -o client-phase4 -lpthread

client-phase4.o: client-phase4.cpp
	g++ -c -std=c++17 client-phase4.cpp -o client-phase4.o

client-phase5: client-phase5.o
	g++ client-phase5.o -o client-phase5 -lpthread

client-phase5.o: client-phase5.cpp
	g++ -c -std=c++17 client-phase5.cpp -o client-phase5.o

clean:
	rm *.o
