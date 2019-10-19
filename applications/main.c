/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      SummerGift   change to new framework
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_gpio.h"

/* defined the LED0 pin: PF7 */
#define LED0_PIN    GET_PIN(B, 5)
#define LED1_PIN    GET_PIN(E, 5)


int main(void)
{
    while (1)
    {
        int i = 0;
        for (i = 0; i < 1000; i++);
        rt_thread_delay(10);
    }

    return RT_EOK;
}






