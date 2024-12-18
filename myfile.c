#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_PATH_LENGTH 50
#define MAX_NAME_LENGTH 30
#define MAX_FILES 2000
#define MAX_STORES 3

typedef struct {
    char name[MAX_NAME_LENGTH];
    int storeNumber;
    char category[MAX_NAME_LENGTH];
    char filename[MAX_PATH_LENGTH];  // Save filename too
} Product;

Product products[MAX_FILES];  // 1-dimensional array to store all products
int product_count = 0;           // Number of products added

// Trim whitespace from a string
char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

// Process everything in a single function
void process_stores_and_categories() {
    const char *stores[] = { "./Dataset/Store1", "./Dataset/Store2", "./Dataset/Store3" };
    const char *categories[] = { "Apparel", "Beauty", "Digital", "Food", "Home", "Market", "Sports", "Toys" };

    for (int i = 0; i < 3; i++) {
        printf("Processing Store %d\n", i + 1);

        for (int j = 0; j < 8; j++) {
            char category_path[MAX_PATH_LENGTH];
            snprintf(category_path, sizeof(category_path), "%s/%s", stores[i], categories[j]);

            DIR *dir = opendir(category_path);
            if (!dir) {
                perror("Error opening category directory");
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

                char filepath[MAX_PATH_LENGTH];
                snprintf(filepath, sizeof(filepath), "%s/%s", category_path, entry->d_name);

                if (entry->d_type == DT_REG) {
                    FILE *file = fopen(filepath, "r");
                    if (!file) {
                        perror("Error opening product file");
                        continue;
                    }

                    char line[MAX_PATH_LENGTH];
                    char name[MAX_NAME_LENGTH] = {0};

                    while (fgets(line, sizeof(line), file)) {
                        if (strncmp(line, "Name:", 5) == 0) {
                            strncpy(name, trim_whitespace(line + 5), MAX_NAME_LENGTH - 1);

                            if (product_count >= MAX_FILES) {
                                printf("Reached the limit of stored products\n");
                                fclose(file);
                                closedir(dir);
                                return;
                            }

                            // Save product information to the array
                            strncpy(products[product_count].name, name, MAX_NAME_LENGTH - 1);
                            products[product_count].storeNumber = i + 1;
                            strncpy(products[product_count].category, categories[j], MAX_NAME_LENGTH - 1);

                            const char *filename = strrchr(filepath, '/');
                            if (filename) {
                                filename++;  // Move past the '/'
                                strncpy(products[product_count].filename, filename, MAX_PATH_LENGTH - 1);
                            }

                            product_count++;
                            break;
                        }
                    }

                    fclose(file);
                }
            }
            closedir(dir);
        }
    }
}

// Print all products stored in the 1-dimensional array
void print_Products() {
    for (int i = 0; i < product_count; i++) {
        printf("Store %d | Category: %s | Product: %s | Filename: %s\n",
               products[i].storeNumber, products[i].category, products[i].name, products[i].filename);
    }
    printf("Total products: %d\n", product_count);
}

int main() {
    process_stores_and_categories();
    print_Products();
    return 0;
}