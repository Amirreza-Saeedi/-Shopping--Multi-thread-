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
#include <ctype.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

#define COMMAND_INPUT 20
#define USERNAME_SIZE 20
#define ITEM_NAME_SIZE 32
#define MAX_ITEMS 10
#define INPUT_SIZE 255
#define OUTPUT_SIZE INPUT_SIZE
#define FILE_PATH_SIZE 256
#define DATE_SIZE 16
#define TIME_SIZE DATE_SIZE
#define TITLE_SIZE 32
#define STORE_SIZE 20
#define CATEGORY_SIZE 16
#define LOG_NAME_SIZE 64
#define MAX_STORES 3
#define MAX_PATH_LENGTH FILE_PATH_SIZE
#define MAX_NAME_LENGTH ITEM_NAME_SIZE
#define MAX_FILES 2000
#define MAX_STORES 3

// Enum for operation types
typedef enum {
    OPERATION_NONE = 0,
    OPERATION_READ,
    OPERATION_SCORE,
    OPERATION_FINAL,
} OperationType;

typedef struct {
    char name[MAX_NAME_LENGTH];
    int storeNumber;
    char category[MAX_NAME_LENGTH];
    char filename[MAX_PATH_LENGTH];  // Save filename too
} Product;

typedef struct {
    char date[DATE_SIZE];
    char time[TIME_SIZE];
    int score;
} ScoreBuffer;

typedef struct {
    int quantity;
} FinalBuffer;

FinalBuffer constructFinalBuffer(int quantity) {
    FinalBuffer fb;
    fb.quantity = quantity;
    return fb;
}

typedef struct {
    int quantity;
    char username[USERNAME_SIZE];
    int order_id;
} ReadBuffer;

typedef struct FileContent {
    char name[ITEM_NAME_SIZE];
    float price;
    float score;
    int entity;
    char date[DATE_SIZE];
    char time[TIME_SIZE];
} FileContent;

Product products[MAX_FILES];  // 1-dimensional array to store all products
ScoreBuffer *scoreBuffers;
FinalBuffer *finalBuffers;
ReadBuffer *readBuffers;
pthread_mutex_t *files_mutex;
OperationType *operations;
FileContent *fileContents;
int product_count = 0;           // Number of products added
pthread_t threads[MAX_FILES];
// Global Mutexs for race condition protection
pthread_mutex_t log_mutex[3];  // TODO
pthread_mutex_t userdiscount_mutex;


// prototypes
void parseWordsAndNumber(char *words, int *number);
void *scoresThreadTask(void *args);
void *finalThreadTask(void *args);




// Construct functions
ReadBuffer constructReadBuffer(int quantity, char username[USERNAME_SIZE], int order_id) {
    ReadBuffer rb;
    rb.quantity = quantity;
    strcpy(rb.username, username);
    rb.order_id = order_id;
    return rb;
}

ScoreBuffer constructScoreBuffer(char date[DATE_SIZE], char time[TIME_SIZE], float score) {
    ScoreBuffer wb;
    strcpy(wb.date, date);
    strcpy(wb.time, time);
    wb.score = score;
    return wb;
}


typedef struct {
    int store_id;
    float total_price;
    float buying_value;
} Basket;

Basket constructBasket(int store_id, float total_price, float buying_value) {
    Basket basket;
    basket.store_id = store_id;
    basket.total_price = total_price;
    basket.buying_value = buying_value;
    return basket;
}

typedef struct OrdersThread {
    char username[USERNAME_SIZE];
    int order_id;
    Basket *baskets;
} OrdersThread;

OrdersThread constructOrdersThread (char *username, int order_id) {
    OrdersThread ot;
    strcpy(ot.username, username);
    ot.order_id = order_id;

    // TODO baskets?
    ot.baskets = (Basket *) malloc(sizeof(Basket) * 3);  // TODO
    return ot;
}

typedef struct Log {
    char item[ITEM_NAME_SIZE];
    char store[STORE_SIZE];
    char category[CATEGORY_SIZE];
    char file[FILE_PATH_SIZE];
    pid_t categoryPID;
    pthread_t tid;
    char username[USERNAME_SIZE];
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
    char username[30];
    int nItems;
    Item items[MAX_ITEMS];
    float threshold;
    // sem_t *semaphore;  // Shared semaphore
} Order;

