#ifndef FOOTHREAD_H
#define FOOTHREAD_H

#include <stddef.h>
#include <semaphore.h>
#include <sys/types.h>


#define FOOTHREAD_THREADS_MAX 256
#define FOOTHREAD_DEFAULT_STACK_SIZE (2 * 1024 * 1024)
#define FOOTHREAD_JOINABLE 0
#define FOOTHREAD_DETACHED 1

typedef struct {
    int join_type;
    size_t stack_size;
} foothread_attr_t;

#define FOOTHREAD_ATTR_INITIALIZER {FOOTHREAD_JOINABLE, FOOTHREAD_DEFAULT_STACK_SIZE}
typedef struct foothread_t {
    pid_t tid;
    int (*start_routine)(void *);
    void *arg;
    int joinable;
    int finished;
    void *retval;
    size_t stack_size;
    char *stack;
    sem_t finished_sem;
} foothread_t;




// Define mutex structure
typedef struct {
    sem_t mutex_sem;    // Semaphore for the mutex
    pid_t owner_tid;    // Thread ID of the thread that currently holds the mutex
} foothread_mutex_t;




typedef struct foothread_barrier_t {
    sem_t barrier_sem;
    int count;
    int id;
} foothread_barrier_t;



void foothread_create(foothread_t *thread, foothread_attr_t *attr, int (*start_routine)(void *), void *arg);
void foothread_exit();
void foothread_attr_setjointype(foothread_attr_t *attr, int join_type);
void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size);
void foothread_mutex_init(foothread_mutex_t *mutex);
void foothread_mutex_lock(foothread_mutex_t *mutex);
void foothread_mutex_unlock(foothread_mutex_t *mutex);
void foothread_mutex_destroy(foothread_mutex_t *mutex);
void foothread_barrier_init(foothread_barrier_t *barrier, int count);
void foothread_barrier_wait(foothread_barrier_t *barrier);
void foothread_barrier_destroy(foothread_barrier_t *barrier);

#endif /* FOOTHREAD_H */
