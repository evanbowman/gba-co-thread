# gba-co-thread

Experimental cooperative threading library for ARM processors in pure C. See co_thread.h and co_thread.c for the tiny threading library. See main.c for an example. Originally implemented for the Gameboy Advance, I've since created bindings for other ARM CPUs, like the ARM Cortex M0 (tested on RP2040).


# API

See co_thread.h for more! Supports semaphores and a bunch of other features.


`co_thread_create(thread_function, arg, config)`

Create a thread, with entry point specified in thread_function. For more
settings, optionally pass a config struct.


`co_thread_yield();`

Suspend execution, switching to a different thread.


`co_thread_exit();`

Clean up a thread's resources. Must be called before a thread returns from its entry function.


# Examples

See main.c for a set of examples. The first test passes exectution back and forth between three threads, while incrementing local variable counters, to demonstrate that threads have their own stacks. The second test spawns a bunch of consumer threads, which wait on a semaphore supplied by the main producer thread.

<img src="example.png"/>
