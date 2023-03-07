#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LENGTH 1000
#define MAX_PAR 500

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

FILE *log_file;

// input type
typedef enum {
    shell_builtin,
    command
} input_type;
//// command type
//typedef enum {
//    cd,
//    echo,f
//   export
//} command_type;


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
    log_file = fopen("log_file.txt", "a");
    fprintf(log_file, "%s", msg);
    fclose(log_file);
}
void on_child_exit () {
    reap_child_zombie();
    write_to_log_file("Child terminated\n");
}

void setup_environment(){  // finished
//    char cwd[MAX_LENGTH];
//    getcwd(cwd, sizeof(cwd)); // current working directory
    chdir(getenv("HOME"));
}

void parse_input (char input[], char *parameters[]) {
    int par_count = 0;
    char *token = strsep(&input, " ");
    parameters[par_count++] = token;
    if(strcmp(token, "cd") == 0 || strcmp(token, "export") == 0 || strcmp(token, "echo") == 0) {
        parameters[par_count++] = input;
    }
    else {
        char *token2 = strsep(&input, " ");
        while (token2 != NULL) {
            parameters[par_count++] = token2;
            token2 = strsep(&input, " ");
        }
    }
    /* A UNIX shell program in C language, can execute commands with or without arguments such as ls, cp, ...etc, and other built-in commands such as cd, export, echo, and exit.*/
}
void read_input (char input[]) {
    fgets(input, MAX_LENGTH, stdin);
    input[strcspn(input, "\n")] = '\0';
}
void evaluate_expression (char *parameters[]) {
    for (int i = 1; parameters[i] != NULL; i++) {
        if(strchr(parameters[i], '$' != NULL)) {
            parameters[i]++;
            parameters[i] = getenv(parameters[i] + 1);
        }
    }
}
void execute_shell_bultin(char *parameters[]) {
    char *command = parameters[0];
//    printf("%s\n", command);
    if (strcmp(command, "cd") == 0) {
        if (parameters[1] == NULL) {
            chdir(getenv("HOME"));
        }
        else {
            if (chdir(parameters[1]) == -1) {
                printf("No such file or directory\n");
            }
        }
    }
    else if (strcmp(command, "echo")) {

    }
    else if (strcmp(command, "export")) {

    }

}
void execute_command(char *parameters[]) {
    pid_t child_id = fork();
    if (child_id < 0) {
        printf("Forking failed\n");
    }
    else if (child_id == 0) {
//        for(int i = 0; parameters[i] != NULL; i++) {
//            printf("%s\n", parameters[i]);
//        }
        execvp(parameters[0], parameters);
        printf("Error\n");
        exit(1);
    }
    else {
        waitpid(child_id, 0, 0);
    }
}
void shell () {
    int command_exit = 0;
    do {
        char input[MAX_LENGTH] = {};
        char *parameters[MAX_PAR] = {};
        input_type inputType;
        int background = 0;
        char curr[MAX_LENGTH];
        getcwd(curr, sizeof(curr));
        printf(GREEN "shell>>" COLOR_RESET ":" BLUE "%s", curr);
        printf(COLOR_RESET "$ ");
        read_input(input);
        parse_input(input, parameters);
        evaluate_expression(parameters);

        if (strcmp(parameters[0], "exit") == 0) {
            command_exit = 1;
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
                execute_command(parameters);
        }
    } while (!command_exit);
}

int main() {  // finished
    fclose(fopen("log_file.txt", "w"));
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();
    return 0;
}