Order *constructOrder(const char *username, int id, Item items[MAX_ITEMS], int nItems, float threshold) {
    Order *o = (Order *) malloc(sizeof(Order));
    strcpy(o->username, username);
    o->id = id;
    o->nItems = nItems;
    for (int i = 0; i < nItems; i++) {
        o->items[i] = items[i];
    }
    o->threshold = threshold;
    // o->semaphore = (sem_t *) mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // sem_init(o->semaphore, 1, nItems * 3);  // Shared semaphore
    return o;
}

typedef struct {
    int store_id;
    Order *order;
} ScoresThread;

ScoresThread constructScoresThread(int store_id, Order *order) {
    ScoresThread st;
    st.store_id = store_id;
    st.order = order;
    return st;
}

typedef struct {
    int final_store;
    Order *order;
    Basket *baskets;
} FinalThread;

FinalThread constructFinalThread(Order *order, Basket *b) {
    FinalThread ft;
    ft.order = order;
    ft.baskets = b;
    return ft;
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
    printf("\033[0mPress any key to continue...");
    getchar();
    getchar();
}

void printLightBlue(const char *text) {
    // ANSI escape code for light blue text
    printf("\033[1;34m%s\033[0m", text);
}

void printYellow(const char *text) {
    // ANSI escape code for yellow text
    printf("\033[1;33m%s\033[0m", text);
}

int getOrderId(const char username[]) {
    const char *directory_path = "./Dataset/Store1/";
    DIR *dir = opendir(directory_path);
    if (!dir) {
        perror("Unable to open directory");
        return -1;
    }

    struct dirent *entry;
    int max_orderid = -1;
    int found = 0; // Flag to track if any log file for the username is found

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Check if it's a regular file
            const char *filename = entry->d_name;

            // Check if the filename starts with the username
            if (strncmp(filename, username, strlen(username)) == 0) {
                // Find the underscore and dot positions
                const char *underscore = strrchr(filename, '_');
                const char *dot = strrchr(filename, '.');
                if (underscore && dot && underscore < dot) {
                    // Extract the order ID
                    int orderid = atoi(underscore + 1);
                    if (orderid > max_orderid) {
                        max_orderid = orderid;
                    }
                    found = 1; // Mark that at least one file was found
                }
            }
        }
    }

    closedir(dir);

    // If no log file was found for the username, return 0
    return found ? max_orderid+1 : 0;
}

void printError(const char msg[]) {  // todo input
    printf("\033[31mERROR: %s!\033[0m\n", msg);
}

void printChildCreation(pid_t parent, pid_t child, const char msg[]) {
    //printf("~ PID %d create child  for %s PID: %d\n", parent, msg, child);
}

void pirntThreadCreation(pid_t parent, pthread_t thread, const char msg[]) {
    //printf("~ PID %d create thread for %s TID: %ld\n", parent, msg, thread);
}

int findItemInUserOrder(Item *items, int nItems, char *name) {
    for (int i = 0; i < nItems; i++) {
        if (strcmp(items[i].name, name) == 0) { 
            return i;
        }
    }
    return -1;
}

void setFilepath(Product *p, char *filepath) {

    snprintf(filepath, FILE_PATH_SIZE, "./Dataset/Store%d/%s/%s", 
            p->storeNumber, p->category, p->filename);

}

int getStoreIDFromName (const char *name) {
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

    // printf("Store%d\n", storeID);
    printf("Dataset/%s/%s_%d.log\n", storeName, log->username, log->orderId);
    
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

int getIndexProductNameStore(const char *name, const char *store) {
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (strncmp(name, products[i].name, MAX_NAME_LENGTH) == 0) {
            if (getStoreIDFromName(store) == products[i].storeNumber)
                return i;
        } 
    }
    return -1;  // not found item
}

int getIndexProductNameCategoryStore(const char *name, const char *store, const char*category) {
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (strncmp(name, products[i].name, MAX_NAME_LENGTH) == 0) {
            if (strncmp(category, products[i].category, MAX_NAME_LENGTH) == 0) {
                if (getStoreIDFromName(store) == products[i].storeNumber)
                    return i;
            }
        } 
    }
    return -1;  // not found item
}

