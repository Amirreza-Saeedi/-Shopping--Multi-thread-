#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>


void clr() {
    system("clear");
}

int main(int argc, char const *argv[])
{   
    clr();
    puts("Hello World!");
    return 0;
}
