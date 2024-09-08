all: linux windows
debug: linux-debug windows-debug

linux: src/*.c
	gcc -O3 -o run src/*.c -lm

windows: src/*c
	x86_64-w64-mingw32-gcc -O3 -o run.exe src/*.c -lm -DWINDOWS

linux-debug: src/*.c
	gcc -g -o run src/*.c -lm -Wall -Wextra -pedantic

windows-debug: src/*c
	x86_64-w64-mingw32-gcc -g -o run.exe src/*.c -lm -Wall -Wextra -pedantic -DWINDOWS
