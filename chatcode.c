#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>

#define MAX_FILES 100
#define MAX_ITEMS 50

// Global Mutexs for race condition protection
pthread_mutex_t log_mutex[3];

// User's item list
char *user_items[MAX_ITEMS];
int user_item_count = 0;

// Function to check if an item matches the user's list
int is_item_in_user_list(const char *item) {
    for (int i = 0; i < user_item_count; i++) {
        if (strcmp(user_items[i], item) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to log matched items
void log_match(const char *item, const char *store, const char *category, const char *file, pthread_t tid) {
    pthread_mutex_lock(&log_mutex); // Lock the log file for thread-safe access

    FILE *log_file = fopen("user1_log1.log", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    fprintf(log_file, "Item: %s, Thread ID: %lu, Store: %s, Category: %s, File: %s\n",
            item, tid, store, category, file);

    fclose(log_file);
    pthread_mutex_unlock(&log_mutex); // Unlock after writing
}

// Thread function to read a file
void *read_file(void *args) {
    char **file_info = (char **)args;
    char *store = file_info[0];
    char *category = file_info[1];
    char *file = file_info[2];

    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char item[128], price[32], quantity[32];
        if (sscanf(line, "%127s %31s %31s", item, price, quantity) == 3) {
            if (is_item_in_user_list(item)) {
                log_match(item, store, category, file, pthread_self());
            }
        }
    }

    fclose(fp);
    pthread_exit(NULL);
}

// Function to handle a category (create threads for each file)
void handle_category(const char *store, const char *category_path, const char *category_name) {
    DIR *dir = opendir(category_path);
    if (dir == NULL) {
        perror("Error opening category");
        return;
    }

    struct dirent *entry;
    pthread_t threads[MAX_FILES];
    char *file_info[MAX_FILES][3];
    int thread_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Regular file
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", category_path, entry->d_name);

            // Allocate file info
            file_info[thread_count][0] = strdup(store);
            file_info[thread_count][1] = strdup(category_name);
            file_info[thread_count][2] = strdup(file_path);

            // Create a thread for each file
            if (pthread_create(&threads[thread_count], NULL, read_file, file_info[thread_count]) != 0) {
                perror("Error creating thread");
            }
            thread_count++;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);

        // Free file info
        free(file_info[i][0]);
        free(file_info[i][1]);
        free(file_info[i][2]);
    }

    closedir(dir);
}

// Function to handle a store (create processes for each category)
void handle_store(const char *store_path, const char *store_name) {
    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Error opening store");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char category_path[256];
            snprintf(category_path, sizeof(category_path), "%s/%s", store_path, entry->d_name);

            pid_t pid = fork();
            if (pid == 0) { // Child process
                handle_category(store_name, category_path, entry->d_name);
                exit(0); // Exit after handling the category
            }
        }
    }

    // Wait for all child processes
    while (wait(NULL) > 0);

    closedir(dir);
}

int main() {
    // Initialize mutex
    pthread_mutex_init(&log_mutex, NULL);

    // User's item list (for simplicity, hardcoded here)
    user_items[0] = "item1";
    user_items[1] = "item2";
    user_items[2] = "item3";
    user_item_count = 3;

    const char *base_path = "./stores";
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        perror("Error opening base directory");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char store_path[256];
            snprintf(store_path, sizeof(store_path), "%s/%s", base_path, entry->d_name);

            pid_t pid = fork();
            if (pid == 0) { // Child process
                handle_store(store_path, entry->d_name);
                exit(0); // Exit after handling the store
            }
        }
    }

    // Wait for all child processes
    while (wait(NULL) > 0);

    closedir(dir);

    // Destroy mutex
    pthread_mutex_destroy(&log_mutex);
    return 0;
}
