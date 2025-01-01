#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>


typedef struct {
    int el;
} E;

typedef struct
{
    int a;
    E *e;
} S;

void *task(void *args) {
    S *s = (S *) args;
    E *e = s->e;
    for (int i = 0; i < 3; i++)
    {
        e[i].el = i + 7;
    }
    return NULL;
}

int main() {
    S *s = (S *) malloc(sizeof(S));
    s->e = (E *) malloc(sizeof(E) * 3);

    pthread_t t;
    pthread_create(&t, NULL, task, s);
    pthread_join(t, NULL);

    puts("sflakdj");
    for (int i = 0; i < 3; i++)
    {
        printf("%d\n", s->e[i].el);
    }

    return 0;
}