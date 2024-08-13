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
const int MAX_URL_SIZE = 1024;
const int MAX_RESPONSE_LINES = 1024;
const int MAX_SALT_VALUE = 50; // For some salt above 50, the string is weirdly encrypted
const char *UNKNOWN_USER_ID = "UNKNOWN_USER_ID";
char UNKNOWN_REPO_NAME[20] = "UNKNOWN_REPO_NAME";  // as array to allow encryption
char *DEFAULT_URL = "http://localhost:8000/delay/ping_pong";

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

    for (i = 0; i < length; i++){
        new_text[i] = new_text[i] ^ key;
    }
    strcpy(original, new_text);
}

char *str_to_hex(char *str) {
    // Converts string to hex
    // Returns malloced string that must be freed
    char *hex = calloc((sizeof(str) * 2) + 1, sizeof(char));
    int i;
    for (i = 0; i < strlen(str); i++) {
        sprintf(&hex[i * 2], "%02x", str[i]);
    }
    hex[i * 2] = '\0';
    return hex;
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
        lines[line_count++] = line;
        line = strtok(NULL, "\n");
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
    for (i = EXPECTED_LINES; i < line_count; i++) {
        line = lines[i];
        debug_printf("PING: Line: %s\n", line);
        if (line!= NULL && strncmp(line, "message-to-user: ", 17) == 0) {
            show_help_to_user(line + 17, msgs_count++);
        }
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
    char request[BUFFER_SIZE] = {0};
    char response[MAX_RESPONSE_SIZE] = {0};
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
             "Connection: close\r\n"
             "\r\n", path, host);

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
    // Returns NULL if not found, or a pointer to the repo name (that must be freed)
    char *result = NULL;
    char *path = get_executable_dir();
    char *pattern = "so[0-9]{4}lab[0-9]g[0-9]{2}";
    char *repo_name = find_folder(path, pattern);
    free(path);
    if (repo_name != NULL) {
        result = calloc(strlen(repo_name) + 1, sizeof(char));
        strcpy(result, repo_name);
    }
    return result;
}
/* end of get_repo_name */

char *get_and_hide_repo_name() {
    int salt = 0;
    int length = 0;
    char *result=NULL;
    char *repo_name = get_repo_name();
    if (repo_name == NULL) {
        debug_printf("Error: Could not find repo name\n");
        // to make it easier for the consumer to always free the returned pointer,
        // lets request memory and copy the default value
        repo_name = malloc(sizeof(UNKNOWN_REPO_NAME));
        strcpy(repo_name, UNKNOWN_REPO_NAME);
        return repo_name;
    } else {
        length = strlen(repo_name);
        // the salt is the last two digits of the repo name
        salt += atoi(repo_name + (length - 2));
        debug_printf("Extracted SALT: %d from repo_name: %s\n", salt, repo_name);
        xor_encrypt(repo_name, salt % MAX_SALT_VALUE);
        result = str_to_hex(repo_name);
        free(repo_name);
        return result;
    }
}

int ping_pong_loop(char *password) {
    int check_error = 0;
    int delay_id = 0;
    int delay_milliseconds = 0;
    char *repo_name = NULL;
    char username[MAX_USERNAME_SIZE] = {0};

    int http_status_code = 0;
    int request_error = 0;
    char PING_URL[MAX_URL_SIZE] = {0};
    char PONG_URL[MAX_URL_SIZE] = {0};
    char response_text[MAX_RESPONSE_SIZE] = {0}; // Buffer to hold the response

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
    repo_name = get_and_hide_repo_name();
    debug_printf("PING: Repo name: %s\n", repo_name);

    // Prepare the URL
    snprintf(PING_URL, sizeof(PING_URL), "%s?user_id=%s&md5=%s",
             URL(), username, repo_name);
    // As evil as Michael Gary Scott. Parameter is named "md5" but its not a md5. It's hex(encrypt(repo_name, salt)).
    free(repo_name);

    if (password != NULL) {
        snprintf(PING_URL, sizeof(PING_URL), "%s&password_to_win=%s", PING_URL, password);
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

int main(){
    ping_pong_loop(NULL);
    return 0;
}
