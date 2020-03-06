all:
	clang++ snake.cpp -O3 -lcurses -lpthread -o snake.o
jit:
	clang++ snake.cpp -O3 -lcurses -lpthread -o snake.o && ./snake.o
clean:
	rm -f snake.o