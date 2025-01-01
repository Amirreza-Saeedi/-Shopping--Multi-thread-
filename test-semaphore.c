#include <stdio.h>  // stdin
#include <stdlib.h>  // memory allocation
#include <pthread.h>  // mutex & threads
#include <semaphore.h>  
#include <unistd.h>  // os system calls
#include <stdbool.h>  // true, false
#include <string.h>

#define MAX_USERS       50
#define MAX_THREADS     1000
#define BANDWIDTH_LIMIT 20
#define STR_LIMIT       100

sem_t bandwidth_semaphore;
pthread_mutex_t user_locks[MAX_USERS];

void print_error(const char *msg) {
    printf("\033[31m%s\033[0m\n", msg);
}

// thread function
void *download(void *args) {
    // decomposit args
    int *input = (int *)args;
    int user_id = input[0];
    int duration = input[1];
    free(args);

    // check if the user is already downloading
    if (pthread_mutex_trylock(&user_locks[user_id]) == 0) {
        // acquire bandwidth
        if (sem_trywait(&bandwidth_semaphore) == 0) {
            printf("~ User %d: downloading for %d seconds...\n", user_id, duration);
            sleep(duration);
            printf("~ User %d: finished downloading.\n", user_id);

            // release bandwidth
            sem_post(&bandwidth_semaphore);
            printf("~ User %d: released bandwidth.\n", user_id);

        } else {
            char formatted_str[STR_LIMIT];
            snprintf(formatted_str, sizeof(formatted_str), "~ User %d: insufficient bandwidth!!!\n", user_id);
            print_error(formatted_str);
        }

        // Release the user's lock
        pthread_mutex_unlock(&user_locks[user_id]);
    } else {
        char formatted_str[STR_LIMIT];
        snprintf(formatted_str, sizeof(formatted_str), "~ User %d: already downloading!!!", user_id);
        print_error(formatted_str);
    }

    return NULL;
}

void clr() {
    system("clear");
}

int main() {
    clr();
    // settings
    printf("Settings:\n");
    printf("\t- MAX_USERS:          %d\n", MAX_USERS);
    printf("\t- MAX_THREADS:        %d\n", MAX_THREADS);
    printf("\t- BANDWIDTH_LIMIT:    %d\n", BANDWIDTH_LIMIT);
    printf("\t- STR_LIMIT:          %d\n", STR_LIMIT);

    // init semaphore & mutexes
    sem_init(&bandwidth_semaphore, false, BANDWIDTH_LIMIT);
    for (int i = 0; i < MAX_USERS; i++)
        pthread_mutex_init(&user_locks[i], NULL);  // NULL -> default
    
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;
    char input[STR_LIMIT];

    while (thread_count < MAX_THREADS) {
        printf("Enter <user id> (0-49) and <download duration> (seconds), or 'exit' to quit:\n> ");

        fgets(input, sizeof(input), stdin);  // read input

        if (input == NULL)  // EOF or error
            break;

        if (strncmp(input, "exit", 4) == 0)  // exit
            break;

        if (strncmp(input, "clr", 3) == 0) {
            clr();
            break;
        }

        int user_id, duration;
        if (sscanf(input, "%d %d", &user_id, &duration) == 2) {  // separate id & duration
            if (user_id >= 0 && user_id < MAX_USERS && duration > 0) {
                int *args = (int*) malloc(2 * sizeof(int));
                args[0] = user_id;
                args[1] = duration;

                pthread_create(&threads[thread_count++], NULL, download, args);  // guaranteed not to be overwrited

            } else {
                // printf("ERROR: invalid id or duration number!!!\n");
                print_error("ERROR: invalid id or duration number !!!");
            }
        } else {
            // printf("ERROR: invalid input format.!!!\n");
            print_error("ERROR: invalid input format !!!");
        }
    }

    // join all
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // destroy all
    sem_destroy(&bandwidth_semaphore);
    for (int i = 0; i < MAX_USERS; i++) {
        pthread_mutex_destroy(&user_locks[i]);
    }

    return 0;
}
