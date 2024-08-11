// This is an easter egg whose purpose is to sleep for a certain amount of time.
// The amount of time to sleep is decided externally on a server, but the idea is that each time will take longer.
// If you find this, DO NOT REMOVE IT, but instead change the value of EASTER_EGG_DISCOVERED to "true" to WIN the game.
// DO NOT SPREAD THE WORD. It's a challenge for each team to discover it.

#include <ctype.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "get_repo_name.c"


#define MAX_RESPONSE_SIZE (1<<20)
const int MAX_USERNAME_SIZE = 256;
const int MAX_URL_SIZE = 1024;
const int MAX_RESPONSE_LINES = 1024;

const char *UNKNOWN_USER_ID = "UNKNOWN_USER_ID";
char UNKNOWN_REPO_NAME[20] = "UNKNOWN_REPO_NAME";  // as array to allow encryption
const char *BASE_URL = "http://localhost:8000/delay/ping_pong";

// FIXME: Make it possible to disable the easter egg with ENV variables

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
// call my_encrypt with each of them and later set DECRYPTED to 1
int DECRYPTED = 0;
char ANTIGREP_VAR[14] = "fkhuyaczubofz\0"; // as array to allow encryption
char ANTIGREP_MSG[85] = "=~E\nEGC^\n^BCY\nGOYYKMOY\nYO^\n^BO\nOD\\CXEDGOD^\n\\KXCKHFO\nfkhuyaczubofz\027\e\0"; // as array to allow encryption


void my_encrypt(char *original, int salt){
    // WARNING: original CAN NOT be a string literal (since they are inmutable)
    // Modifies text in place. Text must be null terminated.
    // Calling this function again with the same salt shall revert the string back to its original state
    int i = 0;
    int key = salt + 42;
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
    // Returns malloced string
    char *hex = calloc((sizeof(str) * 2) + 1, sizeof(char));
    int i;
    for (i = 0; i < strlen(str); i++) {
        sprintf(&hex[i * 2], "%02x", str[i]);
    }
    hex[i * 2] = '\0';
    return hex;
}

char *YELLOW_BG = "\033[30;43m";
char *YELLOW_FG = "\033[33;40m";
char *NORMAL    = "\033[0m";
void show_help_to_user(const char *msg, int order){
    if (DECRYPTED == 0) {
        DECRYPTED = 1;
        my_encrypt(ANTIGREP_VAR, 0);
        my_encrypt(ANTIGREP_MSG, 0);
    }

    char* SKIP_HELP = getenv(ANTIGREP_VAR);
    if (SKIP_HELP == NULL || SKIP_HELP[0] != '1') {
        if (order == 0) {
            // Only show this message once per PINGPONG_LOOP
            printf("%s%s%s\n", YELLOW_FG, ANTIGREP_MSG, NORMAL);
        }
        printf("%s%s%s\n", YELLOW_BG, msg, NORMAL);
    }
}

char *get_url(){
    char *url = getenv("PP_URL");
    if (url == NULL){
        return "http://localhost:8000/delay/ping_pong";
    }
    return url;
}

// FIXME: Make it possible to disable the easter egg with ENV variables

// Callback function to handle the response data
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    strcat(userp, contents);
    return size * nmemb;
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


    for (i = EXPECTED_LINES; i < line_count; i++) {
        line = lines[i];
        debug_printf("PING: Line: %s\n", line);
        if (line!= NULL && strncmp(line, "message-to-user: ", 17) == 0) {
            show_help_to_user(line + 17, i - EXPECTED_LINES);
        }
    }

    return 0; // Success
}

char *get_and_hide_repo_name() {
    int salt = 0;
    int length = 0;
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
        my_encrypt(repo_name, salt);
        repo_name = str_to_hex(repo_name);
        return repo_name;;
    }
}

int ping_pong_loop() {
    int check_error = 0;
    int delay_id = 0;
    int delay_milliseconds = 0;
    char* PP_DEBUG = getenv("PP_DEBUG");
    if (PP_DEBUG != NULL && PP_DEBUG[0] != '0') {
        DEBUG = 1;
    }

    // Get the username
    char username[256];
    if (getlogin_r(username, sizeof(username)) != 0) {
        debug_printf("getlogin_r failed\n");
        strcpy(username, UNKNOWN_USER_ID);
    }
    char *repo_name = get_and_hide_repo_name();
    printf("PING: Repo name: %s\n", repo_name);

    // Prepare the URL
    char PING_URL[1024];
    snprintf(PING_URL, sizeof(PING_URL), "%s?user_id=%s&md5=%s",
             get_url(), username, repo_name);
    // As evil as Michael Gary Scott. Parameter is named "md5" but its not a md5. It's hex(encrypt(repo_name, salt)).
    debug_printf("PING: URL: %s\n", PING_URL);

    // Initialize libcurl
    long http_code = 0;
    CURL *session;
    CURLcode res;
    char response_text[MAX_RESPONSE_SIZE] = {0}; // Buffer to hold the response

    curl_global_init(CURL_GLOBAL_DEFAULT);
    session = curl_easy_init();
    if (session) {
        curl_easy_setopt(session, CURLOPT_URL, PING_URL);
        curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(session, CURLOPT_WRITEDATA, response_text);

        // Perform the request
        res = curl_easy_perform(session);
        curl_easy_getinfo(session, CURLINFO_RESPONSE_CODE, &http_code);

        // Check for errors
        if (res != CURLE_OK) {
            debug_printf("PING: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Process the response
            debug_printf("PING: HTTP code: %ld\n", http_code);
            if (http_code == 200) {
                debug_printf("PING: Response: %s\n", response_text);
            }

            check_error = process_ping_response(response_text, &delay_milliseconds, &delay_id);
            if (check_error != 0) {
                debug_printf("PING: process_ping_response() failed: %d\n", check_error);
            } else {
                debug_printf("PING: delay_id: %d; delay_milliseconds: %d\n", delay_id, delay_milliseconds);
                msleep((long)delay_milliseconds);

                debug_printf("PING: Milliseconds exhausted. Starting PONG.\n");
                char PONG_URL[1024];
                snprintf(PONG_URL, sizeof(PONG_URL), "%s&closing_pp_id=%d", PING_URL, delay_id);
                debug_printf("PONG: URL: %s\n", PONG_URL);
                response_text[0] = '\0'; // Reset the buffer
                curl_easy_setopt(session, CURLOPT_URL, PONG_URL);
                res = curl_easy_perform(session);
                if (res != CURLE_OK) {
                    debug_printf("PONG: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                } else {
                    // Process the response
                    debug_printf("PONG: Response: %s\n", response_text);
                }
            }
        }

        // Cleanup
        curl_easy_cleanup(session);
    }
    curl_global_cleanup();
    return 0;
}


int main(){
    ping_pong_loop();
    return 0;
}
