#define _GNU_SOURCE
#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>
// #include <limits.h>  // CONFLICTS WITH dirnet.h
#include <sys/wait.h>  // wait()
#include <dirent.h>

#define COMMAND_INPUT 20
#define USRENAME_SIZE 20
#define ITEM_NAME_SIZE 32
#define MAX_ITEMS 10
#define INPUT_SIZE 100
#define FILE_PATH_SIZE 256
#define MAX_FILES 100
#define DATE_SIZE 16
#define TIME_SIZE DATE_SIZE
#define TITLE_SIZE 32
#define STORE_SIZE 20
#define CATEGORY_SIZE 16
#define LOG_NAME_SIZE 64
#define MAX_STORES 3

// Global Mutexs for race condition protection
pthread_mutex_t log_mutex[3];

// prototypes
void clr();
void pressAnyKeyToContinue();
int getOrderId();
void printError(const char msg[]);
void printChildCreation(pid_t parent, pid_t child, const char msg[]);
void pirntThreadCreation(pid_t parent, pthread_t thread, const char msg[]);
int findItemInUserOrder(Item *items, int nItems, char *name);
int getStoreIDFromName (char *name);
void log_match(Log *log);
void *read_file(void *args);
void handle_category(const char *store, const char *category_path, const char *category_name, Order *orderPtr);
void handleStore(Order *orderPtr, const char *store_path);
void parse_log_file(const char *filepath, float *sum_buyingValue, float *sum_price);
void *ordersThreadTask(void *args);
void buyMenu();
void printMenu();
void menu();

typedef struct OrdersThread {
    char username[USRENAME_SIZE];
    int order_id;
    float total_price;
} OrdersThread;

OrdersThread constructOrdersThread (char *username, int order_id, float total_price) {
    OrdersThread ot;
    strcpy(ot.username, username);
    ot.order_id = order_id;
    ot.total_price = total_price;
    return ot;
}

typedef struct Log {
    char item[ITEM_NAME_SIZE];
    char store[STORE_SIZE];
    char category[CATEGORY_SIZE];
    char file[FILE_PATH_SIZE];
    pid_t categoryPID;
    pthread_t tid;
    char username[USRENAME_SIZE];
    int orderId;
    float buyingValue;
    float price;
} Log;

Log constructLog(char *item, char *store, char *category, char *file, pid_t categoryPID, pthread_t tid, char *username, int orderId, float buyingValue, float price) {
    Log log;
    strcpy(log.item, item);
    strcpy(log.store, store);
    strcpy(log.category, category);
    strcpy(log.file, file);
    log.categoryPID = categoryPID;
    log.tid = tid;
    strcpy(log.username, username);
    log.orderId = orderId;
    log.buyingValue = buyingValue;
    log.price = price;
    return log;
}

typedef struct FileContent {
    char name[ITEM_NAME_SIZE];
    float price;
    float score;
    int entity;
    char date[DATE_SIZE];
    char time[TIME_SIZE];
} FileContent;