void handle_category(const char *store, const char *category_path, const char *category_name, Order *orderPtr) {

    for (int i = 0; i < orderPtr->nItems; i++)
    {

        // printf("Item: %s\n", orderPtr->items[i].name);
        int index = getIndexProductNameCategoryStore(orderPtr->items[i].name, store,category_name);
        if (index >= 0) {

            // printf("CAT: %s\n", category_name);

            pthread_mutex_lock(&files_mutex[index]);
            readBuffers[index] = constructReadBuffer(orderPtr->items[i].quantity, orderPtr->username, orderPtr->id);
            operations[index] = OPERATION_READ;
            pthread_mutex_unlock(&files_mutex[index]);
            
        } else {  // not found in this store and category
            // printf("!!! %s, %s\n", store,category_name);
        }

    }
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

    struct dirent *entry;

    // printf("handleStore: PID(%d)\n", getpid());
    // printf("store name = %s\n", store_name);
    // printf("store path = %s\n", store_path);

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
            // printf("price = %f\n", price);
        } else {
            printf("Failed to parse line: %s", line);
        }
    }

    fclose(file);
}

void sortBasketsByBuyingValues(Basket baskets[]) {
    Basket tmp;
    if (baskets[0].buying_value < baskets[1].buying_value) {
        tmp = baskets[0];
        baskets[0] = baskets[1];
        baskets[1] = tmp;
    }
    if (baskets[1].buying_value < baskets[2].buying_value)
    {
        tmp = baskets[1];
        baskets[1] = baskets[2];
        baskets[2] = tmp;
    }
    if (baskets[0].buying_value < baskets[1].buying_value)
    {
        tmp = baskets[0];
        baskets[0] = baskets[1];
        baskets[1] = tmp;
    }
}

void *ordersThreadTask(void *args) {
    OrdersThread *otPtr = (OrdersThread*) args;
    char logname[LOG_NAME_SIZE];
    snprintf(logname, sizeof(logname), "%s_%d.log", otPtr->username, otPtr->order_id);

    float maxSumBuyingValue = 0.0;
    float maxSumPrice = 0.0;
    int maxIndex = -1;
    int bestStore = 0;
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
        // printf("Sum of buyingValue for file %s: %.2f, Sum of price: %.2f\n", filepath, sum_buyingValue, sum_price);

        otPtr->baskets[i].buying_value = sum_buyingValue;
        otPtr->baskets[i].total_price = sum_price;
        otPtr->baskets[i].store_id = i + 1;


        if (sum_buyingValue > maxSumBuyingValue) {
            bestStore = i + 1;
            maxSumBuyingValue = sum_buyingValue;
            maxSumPrice = sum_price;
            maxIndex = i;
            strncpy(maxFilePath, filepath, sizeof(maxFilePath) - 1);
        }
    }

    if (maxIndex != -1) {
        // printf("File with the highest sum buyingValue and price: %s\nSum buyingValue: %.2f\nSum price: %.2f\n", maxFilePath, maxSumBuyingValue, maxSumPrice);
    } else {
        printf("No valid log files found.\n");
    }

    // outputs
    // otPtr->total_price = maxSumPrice;
    // otPtr->store_id = bestStore;

    // sort
    sortBasketsByBuyingValues(otPtr->baskets);
    return NULL;
}

// Function to check if username:storename exists in userdiscount.log
int checkUserStore(const char *username, const char *storename) {
    // puts("checkUserStore");
    const char *file_path = "./userdiscount.log";
    

    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Unable to open file");
        return 0;
    }

    char line[INPUT_SIZE];
    int found = 0; // Flag to indicate if the username:storename pair is found


    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = '\0';

        // printf("line: %s\n", line);

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
    if (fprintf(file, "%s:%s\n", username, storename) < 0) {
        perror("Error writing to file");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1; // Success
}

