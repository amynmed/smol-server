
PROG = server

main:
	gcc -o $(PROG) server.c -lws2_32

run: 
	./$(PROG)