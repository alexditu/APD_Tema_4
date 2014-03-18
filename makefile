all: build

build:
	mpic++ tema4.c -o tema4.out -Wall
clean:
	rm tema4.out
