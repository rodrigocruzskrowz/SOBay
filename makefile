CC=gcc
FLAGS=-c

all: frontend backend

frontend: frontend.o utils.o
	gcc frontend.c utils.o -o frontend
	echo "::FRONTEND CRIADO::"

frontend.o: frontend.c utils.h
	gcc frontend.c -c

backend: backend.o utils.o users_lib.o
	gcc backend.o utils.o users_lib.o -o backend
	echo "::BACKEND CRIADO::"

backend.o: backend.c utils.h
	gcc backend.c -c

utils.o: utils.c
	gcc utils.c -c

clean:
	rm frontend backend frontend.o backend.o utils.o
	echo "::CLEANING... DONE::"
