#include "co_thread.h"
#include </opt/devkitpro/libtonc/include/tonc_irq.h>
#include </opt/devkitpro/libtonc/include/tonc_tte.h>
#include </opt/devkitpro/libtonc/include/tonc_video.h>



#define assert(COND, MSG)                           \
    if (!(COND)) {tte_write(MSG); while (true);}    \



void task_thread()
{
    int counter = 0;

    tte_write("hello from a thread! :)\n");

    co_thread_yield();

    assert(++counter == 1, "bad stack");

    tte_write("task thread resumed\n");

    co_thread_exit();
}



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


    if (co_thread_create(task_thread) == 0) {
        tte_write("created thread!\n");
    } else {
        tte_write("thread create failed!\n");
        return 1;
    }

    int counter = 4;

    co_thread_yield();

    assert(++counter == 5, "bad stack");

    tte_write("main thread resumed\n");

    co_thread_yield();

    assert(++counter == 6, "bad stack");

    tte_write("main thread resumed again\n");
}
