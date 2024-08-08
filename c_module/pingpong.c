// This is an easter egg whose purpose is to sleep for a certain amount of time.
// The amount of time to sleep is decided externally on a server, but the idea is that each time will take longer.
// If you find this, DO NOT REMOVE IT, but instead change the value of EASTER_EGG_DISCOVERED to "true" to WIN the game.
// DO NOT SPREAD THE WORD. It's a challenge for each team to discover it.

#include <ctype.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

//#define PP_DEBUG 1
#ifdef PP_DEBUG
# define PP_DEBUG_PRINT(x) printf x
#else
# define PP_DEBUG_PRINT(x) do {} while (0)
#endif

// use it like this
// PP_DEBUG_PRINT(("var1: %d; var2: %d; str: %s\n", var1, var2, str));


const char *GROUP_NUMBER = "so2024lab1g05";  // Replace with the group number
const char *KEY = "KOKO";  // Replace with the key of each Group
const char *EASTER_EGG_DISCOVERED = "false";  // Congrats, you discovered it! Change to "true" and you're done!

const char *BASE_URL = "http://localhost:8000/delay/ping_pong";

// FIXME: Make it possible to disable the easter egg with ENV variables
// FIXME: Make it to register if was running tests (register such thing, but do not sleep/increase-delay in that case)

// Callback function to handle the response data
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    strcat(userp, contents);
    return size * nmemb;
}


/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
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
    const int MAX_LINES = 5;
    const int EXPECTED_LINES = 3;
    char text_copy[1024];
    char *lines[MAX_LINES];
    int line_count = 0;

    // Copy response_text to a modifiable buffer
    strncpy(text_copy, response_text, sizeof(text_copy));
    text_copy[sizeof(text_copy) - 1] = '\0'; // Ensure null-termination

    // Split the response text into lines
    char *line = strtok(text_copy, "\n");
    while (line != NULL && line_count < MAX_LINES) {
        lines[line_count++] = line;
        line = strtok(NULL, "\n");
    }

    // Check if we have exactly EXPECTED_LINES (3) lines
    if (line_count != EXPECTED_LINES) {
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

    return 0; // Success
}


int ping_pong_loop() {
    int check_error = 0;
    int delay_id = 0;
    int delay_milliseconds = 0;

    // Get the username
    char username[256];
    if (getlogin_r(username, sizeof(username)) != 0) {
        PP_DEBUG_PRINT(("getlogin_r"));
        return -1;
    }

    // Prepare the URL
    char PING_URL[1024];
    snprintf(PING_URL, sizeof(PING_URL),
             "%s?user_id=%s&group=%s&key=%s&discovered=%s",
            BASE_URL, username, GROUP_NUMBER, KEY, EASTER_EGG_DISCOVERED);
    PP_DEBUG_PRINT(("PING: URL: %s\n", PING_URL));

    // Initialize libcurl
    CURL *curl;
    CURLcode res;
    char response_text[4096] = {0}; // Buffer to hold the response

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, PING_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_text);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            PP_DEBUG_PRINT(("PING: curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
        } else {
            // Process the response
            PP_DEBUG_PRINT(("PING: Response: %s\n", response_text));

            check_error = process_ping_response(response_text, &delay_milliseconds, &delay_id);
            if (check_error != 0) {
                PP_DEBUG_PRINT(("PING: process_ping_response() failed: %d\n", check_error));
            } else {
                PP_DEBUG_PRINT(("PING: delay_id: %d; delay_milliseconds: %d\n", delay_id, delay_milliseconds));
                msleep((long)delay_milliseconds);

                PP_DEBUG_PRINT(("PING: Milliseconds exhausted. Starting PONG.\n"));
                char PONG_URL[1024];
                snprintf(PONG_URL, sizeof(PONG_URL), "%s&closing_pp_id=%d", PING_URL, delay_id);
                curl_easy_setopt(curl, CURLOPT_URL, PONG_URL);
                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    PP_DEBUG_PRINT(("PONG: curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
                } else {
                    // Process the response
                    PP_DEBUG_PRINT(("PONG: Response: %s\n", response_text));
                }
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}


int main(){
    ping_pong_loop();
    return 0;
}
