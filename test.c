#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_PATH_LENGTH 50
#define MAX_NAME_LENGTH 30
#define MAX_FILES_PER_CATEGORY 700
#define MAX_CATEGORIES 10
#define MAX_STORES 3

typedef struct {
    char name[MAX_NAME_LENGTH];
    int storeNumber;
    char category[MAX_NAME_LENGTH];
} Product;

// 3D array to store extracted data
Product Products[2000];
int ProductCounts[MAX_STORES][MAX_CATEGORIES] = {0};

// Trim whitespace from a string
char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++; // Trim leading space
    if (*str == 0) return str;                 // All spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--; // Trim trailing space
    *(end + 1) = '\0';
    return str;
}

// Extract "Name:" from a file and save it in the 3D array
void extract_name_from_file(const char *filepath, int storeNumber, const char *category) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char line[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH] = {0};

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Name:", 5) == 0) {
            strncpy(name, trim_whitespace(line + 5), sizeof(name) - 1);
                
                strncpy(Product->name, name, MAX_NAME_LENGTH - 1);
                Product->storeNumber = storeNumber;
                strncpy(Product->category, category, MAX_NAME_LENGTH - 1);
                ProductCounts[storeNumber - 1][categoryIndex]++;
            break;
        }
    }

    fclose(file);
}

// Process directory and its subdirectories
void process_directory(const char *directory, int storeNumber) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursively process subdirectories (categories)
            process_directory(path, storeNumber);
        } else if (entry->d_type == DT_REG) {
            // Extract category (subdirectory name)
            char *category = strrchr(directory, '/');
            if (!category) category = directory;
            else category++; // Skip the slash
            extract_name_from_file(path, storeNumber, category);
        }
    }

    closedir(dir);
}

// Process all stores
void process_stores() {
    const char *stores[MAX_STORES] = {"./Dataset/Store1", "./Dataset/Store2", "./Dataset/Store3"};

    for (int i = 0; i < MAX_STORES; i++) {
        printf("Processing Store %d: %s\n", i + 1, stores[i]);
        process_directory(stores[i], i + 1);
    }
}

// Print all Products
void print_Products() {
    int count=0;
    for (int i = 0; i < MAX_STORES; i++) {
        printf("Store %d:\n", i + 1);
        for (int j = 0; j < MAX_CATEGORIES; j++) {
            for (int k = 0; k < ProductCounts[i][j]; k++) {
                Product *Product = &Products[i][j][k];
                printf("  Category: %s | Name: %s | Store: %d\n", Product->category, Product->name, Product->storeNumber);
                count++;
            }
        }
    }
    printf("%d",count);
}


int main() {
    process_stores();
    print_Products();
    return 0;
}
