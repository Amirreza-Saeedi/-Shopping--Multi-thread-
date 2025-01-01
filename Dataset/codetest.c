#include <stdio.h>
#include <string.h>

int main() {
    char str[] = "Hello,World,C,Programming"; // Input string
    const char delim[] = ",";                // Delimiter
    char *token;

    // Get the first token
    token = strtok(str, delim);

    // Walk through other tokens
    while (token != NULL) {
        printf("Token: %s\n", token);
        token = strtok(NULL, delim);
    }

    return 0;
}
