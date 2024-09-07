all: src/*.c
	gcc -g -o run src/*.c -lm -Wall -Wextra -pedantic
