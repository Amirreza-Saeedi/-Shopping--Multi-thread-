#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h> // For O_* constants
#include <sys/stat.h> // For mode constants
#include <unistd.h> // For fork and sleep

#define SEM_NAME "/my_semaphore"

int main() {
    // Create a named semaphore
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 3);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        printf("Child process waiting to enter critical section...\n");
        sem_wait(sem); // Wait (decrement) the semaphore
        printf("Child process entered critical section.\n");
        sleep(2); // Simulate some work in the critical section
        printf("Child process leaving critical section.\n");
        sem_post(sem); // Signal (increment) the semaphore
        _exit(0);
    } else { // Parent process
        printf("Parent process waiting to enter critical section...\n");
        sem_wait(sem); // Wait (decrement) the semaphore
        printf("Parent process entered critical section.\n");
        sleep(2); // Simulate some work in the critical section
        printf("Parent process leaving critical section.\n");
        sem_post(sem); // Signal (increment) the semaphore

        // Wait for the child process to finish
        wait(NULL);
    }

    // Close and unlink the semaphore
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}