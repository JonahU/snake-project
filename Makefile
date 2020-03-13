all:
	clang++ snake.cpp -O3 -std=c++17 -lcurses -lpthread -o snake.o
jit:
	clang++ snake.cpp -O3 -std=c++17 -lcurses -lpthread -o snake.o && ./snake.o
clean:
	rm -f snake.o