FileContent constructFileContent(char *name, float price, float score, int entity, char *date, char *time) {
    FileContent fileContent;
    strcpy(fileContent.name, name);
    fileContent.price = price;
    fileContent.score = score;
    fileContent.entity = entity;
    strcpy(fileContent.date, date);
    strcpy(fileContent.time, time);
    return fileContent;
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


typedef struct ThreadArgs {
    Order *orderPtr;
    char store[STORE_SIZE];
    char category[CATEGORY_SIZE];
    char file[FILE_PATH_SIZE];
    pid_t categoryPID;
} ThreadArgs;

ThreadArgs constructThreadArgs(Order *orderPtr, char store[STORE_SIZE], char category[CATEGORY_SIZE], char file[FILE_PATH_SIZE], pid_t categoryPID) {
    ThreadArgs threadArgs;
    threadArgs.orderPtr = orderPtr;
    strcpy(threadArgs.store, store);
    strcpy(threadArgs.category, category);
    strcpy(threadArgs.file, file);    
    threadArgs.categoryPID = categoryPID;
    return threadArgs;
}

void clr() {
    system("clear");
}

void pressAnyKeyToContinue() {
    printf("Press any key to continue...");
    getchar();
    getchar();
}

int getOrderId() {  // todo to get user last order id
    return 0;
}

void printError(const char msg[]) {  // todo input
    printf("\033[31mERROR: %s!\033[0m\n", msg);
}

void printChildCreation(pid_t parent, pid_t child, const char msg[]) {
    printf("~ PID %d create child  for %s PID: %d\n", parent, msg, child);
}

void pirntThreadCreation(pid_t parent, pthread_t thread, const char msg[]) {
    printf("~ PID %d create thread for %s TID: %ld\n", parent, msg, thread);
}

int findItemInUserOrder(Item *items, int nItems, char *name) {
    for (int i = 0; i < nItems; i++) {
        if (strcmp(items[i].name, name) == 0) { 
            return i;
        }
    }
    return -1;
}

int getStoreIDFromName (char *name) {
    switch (name[5])
    {
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    default:
        return -1;
    }
}

// Function to log matched items
void log_match(Log *log) {

    int storeID = getStoreIDFromName(log->store);
    int mutexIdx = storeID - 1;
    pthread_mutex_lock(&log_mutex[mutexIdx]); // Lock the log file for thread-safe access

    char logFileName[LOG_NAME_SIZE];
    char storeName[STORE_SIZE];
    sprintf(storeName, "Store%d", storeID);
    sprintf(logFileName, "Dataset/%s/%s_%d.log", storeName, log->username, log->orderId);
    FILE *log_file = fopen(logFileName, "a");
    if (log_file == NULL) {
        printError("Error opening log file");
        pthread_mutex_unlock(&log_mutex[mutexIdx]);
        return;
    }
    //Item,ThreadID,Store,Category,File,CategoryPID,buyingValue(float),price(float)"
    fprintf(log_file, "%s,%ld,%s,%s,%s,%d,%f,%f\n",
            log->item, log->tid, log->store, log->category, log->file, log->categoryPID,log->buyingValue,log->price);

    fclose(log_file);
    pthread_mutex_unlock(&log_mutex[mutexIdx]); // Unlock after writing
}



// thread task
void *read_file(void *args) {
    // unpacking
    ThreadArgs *threadArgsPtr = (ThreadArgs *) args;
    
    //char ***file_info = threadArgsPtr->file_info;
    // char *store = threadArgsPtr->store;
    // char *category = threadArgsPtr->category;
    // char *file = threadArgsPtr->file;
    
    Order *orderPtr = threadArgsPtr->orderPtr;
    Item *items = orderPtr->items;
    int nItems = orderPtr->nItems;
    int orderId = orderPtr->id;
    char *username = orderPtr->username;
    
    pid_t categoryPID = threadArgsPtr->categoryPID;

    // printf("file = %s ThreadID: %ld\n", threadArgsPtr->file,pthread_self());
    FILE *f = fopen(threadArgsPtr->file, "r");
    if (f == NULL) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    char input[INPUT_SIZE];
    char tmp[TITLE_SIZE];
    // name
    fgets(input, sizeof(input), f);
    char name[ITEM_NAME_SIZE];
    int idx = 6;
    while (input[idx++] != '\n');
    strncpy(name, input + 6, idx - 7);

    // price
    fgets(input, sizeof(input), f);
    float price;
    sscanf(input, "%s %f", tmp, &price);

    // score
    fgets(input, sizeof(input), f);
    float score;
    sscanf(input, "%s %f", tmp, &score);

    // entity
    fgets(input, sizeof(input), f);
    int entity;
    sscanf(input, "%s %d", tmp, &entity);

    getc(f);  // \n

    // date
    fgets(input, sizeof(input), f);
    char date[DATE_SIZE];
    char time[TIME_SIZE];
    sscanf(input, "%s %s %s %s", tmp, tmp, date, time);

    FileContent fileContent = constructFileContent(name, price, score, entity, date, time);

    // match name
    int itemIdx = findItemInUserOrder(items, nItems, name);

    // calculate buying-value
    if (itemIdx > -1) {  // found
        float buyingValue = score / price;
        if (entity < items[itemIdx].quantity) {
            buyingValue *= (float) entity / items[itemIdx].quantity;
        }

        // log file
        Log log = constructLog(name, threadArgsPtr->store, threadArgsPtr->category, threadArgsPtr->file, categoryPID, pthread_self(), username, orderId, buyingValue, price);
        log_match(&log);

    } else {  // not found

    }


    fclose(f);
    free(args);
    pthread_exit(NULL);
}

void handle_category(const char *store, const char *category_path, const char *category_name, Order *orderPtr) {
    DIR *dir = opendir(category_path);
    if (dir == NULL) {
        perror("Error opening category");
        return;
    }

    static int a = 1;
    // printf("CATEGORY: %s", category_name);
    // char *ptr = category_name;
    // printf("#%d ", a++);
    // while (*ptr)
    // {
    //     printf("%c", *(ptr++));
    // }
    // puts("");
    // printf("%c",*category_name);

    struct dirent *entry;
    pthread_t threads[MAX_FILES];
    char *file_info[MAX_FILES][3];
    
    int thread_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Regular file
            char file_path[FILE_PATH_SIZE];
            snprintf(file_path, sizeof(file_path), "%s/%s", category_path, entry->d_name);

            // Allocate file info
            file_info[thread_count][0] = strdup(store);
            file_info[thread_count][1] = strdup(category_name);
            file_info[thread_count][2] = strdup(file_path);
      
            // args
            //args = constructThreadArgs(orderPtr, file_info[thread_count][0], file_info[thread_count][1], file_info[thread_count][2], getpid());

            //printf("file: %s\n", args.file);
            //Create a thread for each file
            // if (pthread_create(&threads[thread_count], NULL, read_file, &args) != 0) {
            //     perror("Error creating thread");
            // }
            ThreadArgs *args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
            *args = constructThreadArgs(orderPtr, file_info[thread_count][0], file_info[thread_count][1], file_info[thread_count][2], getpid());
            //puts(args->file);
            if (pthread_create(&threads[thread_count], NULL, read_file, args) != 0) {
                perror("Error creating thread");
            }
            thread_count++;

            // if (thread_count == 3)  // todo
            //     break;
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
    static int a = 1;

    DIR *dir = opendir(store_path);
    if (dir == NULL) {
        perror("Error opening store");
        return;
    }
    // Get store directory name from Path name
    const char *tmp = strrchr(store_path, '/');
    char store_name[STORE_SIZE];
    strcpy(store_name, tmp+1);  

    // alo

    
    struct dirent *entry;

    printf("handleStore: PID(%d)\n", getpid());
    printf("store name = %s\n", store_name);
    printf("store path = %s\n", store_path);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char category_path[256];
            snprintf(category_path, sizeof(category_path), "%s/%s", store_path, entry->d_name);

            // printf("#%d store PID(%d) category path = %s\n", a++, getpid(), category_path);
            pid_t pid = fork();

            if (pid < 0) {
                printError("store fork() category failed");

            } else if (pid == 0) { // Child process
                // puts(category_path);
                handle_category(store_name, category_path, entry->d_name, orderPtr);
                _exit(0); // Exit after handling the category

            } else {  // parent

                printChildCreation(getpid(), pid, category_path);
            }
            
            // break; // todo 
        }
    }

    // Wait for all child processes
    while (wait(NULL) > 0);

    closedir(dir);
}

