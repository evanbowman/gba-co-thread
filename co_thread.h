#pragma once

#include <stdint.h>


#ifndef CO_THREAD_STACK_SIZE
#define CO_THREAD_STACK_SIZE 1024
#endif


typedef void* co_thread;


typedef void (*co_thread_fn)(void);



////////////////////////////////////////////////////////////////////////////////
//
// co_ThreadConfiguration
//
// Used to pass configuration settings to co_thread_create.
//
typedef struct co_ThreadConfiguration {
    // Size of the thread's stack.
    uint32_t stack_size_;

    // Optionally provide memory for the thread's header and stack. If null, the
    // system will call malloc.
    void* memory_;

} co_ThreadConfiguration;
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_create(entry_point)
//
// Create a thread, with entry point supplied by co_thread_fn. Optionally
// specify aditional settings in the config parameter.
//
co_thread co_thread_create(co_thread_fn entry_point,
                           co_ThreadConfiguration* config);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_yield()
//
// Halt execution of the current thread, and switch to another. Does nothing if
// you have not created any threads.
//
void co_thread_yield();
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_resume(thread)
//
// Yield and transfer control to a specific thread.
//
void co_thread_resume(co_thread thread);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_cond_wait(thread)
//
// Yield, do not schedule the thread until condition evaluates to true.
//
void co_thread_cond_wait(int (*cond)(void*), void* cond_arg);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_exit()
//
// The only correct way to stop a co_thread. In the future, this library might
// perhaps handle thread exit automatically. For now, before returning from the
// entry_point function passed to co_thread_create, you must call
// co_thread_exit() as a final step.
//
// NOTE: co_thread_exit will return immediately and do nothing when called from
// the main thread.
//
void co_thread_exit();
//
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
//
// co_thread_join()
//
void co_thread_join(co_thread thread);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_Semaphore
//
typedef struct co_Semaphore {
    int value_;
} co_Semaphore;
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_wait(sem)
//
void co_sem_init(co_Semaphore* sem, int value);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_wait(sem)
//
void co_sem_wait(co_Semaphore* sem);
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_post(sem)
//
void co_sem_post(co_Semaphore* sem);
//
////////////////////////////////////////////////////////////////////////////////
