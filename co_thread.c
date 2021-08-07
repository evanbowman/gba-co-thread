#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "co_thread.h"



enum {
    co_stack_canary = 0xCA
};



// I managed to reverse-engineer the layout of the jmp_buf in
// arm-none-eabi-gcc. All we really care about, are the program counter, and the
// stack pointer... let's hope the arm toolchain doesn't change its
// implementation of setjmp any time soon :)
struct __jmp_buf_decoded {
    uint32_t unknown_cpuset_0_[9];
    uint32_t fp_;
    uint32_t pc_;
    uint32_t sp_;
    uint32_t unknown_cupset_1_[11];
};



// Defined to avoid pedantic errors about type-punned ptr dereference.
static inline void __jmp_buf_set_pc(struct __jmp_buf_decoded* info, void* addr)
{
    info->pc_ = (intptr_t)addr;
}


static inline void __jmp_buf_set_sp(struct __jmp_buf_decoded* info, void* addr)
{
    // I have no idea if I have the fp and the sp labelled correctly. Maybe you
    // only really need to set one of them, but why not...
    info->fp_ = (intptr_t)addr;
    info->sp_ = (intptr_t)addr;
}



typedef struct co_ThreadInfo {
    jmp_buf cpu_state_;
    struct co_ThreadInfo* next_;
    // Stack might be appended to the end of the ThreadInfo struct. Or it might
    // not be, if we're the main thread.
    // u8 stack_[CO_THREAD_STACK_SIZE];
} co_ThreadInfo;



static co_ThreadInfo co_main_threadinfo;



static co_ThreadInfo* co_thread_list;
static co_ThreadInfo* co_current_thread;

// A thread cannot easily free itself, because doing so would destroy the
// executing thread's own stack So we keep a list of threads to clean up, and
// another thread will take care of the deallocation later.
static co_ThreadInfo* co_completed_threads;



static void co_completed_collect()
{
    while (co_completed_threads) {
        co_ThreadInfo* next = co_completed_threads->next_;
        free(co_completed_threads);
        co_completed_threads = next;
    }
}


static void co_on_resume()
{
    co_completed_collect();

    if (*((unsigned char*)co_current_thread + sizeof *co_current_thread) !=
        co_stack_canary) {

        if (co_current_thread != &co_main_threadinfo) {
            // We've detected a stack overflow. At the very least, we should not
            // execute this thread anymore. Really, we should kill the whole
            // program.
            // NOTE: The main thread does not have a stack canary, because the
            // crt0 created the main thread, not this library. We cannot make
            // any assumptions about the main thread's stack size.
            co_thread_exit();
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_create
//
////////////////////////////////////////////////////////////////////////////////



co_thread co_thread_create(co_thread_fn entry_point,
                           co_ThreadConfiguration* config)
{
    if (co_thread_list == NULL) {
        // If we've never called threadinfo_init before, let's initialize a
        // sentinel structure for the main thread.
        co_thread_list = &co_main_threadinfo;
        co_current_thread = &co_main_threadinfo;
        co_main_threadinfo.next_ = NULL;

        // NOTE: cpu_state_ for the main thread is uninitialized, but it's fine,
        // it'll be initialized when we first call co_thread_yield().
    }

    uint32_t stack_size;

    if (config) {
        stack_size = config->stack_size_;
    } else {
        stack_size = CO_THREAD_STACK_SIZE;
    }

    co_ThreadInfo* thread = malloc(sizeof(co_ThreadInfo) + stack_size);

    if (thread == NULL) {
        return NULL;
    }

    *((unsigned char*)thread + sizeof *thread) = co_stack_canary;

    setjmp(thread->cpu_state_);

    __jmp_buf_set_pc((struct __jmp_buf_decoded*)&thread->cpu_state_,
                     (void*)entry_point);
    __jmp_buf_set_sp((struct __jmp_buf_decoded*)&thread->cpu_state_,
                     // The stack follows the struct members, see the definition
                     // of co_ThreadInfo.
                     thread + sizeof(co_ThreadInfo)
                     // Stack starts at end.
                     + stack_size);

    thread->next_ = co_thread_list;
    co_thread_list = thread;

    return thread;
}



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_yield
//
////////////////////////////////////////////////////////////////////////////////



void co_thread_yield()
{
    if (co_thread_list == NULL) {
        // There is only one thread, the main thread. Nothing to switch to.
        return;
    }

    const int resumed = setjmp(co_current_thread->cpu_state_);

    if (resumed) {
        // Another thread has resumed, now we can free up a previously exited
        // thread.
        co_on_resume();
        return;
    }

    co_current_thread = co_current_thread->next_;

    if (co_current_thread == NULL) {
        // Wrap around
        co_current_thread = co_thread_list;
    }

    longjmp(co_current_thread->cpu_state_, 1);
}



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_resume
//
////////////////////////////////////////////////////////////////////////////////



void co_thread_resume(co_thread thread)
{
    if (co_thread_list == NULL) {
        return;
    }

    const int resumed = setjmp(co_current_thread->cpu_state_);

    if (resumed) {
        co_on_resume();
        return;
    }

    co_current_thread = thread;

    longjmp(co_current_thread->cpu_state_, 1);
}



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_exit
//
////////////////////////////////////////////////////////////////////////////////



void co_thread_exit()
{
    if (co_thread_list == NULL) {
        return;
    }

    if (co_current_thread == &co_main_threadinfo) {
        // You cannot exit the main thread yet. We may allow it in the future.
        return;
    }

    co_ThreadInfo* iter = co_thread_list;
    co_ThreadInfo* prev = NULL;
    while (iter) {
        if (iter == co_current_thread) {
            if (prev) {
                prev->next_ = iter->next_;
            } else {
                co_thread_list = iter->next_;
            }
            break;
        }

        prev = iter;
        iter = iter->next_;
    }

    iter->next_ = co_completed_threads;
    co_completed_threads = iter;

    co_thread_yield();
}




////////////////////////////////////////////////////////////////////////////////
//
// co_thread_join
//
////////////////////////////////////////////////////////////////////////////////



void co_thread_join(co_thread thread)
{
    while (true) {
        co_ThreadInfo* iter = co_thread_list;
        while (iter) {
            if (iter == thread) {
                break;
            }
            iter = iter->next_;
        }
        if (iter == thread) {
            co_thread_yield();
        } else {
            return;
        }
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_init
//
////////////////////////////////////////////////////////////////////////////////



void co_sem_init(co_Semaphore* sem, int value)
{
    sem->value_ = value;
}



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_wait
//
////////////////////////////////////////////////////////////////////////////////



void co_sem_wait(co_Semaphore* sem)
{
    while (true) {
        if (sem->value_ > 0) {
            sem->value_--;
            return;
        }

        co_thread_yield();
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// co_sem_post
//
////////////////////////////////////////////////////////////////////////////////



void co_sem_yield(co_Semaphore* sem)
{
    sem->value_++;
}