void parse_log_file(const char *filepath, float *sum_buyingValue, float *sum_price) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char line[INPUT_SIZE];

    while (fgets(line, sizeof(line), file)) {
        float buyingValue, price;

        // Parsing each line assuming the format is:
        // Item,ThreadID,Store,Category,File,CategoryPID,buyingValue,price(float)
        if (sscanf(line, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%f,%f", &buyingValue, &price) == 2) {
            *sum_buyingValue += buyingValue;
            *sum_price += price;

            // Debugging to check each parsed price
            printf("price = %f\n", price);
        } else {
            printf("Failed to parse line: %s", line);
        }
    }

    fclose(file);
}

void *ordersThreadTask(void *args) {
    OrdersThread *otPtr = (OrdersThread*) args;
    char logname[LOG_NAME_SIZE];
    snprintf(logname, sizeof(logname), "%s_%d.log", otPtr->username, otPtr->order_id);

    float maxSumBuyingValue = 0.0;
    float maxSumPrice = 0.0;
    int maxIndex = -1;
    char maxFilePath[FILE_PATH_SIZE] = {0};
    const char *storeDirs[MAX_STORES] = {
        "Store1", "Store2", "Store3"
    };

    for (int i = 0; i < MAX_STORES; i++) {
        char filepath[FILE_PATH_SIZE];
        snprintf(filepath, sizeof(filepath), "./Dataset/%s/%s", storeDirs[i], logname);

        float sum_buyingValue = 0.0;
        float sum_price = 0.0;

        parse_log_file(filepath, &sum_buyingValue, &sum_price);
        printf("Sum of buyingValue for file %s: %.2f, Sum of price: %.2f\n", filepath, sum_buyingValue, sum_price);

        if (sum_buyingValue > maxSumBuyingValue) {
            maxSumBuyingValue = sum_buyingValue;
            maxSumPrice = sum_price;
            maxIndex = i;
            strncpy(maxFilePath, filepath, sizeof(maxFilePath) - 1);
        }
    }

    if (maxIndex != -1) {
        printf("File with the highest sum buyingValue and price: %s\nSum buyingValue: %.2f\nSum price: %.2f\n", maxFilePath, maxSumBuyingValue, maxSumPrice);
    } else {
        printf("No valid log files found.\n");
    }

    otPtr->total_price = maxSumPrice;
    ((OrdersThread*) args)->total_price = maxSumPrice;
    // return (void*) &maxSumPrice;
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
            getchar();
            char line[INPUT_SIZE];
            // fgets(line, sizeof(line), stdin); // Read the full input line

            // // Parse the input, where name includes everything before the number
            // if (sscanf(line, "%[^\t\n0-9] %d", name, &quantity) == 2) {
            //     // Remove trailing spaces from the name
            //     int len = strlen(name);
            //     while (len > 0 && name[len - 1] == ' ') {
            //         name[len - 1] = '\0';
            //         len--;
            //     }
            //     printf("Full Name: '%s'\n", name);
            //     printf("Quantity: %d\n", quantity);
            // } else {
            //     printf("Invalid input format.\n");
            //     i--;
            //     continue;
            // }
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
            // printf("PID(%d) store1\n", getpid());
            handleStore(&order, "./Dataset/Store1");

           _exit(0); // todo ok?
        } else {
            printChildCreation(getpid(), store1Process, "Store1");

        }

        // store2
        pid_t store2Process = fork();
        if (store2Process < 0) {  // error
            printError("user fork() store2 failed");
        } else if (store2Process == 0) {  // child
            // printf("PID(%d) store2\n", getpid());
            handleStore(&order, "./Dataset/Store2");

           _exit(0); // todo ok?
        } else {
            printChildCreation(getpid(), store2Process, "Store2");

        }

        // store3
        pid_t store3Process = fork();
        if (store3Process < 0) {  // error
            printError("user fork() store3 failed");
        } else if (store3Process == 0) {  // child
            // printf("PID(%d) store3\n", getpid());
            handleStore(&order, "./Dataset/Store3");

           _exit(0); // todo ok?
        } else {
            printChildCreation(getpid(), store3Process, "Store3");
            
        }

        while(wait(NULL) > 0);


        // thread creation
        // username
        // order id
        OrdersThread *otPtr = (OrdersThread*) malloc(sizeof(OrdersThread));
        *otPtr = constructOrdersThread(username, orderId, 0.0);
        pthread_create(&ordersThread, NULL, ordersThreadTask, otPtr);
    
        // all thread join
        void *returnValue;
        pthread_join(ordersThread, NULL);
        printf("total price = %f\n", otPtr->total_price);
        // otPtr->total_price = *((float*)returnValue);

        // free otPtr
        free(otPtr);

       _exit(0);  // todo ok?

    } else {  // parent
        // todo
        char str[INPUT_SIZE];
        sprintf(str, "uesr %s", username);
        printChildCreation(getpid(), pid, str);
        while(wait(NULL) > 0);  // drop status
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
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&log_mutex[i], NULL);
    }
    
    puts("Hello World!");
    // clr();
    menu();
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&log_mutex[i]);
    }
    
    return 0;
}