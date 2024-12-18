#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_FILES 3000

// Enum for operation types
typedef enum {
    OPERATION_NONE = 0,
    OPERATION_READ,
    OPERATION_WRITE
} OperationType;

// Structure for each file's shared data
typedef struct {
    char filename[256];        // File name
    pthread_mutex_t mutex;     // Mutex for thread-safe access
    OperationType operation;   // Current operation (read/write/none)
    struct UserOrder {
        int order_id;          // Example field
        char data[1024];       // Example field
    } user_order;              // Shared UserOrder structure
} FileData;

// Global array for all files
FileData file_data[NUM_FILES];


void *thread_function(void *arg) {
    int index = *(int *)arg; // Thread-specific file index
    FileData *data = &file_data[index];

    while (1) {
        pthread_mutex_lock(&data->mutex); // Lock the file data
        OperationType operation = data->operation;

        if (operation == OPERATION_READ) {
            // Perform file read operation
            FILE *file = fopen(data->filename, "r");
            if (file) {
                printf("Thread %d reading from file: %s\n", index, data->filename);
                // Example read logic
                fgets(data->user_order.data, sizeof(data->user_order.data), file);
                fclose(file);
            } else {
                perror("Error opening file for read");
            }
            data->operation = OPERATION_NONE; // Reset operation after reading
        } else if (operation == OPERATION_WRITE) {
            // Perform file write operation
            FILE *file = fopen(data->filename, "w");
            if (file) {
                printf("Thread %d writing to file: %s\n", index, data->filename);
                // Example write logic
                fprintf(file, "Order ID: %d\n", data->user_order.order_id);
                fprintf(file, "Data: %s\n", data->user_order.data);
                fclose(file);
            } else {
                perror("Error opening file for write");
            }
            data->operation = OPERATION_NONE; // Reset operation after writing
        }

        pthread_mutex_unlock(&data->mutex); // Unlock the file data
        usleep(10000); // Avoid busy waiting (adjust as needed)
    }

    return NULL;
}


int main() {
    pthread_t threads[NUM_FILES];
    int indices[NUM_FILES]; // Indices for passing to threads

    // Initialize file data and create threads
    for (int i = 0; i < NUM_FILES; i++) {
        snprintf(file_data[i].filename, sizeof(file_data[i].filename), "file_%d.txt", i);
        pthread_mutex_init(&file_data[i].mutex, NULL);
        file_data[i].operation = OPERATION_NONE;
        file_data[i].user_order.order_id = 0;
        strcpy(file_data[i].user_order.data, "");

        indices[i] = i;
        if (pthread_create(&threads[i], NULL, thread_function, &indices[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // Parent process logic to set operations
    while (1) {
        // Example: Set a read operation for file 0
        pthread_mutex_lock(&file_data[0].mutex);
        file_data[0].operation = OPERATION_READ;
        pthread_mutex_unlock(&file_data[0].mutex);

        sleep(2); // Simulate some time before another operation

        // Example: Set a write operation for file 1
        pthread_mutex_lock(&file_data[1].mutex);
        file_data[1].user_order.order_id = 42;
        strcpy(file_data[1].user_order.data, "Sample order data");
        file_data[1].operation = OPERATION_WRITE;
        pthread_mutex_unlock(&file_data[1].mutex);

        sleep(2); // Simulate some time before the next cycle
    }

    // Join threads (if you ever terminate)
    for (int i = 0; i < NUM_FILES; i++) {
        pthread_join(threads[i], NULL);
        pthread_mutex_destroy(&file_data[i].mutex);
    }

    return 0;
}
