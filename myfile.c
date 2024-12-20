#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to check if username:storename exists in userdiscount.log
int checkUserStore(const char *username, const char *storename) {
    const char *file_path = "./userdiscount.log";
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Unable to open file");
        return 0;
    }

    char line[256];
    int found = 0; // Flag to indicate if the username:storename pair is found

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';

        // Parse the line and check if it matches username:storename
        char *delimiter = strchr(line, ':');
        if (delimiter) {
            *delimiter = '\0'; // Split the string into username and storename
            const char *file_username = line;
            const char *file_storename = delimiter + 1;

            if (strcmp(file_username, username) == 0 && strcmp(file_storename, storename) == 0) {
                found = 1; // Pair found
                break;
            }
        }
    }

    fclose(file);
    return found;
}

int writeUserStore(const char *username, const char *storename) {
    const char *file_path = "./userdiscount.log";
    FILE *file = fopen(file_path, "a"); // Open in append mode
    if (!file) {
        perror("Unable to open file");
        return 0;
    }

    // Write the pair in the format username:storename followed by a newline
    if (fprintf(file, "\n%s:%s\n", username, storename) < 0) {
        perror("Error writing to file");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1; // Success
}

int main() {
    const char *username = "ads";
    const char *storename = "sr";
    //writeUserStore(username,storename);
    if (checkUserStore(username, storename)) {
        printf("The pair '%s:%s' exists in the log file.\n", username, storename);
    } else {
        printf("The pair '%s:%s' does not exist in the log file.\n", username, storename);
    }

    return 0;
}
