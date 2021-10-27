all:
	gcc source/shell.c -o bin/shell -fsanitize=address,leak -Wall -Werror -g
