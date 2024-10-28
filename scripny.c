#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_SCRIPTS 1000
#define PAGE_SIZE 15
#define SCRIPT_PATH "scripts/"
#define SCRIPT_PATH_LENGTH 256
#define DESCRIPTION_LENGTH 1000
#define SEARCH_LENGTH 256

/* Disables canonical mode and echo of input characters */
void enable_raw_mode() {
    struct termios tm;
    tcgetattr(STDIN_FILENO, &tm);
    tm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tm);
    printf("\033[?25l");
}

/* Restores back cannonical mode and echo of input characters */
void disable_raw_mode() {
    struct termios tm;
    tcgetattr(STDIN_FILENO, &tm);
    tm.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tm);
    printf("\033[?25h");
}

/* Finds all the shell scripts ('*.sh' files) within 'SCRIPT_PATH' directory and stores it in 'scripts' variable */
void get_scripts(const char *base_path, char ***scripts, int *script_count) {
    struct dirent *dir;
    DIR *d = opendir(base_path);
    if (!d) {
        return;
    }
    char path[SCRIPT_PATH_LENGTH];
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
            snprintf(path, sizeof(path), "%s%s", base_path, dir->d_name);
            if (dir->d_type == DT_DIR) {
                get_scripts(strcat(path, "/"), scripts, script_count);
            } else if (dir->d_type == DT_REG && strstr(dir->d_name, ".sh")) {
                *scripts = realloc(*scripts, (*script_count + 1) * sizeof(char *));
                if (*scripts == NULL) {
                    perror("Failed to realloc memory for scripts");
                    closedir(d);
                    return;
                }
                (*scripts)[(*script_count)++] = strdup(path);
            }
        }
    }
    closedir(d);
}

/* Compares scripts based on lexicography and directory depth for use in sorting */
int compare_scripts(const void *a, const void *b) {
    const char *pathA = *(const char **)a;
    const char *pathB = *(const char **)b;
    int depthA = 0, depthB = 0;
    for (const char *p = pathA; *p; p++) {
        depthA += (*p == '/');
    }
    for (const char *p = pathB; *p; p++) {
        depthB += (*p == '/');
    }
    return depthA != depthB ? depthA - depthB : strcmp(pathA, pathB);
}

/* Reads and stores the third line of passed script, ASSUMMING it to provide description of the script */
void get_description(const char *script, char *description) {
    FILE *file = fopen(script, "r");
    if (file) {
        char line[DESCRIPTION_LENGTH];
        fgets(line, DESCRIPTION_LENGTH, file);
        fgets(line, DESCRIPTION_LENGTH, file);
        if (fgets(description, DESCRIPTION_LENGTH, file) == NULL || description[0] == '\0') {
            strcpy(description, "No description available\n");
        } else if (description[0] == '#') {
            memmove(description, description + 1, strlen(description));
        }
        fclose(file);
    } else {
        strcpy(description, "Failed to read script\n");
    }
}

/* Prints the navigable list of 'scripts', 'description', 'filter_term'(if not empty), and small help */
void display(char *scripts[], int script_count, int current_index, int issearch, char *filter_term) {
    printf("\033[H\033[J\033[38;5;208m[SCRIPNY]\033[0m\n\n");
    if (script_count > 0) {
        int start = (current_index / PAGE_SIZE) * PAGE_SIZE, end = start + PAGE_SIZE;
        if (end > script_count) {
            end = script_count;
        }
        for (int i = start; i < end; i++) {
            printf("%s %s\033[0m\n", (i == current_index ? "\033[1;95m>\033[1;96m" : " "), strncmp(scripts[i], SCRIPT_PATH, strlen(SCRIPT_PATH)) == 0 ? scripts[i] + strlen(SCRIPT_PATH) : scripts[i]);
        }
        char description[DESCRIPTION_LENGTH];
        get_description(scripts[current_index], description);
        printf("\n\033[3m\033[38;5;208mDescription:%s\033[0m\n", description);
    }
    if (issearch && strcmp(filter_term, "") != 0) {
        printf("\033[3m\033[1;95mFilter term:%s\033[0m\n", filter_term);
    }
    printf("\033[0;93mj = down, k = up, x = execute, q = quit, / = search, esc = clear search\033[0m\n\n");
}

