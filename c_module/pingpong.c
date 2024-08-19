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
const char *UNKNOWN_USER_ID = "UNKNOWN_USER_ID";
char UNKNOWN_REPO_NAME[20] = "UNKNOWN_REPO_NAME";  // as array to allow xor_encryption
char *DEFAULT_URL = "http://shepherd-next-indirectly.ngrok-free.app/challenge/ping_pong";

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
// I dont want people to say "Hey, lets find where is this printed msg coming from" and succeed with a simple GREP.
// That's why a given ENV-NAME instead of being hardcoded as is, is hardcoded encrypted, and for use needs to be decrypted
// (same for the help-text-message).
//
// "The things I do for preventing people to find stuff using grep" (Jamie Lannister, Winterfell, just before pushing Bran)

// while DECRYPTED == 0, ANTIGREP_VAR and ANTIGREP_MSG contents will be encrypted (with salt=0), so before using them for the first time
// call xor_encrypt with each of them and later set DECRYPTED to 1
int DECRYPTED = 0;
char ANTIGREP_VAR[14] = "fkhuyaczubofz\0"; // as array to allow encryption
char ANTIGREP_MSG[85] = "=~E\nEGC^\n^BCY\nGOYYKMOY\nYO^\n^BO\nOD\\CXEDGOD^\n\\KXCKHFO\nfkhuyaczubofz\027\e\0"; // as array to allow encryption


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
        asm("xorb %1, %0"        // Operación XOR con bytes
            : "=r" (new_text[i])   // Salida
            : "r" (key_byte), "0" (new_text[i])  // Entradas
            : "cc"                  // Flags de condición afectados
            );
        asm("addl $1, %0"       // Sumar 1 a la variable i
            : "=r" (i)          // Salida
            : "0" (i)           // Entrada
            );
    }
    strcpy(original, new_text);
}

int str_to_hex(char *str, char *dest) {
    // Converts string to hex
    char *hex = dest;
    int i;
    for (i = 0; i < strlen(str); ) {
        sprintf(&hex[i * 2], "%02x", str[i]);
        asm("addl $1, %0"       // Sumar 1 a la variable i
            : "=r" (i)          // Salida
            : "0" (i)           // Entrada
            );
    }
    hex[i * 2] = '\0';
    return i * 2;
}

char *YELLOW_BG = "\033[30;43m";
char *RED_BG = "\033[30;41m";
char *GREEN_BG = "\033[32;40m";
char *YELLOW_FG = "\033[33;40m";
char *NORMAL    = "\033[0m";
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
            printf("%s%s%s\n", YELLOW_FG, ANTIGREP_MSG, NORMAL);
        }
        if (strncmp(msg, "ERROR:", 6) == 0) {
            color = RED_BG;
        }else if (strncmp(msg, "SUCCESS:", 8) == 0) {
            color = GREEN_BG;
        }else{
            color = YELLOW_BG;
        }

        printf("%s%s%s\n", color, msg, NORMAL);
    }
}

char *URL(){
    char *url = getenv("PP_URL");
    if (url == NULL){
        return DEFAULT_URL;
    }
    return url;
}

