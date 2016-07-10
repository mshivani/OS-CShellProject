# choose your compiler
CC=gcc
#CC=gcc -Wall

mysh: sh.o get_path.o main.c 
	$(CC) -DHAVE_KSTAT -g main.c sh.o get_path.o -o mysh -lkstat
#	$(CC) -g main.c sh.o get_path.o bash_getcwd.o -o mysh

sh.o: sh.c sh.h
	$(CC) -DHAVE_KSTAT  -g -c sh.c -lkstat

get_path.o: get_path.c get_path.h
	$(CC) -DHAVE_KSTAT  -g -c get_path.c -lkstat

clean:
	rm -rf sh.o get_path.o mysh

