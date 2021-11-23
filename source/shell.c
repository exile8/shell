#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define MAXPATH 256
#define AND 0
#define OR 1
#define END 0
#define CONT 1

struct exec_environ {
    pid_t *pids;
    int num_cur_procs;
    pid_t *bg_pids;
    int num_bg_pids;
    int bg_flag;
};

typedef struct exec_environ *Execenv;

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
    if (!strcmp(word_ptr, "|") || !strcmp(word_ptr, "&&") || !strcmp(word_ptr, "||")) {
        return 1;
    } else {
        return 0;
    }
}

char **get_args(int *end_fl, char *separator) {
    char **arg_ptr = NULL;
    char last_symb = '\0';
    int len = 0;
    while (last_symb != '\n') {
        arg_ptr = realloc(arg_ptr, (len + 1) * sizeof(char *));
        arg_ptr[len] = get_word(&last_symb);
        if (check_separator(arg_ptr[len])) {
            strcpy(separator, arg_ptr[len]);
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
    strcpy(separator, "");
    *end_fl = 1;
    arg_ptr[len] = NULL;
    return arg_ptr;
}

char ***get_list(int *num_seps, int **links, int *input_err_flag) {
    char ***list = NULL;
    char cur_separator[3] = {'\0'};
    int *link_array = NULL;
    int num_pipes = 0;
    int num_links = 0;
    int end_flag = 0;
    int len = 0;
    while (end_flag != 1) {
        list = realloc(list, (len + 1) * sizeof(char **));
        list[len] = get_args(&end_flag, cur_separator);
        if (!strcmp(cur_separator, "&&")) {
            link_array = realloc(link_array, (len + 1) * sizeof(int));
            link_array[len] = AND;
            num_links++;
        }
        if (!strcmp(cur_separator, "||")) {
            link_array = realloc(link_array, (len + 1) * sizeof(int));
            link_array[len] = OR;
            num_links ++;
        }
        if (!strcmp(cur_separator, "|")) {
            num_pipes++;
        }
        len++;
    }
    *links = link_array;
    *num_seps = len - 1;
    list = realloc(list, (len + 1) * sizeof(char **));
    list[len] = NULL;
    if (num_links > 0 && num_pipes > 0) {
        *input_err_flag = 1;
    }
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

void reset(char ***list, Execenv state, int *links, int mode) {
    if (mode == END && state->bg_pids != NULL) {
        free(state->bg_pids);
    }
    if (links != NULL) {
        free(links);
    }
    if (state->pids != NULL) {
        free(state->pids);
        state->pids = NULL;
    }
    if (mode == CONT) {
        state->bg_flag = 0;
    }
    remove_list(list);
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

char ***prepare_list(char ***list, int *input_fd, int *output_fd, int num_seps, int *bg_flag, int *links) {
    int input_index = -1, output_index = -1, amp_index = -1;
    if (links != NULL) {
        return list;
    }
    if (list[0][0] == NULL) {
        return list;
    }
    *bg_flag = set_bg_flag(list[num_seps], &amp_index);
    if (amp_index != -1) {
        free(list[num_seps][amp_index]);
        list[num_seps][amp_index] = NULL;
    }
    *input_fd = get_input_fd(list[0], &input_index);
    if (input_index != -1) {
        list[0] = cut_args(list[0], input_index);
    }
    *output_fd = get_output_fd(list[num_seps], &output_index);
    if (output_index != -1) {
        list[num_seps] = cut_args(list[num_seps], output_index);
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

void add_bg_pids(Execenv state, int num_new_pids) {
    state->bg_pids = realloc(state->bg_pids, sizeof(pid_t) * (state->num_bg_pids + num_new_pids));
    for (int i = 0; i < num_new_pids; i++) {
        state->bg_pids[state->num_bg_pids + i] = state->pids[i];
    }
    state->num_bg_pids += num_new_pids;
}

int wait_pipeline(pid_t *pids, int num_pids) {
    for (int i = 0; i < num_pids; i++) {
        if (waitpid(pids[i], NULL, 0) < 0) {
            perror("waitpid");
            return 1;
        }
    }
    return 0;
}

int exec_pipeline(char ***cmd_list, int input_fd, int output_fd, int num_seps, Execenv state) {
    int (*fd)[2] = malloc(sizeof(int [2]) * (num_seps + 2));
    state->pids = malloc(sizeof(pid_t) * (num_seps + 1));
    fd[0][0] = input_fd;
    fd[0][1] = -1;
    fd[num_seps + 1][0] = -1;
    fd[num_seps + 1][1] = output_fd;
    for (int i = 1; i < num_seps + 2; i++) {
        if (i != num_seps + 1 && pipe(fd[i]) < 0) {
            perror("pipe");
            free(fd);
            return 1;
        }
        state->pids[i - 1] = exec_with_redirect(cmd_list[i - 1], fd[i - 1], fd[i]);
        if (state->pids[i - 1] == 1) {
            free(fd);
            return 1;
        } else {
            state->num_cur_procs++;
        }
    }
    if (state->bg_flag) {
        add_bg_pids(state, num_seps + 1);
        printf("PID: %d\n", state->pids[num_seps]);
    } else {
        if (wait_pipeline(state->pids, num_seps + 1) > 0) {
            free(fd);
            return 1;
        }
    }
    free(fd);
    return 0;
}

int exec_chain(char ***cmd_list, Execenv state, int num_seps, int *links) {
    int wstatus;
    state->pids = malloc(sizeof(pid_t));
    state->num_cur_procs = 1;
    for (int i = 0; i < num_seps + 1; i++) {
        *state->pids = fork();
        if (*state->pids < 0) {
            perror("fork");
            return 1;
        }
        if (*state->pids == 0) {
            execvp(cmd_list[i][0], cmd_list[i]);
            perror("exec");
            return 1;
        }
        if (waitpid(*state->pids, &wstatus, 0) < 0) {
            perror("waitpid");
            return 1;
        }
        if (i < num_seps) {
            if (wstatus == 0 && links[i] == OR) {
                break;
            }
            if (wstatus != 0 && links[i] == AND) {
                break;
            }
        }
    }
    return 0;
}

int exec(char ***cmd_list, int input_fd, int output_fd, int num_seps, Execenv state, int *links, int input_err_flag) {
    if (cmd_list[0][0] == NULL) {
        return 0;
    }
    if (input_err_flag == 1) {
        puts("Wrong input");
        return 0;
    }
    if (!strcmp(cmd_list[0][0], "cd") && num_seps == 0) {
        change_directory(cmd_list[0]);
        return 0;
    }
    if (links != NULL) {
        return exec_chain(cmd_list, state, num_seps, links);
    }
    return exec_pipeline(cmd_list, input_fd, output_fd, num_seps, state);
}

int is_exit(char *first_arg) {
    if (first_arg == NULL) {
        return 0;
    }
    return !strcmp(first_arg, "exit") || !strcmp(first_arg, "quit");
}

void term_bg_pids(pid_t *pids, int num_bg_pids) {
    for (int i = 0; i < num_bg_pids; i++) {
        if (kill(pids[i], 0) != -1) {
            kill(pids[i], SIGTERM);
        }
    }
}

void print_prompt(const char *username, char *cur_dir) {
    printf("%s:%s$ ", username, cur_dir);
}

int main() {
    int input_fd, output_fd, num_seps, input_err_flag;
    struct exec_environ shell_state = {NULL, 0, NULL, 0, 0};
    Execenv state = &shell_state;
    int *links = NULL;
    const char *user = getenv("USER");
    char ***command;
    while (1) {
        input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
        input_err_flag = 0;
        num_seps = 0;
        print_prompt(user, getenv("PWD"));
        command = get_list(&num_seps, &links, &input_err_flag);
        command = prepare_list(command, &input_fd, &output_fd, num_seps, &state->bg_flag, links);
        if (is_exit(command[0][0])) {
            term_bg_pids(state->bg_pids, state->num_bg_pids);
            reset(command, state, links, END);
            return 0;
        }
        if (exec(command, input_fd, output_fd, num_seps, state, links, input_err_flag) > 0) {
            reset(command, state, links, END);
            exit(1);
        }
        reset(command, state, links, CONT);
    }
}
