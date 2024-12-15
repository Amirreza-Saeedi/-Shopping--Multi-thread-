#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>

#define COMMAND_INPUT 20


void clr() {
    system("clear");
}

void printMenu() {
    printf("\n\n");
    printf(" 1. buy\n");
    printf(" 2. exit\n");
    printf("> ");
}

void buyMenu() {
    puts("\tBUY MENU");

}

int main(int argc, char const *argv[]) {   
    clr();
    puts("Hello World!");

    // char comamnd[COMMAND_INPUT];

    while (true) {
        printMenu();
        char c = getc(stdin); // todo
        printf("%c\n", c);

        switch (c)
        {
        case 1:
            buyMenu();
            break;
        
        case 2:
            break;
        }
    }
    



    return 0;
}
