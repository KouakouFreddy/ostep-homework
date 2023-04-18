#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>


#include "common_threads.h"

// If done correctly, each thread should print their "start" message
// before either prints their "end" message. Tested by adding sleep(1)
// calls in various locations.

// You likely need two semaphores to do this correctly.

typedef struct __rendezvous_t {
        // add semaphores and other information here
        sem_t sema_A;
        sem_t sema_B;
} rendezvous_t;


// the single rendezvous we are using for this program
rendezvous_t b;

void
rendezvous_init(rendezvous_t *b) {
        // initialization code goes here
        sleep(1);
        sem_init(&b->sema_A, 0, 0);
        sem_init(&b->sema_B, 0, 0);

}

void
rendezvous_a(rendezvous_t *b) {
        // called when thread A reaches the rendezvous
        sleep(1);
        sem_post(&b->sema_A);
        sem_wait(&b->sema_B);

}

void
rendezvous_b(rendezvous_t *b) {
        // called when thread B reaches the rendezvous
        sleep(1);
        sem_post(&b->sema_B);
        sem_wait(&b->sema_A);

}

//
// XXX: don't change below here (just run it!)
//

typedef struct __tinfo_t {
        int thread_id;
        int test_case;
} tinfo_t;

struct myvec {
        int *elems;
        int size;
        int capacity;
} vec;

pthread_mutex_t vecmtx = PTHREAD_MUTEX_INITIALIZER;

void
myvec_init(int nelems) {
        vec.elems = calloc(nelems, sizeof(vec.elems[0]));
        vec.size = 0;
        vec.capacity = nelems;

        //assert(vec.elems != NULL); // XXX: not thread safe (but ok for now) 

        printf("myvec_init: %d elements allocated (capacity = %d) (size = %d) (elems = %p) (vec = %p) (vecmtx = %p) (vec.elems = %p) (vec.size = %p) (vec.capacity = %p) (vecmtx = %p) (vecmtx = %p) (vecmtx = %p)" , nelems, vec.capacity, vec.size, vec.elems, &vec, &vecmtx, vec.elems, &vec.size, &vec.capacity, &vecmtx, &vecmtx, &vecmtx);

}

void
myvec_push(int x) {
        pthread_mutex_lock(&vecmtx);
        assert(vec.size < vec.capacity);
        vec.elems[vec.size++] = x;
        pthread_mutex_unlock(&vecmtx);
}

void
myvec_pop(int x) {
        pthread_mutex_lock(&vecmtx);
        assert(vec.size >= 0);
        vec.elems[--vec.size] = x;
        pthread_mutex_unlock(&vecmtx);
}

bool
myvec_full() {
        return vec.size == vec.capacity;
}

void
myvec_clear() {
        vec.size = 0;
}

void *thread_A(void *arg) {
        tinfo_t *t = (tinfo_t *)arg;

        if (t->test_case == 0)
                sleep(1);

        printf("thread A: start\n");
        myvec_push(0);

        if (t->test_case == 1)
                sleep(1);

        rendezvous_a(&b);

        if (t->test_case == 2)
                sleep(1);

        printf("thread A: end\n");
        myvec_push(2);

        if (t->test_case == 3)
                sleep(1);
        return NULL;
}

void *thread_B(void *arg) {
        tinfo_t *t = (tinfo_t *)arg;

        if (t->test_case == 4)
                sleep(1);

        printf("thread B: start\n");
        myvec_push(1);

        if (t->test_case == 5)
                sleep(1);

        rendezvous_b(&b);

        if (t->test_case == 6)
                sleep(1);

        printf("thread B: end\n");
        myvec_push(3);

        if (t->test_case == 7)
                sleep(1);

        return NULL;
}

// run with a single argument indicating the number of 
// threads you wish to create (1 or more)
int
main(int argc, char *argv[]) {
        int num_threads = 2;
        assert(num_threads > 0);

        pthread_t p[num_threads];
        tinfo_t t[num_threads];

        myvec_init(4);

        for (int tc = 0; tc < 8; tc++) {
                printf("testcase %d\n", tc);

                rendezvous_init(&b);

                // start the threads
                t[0].thread_id = 0;
                t[0].test_case = tc;
                Pthread_create(&p[0], NULL, thread_A, &t[0]);

                t[1].thread_id = 0;
                t[1].test_case = tc;
                Pthread_create(&p[1], NULL, thread_B, &t[1]);

                // wait for them to finish
                for (int i = 0; i < num_threads; i++) {
                        Pthread_join(p[i], NULL);
                }

                assert(myvec_full());

                // Starts should be at the beginning
                assert((vec.elems[0] == 0 && vec.elems[1] == 1) ||
                       (vec.elems[0] == 1 && vec.elems[1] == 0));

                // Ends should be at the end
                assert((vec.elems[2] == 2 && vec.elems[3] == 3) ||
                       (vec.elems[2] == 3 && vec.elems[3] == 2));

                myvec_clear();
        }

        return 0;
}
