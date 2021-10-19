#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

char *get_word(char *end) {
    char *word_ptr = NULL;
    char symbol;
    int len = 0;
    symbol = getchar();
    /* Пропускаем лишние пробелы и табуляции */
    while (symbol == ' ' || symbol == '\t') {
        symbol = getchar();
    }
    /* Считываем слово до следующего разделительного символа */
    while (symbol != ' ' && symbol != '\t' && symbol != '\n') {
        word_ptr = realloc(word_ptr, (len + 1) * sizeof(char));
        word_ptr[len] = symbol;
        len++;
        symbol = getchar();
    }
    *end = symbol;
    word_ptr = realloc(word_ptr, (len + 1) * sizeof(char));
    word_ptr[len] = '\0';
    return word_ptr;
}

char **get_list() {
    char **list = NULL;
    char last_symb = '\0';
    int len = 0;
    /* Считываем строку до переноса, добавляя слова в динамический массив */
    while (last_symb != '\n' && last_symb != '|') {
        list = realloc(list, (len + 1) * sizeof(char *));
        list[len] = get_word(&last_symb);
        if (!strcmp(list[len], "<") && last_symb != '\n') {
            free(list[len]);
            *input_stream = get_word(&last_symb);
            continue;
        }
        if (!strcmp(list[len], ">") && last_symb != '\n') {
            free(list[len]);
            *output_stream = get_word(&last_symb);
            continue;
        }
        len++;
    }
    /* Последняя строка в массиве - пустая */
    if (last_symb == '|') {
        *pipe_flag = 1;
        len--;
        free(list[len]);
        list[len] = NULL;
        return list;
    }
    list = realloc(list, (len + 1) * sizeof(char *));
    list[len] = NULL;
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
        free(input_stream);
        return 1;
    }
    if (dup2(fd, STDIN_FILENO) < 0) {
        perror("dup2");
        free(input_stream);
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        free(input_stream);
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
        free(output_stream);
        return 1;
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        free(output_stream);
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        free(output_stream);
        return 1;
    }
    free(output_stream);
    return 0;
}

int execute(char **command, char *input_stream, char *output_stream) {
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_input(input_stream) > 0) {
            return 1;
        }
        if (redirect_output(output_stream) > 0) {
            return 1;
        }
        execvp(command[0], command);
        /* Ошибка exec */
        perror("exec");
        return 1;
    }
    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }
    return 0;
}

int execute_pipe(char **arg1, char **arg2, char *input_stream, char *output_stream) {
    pid_t pid;
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return 1;
    }
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_input(input_stream) > 0) {
            return 1;
        }
        if (dup2(fd[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(fd[0]) < 0) {
            perror("close");
            return 1;
        }
        if (close(fd[1]) < 0) {
            perror("close");
            return 1;
        }
        execvp(arg1[0], arg1);
        perror("exec");
        return 1;
    }
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_output(output_stream) > 0) {
            return 1;
        }
        if (dup2(fd[0], STDIN_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(fd[0]) < 0) {
            perror("close");
            return 1;
        }
        if (close(fd[1]) < 0) {
            perror("close");
            return 1;
        }
        execvp(arg2[0], arg2);
        perror("exec");
        return 1;
    }
    if (close(fd[0]) < 0) {
        perror("close");
        return 1;
    }
    if (close(fd[1]) < 0) {
        perror("close");
        return 1;
    }
    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }
    if (wait(NULL) < 0) {
        perror("wait");
        return 1;
    }
    return 0;
}

int main() {
    char *input = NULL, *output = NULL;
    int pipe_flag = 0;
    char **command = get_list(&pipe_flag, &input, &output);
    char **command2 = NULL;
    while (1) {
        if (!strcmp(command[0], "exit") || !strcmp(command[0], "quit")) {
            if (input != NULL) {
                free(input);
            }
            if (output != NULL) {
                free(output);
            }
            command = remove_list(command);
            return 0;
        }
        if (pipe_flag == 1) {
            command2 = get_list(&pipe_flag, &input, &output);
            if (execute_pipe(command, command2, input, output) > 0) {
                command = remove_list(command);
                command2 = remove_list(command2);
                if (input != NULL) {
                    free(input);
                }
                if (output != NULL) {
                    free(output);
                }
                exit(1);
            }
        } else if (execute(command, input, output) > 0) {
            command = remove_list(command);
            if (input != NULL) {
                free(input);
            }
            if (output != NULL) {
                free(output);
            }
            exit(1);
        }
        command = remove_list(command);
        if (command2 != NULL) {
            command2 = remove_list(command2);
        }
        if (input != NULL) {
            free(input);
            input = NULL;
        }
        if (output != NULL) {
            free(output);
            output = NULL;
        }
        pipe_flag = 0;
        command = get_list(&pipe_flag, &input, &output);
    }
}
