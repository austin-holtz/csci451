all: master p1 p2 p3

master: master.c
	gcc -o master master.c

p1: program1.c utils.c
	gcc -o p1 program1.c utils.c

p2: program2.c
	gcc -o p2 program2.c

p3: program3.c
	gcc -o p3 program3.c