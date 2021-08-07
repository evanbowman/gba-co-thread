#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "co_thread.h"


typedef uint32_t u32;



// I managed to reverse-engineer the layout of the jmp_buf in
// arm-none-eabi-gcc. All we really care about, are the program counter, and the
// stack pointer... let's hope the arm toolchain doesn't change its
// implementation of setjmp any time soon :)
struct __jmp_buf_decoded {
    u32 unknown_cpuset_0_[9];
    u32 fp_;
    u32 pc_;
    u32 sp_;
    u32 unknown_cupset_1_[11];
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



////////////////////////////////////////////////////////////////////////////////
//
// co_thread_create
//
////////////////////////////////////////////////////////////////////////////////



int co_thread_create(co_thread_fn entry_point)
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

    co_ThreadInfo* thread =
        (co_ThreadInfo*)malloc(sizeof(co_ThreadInfo) + CO_THREAD_STACK_SIZE);

    if (thread == NULL) {
        return 1;
    }

    setjmp(thread->cpu_state_);

    __jmp_buf_set_pc((struct __jmp_buf_decoded*)&thread->cpu_state_,
                     (void*)entry_point);
    __jmp_buf_set_sp((struct __jmp_buf_decoded*)&thread->cpu_state_,
                     // The stack follows the struct members, see the definition
                     // of co_ThreadInfo.
                     thread + sizeof(co_ThreadInfo)
                     // Stack starts at end.
                     + CO_THREAD_STACK_SIZE);

    thread->next_ = co_thread_list;
    co_thread_list = thread;

    return 0;
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

    // We need to yield to a different thread now. But, we also need to free the
    // current thread, which co_thread_yield expects to be not null. Create a
    // dummy thread header on the stack, and set it to the current thread, so
    // that we can free memory for the current thread.
    co_ThreadInfo tmp;
    memcpy(&tmp, iter, sizeof tmp);

    free(iter);

    co_current_thread = &tmp;
    co_thread_yield();
}
