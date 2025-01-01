#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT 1000   // Maximum input length
#define MAX_WORDS 900    // Maximum combined words length (excluding the integer)

// Function to parse words and an integer from input
void parseInput(char *words, int *number) {
    char input[MAX_INPUT];     // Input buffer
    printf("Enter words followed by an integer: ");
    
    // Read the entire line of input
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading input.\n");
        exit(1);
    }

    // Tokenize the input
    char *token = strtok(input, " ");  // Split input by spaces
    char temp[MAX_WORDS] = "";         // Temporary buffer to store words
    int lastInt = 0;                   // Temporary integer to check for the number
    
    while (token != NULL) {
        // Check if the current token is an integer
        char *endptr;
        lastInt = strtol(token, &endptr, 10); // Convert to integer
        
        if (*endptr == '\0' || *endptr == '\n') { // If the token is a valid integer
            *number = lastInt;  // Save the integer
        } else {
            // Append the word to the temporary buffer
            strcat(temp, token);
            strcat(temp, " ");  // Add space between words
        }
        
        token = strtok(NULL, " "); // Move to the next token
    }
    
    // Remove the trailing space from temp if it exists
    if (strlen(temp) > 0) {
        temp[strlen(temp) - 1] = '\0';
    }
    
    // Copy the combined words into the words buffer
    strcpy(words, temp);
}

int main() {
    char words[MAX_WORDS];  // Buffer to hold words
    int number;             // Variable to store the integer

    // Call the function to parse input
    parseInput(words, &number);

    // Display the results
    printf("Words: \"%s\"\n", words);
    printf("Integer: %d\n", number);

    return 0;
}