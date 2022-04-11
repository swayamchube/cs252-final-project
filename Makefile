client-phase1: client-phase1.o
	g++ build/client-phase1.o -o build/client-phase1 -lpthread

client-phase1.o: client-phase1.cpp
	g++ -c -std=c++17 client-phase1.cpp -o build/client-phase1.o

client-phase2: client-phase2.o
	g++ build/client-phase2.o -o build/client-phase2 -lpthread

client-phase2.o: client-phase2.cpp
	g++ -c -std=c++17 client-phase2.cpp -o build/client-phase2.o

client-phase3: client-phase3.o
	g++ build/client-phase3.o -o build/client-phase3 -lpthread

client-phase3.o: client-phase3.cpp
	g++ -c -std=c++17 client-phase3.cpp -o build/client-phase3.o

clean:
	rm build/*