void buyMenu() {
    puts("\tBUY MENU");

    // user
    char username[USERNAME_SIZE];
    char output[OUTPUT_SIZE];

    sprintf(output, "Username (max=%d): ", USERNAME_SIZE);
    printLightBlue(output);
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
        int orderId = getOrderId(username);
        int nItems;
        Item items[MAX_ITEMS];

        sprintf(output, "Number of items (max=%d): ", MAX_ITEMS);
        printLightBlue(output);
        scanf("%d", &nItems);

        sprintf(output, "ItemList%d <name> <quantity> (max=%d):\n", orderId, nItems);
        printLightBlue(output);
        getchar();  // \n

        for (int i = 0; i < nItems; i++) {
            // name & quantity
            int quantity;
            char name[ITEM_NAME_SIZE];

            sprintf(output, "#%d. ", i + 1);
            printLightBlue(output);

            parseWordsAndNumber(name, &quantity);
            char line[INPUT_SIZE];
            items[i] = constructItem(name, quantity);
        }

        // threshold
        float threshold;
        printLightBlue("Thresholds (-1 default): ");
        scanf("%f", &threshold);

        if(threshold == -1.0)
        {
            threshold =500.0;
        }

        // order
        Order *order = constructOrder(username, orderId, items, nItems, threshold);

        // 3 store processes 
        // store1
        pid_t store1Process = fork();
        if (store1Process < 0) {  // error
            printError("user fork() store1 failed");
        } else if (store1Process == 0) {  // child
            // printf("PID(%d) store1\n", getpid());
            handleStore(order, "./Dataset/Store1");

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
            handleStore(order, "./Dataset/Store2");

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
            handleStore(order, "./Dataset/Store3");

           _exit(0); // todo ok?
        } else {
            printChildCreation(getpid(), store3Process, "Store3");
            
        }


        while(wait(NULL) > 0);
        sleep(5);  // TODO

        // orders thread
        OrdersThread *otPtr = (OrdersThread*) malloc(sizeof(OrdersThread));
        *otPtr = constructOrdersThread(username, orderId);
        pthread_create(&ordersThread, NULL, ordersThreadTask, otPtr);
        pthread_join(ordersThread, NULL);

        // show baskets
        printYellow("Baskets:\n");
        for (int i = 0; i < MAX_STORES; i++) {
            char output[OUTPUT_SIZE];
            sprintf(output, "Store %d: buying_value = %.2f, total_price = %.2f\n", 
                otPtr->baskets[i].store_id, otPtr->baskets[i].buying_value, otPtr->baskets[i].total_price);
            // printf();
            printYellow(output);
        } 

        if (otPtr->baskets[0].buying_value != 0) {  // dont check threshold

            // final thread 
            FinalThread *ftPtr = (FinalThread *) malloc(sizeof(FinalThread));
            *ftPtr = constructFinalThread(order, otPtr->baskets);
            pthread_create(&finalThread, NULL, finalThreadTask, ftPtr);
            pthread_join(finalThread, NULL);
            
            int finalStore = ftPtr->final_store;
            if (finalStore != -1) {  // if bought

                printf("User '%s' order id '%d' store '%d' BOUGHT!\n", username, orderId, finalStore);

                // scores thread
                ScoresThread *stPtr = (ScoresThread*) malloc(sizeof(ScoresThread));
                *stPtr = constructScoresThread(finalStore, order);

                pthread_create(&scoresThread, NULL, scoresThreadTask, stPtr);
                pthread_join(scoresThread, NULL);

                // update userdiscount.log
                char storeName[STORE_SIZE];
                sprintf(storeName, "Store%d", finalStore);
                pthread_mutex_lock(&userdiscount_mutex);
                writeUserStore(username, storeName);
                pthread_mutex_unlock(&userdiscount_mutex);

                free(stPtr);
                
            } else {
                printf("User '%s' order id '%d' NOT BOUGHT!\n", username, orderId);
                
            }

            free(ftPtr);

        } else {  // dont buy
            printf("User '%s' order id '%d' NOT BOUGHT!\n", username, orderId);

        }


        free(otPtr->baskets);
        free(otPtr);

       _exit(0);

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

void setCurTime(char *str) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(str, sizeof(str), "%H:%M:%S", tm_info);
}

void setCurDate(char *str) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(str, sizeof(str), "%Y-%m-%d", tm_info);
}



void *finalThreadTask(void *args) {
    FinalThread *ft = (FinalThread *) args;
    Order *o = ft->order;
    Basket *baskets = ft->baskets;
    int finalStore = -1;  // flag to not bought
    float threashold = o->threshold;

    int sw = 1;
    for (int i = 0; i < MAX_STORES && sw; i++) {
        char storeName[STORE_SIZE];
        // printf("store id: %d\n", baskets[i].store_id);
        sprintf(storeName, "Store%d", baskets[i].store_id);

        // apply discount if available
        int hasDiscount = checkUserStore(o->username, storeName);
        float basketPrice = hasDiscount ? baskets[i].total_price / 10 : baskets[i].total_price;

        printf("Basket Price: %.2f, Total Price: %.2f\n", basketPrice, baskets[i].total_price);

        // check threshold
        if (threashold >= basketPrice && basketPrice > 0) {  // if basketPrice == 0 dont buy
            Item *items = o->items;
            int nItems = o->nItems;

            for (int j = 0; j < nItems; j++) {  // guaranteed at least one valid item exists
                const char *itemName = items[j].name;
                int index = getIndexProductNameStore(itemName, storeName);
                if (index != -1) {

                    pthread_mutex_lock(&files_mutex[index]);
                    finalBuffers[index] = constructFinalBuffer(items[j].quantity);
                    operations[index] = OPERATION_FINAL;
                    pthread_mutex_unlock(&files_mutex[index]);

                } else {  // not found
                }
            }        

            finalStore = baskets[i].store_id;
            sw = 0;
        }
    }
    
    // output
    ft->final_store = finalStore;
    return NULL;
}

