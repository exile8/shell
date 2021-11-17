#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAXPATH 256

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

int check_separator(char *word_ptr) {
    if (!strcmp(word_ptr, "|")) {
        return 1;
    }
    return 0;
}

char **get_args(int *end_fl) {
    char **arg_ptr = NULL;
    char last_symb = '\0';
    int len = 0;
    while (last_symb != '\n') {
        arg_ptr = realloc(arg_ptr, (len + 1) * sizeof(char *));
        arg_ptr[len] = get_word(&last_symb);
        if (check_separator(arg_ptr[len])) {
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

char ***get_list(int *num_pipes) {
    char ***list = NULL;
    int end_flag = 0;
    int len = 0;
    while (end_flag != 1) {
        list = realloc(list, (len + 1) * sizeof(char **));
        list[len] = get_args(&end_flag);
        len++;
    }
    *num_pipes = len - 1;
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

int set_bg_flag(char **cmd, int *amp_index) {
    for (int i = 1; cmd[i]; i++) {
        if (!strcmp(cmd[i], "&") && cmd[i + 1] == NULL) {
            *amp_index = i;
            return 1;
        }
    }
    return 0;
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

char ***prepare_list(char ***list, int *input_fd, int *output_fd, int num_pipes, int *bg_flag) {
    int input_index = -1, output_index = -1, amp_index = -1;
    if (list[0][0] == NULL) {
        return list;
    }
    *bg_flag = set_bg_flag(list[0], &amp_index);
    if (amp_index != -1 && num_pipes == 0) {
        free(list[0][amp_index]);
        list[0][amp_index] = NULL;
    }
    *input_fd = get_input_fd(list[0], &input_index);
    if (input_index != -1) {
        list[0] = cut_args(list[0], input_index);
    }
    *output_fd = get_output_fd(list[num_pipes], &output_index);
    if (output_index != -1) {
        list[num_pipes] = cut_args(list[num_pipes], output_index);
    }
    return list;
}

void change_directory(char **cmd) {
    char *new_dir = NULL;
    char new_path[MAXPATH];
    if (cmd[1] == NULL || !strcmp(cmd[1], "~")) {
        new_dir = getenv("HOME");
    } else if (!strcmp(cmd[1], "-")) {  
        new_dir = getenv("OLDPWD");
        if (new_dir != NULL) {
            puts(new_dir);
        }
    } else {
        new_dir = cmd[1];
    }
    if (chdir(new_dir) < 0) {
        perror("chdir");
    }
    getcwd(new_path, MAXPATH);
    if (setenv("OLDPWD", getenv("PWD"), 1) < 0) {
        perror("setenv");
    }
    if (setenv("PWD", new_path, 1) < 0) {
        perror("setenv");
    }
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

int exec_background(char **cmd, int input_fd, int output_fd, pid_t *new_bg_pid) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (redirect_io(input_fd, output_fd) < 0) {
            return 1;
        }
        execvp(cmd[0], cmd);
        perror("exec");
        return 1;
    }
    *new_bg_pid = pid;
    return 0;
}

pid_t exec_with_redirect(char **cmd, int input_pipe[], int output_pipe[]) {
    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        if (input_pipe[1] != -1 && close(input_pipe[1]) < 0) {
            perror("close");
            return 1;
        }
        if (output_pipe[0] != -1 && close(output_pipe[0]) < 0) {
            perror("close");
            return 1;
        }
        if (redirect_io(input_pipe[0], output_pipe[1]) > 0) {
            return 1;
        }
        execvp(cmd[0], cmd);
        perror("exec");
        return 1;
    }
    if (input_pipe[0] != STDIN_FILENO && close(input_pipe[0]) < 0) {
        perror("close");
        return 1;
    }
    if (input_pipe[1] != -1 && close(input_pipe[1]) < 0) {
        perror("close");
        return 1;
    }
    return pid;
}

int exec_pipeline(char ***cmd_list, int input_fd, int output_fd, int num_pipes) {
    int (*fd)[2] = malloc(sizeof(int [2]) * (num_pipes + 2));
    pid_t *pids = malloc(sizeof(pid_t) * (num_pipes + 1));
    fd[0][0] = input_fd;
    fd[0][1] = -1;
    fd[num_pipes + 1][0] = -1;
    fd[num_pipes + 1][1] = output_fd;
    for (int i = 1; i < num_pipes + 2; i++) {
        if (i != num_pipes + 1 && pipe(fd[i]) < 0) {
            perror("pipe");
            free(fd);
            free(pids);
            return 1;
        }
        pids[i - 1] = exec_with_redirect(cmd_list[i - 1], fd[i - 1], fd[i]);
        if (pids[i - 1] == 1) {
            free(fd);
            free(pids);
            return 1;
        }
    }
    for (int i = 0; i < num_pipes + 1; i++) {
        if (waitpid(pids[i], NULL, 0) < 0) {
            perror("waitpid");
            free(fd);
            free(pids);
            return 1;
        }
    }
    free(fd);
    free(pids);
    return 0;
}

int exec(char ***cmd_list, int input_fd, int output_fd, int num_pipes) {
    if (cmd_list[0][0] == NULL) {
        return 0;
    }
    if (!strcmp(cmd_list[0][0], "cd") && num_pipes == 0) {
        change_directory(cmd_list[0]);
        return 0;
    }
    return exec_pipeline(cmd_list, input_fd, output_fd, num_pipes);
}

int is_exit(char *first_arg) {
    if (first_arg == NULL) {
        return 0;
    }
    return !strcmp(first_arg, "exit") || !strcmp(first_arg, "quit");
}

void print_prompt(const char *username, char *cur_dir) {
    printf("%s:%s$ ", username, cur_dir);
}

int main() {
    int input_fd, output_fd, num_pipes, bg_flag, num_jobs = 0;
    pid_t *bg_jobs = NULL, cur_bg_pid;
    const char *user = getenv("USER");
    char ***command;
    unsetenv("OLDPWD");
    while (1) {
        input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
        num_pipes = 0;
        bg_flag = 0;
        print_prompt(user, getenv("PWD"));
        command = get_list(&num_pipes);
        command = prepare_list(command, &input_fd, &output_fd, num_pipes, &bg_flag);
        if (is_exit(command[0][0])) {
            for (int i = 0; i < num_jobs; i++) {
                if (waitpid(bg_jobs[i], NULL, 0) < 0) {
                    perror("waitpid");
                    if (bg_jobs != NULL) {
                        free(bg_jobs);
                    }
                    remove_list(command);
                    return 1;
                }
            }
            if (bg_jobs != NULL) {
                free(bg_jobs);
            }
            remove_list(command);
            return 0;
        }
        if (bg_flag) {
            if (exec_background(command[0], input_fd, output_fd, &cur_bg_pid) > 0) {
                if (bg_jobs != NULL) {
                    free(bg_jobs);
                }
                remove_list(command);
                exit(1);
            }
            bg_jobs = realloc(bg_jobs, sizeof(pid_t) * (num_jobs + 1));
            bg_jobs[num_jobs] = cur_bg_pid;
            num_jobs++;
        }
        else if (exec(command, input_fd, output_fd, num_pipes) > 0) {
            if (bg_jobs != NULL) {
                free(bg_jobs);
            }
            remove_list(command);
            exit(1);
        }
        remove_list(command);
    }
}
