all:
	gcc source/myshell.c -o bin/myshell -Wall -Werror -fsanitize=leak
