#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

char *get_word(char *end) {
    char *word_ptr = NULL;
    char symbol;
    int word_len = 0;
    symbol = getchar();
    /* Пропускаем лишние пробелы и табуляции */
    while (symbol == ' ' || symbol == '\t') {
        symbol = getchar();
    }
    /* Считываем слово до следующего разделительного символа */
    while (symbol != ' ' && symbol != '\t' && symbol != '\n') {
        word_ptr = realloc(word_ptr, (word_len + 1) * sizeof(char));
        word_ptr[word_len] = symbol;
        word_len++;
        symbol = getchar();
    }
    *end = symbol;
    word_ptr = realloc(word_ptr, (word_len + 1) * sizeof(char));
    word_ptr[word_len] = '\0';
    return word_ptr;
}

char **get_list(char **input_stream, char **output_stream) {
    char **list = NULL;
    char last_symb = '\0';
    int list_len = 0;
    char *filename = NULL;
    /* Считываем строку до переноса, добавляя слова в динамический массив */
    while (last_symb != '\n') {
        list = realloc(list, (list_len + 1) * sizeof(char *));
        list[list_len] = get_word(&last_symb);
        if (!strcmp(list[list_len], "<")) {
            free(list[list_len]);
            filename = get_word(&last_symb);
            *input_stream = filename;
            continue;
        }
        if (!strcmp(list[list_len], ">")) {
            free(list[list_len]);
            filename = get_word(&last_symb);
            *output_stream = filename;
            continue;
        }
        list_len++;
    }
    /* Последняя строка в массиве - пустая */
    list = realloc(list, (list_len + 1) * sizeof(char *));
    list[list_len] = NULL;
    return list;
}

char **remove_list(char **list) {
    int index = 0;
    while (list[index] != NULL) {
        free(list[index]);
        index++;
    }
    free(list);
    return NULL;
}

int redirect_input(char *input_stream) {
    if (input_stream == NULL) {
        return 0;
    }
    int fd = open(input_stream, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    if (dup2(fd, 0) < 0) {
        perror("dup2");
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    free(input_stream);
    return 0;
}

int redirect_output(char *output_stream) {
    if (output_stream == NULL) {
        return 0;
    }
    int fd = open(output_stream, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    if (dup2(fd, 1) < 0) {
        perror("dup2");
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    free(output_stream);
    return 0;
}

int main() {
    char **command = NULL;
    char *input = NULL, *output = NULL;
    pid_t pid;
    while (1) {
        command = get_list(&input, &output);
        if (!strcmp(command[0], "exit") || !strcmp(command[0], "quit")) {
            command = remove_list(command);
            return 0;
        }
        pid = fork();
        if (pid < 0) {
            perror("fork");
            command = remove_list(command);
            exit(1);
        }
        if (pid == 0) {
            if (redirect_input(input) > 0) {
                command = remove_list(command); 
                return 1;
            }
            if (redirect_output(output) > 0) {
                command = remove_list(command);
                return 1;
            }
            execvp(command[0], command);
            /* Ошибка exec */
            perror("exec");
            command = remove_list(command);
            exit(1);
        }
        if (wait(NULL) < 0) {
            command = remove_list(command);
            perror("wait");
            exit(1);
        }
        if (input != NULL) {
            free(input);
        }
        if (output != NULL) {
            free(output);
        }
        input = NULL;
        output = NULL;
        command = remove_list(command);
        command = NULL;
    }
}
