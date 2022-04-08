all: example.o

example.o: example.cpp ThreadPool.h
	clang++ -std=c++20 example.cpp -o example.o

clean:
	rm -f *.o
