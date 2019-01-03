FLAGS = -std=c99 -Wall
DEPS = -lreadline

default: build

build:
	cc $(FLAGS) src/main.c src/mpc.c $(DEPS) -o lisp

run: build
	./lisp

clean:
	rm lisp
