shell: bin
	gcc source/shell.c -o bin/shell -fsanitize=address,leak -Wall -Werror -g
bin:
	mkdir bin
clean:
	rm -r bin
