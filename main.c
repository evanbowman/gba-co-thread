#include "co_thread.h"
#include </opt/devkitpro/libtonc/include/tonc_irq.h>
#include </opt/devkitpro/libtonc/include/tonc_tte.h>
#include </opt/devkitpro/libtonc/include/tonc_video.h>



#define assert(COND, MSG)                           \
    if (!(COND)) {tte_write(MSG); while (true);}    \



////////////////////////////////////////////////////////////////////////////////
//
// 1) A basic test, just yields back and forth between threads
//
////////////////////////////////////////////////////////////////////////////////


void task_thread()
{
    int counter = 0;

    tte_write("hello from a thread! :)\n");

    co_thread_yield();

    assert(++counter == 1, "bad stack");

    tte_write("task thread resumed\n");

    co_thread_exit();
}



int basic_test()
{
    co_thread task = co_thread_create(task_thread, NULL, NULL);
    co_thread task2 = co_thread_create(task_thread, NULL, NULL);
    if (task && task2) {
        tte_write("created threads!\n");
    } else {
        tte_write("thread create failed!\n");
        return 1;
    }

    int counter = 4;

    co_thread_yield();

    assert(++counter == 5, "bad stack");

    tte_write("main thread resumed\n");

    co_thread_join(task);
    co_thread_join(task2);

    assert(++counter == 6, "bad stack");

    tte_write("main thread resumed again\n");

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// 2) Semaphore producer/consumer example
//
////////////////////////////////////////////////////////////////////////////////


void sem_test_task()
{
    co_Semaphore* sem = co_thread_arg();

    co_sem_wait(sem);

    tte_write("consume, thread done!\n");

    co_thread_exit();
}



int sem_test()
{
    tte_write("\nproducer/consumer sem test:\n");

    co_Semaphore sem;
    co_sem_init(&sem, 0);

    enum {
        sem_thrd_count = 4
    };

    co_thread threads[sem_thrd_count];

    for (int i = 0; i < sem_thrd_count; ++i) {

        threads[i] = co_thread_create(sem_test_task, &sem, NULL);

        if (!threads[i]) {
            tte_write("thread create failed");
            return 1;
        }
    }

    for (int i = 0; i < 100; ++i) {
        // Just to demonstrate that the sem threads are in fact blocked on the
        // semaphore.
        co_thread_yield();
    }

    tte_write("producer start\n");

    for (int i = 0; i < sem_thrd_count; ++i) {
        co_sem_post(&sem);
        tte_write("produce!\n");
        co_thread_yield();
    }

    for (int i = 0; i < sem_thrd_count; ++i) {
        co_thread_join(threads[i]);
    }

    tte_write("sem test success");

    return 0;
}



////////////////////////////////////////////////////////////////////////////////



int main()
{
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;

    tte_init_se(0,
                BG_CBB(0)|BG_SBB(31),
                0,
                CLR_YELLOW,
                14,
                NULL,
                NULL);

    pal_bg_bank[1][15]= CLR_RED;
    pal_bg_bank[2][15]= CLR_GREEN;
    pal_bg_bank[3][15]= CLR_BLUE;
    pal_bg_bank[4][15]= CLR_WHITE;
    pal_bg_bank[5][15]= CLR_MAG;
    pal_bg_bank[4][14]= CLR_GRAY;


    basic_test();

    sem_test();
}
