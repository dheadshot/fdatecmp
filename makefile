# For MinGW
CC = gcc
# For TCC
# CC = tcc

fdatecmp.exe: fdatecmp.o
	${CC} fdatecmp.o -lkernel32 -o fdatecmp.exe

fdatecmp.o: fdatecmp.c
	${CC} -c fdatecmp.c