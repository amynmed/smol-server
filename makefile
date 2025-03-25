
EXEC_NAME = server

main:
	gcc -o $(EXEC_NAME) server.c -lws2_32

run: 
	./$(EXEC_NAME)