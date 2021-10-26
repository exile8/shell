#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

char skip_spaces(char cur_symbol) {
    while (cur_symbol == ' ' || cur_symbol == '\t') {
        cur_symbol = getchar();
    }
    return cur_symbol;
}

char *get_word(char *end) {
    char *word_ptr = NULL;
    char symbol;
    int len = 0;
    symbol = getchar();
    symbol = skip_spaces(symbol);
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

char **get_args(char *end_fl) {
    char **arg_ptr = NULL;
    char last_symb = '\0';
    int len = 0;
    while (last_symb != '\n') {
        arg_ptr = realloc(arg_ptr, (len + 1) * sizeof(char *));
        arg_ptr[len] = get_word(&last_symb);
        if (!strcmp(arg_ptr[len], "|")) {
            break;
        }
        len++;
    }
    if (last_symb != '\n') {
        free(arg_ptr[len]);
        arg_ptr[len] = NULL;
        return arg_ptr;
    }
    arg_ptr = realloc(arg_ptr, (len + 1) * sizeof (char *));
    *end_fl = 1;
    arg_ptr[len] = NULL;
    return arg_ptr;
}

char ***get_list(int *pipe_num) {
    char ***list = NULL;
    char end_flag = 0;
    int len = 0;
    while (end_flag != 1) {
        list = realloc(list, (len + 1) * sizeof(char **));
        list[len] = get_args(&end_flag);
        len++;
    }
    *pipe_num = len - 1;
    list = realloc(list, (len + 1) * sizeof(char **));
    list[len] = NULL;
    return list;
}

char ***remove_list(char ***list) {
    for (int i = 0; list[i]; i++) {
        for (int j = 0; list[i][j]; j++) {
            free(list[i][j]);
        }
        free(list[i]);
    }
    free(list);
    return NULL;
}

char ***prepare_list(char ***list, int *input_fd, int *output_fd, int pipe_num) {
    int input_index = 0, output_index = 0;
    for (int i = 0; list[0][i]; i++) {
        if (!strcmp(list[0][i], "<")) {
            *input_fd = open(list[0][i + 1], O_RDONLY);
            input_index = i;
        }
    }
    for (int i = 0; list[pipe_num][i]; i++) {
        if (!strcmp(list[pipe_num][i], ">")) {
            *output_fd = open(list[pipe_num][i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            output_index = i;
        }
    }
    if (input_index != 0) {
        for (int i = input_index; list[0][i]; i++) {
            free(list[0][i]);
            list[0][i] = NULL;
        }
    }
    if (output_index != 0) {
        for (int i = output_index; list[pipe_num][i]; i++) {
            free(list[pipe_num][i]);
            list[0][i] = NULL;
        }
    }
    return list;
}

int redirect_input(int fd) {
    if (fd == STDIN_FILENO) {
        return 0;
    }
    if (dup2(fd, STDIN_FILENO) < 0) {
        perror("dup2");
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    return 0;
}

int redirect_output(int fd) {
    if (fd == STDOUT_FILENO) {
        return 0;
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2");
        return 1;
    }
    if (close(fd) < 0) {
        perror("close");
        return 1;
    }
    return 0;
}

int execute(char ***cmd, int input_fd, int output_fd, int pipe_num) {
    int fd[pipe_num][2];
    int i;
    pid_t pid;

    if (pipe_num == 0) {
        if ((pid = fork()) < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            if (redirect_input(input_fd) > 0) {
                return 1;
            }
            if (redirect_output(output_fd) > 0) {
                return 1;
            }

            execvp(cmd[0][0], cmd[0]);
            perror("exec");
            return 1;
        } else {
            if (waitpid(pid, NULL, 0) < 0) {
                perror("waitpid");
                return 1;
            }
            return 0;
        }
    }
    
    if (pipe(fd[0]) < 0) {
        perror("pipe");
        return 1;
    }
    if ((pid = fork()) < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_input(input_fd) < 0) {
            return 1;
        }
        if (dup2(fd[0][1], STDOUT_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(fd[0][0]) < 0) {
            close(fd[0][1]);
            perror("close");
            return 1;
        }
        if (close(fd[0][1]) < 0) {
            perror("close");
            return 1;
        }

        execvp(cmd[0][0], cmd[0]);
        perror("exec");
        return 1;
    } else {
        if (close(fd[0][1]) < 0) {
            perror("close");
            return 1;
        }
        if (waitpid(pid, NULL, 0) < 0) {
            perror("waitpid");
            return 1;
        }
    }

    for (i = 1; i < pipe_num; i++) {
        if (pipe(fd[i]) < 0) {
            perror("pipe");
            return 1;
        }

        if ((pid = fork()) < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            if (dup2(fd[i - 1][0], STDIN_FILENO) < 0) {
                perror("dup2");
                return 1;
            }
            if (close(fd[i - 1][0]) < 0) {
                close(fd[i - 1][1]);
                perror("close");
                return 1;
            }
            
            if (dup2(fd[i][1], STDOUT_FILENO) < 0) {
                perror("dup2");
                return 1;
            }
            if (close(fd[i][0]) < 0) {
                close(fd[i][1]);
                perror("close");
                return 1;
            }
            if (close(fd[i][1]) < 0) {
                perror("close");
                return 1;
            }

            execvp(cmd[i][0], cmd[i]);
            perror("exec");
            return 1;
        } else {
            if (close(fd[i][1]) < 0) {
                perror("close");
                return 1;
            }
            if (waitpid(pid, NULL, 0) < 0) {
                perror("waitpid");
                return 1;
            }
        }
    }

    if ((pid = fork()) < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (dup2(fd[i - 1][0], STDIN_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(fd[i - 1][0]) < 0) {
            close(fd[i - 1][1]);
            perror("close");
            return 1;
        }
        if (redirect_output(output_fd) > 0) {
            return 1;
        }

        execvp(cmd[i][0], cmd[i]);
        perror("exec");
        return 1;
    } else {
        if (close(fd[i - 1][0]) < 0) {
            perror("close");
            return 1;
        }
        if (waitpid(pid, NULL, 0) < 0) {
            perror("waitpid");
            return 1;
        }
    }
    return 0;
}

int main() {
    int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO, pipes = 0;
    char ***command = get_list(&pipes);
    command = prepare_list(command, &input_fd, &output_fd, pipes);
    while (1) {
        if (!strcmp(command[0][0], "exit") || !strcmp(command[0][0], "quit")) {
            command = remove_list(command);
            return 0;
        }
        if (execute(command, input_fd, output_fd, pipes) > 0) {
            command = remove_list(command);
            exit(1);
        }
        command = remove_list(command);
        input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
        pipes = 0;
        command = get_list(&pipes);
        command = prepare_list(command, &input_fd, &output_fd, pipes);
    }
}
