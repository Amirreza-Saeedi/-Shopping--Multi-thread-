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

#define MAX 1000
typedef struct
{
    char str[MAX];
} Output;


void *threadTask(void *args) {
    Output *output = (Output *) args;
    char filename[50] = "test-thread-input.txt";
    FILE *file = fopen(filename, "r");
    if (file) {
        fgets(output->str, sizeof(output->str), file);
    }
    fclose(file);
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t t;
    Output output;
    if (pthread_create(&t, NULL, threadTask, &output) != 0) {
        perror("thread");
    }
    sleep(1);
    printf("%s", output.str);
    return 0;
}
