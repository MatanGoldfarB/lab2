all: looper myshell

looper: LineParser.o looper.o
	gcc -g -Wall -o looper LineParser.o looper.o

myshell: myshell.o LineParser.o
	gcc -g -Wall -o myshell myshell.o LineParser.o

LineParser.o: LineParser.c LineParser.h
	gcc -g -Wall -c -o LineParser.o LineParser.c  

looper.o: looper.c
	gcc -g -Wall -c -o looper.o looper.c

myshell.o: myshell.c
	gcc -g -Wall -c -o myshell.o myshell.c  

.PHONY: clean 

clean:
	rm -f *.o looper myshell