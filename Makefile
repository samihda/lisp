default: build

build:
	cc -std=c99 -Wall src/main.c -lreadline -o lisp

run: build
	./lisp
