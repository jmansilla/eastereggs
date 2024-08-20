#include <stdio.h>
#include "obfuscated.c"

char *PASSWORD(){
    char *pwd = getenv("EXAMPLE_CLIENT_PASSWORD");
    if (pwd == NULL){
        return NULL;
    }
    return pwd;
}

int main(){
    char *pwd = PASSWORD();
    ping_pong_loop(pwd);
    return 0;
}
