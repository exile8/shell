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
            free(arg_ptr[len]);
            arg_ptr[len] = NULL;
            return arg_ptr;
        }
        len++;
    }
    if (arg_ptr[len - 1][0] == '\0') {
        free(arg_ptr[len - 1]);
        len--;
    } else {
        arg_ptr = realloc(arg_ptr, (len + 1) * sizeof(char *));
    }
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

void remove_list(char ***list) {
    for (int i = 0; list[i]; i++) {
        for (int j = 0; list[i][j]; j++) {
            free(list[i][j]);
        }
        free(list[i]);
    }
    free(list);
}

int get_input_fd(char **cmd, int *input_pos) {
    int fd_input = 0;
    for (int i = 1; cmd[i]; i++) {
        if (!strcmp(cmd[i - 1], "<")) {
            *input_pos = i - 1;
            fd_input = open(cmd[i], O_RDONLY);
        }
    }
    return fd_input;
}

int get_output_fd(char **cmd, int *output_pos) {
    int fd_output = 1;
    for (int i = 1; cmd[i]; i++) {
        if (!strcmp(cmd[i - 1], ">")) {
            *output_pos = i - 1;
            fd_output = open(cmd[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        }
    }
    return fd_output;
}

char **cut_args(char **cmd, int cut_pos) {
    int i;
    free(cmd[cut_pos]);
    free(cmd[cut_pos + 1]);
    for (i = cut_pos; cmd[i + 2]; i++) {
        cmd[i] = cmd[i + 2];
    }
    cmd[i] = NULL;
    cmd[i + 1] = NULL;
    return cmd;
}

char ***prepare_list(char ***list, int *input_fd, int *output_fd, int pipe_num) {
    int input_index = -1, output_index = -1;
    *input_fd = get_input_fd(list[0], &input_index);
    if (input_index != -1) {
        list[0] = cut_args(list[0], input_index);
    }
    *output_fd = get_output_fd(list[pipe_num], &output_index);
    if (output_index != -1) {
        list[pipe_num] = cut_args(list[pipe_num], output_index);
    }
    return list;
}

int redirect_io(int input_fd, int output_fd) {
    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(input_fd) < 0) {
            perror("close");
            return 1;
        }
    }
    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            return 1;
        }
        if (close(output_fd) < 0) {
            perror("close");
            return 1;
        }
    }
    return 0;
}

pid_t exec_with_redirect(char **cmd, int input_pipe[], int output_pipe[]) {
    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_io(input_pipe[0], output_pipe[1]) > 0) {
            return 1;
        }
        if (output_pipe[0] > 0 && close(output_pipe[0]) > 0) {
            perror("close");
            return 1;
        }
        execvp(cmd[0], cmd);
        perror("exec");
        return 1;
    }
    return pid;
}

int exec_pipeline(char ***cmd, int input_fd, int output_fd, int pipe_num) {
    int fd[pipe_num + 2][2];
    pid_t pid;

    fd[0][0] = input_fd;
    fd[pipe_num + 1][0] = -1;
    fd[pipe_num + 1][1] = output_fd;
    for (int i = 1; i < pipe_num + 2; i++) {
        if (i != pipe_num + 1 && pipe(fd[i]) < 0) {
            perror("pipe");
            return 1;
        }
        pid = exec_with_redirect(cmd[i - 1], fd[i - 1], fd[i]);
        if (pid == 1) {
            return 1;
        }
        if (fd[i][1] != STDOUT_FILENO) {
            if (close(fd[i][1]) < 0) {
                perror("close");
                return 1;
            }
        }
        if (waitpid(pid, NULL, 0) < 0) {
            perror("waitpid");
            return 1;
        }
    }
    return 0;
}

int is_exit(char *first_arg) {
    return !strcmp(first_arg, "exit") || !strcmp(first_arg, "quit");
}

int main() {
    int input_fd, output_fd, num_pipes;
    char ***command;
    const char *prompt = "$ ";
    while (1) {
        input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
        num_pipes = 0;
        fputs(prompt, stdout);
        command = get_list(&num_pipes);
        command = prepare_list(command, &input_fd, &output_fd, num_pipes);
        if (is_exit(command[0][0])) {
            remove_list(command);
            return 0;
        }
        if (exec_pipeline(command, input_fd, output_fd, num_pipes) > 0) {
            remove_list(command);
            exit(1);
        }
        remove_list(command);
    }
}
