#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>
#include <limits.h>

#define COMMAND_INPUT 20
#define USRENAME_SIZE 20
#define ITEM_NAME_SIZE 20
#define MAX_ITEMS 10
#define INPUT_SIZE 100

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
    int nItems;
    Item items[MAX_ITEMS];
    float threshold;
} Order;

Order constructOrder(int id, Item items[MAX_ITEMS], int nItems, float threshold) {
    Order order;
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

int getItemId() {
    return 0;
}

void printError(const char msg[]) {  // todo input
    printf("ERROR: %s\n", msg);
}

void buyMenu() {
    puts("\tBUY MENU");

    

    // user
    char username[USRENAME_SIZE];
    printf("Username (max=%d): ", USRENAME_SIZE);
    scanf("%s", username);

    // item
    int itemId = getItemId();
    int nItems;
    Item items[MAX_ITEMS];
    printf("Number of items (max=%d): ", MAX_ITEMS);
    scanf("%d", &nItems);
    printf("ItemList%d <name> <quantity> (max=%d):\n", itemId, nItems);
    
    for (int i = 0; i < nItems; i++) {
        int quantity;
        char name[ITEM_NAME_SIZE];
        char input[INPUT_SIZE];  // todo fgets & sscanf
        printf("#%d. ", i + 1);
        scanf("%s %d", name, &quantity);
        items[i] = constructItem(name, quantity);
    }

    float threshold;
    printf("Thresholds: ");
    scanf("%d", &threshold);

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