void *scoresThreadTask(void *args) {
    // convert
    ScoresThread *st = (ScoresThread*) args;
    int nItems = st->order->nItems;
    int storeID = st->store_id;
    char storeName[STORE_SIZE];
    sprintf(storeName, "Store%d", storeID);
    int *scores = (int *) malloc(sizeof(int) * nItems);
    Item *items = st->order->items;

    for (int i = 0; i < nItems; i++) {
        const char *itemName = items[i].name;
        int index = getIndexProductNameStore(itemName, storeName);
        
        if (index != -1) {
            
            // read user inputs
            char date[DATE_SIZE];
            setCurDate(date);
            char time[TIME_SIZE];
            setCurTime(time);
            int score;
            char output[OUTPUT_SIZE];
            sprintf(output, "User '%s' enter item '%s' score: ", st->order->username, itemName); 
            printLightBlue(output);
            scanf("%d", &score);


            pthread_mutex_lock(&files_mutex[index]);
            scoreBuffers[index] = constructScoreBuffer(date, time, score);
            operations[index] = OPERATION_SCORE;
            pthread_mutex_unlock(&files_mutex[index]);

        } else {  // not found

        }
    }
    

    free(scores);
    return NULL;
}

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

void parseWordsAndNumber(char *words, int *number) {
    char input[INPUT_SIZE];     // Input buffer
    // printf("Enter words followed by an integer: ");
    
    // Read the entire line of input
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printError("Error reading input.\n");
        exit(1);
    }

    // Tokenize the input
    char *token = strtok(input, " ");  // Split input by spaces
    char temp[INPUT_SIZE] = "";         // Temporary buffer to store words
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

FileContent read_file(int index) {
    
    Product *p = &products[index];
    ReadBuffer *rb = &readBuffers[index];
    int orderID = rb->order_id; 
    char username[USERNAME_SIZE];
    strcpy(username, rb->username);
    char path[FILE_PATH_SIZE];
    setFilepath(p, path);

    FILE *f = fopen(path, "r");
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
    // printf("input: %s\n", input);
    while (input[idx++] != '\n');  // TODO exception
    strncpy(name, input + 6, idx - 7);
    name[idx - 7] = '\0';
    // printf("name: %s\n", name);

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

    fclose(f);

    FileContent fileContent = constructFileContent(name, price, score, entity, date, time);
    return fileContent;
}

void write_file(int index, FileContent *fc) {
    Product *p = &products[index];
    char path[FILE_PATH_SIZE];
    setFilepath(p, path);
    
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    // printf("Name: %s\n", fc->name);
    // printf("Price: %.2f\n", fc->price);
    // printf("Score: %.1f\n", fc->score);
    // printf("Entity: %d\n\n", fc->entity);
    // printf("Last Modified: %s %s\n", fc->date, fc->time);

    // Write the data in the specified format
    fprintf(f, "Name: %s\n", fc->name);
    fprintf(f, "Price: %.2f\n", fc->price);
    fprintf(f, "Score: %.1f\n", fc->score);
    fprintf(f, "Entity: %d\n\n", fc->entity);
    fprintf(f, "Last Modified: %s %s\n", fc->date, fc->time);

    fclose(f);
    // TODO
}