/* Filters 'scripts' based on 'filter_term' where matching is stored (rather pointed) in 'filtered_scripts' */
void filter_scripts(char *scripts[], int script_count, char *filter_term, char ***filtered_scripts, int *filtered_count) {
    *filtered_count = 0;
    for (int i = 0; i < script_count; i++) {
        if (strstr(scripts[i], filter_term)) {
            *filtered_scripts = realloc(*filtered_scripts, (*filtered_count + 1) * sizeof(char *));
            if (*filtered_scripts == NULL) {
                perror("Failed to realloc memory for filtered scripts");
                return;
            }
            (*filtered_scripts)[(*filtered_count)++] = scripts[i];
        }
    }
}

/* Asks user to enter filter term and stores it in 'filter_term' */
void get_filterterm(char *filter_term, int *issearch, int *current_index) {
    printf("Search:");
    printf("\033[?25h");
    char input = '\0';
    int index = 0;
    while (1) {
        input = getchar();
        if (input == '\n') {
            filter_term[index] = '\0';
            *issearch = 1;
            *current_index = 0;
            break;
        } else if (input == 27) {
            printf("\033[H\033[J");
            *issearch = 0;
            *current_index = 0;
            strcpy(filter_term, "");
            break;
        } else if (input == 8 || input == 127) {
            if (index > 0) {
                filter_term[--index] = '\0';
                printf("\b \b");
            }
        } else if (index < SEARCH_LENGTH - 1) {
            filter_term[index++] = input;
            putchar(input);
        }
    }
    printf("\033[?25l");
}

/* Executes passed script and also prints its status of exit */
void execute_script(const char *script) {
    printf("Press y to confirm execution of (%s): ", script);
    if (getchar() != 'y') {
        return;
    }
    disable_raw_mode();
    printf("\nExecuting %s...\n-----------------\n", script);
    int status = system(script);
    printf("\n-----------------\n%s\033[0m\n", WIFEXITED(status) ? (WEXITSTATUS(status) == 0 ? "\033[0;92mSUCCESSFUL" : "\033[0;91mFAILED") : "\033[0;93mINTERRUPTED");
    enable_raw_mode();
    printf("Press Enter to go back...\n");
    while (getchar() != '\n');
}

/* Main function where mainly following is done:
1) call get_scripts to store list of scripts inside 'scripts' variable
2) call qsort to sort the 'scripts' according to the 'compare_scripts' function
3) call filter_scripts to store filtered scripts based on 'filter_term'(which is set to "" at first to match all scripts) in 'filtered_scripts' variable
4) call display to show the main program menu
5) wait for input of j or k to change 'current_index' of scripts list
6) wait for input of x to call 'execute_script' to run script at 'current_index'
7) wait for input of / to call 'get_filterterm' to get user inputted 'filter_term' and esc to reset it to empty
8) continue looping from 3) until input is q which exits the program */

int main() {
    enable_raw_mode();
    char **scripts = NULL, **filtered_scripts = NULL, filter_term[SEARCH_LENGTH] = "";
    int script_count = 0, filtered_count = 0, current_index = 0, issearch = 0;
    get_scripts(SCRIPT_PATH, &scripts, &script_count);
    qsort(scripts, script_count, sizeof(char *), compare_scripts);

    char ch;
    do {
        filter_scripts(scripts, script_count, filter_term, &filtered_scripts, &filtered_count);
        display(filtered_scripts, filtered_count, current_index, issearch, filter_term);
        ch = getchar();
        switch (ch) {
            case 'j':
                if (filtered_count) {
                    current_index = (current_index + 1) % (filtered_count);
                }
                break;
            case 'k':
                if (filtered_count) {
                    current_index = ((current_index - 1 + filtered_count) % (filtered_count));
                }
                break;
            case 'x':
                if (filtered_count) {
                    execute_script(filtered_scripts[current_index]);
                }
                break;
            case '/':
                get_filterterm(filter_term, &issearch, &current_index);
                break;
            case 27:
                issearch = 0;
                current_index = 0;
                strcpy(filter_term, "");
                break;
        }
    } while (ch != 'q');

    disable_raw_mode();
    for (int i = 0; i < script_count; i++) {
        free(scripts[i]);
    }
    free(scripts);
    free(filtered_scripts);

    return 0;
}
