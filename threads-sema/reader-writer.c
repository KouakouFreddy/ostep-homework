#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "common_threads.h"

//
// Your code goes in the structure and functions below
//

typedef struct __rwlock_t
{
    int readers;
    int writers;
    pthread_mutex_t lock;
    pthread_cond_t readers_ok_to_read;
    pthread_cond_t writers_ok_to_write;
    int writer_active;

} rwlock_t;

void rwlock_init(rwlock_t *rw)
{
    rw->readers = 0;
    rw->writers = 0;
    rw->writer_active = 0;
    pthread_mutex_init(&rw->lock, NULL);
    pthread_cond_init(&rw->readers_ok_to_read, NULL);
    pthread_cond_init(&rw->writers_ok_to_write, NULL);
}

void rwlock_acquire_readlock(rwlock_t *rw)
{
    pthread_mutex_lock(&rw->lock);
    while (rw->writers > 0 || rw->writer_active)
    {
        pthread_cond_wait(&rw->readers_ok_to_read, &rw->lock);
    }
    rw->readers++;
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw)
{
    pthread_mutex_lock(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
    {
        pthread_cond_signal(&rw->writers_ok_to_write);
    }
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw)
{
    pthread_mutex_lock(&rw->lock);
    while (rw->writers > 0 || rw->readers > 0)
    {
        pthread_cond_wait(&rw->writers_ok_to_write, &rw->lock);
    }
    rw->writers++;
    rw->writer_active = 1;
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_release_writelock(rwlock_t *rw)
{
    pthread_mutex_lock(&rw->lock);

    rw->writers--;
    rw->writer_active = 0;
    if (rw->writers > 0)
    {
        pthread_cond_signal(&rw->writers_ok_to_write);
    }
    else
    {
        pthread_cond_broadcast(&rw->readers_ok_to_read);
    }

    pthread_cond_signal(&rw->writers_ok_to_write);
    pthread_cond_broadcast(&rw->readers_ok_to_read);
    pthread_mutex_unlock(&rw->lock);
}

//
// Don't change the code below (just use it!)
//

int loops;
int value = 0;

rwlock_t lock;

void *reader(void *arg)
{
    int i;
    for (i = 0; i < loops; i++)
    {
        rwlock_acquire_readlock(&lock);
        printf("read %d\n", value);
        rwlock_release_readlock(&lock);
    }
    return NULL;
}

void *writer(void *arg)
{
    int i;
    for (i = 0; i < loops; i++)
    {
        rwlock_acquire_writelock(&lock);
        value++;
        printf("write %d\n", value);
        rwlock_release_writelock(&lock);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    assert(argc == 4);
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    loops = atoi(argv[3]);

    pthread_t pr[num_readers], pw[num_writers];

    rwlock_init(&lock);

    printf("begin\n");

    int i;
    for (i = 0; i < num_readers; i++)
        Pthread_create(&pr[i], NULL, reader, NULL);
    for (i = 0; i < num_writers; i++)
        Pthread_create(&pw[i], NULL, writer, NULL);

    for (i = 0; i < num_readers; i++)
        Pthread_join(pr[i], NULL);
    for (i = 0; i < num_writers; i++)
        Pthread_join(pw[i], NULL);

    printf("end: value %d\n", value);

    return 0;
}

