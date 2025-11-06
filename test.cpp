#include <iostream>
#include <linux/can.h>

static void ssss(struct can_frame &frame)
{
    printf("hello world\n");
}
int main(void)
{
    // test();
    struct can_frame frame;
    ssss(frame);
    return 0;
}

/* g++ test.cpp -g -o test && ./test */
