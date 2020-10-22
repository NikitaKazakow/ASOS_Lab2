all: main operations

main: 
	gcc -g ./src/main.c -o ./bin/main -lrt

operations: add div mul root sub

add:
	gcc -g ./src/operations/addition.c -o ./bin/addition -lrt

div:
	gcc -g ./src/operations/division.c -o ./bin/division -lrt

mul:
	gcc -g ./src/operations/multiplication.c -o ./bin/multiplication -lrt

root:
	gcc -g ./src/operations/root.c -o ./bin/root -lrt -lm

sub:
	gcc -g ./src/operations/subtraction.c -o ./bin/subtraction -lrt