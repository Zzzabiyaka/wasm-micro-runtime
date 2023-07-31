/*
 * Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __wasi__
#error This example only compiles to WASM/WASI target
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

enum CONSTANTS {
    NUM_ITER = 200000,
    NUM_RETRY = 5,
    MAX_NUM_THREADS = 8,
};

unsigned int threads_executed = 0;
unsigned int threads_creation_tried = 0;

void *
check_if_prime(void* arg)
{
    (void)(arg);
    __atomic_fetch_add(&threads_executed, 1,__ATOMIC_RELAXED);
    return NULL;
}

void
spawn_thread(pthread_t *thread)
{
    int status_code = -1;
    for (int tries = 0; status_code != 0 && tries < NUM_RETRY; ++tries) {
        status_code = pthread_create(thread, NULL, &check_if_prime, NULL);
        ++threads_creation_tried;
        assert(status_code == 0 || status_code == EAGAIN);
        if (status_code == EAGAIN) {
            usleep(1000);
        }
    }

    assert(status_code == 0 && "Thread creation should succeed");
}

int
main(int argc, char **argv)
{
    pthread_t threads[MAX_NUM_THREADS];
    double percentage = 0.1;

    for (int iter = 0; iter < NUM_ITER; ++iter) {
        if (iter > NUM_ITER * percentage) {
            fprintf(stderr, "Spawning stress test is %d%% finished\n",
                    (unsigned int)(percentage * 100));
            percentage += 0.1;
        }

        unsigned int thread_num = iter % MAX_NUM_THREADS;
        if (threads[thread_num] != 0) {
            assert(pthread_join(threads[thread_num], NULL) == 0);
        }

        usleep(1000);
        spawn_thread(&threads[thread_num]);
        assert(threads[thread_num] != 0);
    }

    for (int i = 0; i < MAX_NUM_THREADS; ++i) {
        assert(threads[i] == 0 || pthread_join(threads[i], NULL) == 0);
    }

    // Validation
    assert(threads_creation_tried >= threads_executed && "Test executed more threads than were created");
    assert((1. * threads_creation_tried) / threads_executed < 2.5 && "Ensuring that we're retrying thread creation less than 2.5 times on average ");

    fprintf(stderr, "Spawning stress test finished successfully executed %d threads with retry ratio %f\n", threads_creation_tried, (1. * threads_creation_tried) / threads_executed);
    return 0;
}
