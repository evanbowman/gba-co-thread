# gba-co-thread

Experimental cooperative threading library for Gameboy Advance in pure C. See co_thread.h and co_thread.c for the tiny threading library. See main.c for an example. Requires some more unit testing before it's ready for use.

# API

`co_create_thread(thread_function)`

Create a thread, with entry point specified in thread_function. Returns 1 upon failure.


`co_thread_yield();`

Suspend execution, switching to a different thread.


`co_thread_exit();`

Clean up a thread's resources. Must be called before a thread returns from it's entry function.
