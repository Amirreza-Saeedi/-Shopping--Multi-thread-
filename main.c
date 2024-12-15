#define _GNU_SOURCE
#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>
#include <limits.h>
#include <sys/wait.h>  // wait()
#include <dirent.h>

#define COMMAND_INPUT 20
#define USRENAME_SIZE 20
#define ITEM_NAME_SIZE 20
#define MAX_ITEMS 10
#define INPUT_SIZE 100
#define STORE_PATH 128
#define MAX_FILES 100

// Global Mutex for race condition protection
pthread_mutex_t log_mutex;

// User's item list
char *user_items[MAX_ITEMS];
int user_item_count = 0;

// prototypes

typedef struct ThreadArgs {
    Order *orderPtr;
    char **file_info;
} ThreadArgs;

ThreadArgs constructThreadArgs(Order *orderPtr, char **file_info) {
    ThreadArgs threadArgs;
    threadArgs.orderPtr = orderPtr;
    threadArgs.file_info = file_info;
    return threadArgs;
}

typedef struct Item {
    char name[ITEM_NAME_SIZE];
    int quantity;
} Item;

Item constructItem(char *name, int quantity) {
    Item item;
    strcpy(item.name, name);
    item.quantity = quantity;
    return item;

}

typedef struct Order {
    int id;
    char username[USRENAME_SIZE];
    int nItems;
    Item items[MAX_ITEMS];
    float threshold;
} Order;

Order constructOrder(char username[], int id, Item items[MAX_ITEMS], int nItems, float threshold) {
    Order order;
    strcpy(order.username, username);
    order.id = id;
    order.nItems = nItems;
    for (int i = 0; i < nItems; i++) {
        order.items[i] = items[i];
    }
    order.threshold = threshold;
    return order;
}

void clr() {
    system("clear");
}

void pressAnyKeyToContinue() {
    printf("Press any key to continue...");
    getchar();
    getchar();
}

int getOrderId() {
    return 0;
}

void printError(const char msg[]) {  // todo input
    printf("\033[31mERROR: %s!\033[0m\n", msg);
}

void printChildCreation(pid_t parent, pid_t child, const char msg[]) {
    printf("PID %d create child  for %s PID: %d\n", parent, msg, child);
}

void pirntThreadCreation(pid_t parent, pthread_t thread, const char msg[]) {
    printf("PID %d create thread for %s TID: %ld\n", parent, msg, thread);
}

int is_item_in_user_list(const char *item) {
    for (int i = 0; i < user_item_count; i++) {
        if (strcmp(user_items[i], item) == 0) { // TODO: check user_items 
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


// thread task
void *read_file(void *args) {
    ThreadArgs *threadArgsPtr = (ThreadArgs *) args;
    char **file_info = threadArgsPtr->file_info;
    // char **file_info = (char **)args;
    char *store = file_info[0];
    char *category = file_info[1];
    char *file = file_info[2];
    Order *orderPtr = threadArgsPtr->orderPtr;
    Item items[] = orderPtr->items;
    int nItem = orderPtr->nItems;

    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    // name & score & entity
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

void handle_category(const char *store, const char *category_path, const char *category_name, Order *orderPtr) {
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


void handleStore(Order *orderPtr, const char *store_path) {
    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Error opening store");
        return;
    }
    // Get store directory name from Path name
    char *store_name = strrchr(store_path, '/');
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char category_path[256];
            snprintf(category_path, sizeof(category_path), "%s/%s", store_path, entry->d_name);

            pid_t pid = fork();

            if (pid == 0) { // Child process
                handle_category(store_name, category_path, entry->d_name, orderPtr);
                exit(0); // Exit after handling the category
            }
        }
    }

    // Wait for all child processes
    while (wait(NULL) > 0);

    closedir(dir);
}

void buyMenu() {
    puts("\tBUY MENU");

    // user
    char username[USRENAME_SIZE];
    printf("Username (max=%d): ", USRENAME_SIZE);
    scanf("%s", username);
    // todo exit option

    // user process
    pid_t pid = fork();
    if (pid < 0) {  // error
        printError("main fork() user failed");

    } else if (pid == 0) {  // child

        // 3 threads
        pthread_t   ordersThread, 
                    scoresThread, 
                    finalThread;
      
        // item
        int orderId = getOrderId();
        int nItems;
        Item items[MAX_ITEMS];
        printf("Number of items (max=%d): ", MAX_ITEMS);
        scanf("%d", &nItems);
        printf("ItemList%d <name> <quantity> (max=%d):\n", orderId, nItems);
        
        for (int i = 0; i < nItems; i++) {
            // name & quantity
            int quantity;
            char name[ITEM_NAME_SIZE];
            char input[INPUT_SIZE];  // todo fgets & sscanf
            printf("#%d. ", i + 1);
            scanf("%s %d", name, &quantity);
            items[i] = constructItem(name, quantity);
        }

        // threshold
        float threshold;
        printf("Thresholds: ");
        scanf("%f", &threshold);

        // order
        Order order = constructOrder(username, orderId, items, nItems, threshold);

        // 3 store processes 
        // store1
        pid_t store1Process = fork();
        if (store1Process < 0) {  // error
            printError("user fork() store1 failed");
        } else if (store1Process == 0) {  // child
            handleStore(&order, "./Dataset/Store1");

            _exit(NULL); // todo ok?
        } else {
            printChildCreation(getpid(), store1Process, "Store1");

        }

        // store2
        pid_t store2Process = fork();
        if (store2Process < 0) {  // error
            printError("user fork() store2 failed");
        } else if (store2Process == 0) {  // child
            handleStore(&order, "./Dataset/Store2");

            _exit(NULL); // todo ok?
        } else {
            printChildCreation(getpid(), store2Process, "Store2");

        }

        // store3
        pid_t store3Process = fork();
        if (store3Process < 0) {  // error
            printError("user fork() store3 failed");
        } else if (store3Process == 0) {  // child
            handleStore(&order, "./Dataset/Store3");

            _exit(NULL); // todo ok?
        } else {
            printChildCreation(getpid(), store3Process, "Store3");

        }


        _exit(NULL);  // todo ok?

    } else {  // parent
        // todo
        char str[INPUT_SIZE];
        sprintf(str, "uesr %s", username);
        printChildCreation(getpid(), pid, str);
        wait(NULL);  // drop status
    }

    pressAnyKeyToContinue();
}

void printMenu() {
    printf("\n\n");
    puts("\t MAIN MENU");
    puts(" 1. buy\n");
    puts(" 2. exit\n");
    printf("> ");
}

// TODO 
void menu() {

    while (true) {
        printMenu();
        char c = getc(stdin); // todo
        getchar();
        printf("%c\n", c);

        switch (c)
        {
        case '1':
            buyMenu();
            break;
        
        case '2':
            break;
        }
    }
    
}

int main(int argc, char const *argv[]) {   
    clr();
    puts("Hello World!");
    menu();
    return 0;
}