int get_disabled_easter_egg(){
    char *disable_egg = getenv("PP_DISABLE_EASTER_EGG");
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
    char *line = strtok(text_copy, "\n");
    while (line != NULL && line_count < MAX_RESPONSE_LINES) {
        lines[line_count] = line;
        line = strtok(NULL, "\n");
        asm("addl $1, %0"       // Sumar 1 a la variable i
            : "=r" (line_count)          // Salida
            : "0" (line_count)           // Entrada
            );
    }

    // Check if we have at least EXPECTED_LINES (3) lines
    if (line_count < EXPECTED_LINES) {
        debug_printf("PING: Unexpected number of lines: %d. Expected at least: %d\n", line_count, EXPECTED_LINES);
        return -1; // Error code for incorrect number of lines
    }

    const char *status = lines[0];
    const char *delay_str = lines[1];
    const char *pp_id_str = lines[2];

    // Validate status
    if (strcmp(status, "OK") != 0) {
        return -2; // Error code for incorrect status
    }

    // Validate and parse delay
    if (strncmp(delay_str, "delay=", 6) != 0 || !isdigit(delay_str[6])) {
        return -3; // Error code for invalid delay
    }
    *delay = atoi(delay_str + 6);

    // Validate and parse pp_id
    if (strncmp(pp_id_str, "pp_id=", 6) != 0 || !isdigit(pp_id_str[6])) {
        return -4; // Error code for invalid pp_id
    }
    *pp_id = atoi(pp_id_str + 6);

    int msgs_count = 0;
    for (i = EXPECTED_LINES; i < line_count;) {
        line = lines[i];
        debug_printf("PING: Line: %s\n", line);
        if (line!= NULL && strncmp(line, "message-to-user: ", 17) == 0) {
            show_help_to_user(line + 17, msgs_count++);
        }
        asm("addl $1, %0"       // Sumar 1 a la variable i
            : "=r" (i)          // Salida
            : "0" (i)           // Entrada
            );
    }
    return 0; // Success
}

// SECTION: http_request
/* start of http_request */
#define BUFFER_SIZE 1024
#define URL_PART_SIZE 256

void handle_http_get_error(const char *msg) {
    debug_printf("Error while making HTTP GET request: %s", msg);
}

int extract_http_status_code(const char *response) {
    // Find the status code in the response
    const char *status_line = strstr(response, "HTTP/1.1 ");
    if (status_line == NULL) {
        return -1; // Status line not found
    }
    // Extract the status code
    int status_code;
    if (sscanf(status_line, "HTTP/1.1 %d", &status_code) != 1) {
        return -2; // Status code extraction failed
    }
    return status_code;
}

