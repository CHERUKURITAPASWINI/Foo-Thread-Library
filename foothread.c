#define _GNU_SOURCE
#include "foothread.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>
#include <semaphore.h>
#define CHECK_SEM_ERROR(result, message) \
    do { \
        if (result != 0) { \
            perror(message); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define gettid() syscall(SYS_gettid)

#define MAX_THREADS FOOTHREAD_THREADS_MAX

// Struct to hold thread information


// Global array to hold thread information
static foothread_t thread_pool[MAX_THREADS] = {0};

// Mutex for thread pool access
static foothread_mutex_t pool_mutex;

// Barrier struct

// Global barrier


// Initialize the barrier
void initialize_barrier(foothread_barrier_t* barrier,int  count) {
    sem_init(&barrier->barrier_sem, 0, 0);
    barrier->count = count;
}

void wait_barrier(foothread_barrier_t *barrier) {
    
    barrier->count--;
    
   // printf("Barrier count is %d\n",barrier->count);
    
    if (barrier->count <= 0) {
        //printf("Barrier count is %d\n",barrier->count);
        for (int i = 0; i < MAX_THREADS; i++) {
            
            sem_post(&barrier->barrier_sem);
        }
    } else {
        sem_wait(&barrier->barrier_sem);
    }
    
    
}

// Thread start function
int *thread_start(void *arg) {
    foothread_t *thread_info = (foothread_t *)arg;
    
    // Execute thread function
    thread_info->retval = (void *)(intptr_t)thread_info->start_routine(thread_info->arg);
    
    // Mark thread as finished
    thread_info->finished = 1;
    sem_post(&thread_info->finished_sem);
    
    return 0;
}

void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg) {
    // Initialize pool mutex
    static int pool_initialized = 0;
    if (!pool_initialized) {
        foothread_mutex_init(&pool_mutex);
        pool_initialized = 1;
    }
    
    // Find an empty slot in the thread pool
    foothread_mutex_lock(&pool_mutex);
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].tid == 0) {
            break;
        }
    }
    
    if (i == MAX_THREADS) {
        fprintf(stderr, "Maximum thread limit reached\n");
        foothread_mutex_unlock(&pool_mutex);
        return;
    }
    
    // Set thread attributes
    thread_pool[i].tid = gettid();
    thread_pool[i].start_routine = start_routine;
    thread_pool[i].arg = arg;
    thread_pool[i].joinable = (attr == NULL || attr->join_type == FOOTHREAD_JOINABLE);
    thread_pool[i].finished = 0;
    thread_pool[i].stack_size = (attr != NULL) ? attr->stack_size : FOOTHREAD_DEFAULT_STACK_SIZE;
    thread_pool[i].stack = malloc(thread_pool[i].stack_size);
    sem_init(&thread_pool[i].finished_sem, 0, 0);
    
    // Create thread
    clone(thread_start, thread_pool[i].stack + thread_pool[i].stack_size, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM, &thread_pool[i]);
    
    foothread_mutex_unlock(&pool_mutex);
    
    // Return thread identifier
    if (thread != NULL) {
        *thread = thread_pool[i];
    }
}

void foothread_exit() {
    foothread_mutex_lock(&pool_mutex);
    
    // Mark this thread as finished
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].tid == gettid()) {
            thread_pool[i].finished = 1;
            sem_post(&thread_pool[i].finished_sem);
            break;
        }
    }
    
    // Wait for all joinable threads to finish
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_pool[i].tid != 0 && thread_pool[i].joinable && !thread_pool[i].finished) {
            sem_wait(&thread_pool[i].finished_sem);
        }
    }
    
    foothread_mutex_unlock(&pool_mutex);
    
    // Terminate process
    exit(0);
}

void foothread_attr_setjointype(foothread_attr_t *attr, int join_type) {
    if (attr != NULL) {
        attr->join_type = join_type;
    }
}

void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size) {
    if (attr != NULL) {
        attr->stack_size = stack_size;
    }
}

void foothread_mutex_init(foothread_mutex_t *mute) {
    // Initialize the semaphore with value 1 (binary semaphore)
    int sem_result = sem_init(&(mute->mutex_sem), 0, 1);
    CHECK_SEM_ERROR(sem_result, "foothread_mutex_init: sem_init failed");
    
    // Initialize the mutex owner's thread ID to 0
    mute->owner_tid = 0;
}

void foothread_mutex_lock(foothread_mutex_t *mutex) {
    // Attempt to acquire the semaphore (lock the mutex)
    int sem_result = sem_wait(&(mutex->mutex_sem));
    CHECK_SEM_ERROR(sem_result, "foothread_mutex_lock: sem_wait failed");
    
    // Store the thread ID of the locking thread
    mutex->owner_tid = gettid();
}

void foothread_mutex_unlock(foothread_mutex_t *mutex) {
    // Check if the current thread is the owner of the mutex
    if (mutex->owner_tid != gettid()) {
        fprintf(stderr, "foothread_mutex_unlock: Attempted to unlock mutex owned by another thread\n");
        exit(EXIT_FAILURE);
    }
    
    // Release the semaphore (unlock the mutex)
    int sem_result = sem_post(&(mutex->mutex_sem));
    CHECK_SEM_ERROR(sem_result, "foothread_mutex_unlock: sem_post failed");
    
    // Reset the owner thread ID to 0
    mutex->owner_tid = 0;
}

void foothread_mutex_destroy(foothread_mutex_t *mutex) {
    // Destroy the semaphore
    int sem_result = sem_destroy(&(mutex->mutex_sem));
    CHECK_SEM_ERROR(sem_result, "foothread_mutex_destroy: sem_destroy failed");
}

void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    
    initialize_barrier(barrier,count);
}

void foothread_barrier_wait(foothread_barrier_t *barrier) {

    wait_barrier(barrier);
}

void foothread_barrier_destroy(foothread_barrier_t *barrier) {
    (void)barrier;
    sem_destroy(&barrier->barrier_sem);
}