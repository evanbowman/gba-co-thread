#pragma once



#ifndef CO_THREAD_STACK_SIZE
#define CO_THREAD_STACK_SIZE 1024
#endif


typedef void (*co_thread_fn)();




////////////////////////////////////////////////////////////////////////////////
//
// co_thread_create(entry_point)
//
//  Create a thread, with entry point supplied by co_thread_fn.
//
int co_thread_create(co_thread_fn entry_point);
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
