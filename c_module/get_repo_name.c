#include <libgen.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
const int MAX_PATH_SIZE = 4096;
char *get_executable_dir() {
    char *path = calloc(MAX_PATH_SIZE, sizeof(char));
    ssize_t len = readlink("/proc/self/exe", path, MAX_PATH_SIZE - 1);
    if (len != -1) {
        path[len] = '\0';
        return path;
    } else {
        free(path);
        return NULL;
    }
}
#elif __APPLE__
#include <libproc.h>
const int MAX_PATH_SIZE = PROC_PIDPATHINFO_MAXSIZE;
char *get_executable_dir() {
    char *path = calloc(MAX_PATH_SIZE, sizeof(char));
    pid_t pid = getpid();
    if (proc_pidpath(pid, path, MAX_PATH_SIZE) > 0) {
        path[MAX_PATH_SIZE - 1] = '\0';
        return path;
    } else {
        free(path);
        return NULL;
    }
}
#else
char *get_executable_dir() {
    return NULL;
}
#endif


// Function to find the folder matching the pattern
char* find_folder(char *path, char *pattern) {
    const int MAX_PIECES = MAX_PATH_SIZE / 2;
    regex_t regex;
    regcomp(&regex, pattern, REG_EXTENDED);

    // Split the path into pieces
    char *pieces[MAX_PIECES];
    int num_pieces = 0;

    char *token = strtok(path, "/");
    while (token != NULL && num_pieces < MAX_PIECES - 1) {
        pieces[num_pieces++] = token;
        token = strtok(NULL, "/");
    }
    pieces[num_pieces] = NULL;  // Null-terminate the array of pieces

    // Check each piece against the regex pattern (traverse backwards)
    for (int i = num_pieces-1; i > 0; i--) {
        if (regexec(&regex, pieces[i], 0, NULL, 0) == 0) {
            regfree(&regex);
            return pieces[i];
        }
    }

    regfree(&regex);
    return NULL;
}


char *get_repo_name() {
    // Traverses-back the directory structure to find the name of the repo.
    // Must start with "so2024lab1g" (actually, using the current year)
    char *path = get_executable_dir();
    char *pattern = "so[0-9]{4}lab[0-9]g[0-9]{2}";
    char *repo_name = find_folder(path, pattern);

    return repo_name;
}



// int main() {
//     char *repo_name = get_repo_name();
//     if (repo_name == NULL) {
//         printf("Error: Could not find repo name\n");
//     } else {
//         printf("Repo name: %s\n", repo_name);
//     }
//     return 0;
// }