void *thread_task(void *arg) {
    //products[index]
    int index = *(int *)arg; // Thread-specific file index
    //printf("index: %d\n", index);

    // globals
    
    while (1) {
        pthread_mutex_lock(&files_mutex[index]); // Lock the file data
        OperationType operation = operations[index];

        // if (strcmp(products[index].name, "Jeans") == 0 && products[index].storeNumber == 1)
        // {
        //     printf("^^^ name(%s), thread(%lu), op(%d)\n", products[index].name, pthread_self(), operation);
        // }
        

        if (operation == OPERATION_READ) {
            
            fileContents[index] = read_file(index);

            FileContent *fc = &fileContents[index];
            ReadBuffer *rb = &readBuffers[index];
            Product *p = &products[index];
            
            // calculate buying-value
            float buyingValue = 1000 * fc->score / fc->price;
            if (fc->entity < rb->quantity) {
                // buyingValue *= (float) fc->entity / rb->quantity;
                buyingValue = 0;  // TODO cuases not buying if not enough
            }

            // log file
            char store[STORE_SIZE];
            sprintf(store, "Store%d", p->storeNumber);
            char path[FILE_PATH_SIZE];
            setFilepath(p, path);
            Log log = constructLog(p->name, store, p->category, path, getpid(), pthread_self(), 
                rb->username, rb->order_id, buyingValue, fc->price);
            printf("STORE: %s\n", store);
            log_match(&log);

            operations[index] = OPERATION_NONE; // Reset operation after reading

        } else if (operation == OPERATION_SCORE) {
            char path[FILE_PATH_SIZE];
            fileContents[index] = read_file(index);
            setFilepath(&products[index], path);
            FILE *file = fopen(path, "w");
            if (file) {
                // printf("Thread %d writing to file: %s\n", index, products[index].filename);
                
                FileContent *fc = &fileContents[index];                ScoreBuffer *sb = &scoreBuffers[index];
                strcpy(fc->date, sb->date);
                strcpy(fc->time, sb->time);
                fc->score = (float) sb->score;  // TODO cal_score()
                write_file(index, fc);
                puts("&&& op score done");

                fclose(file);
            } else {
                perror("Error opening file for write");
            }
            operations[index] = OPERATION_NONE; // Reset operation after writing

        } else if (operation == OPERATION_FINAL) {
            char path[FILE_PATH_SIZE];
            setFilepath(&products[index], path);
            fileContents[index] = read_file(index);
            FILE *file = fopen(path, "w");
            if (file) {
                // printf("Thread %d writing to file: %s\n", index, products[index].filename);
                FileContent *fc = &fileContents[index];
                FinalBuffer *fb = &finalBuffers[index];
                fc->entity -= fb->quantity;  // guaranteed wouldn't be < 0
                write_file(index, fc);
                puts("$$$ op final done");


                fclose(file);
            } else {
                perror("Error opening file for write");
            }
            

            operations[index] = OPERATION_NONE;
        }

        pthread_mutex_unlock(&files_mutex[index]); // Unlock the file data
        sleep(1); // TODO Avoid busy waiting (adjust as needed)
    }

    return NULL;
}

void initial_thread(int index) {
    //printf("init index: %d\n", index);
    int *i = (int *) malloc(sizeof(int));
    *i = index;
    
    pthread_mutex_init(&files_mutex[index],NULL);
    operations[index] = OPERATION_NONE;

    if (pthread_create(&threads[index], NULL, thread_task, i) != 0) {
        printError("thread creation failed!");
    }
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

                            initial_thread(product_count);
                            
                            product_count++;
                            // break;
                        }
                    }

                    fclose(file);
                }
            }
            closedir(dir);
        }
    }
}

// TODO 
void menu() {

    while (true) {
        printMenu();
        char c = getc(stdin); // todo
        getchar();
        // printf("%c\n", c);

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
    
    // Allocate shared memory
    size_t shared_memory_size = sizeof(ScoreBuffer) * MAX_FILES +
                                sizeof(ReadBuffer) * MAX_FILES +
                                sizeof(pthread_mutex_t) * MAX_FILES +
                                sizeof(OperationType) * MAX_FILES +
                                sizeof(FileContent) * MAX_FILES +
                                sizeof(FinalBuffer) * MAX_FILES;

    // Create shared memory region
    void *shared_memory = mmap(NULL, shared_memory_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // Initialize pointers to shared memory
    scoreBuffers = (ScoreBuffer *)shared_memory;
    readBuffers = (ReadBuffer *)(scoreBuffers + MAX_FILES);
    files_mutex = (pthread_mutex_t *)(readBuffers + MAX_FILES);
    operations = (OperationType *)(files_mutex + MAX_FILES);
    fileContents = (FileContent *)(operations + MAX_FILES);
    finalBuffers = (FinalBuffer *) (fileContents + MAX_FILES);

    
    // Initialize mutexes
    pthread_mutex_init(&userdiscount_mutex, NULL);

    for (int i = 0; i < MAX_FILES; i++) {
        if (pthread_mutex_init(&files_mutex[i], NULL) != 0) {
            perror("Mutex initialization failed");
            exit(1);
        }
    }

    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&log_mutex[i], NULL);
    }
    
    process_stores_and_categories();

    // clr();
    menu();
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&log_mutex[i]);
    }
    
    return 0;
}