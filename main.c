#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LENGTH 1000
#define MAX_PAR 100
#define LOG_FILE "/home/mohamed/CLionProjects/OSlab1/log_file.txt"

#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"
#define COLOR_RESET "\x1b[0m"

FILE *log_file;

// input type
typedef enum {
    shell_builtin,
    command,
    exit_shell
} input_type;

// remove spaces at the start and end of the input
char *remove_spaces(char *input) {
    while (input[0] == ' ') {
        input++;
    }
    while (input[strlen(input) - 1] == ' ') {
        input[strlen(input) - 1] = '\0';
    }
    return input;
}

void reap_child_zombie() {
    while(1) {
        int status;
        pid_t child = waitpid((pid_t)(-1), &status, WNOHANG);
        if (child == 0)
            break;
        else if (child == -1)
            break;
    }
}

void write_to_log_file(const char *msg) {
    log_file = fopen(LOG_FILE, "a");
    fprintf(log_file, "%s", msg);
    fclose(log_file);
}

void on_child_exit () {
    reap_child_zombie();
    write_to_log_file("Child process was terminated\n");
}

// set the home as default
void setup_environment(){
    chdir(getenv("HOME"));
}

// function used to replace $x with the value of x
char *replace (char *original, int foundAt) {
    char variable[MAX_LENGTH]; int cvar = 0;
    char res[MAX_LENGTH]; int cres = 0;
    for (int i = 0; i < foundAt; i++) {
        res[cres++] = original[i];
    }
    for (int i = foundAt + 1; i < strlen(original); i++) {
        if (original[i] != '$' && original[i] != ' ' && original[i] != '"') {
            variable[cvar++] = original[i];
        }
        else {
            variable[cvar++] = '\0';
            break;
        }
    }
    char *value = getenv(variable);
    if (value != NULL) {
        for (int i = 0; i < strlen(value); i++) {
            res[cres++] = value[i];
        }
    }
    for (int i = foundAt + strlen(variable) + 1; i < strlen(original); i++) {
        res[cres++] = original[i];
    }
    res[cres++] = '\0';
    strcpy(original, res);
    return original;
}

void parse_input (char input[], char *parameters[]) {
    char *token, *copy, *delim = " ";
    int par_count = 0;

    // after export update the input expression
    replaceAll :
    {
        int flag = 0;
        int i = 0;
        while (i < strlen(input)) {
            if (input[i] == '$') {
                flag = 1;
                break;
            }
            i++;
        }
        if (flag) {
            input = replace(input, i);
            printf("%s\n", input);
            goto replaceAll;
        }
    }
    copy = strdup(input);
    token = strtok(copy, delim);
    char *tmp = strdup(token);
    if(strcmp(tmp, "cd") == 0 || strcmp(tmp, "export") == 0 || strcmp(tmp, "echo") == 0) {
        while (token != NULL && par_count < MAX_PAR) {
            parameters[par_count++] = strdup(token);
            token = strtok(NULL, "");
        }
    }
    else {
        while (token != NULL && par_count < MAX_PAR) {
            parameters[par_count++] = token;
            token = strtok(NULL, " ");
        }
    }
}

void read_input (char input[]) {
    char curr[MAX_LENGTH];
    getcwd(curr, sizeof(curr));
    printf(GREEN "shell>>" COLOR_RESET ":" BLUE "%s", curr);
    printf(COLOR_RESET "$ ");
    fgets(input, MAX_LENGTH, stdin);
    input[strcspn(input, "\n")] = '\0';
}

void cd (char *parameters[]) {
    char path[MAX_LENGTH]; char *ptr; int c = 0;
    if(parameters[1] == NULL || strcmp(parameters[1], "~") == 0) {
        chdir(getenv("HOME"));
        return;
    }
    else if(*parameters[1] == '~') {
        ptr = getenv("HOME");
        for (int i = 0; i < strlen(ptr); i++) {
            path[c++] = ptr[i];
        }
        path[c++] = '\0';
        parameters[1]++;
        strcat(path, parameters[1]);
        if (chdir(path) != 0) {
            perror("cd");
            usleep(200000);
        }
    }
    else {
        if (chdir(parameters[1]) != 0) {
            perror("cd");
            usleep(200000);
        }
    }
}

void export (char command[]) {
    char *name, *value;
    name = strsep(&command, "=");
    value = strdup(command);
    if (value[0] == '"' && value[strlen(value) - 1] == '"') {
        value[strlen(value) - 1] = '\0';
        value++;
    }
    setenv(name, value, 1);
}

void echo (char command[]) {
    printf("%s\n", command);
    if (command[0] == '"' && command[strlen(command) - 1] == '"') {
        command[strlen(command) - 1] = '\0';
        command++;
    }
    printf("%s\n", command);
}

void execute_shell_bultin(char *parameters[]) {
    char *command = parameters[0];
    if (strcmp(command, "cd") == 0) {
        cd(parameters);
    }
    else if (strcmp(command, "echo") == 0) {
        echo(parameters[1]);
    }
    else if (strcmp(command, "export") == 0) {
        export(parameters[1]);
    }

}

void execute_command(char *parameters[], int background) {
    pid_t child_id = fork();
    int status;
    if (child_id < 0) {
        perror("Forking failed");
        exit(1);
    }
    else if (child_id == 0) {
        if (background) {
            printf("process: %d\n", getpid());
        }
        execvp(parameters[0], parameters);
        perror("execvp");
        usleep(20000);
        exit(1);
    }
    else {
        if (background) {
            waitpid(child_id, &status, WNOHANG);
        }
        else {
            waitpid(child_id, &status, 0);
        }
    }
}
void shell () {
    int command_exit = 0;
    do {
        char input[MAX_LENGTH] = {};
        char *parameters[MAX_PAR] = {};
        input_type inputType;
        int background = 0;
        read_input(input);
        strcpy(input, remove_spaces(input));
        if (input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';
        }
        parse_input(remove_spaces(input), parameters);
        if (strcmp(parameters[0], "exit") == 0) {
            command_exit = 1;
            inputType = exit_shell;
        }
        else if (strcmp(parameters[0], "cd") == 0 || strcmp(parameters[0], "echo") == 0 ||
            strcmp(parameters[0], "export") == 0) {
            inputType = shell_builtin;
        }
        else {
            inputType = command;
        }
        switch (inputType) {
            case shell_builtin:
                execute_shell_bultin(parameters);
                break;
            case command:
                execute_command(parameters, background);
                break;
            case exit_shell:
                break;
        }
    } while (!command_exit);
}

int main() {
    fclose(fopen(LOG_FILE, "w"));
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();
    return 0;
}