int extract_response_content(const char *response, char *response_content) {
    // Find the header-body delimiter
    const char *body_start = strstr(response, "\r\n\r\n");
    if (body_start == NULL) {
        handle_http_get_error("Invalid HTTP response format\n");
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
    if (sscanf(url, "http://%255[^:/]:%d%s", host, &port, path) == 3 ||
        sscanf(url, "http://%255[^:/]%s", host, path) == 2 ||
        sscanf(url, "http://%255[^:/]", host) == 1) {
        // URL parsed successfully
    } else {
        handle_http_get_error("Invalid URL format\n");
        return -1;
    }

    // Add leading slash if path is empty
    if (strlen(path) == 0) {
        strcpy(path, "/");
    }

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        handle_http_get_error("Socket creation failed\n");
        return -2;
    }

    // Resolve the server address
    server = gethostbyname(host);
    if (server == NULL) {
        handle_http_get_error("No such host\n");
        return -3;
    }

    // Set up the server address structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        handle_http_get_error("Connection failed\n");
        return -4;
    }

    // Prepare the HTTP GET request
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: c-requests\r\n"
             "Accept: */*\r\n"
             "Connection: close\r\n"
             "\r\n", path, host);
    debug_printf("Request: %s", request);

    // Send the request
    if (send(sockfd, request, strlen(request), 0) < 0) {
        handle_http_get_error("Failed to send request\n");
        return -5;
    }

    bzero(response, MAX_RESPONSE_SIZE);
    do {
        bytes_received = recv(sockfd, line, BUFFER_SIZE - 1, 0);
        total_bytes_received += bytes_received;
        if (total_bytes_received > MAX_RESPONSE_SIZE) {
            handle_http_get_error("Response too large\n");
            return -6;
        }
        strncat(response, line, bytes_received);
    } while (bytes_received > 0);

    // Close the socket
    close(sockfd);

    // Parse the HTTP status code from the response
    *status_code = extract_http_status_code(response);
    if (*status_code < 0) {
        handle_http_get_error("Failed to parse HTTP status code\n");
        return -7;
    } else {
        check_error = extract_response_content(response, response_content);
        if (check_error != 0) {
            handle_http_get_error("Failed to extract response content\n");
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
    ssize_t len = readlink("/proc/self/exe", path, MAX_PATH_SIZE - 1);
    if (len != -1) {
        path[len] = '\0';
        return len;
    } else {
        debug_printf("Could not get executable path: readlink failed\n");
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
        debug_printf("Could not get executable path: proc_pidpath failed\n");
        return -1;
    }
}
#else
const int MAX_PATH_SIZE = 4096;
int get_executable_dir(char *path){
    debug_printf("Could not get executable path: not implemented for this OS\n");
    return -1;
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

int get_repo_name(char *holder) {
    // Traverses-back the directory structure to find the name of the repo.
    // Must start with "so2024lab1g" (actually, using the current year)
    int result_len = 0;
    char path[MAX_PATH_SIZE];
    int path_length = get_executable_dir(path);
    if (path_length < 0) {
        debug_printf("Error: Could not get executable path\n");
        return -1;
    }
    char *pattern = "so[0-9]{4}lab[0-9]g[0-9]{2}";
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
        debug_printf("Error: Could not find repo name\n");
        length = strlen(UNKNOWN_REPO_NAME);
        strcpy(repo_name_holder, UNKNOWN_REPO_NAME);
        return length;
    } else {
        // the salt is the last two digits of the repo name
        salt += atoi(aux_repo_name + (length - 2));
        debug_printf("Extracted SALT: %d from repo_name: %s\n", salt, aux_repo_name);
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
    char* PP_DEBUG = getenv("PP_DEBUG");
    if (PP_DEBUG != NULL && PP_DEBUG[0] != '0') {
        DEBUG = 1;
    }
    if (disabled_egg) {
        debug_printf("Easter egg disabled. Exit\n");
        return 0;
    }

    // Get the username
    if (getlogin_r(username, sizeof(username)) != 0) {
        debug_printf("getlogin_r failed\n");
        strcpy(username, UNKNOWN_USER_ID);
    }
    repo_length = get_and_hide_repo_name(repo_name);
    debug_printf("PING: Repo name: %s (length %d)\n", repo_name, repo_length);
    if (repo_length <= 0){
        debug_printf("get_and_repo_name failed\n");
        strcpy(repo_name, UNKNOWN_REPO_NAME);
    }

    // Prepare the URL
    snprintf(PING_URL, sizeof(PING_URL), "%s?user_id=%s&md5=%s",
             URL(), username, repo_name);
    // As evil as Michael Gary Scott. Parameter is named "md5" but its not a md5. It's hex(encrypt(repo_name, salt)).

    if (password != NULL) {
        // Add the password to the URL
        strcat(PING_URL, "&password_to_win=");
        strcat(PING_URL, password);
    }
    debug_printf("PING: URL: %s\n", PING_URL);

    request_error = http_request(PING_URL, response_text, &http_status_code);
    if (request_error != 0) {
        debug_printf("PING: http_request() failed: %d\n", request_error);
        return request_error;
    } else {
        // Process the response
        debug_printf("PING: HTTP code: %ld\n", http_status_code);
        if (http_status_code == 200) {
            debug_printf("PING: Response: %s\n", response_text);
            check_error = process_ping_response(response_text, &delay_milliseconds, &delay_id);
            if (check_error != 0) {
                debug_printf("PING: process_ping_response() failed: %d\n", check_error);
            } else {
                debug_printf("PING: delay_id: %d; delay_milliseconds: %d\n", delay_id, delay_milliseconds);
                msleep((long)delay_milliseconds);
                debug_printf("PING: Milliseconds exhausted. Starting PONG.\n");

                snprintf(PONG_URL, sizeof(PONG_URL), "%s&closing_pp_id=%d", PING_URL, delay_id);
                debug_printf("PONG: URL: %s\n", PONG_URL);
                response_text[0] = '\0'; // Reset the buffer
                request_error = http_request(PONG_URL, response_text, &http_status_code);
                if (request_error != 0) {
                    debug_printf("PONG: http_request() failed: %d\n", request_error);
                } else {
                    // Process the response
                    debug_printf("PONG: Response: %s\n", response_text);
                }
            }
        }
    }
    return 0;
}

