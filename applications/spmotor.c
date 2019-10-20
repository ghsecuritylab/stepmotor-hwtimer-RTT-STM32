/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      ShineRoyal   new file
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_gpio.h"
#include <stdlib.h>         //using function atoi
#include "spmotor.h"

typedef enum
{
    CW = 1,//高电平顺时针
    CCW = 0,//低电平逆时针  //left
} DIR_Type; //运行方向


typedef struct
{
    rt_int32_t target_pos; //有符号方向
    rt_int32_t current_pos; //有符号方向
    DIR_Type motor_dir; //顺时针
} sp_data;



#define SP1_PULSE GET_PIN(B, 4)
#define SP1_DIR   GET_PIN(E, 4)

#define SP1_TIMER_NAME "timer4"

#define SP2_PULSE GET_PIN(B, 5)
#define SP2_DIR   GET_PIN(E, 5)

#define SP2_TIMER_NAME "timer3"


#define SP_EVENT_READY  (1<<0)      //when the stepmotor is ready

static sp_data sp1;
static sp_data sp2;

static rt_event_t sp1_event = RT_NULL;  //记录sp1发生的事件
static rt_device_t sp1_hwtimer = RT_NULL;   /* 定时器设备句柄 */

static rt_event_t sp2_event = RT_NULL;  //记录sp1发生的事件
static rt_device_t sp2_hwtimer = RT_NULL;   /* 定时器设备句柄 */




static rt_err_t sp1_timeout_callback(rt_device_t dev, rt_size_t size)
{
    static rt_uint8_t io_status = 0;

    io_status = rt_pin_read(SP1_PULSE);
    if (PIN_HIGH == io_status)
    {
        rt_pin_write(SP1_PULSE, PIN_LOW);
    }
    //PIN_LOW == io_status
    else if (sp1.target_pos > sp1.current_pos)
    {
        sp1.current_pos++;
        rt_pin_write(SP1_PULSE, PIN_HIGH);
    }
    else if (sp1.target_pos < sp1.current_pos)
    {
        sp1.current_pos--;
        rt_pin_write(SP1_PULSE, PIN_HIGH);
    }
    else//sp1.target_pos == sp1.current_pos
    {
        rt_device_control(sp1_hwtimer, HWTIMER_CTRL_STOP, RT_NULL);   //停止定时器
        rt_event_send(sp1_event, SP_EVENT_READY);                     //发送完成事件
    }
    return RT_EOK;
}


static rt_err_t sp2_timeout_callback(rt_device_t dev, rt_size_t size)
{
    static rt_uint8_t io_status = 0;

    io_status = rt_pin_read(SP2_PULSE);
    if (PIN_HIGH == io_status)
    {
        rt_pin_write(SP2_PULSE, PIN_LOW);
    }
    //PIN_LOW == io_status
    else if (sp2.target_pos > sp2.current_pos)
    {
        sp2.current_pos++;
        rt_pin_write(SP2_PULSE, PIN_HIGH);
    }
    else if (sp2.target_pos < sp2.current_pos)
    {
        sp2.current_pos--;
        rt_pin_write(SP2_PULSE, PIN_HIGH);
    }
    else//sp2.target_pos == sp2.current_pos
    {
        rt_device_control(sp2_hwtimer, HWTIMER_CTRL_STOP, RT_NULL);   //停止定时器
        rt_event_send(sp2_event, SP_EVENT_READY);                     //发送完成事件
    }

    return RT_EOK;
}


static int sp1_init(void)
{
    rt_pin_write(SP1_PULSE, PIN_LOW);
    rt_pin_mode(SP1_PULSE, PIN_MODE_OUTPUT);
    rt_pin_mode(SP1_DIR, PIN_MODE_OUTPUT);

    //创建事件
    if (sp1_event == NULL)
    {
        sp1_event = rt_event_create("ev_sp1", RT_IPC_FLAG_FIFO);
        if (sp1_event == RT_NULL)
        {
            rt_kprintf("create sp1_event failed.\n");
            return -1;
        }
    }

    rt_err_t ret = RT_EOK;
    rt_hwtimer_mode_t mode;         /* 定时器模式 */

    /* 查找定时器设备 */
    sp1_hwtimer = rt_device_find(SP1_TIMER_NAME);
    if (sp1_hwtimer == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", SP1_TIMER_NAME);
        return RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(sp1_hwtimer, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", SP1_TIMER_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(sp1_hwtimer, sp1_timeout_callback);

    /* 设置模式为周期性定时器 */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(sp1_hwtimer, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }
    rt_event_send(sp1_event, SP_EVENT_READY);                     //发送完成事件
    return ret;
}
INIT_APP_EXPORT(sp1_init);

static int sp2_init(void)
{
    rt_pin_write(SP2_PULSE, PIN_LOW);
    rt_pin_mode(SP2_PULSE, PIN_MODE_OUTPUT);
    rt_pin_mode(SP2_DIR, PIN_MODE_OUTPUT);
    //创建事件
    if (sp2_event == NULL)
    {
        sp2_event = rt_event_create("ev_sp2", RT_IPC_FLAG_FIFO);
        if (sp2_event == RT_NULL)
        {
            rt_kprintf("create sp2_event failed.\n");
            return -1;
        }
    }

    rt_err_t ret = RT_EOK;
    rt_hwtimer_mode_t mode;         /* 定时器模式 */

    /* 查找定时器设备 */
    sp2_hwtimer = rt_device_find(SP2_TIMER_NAME);
    if (sp2_hwtimer == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", SP2_TIMER_NAME);
        return RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(sp2_hwtimer, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", SP2_TIMER_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(sp2_hwtimer, sp2_timeout_callback);

    /* 设置模式为周期性定时器 */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(sp2_hwtimer, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }
    rt_event_send(sp2_event, SP_EVENT_READY);                     //发送完成事件
    return ret;
}

INIT_APP_EXPORT(sp2_init);



int sp1_locate_rle(rt_uint32_t frequency, rt_int32_t num, DIR_Type dir)
{
    rt_uint32_t e;
    if (rt_event_recv(sp1_event, SP_EVENT_READY,
                      RT_EVENT_FLAG_OR,
                      RT_WAITING_NO, &e) != RT_EOK)
    {
        rt_kprintf("sp1 is not ready\n");
        return 1;
    }

    sp1.motor_dir = dir;
    sp1.target_pos = sp1.motor_dir ? sp1.target_pos + num : sp1.target_pos - num;

    rt_pin_write(SP1_DIR, sp1.motor_dir == CW ? PIN_HIGH : PIN_LOW);    //设置方向脚

    rt_hwtimerval_t timeout_s;               /* 定时器超时值 */
    timeout_s.sec = 0;                       /* 秒 */
    timeout_s.usec = 500000 / frequency - 1; /* 微秒 */
    if (rt_device_write(sp1_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    rt_kprintf("sp1 begin run,freq=%d,target=%d,dir=%s\n", frequency, sp1.target_pos, sp1.motor_dir ? "CW" : "CCW");
    return 0;
}

int sp1_locate_abs(rt_uint32_t frequency, rt_int32_t num)
{
    rt_uint32_t e;
    if (rt_event_recv(sp1_event, SP_EVENT_READY,
                      RT_EVENT_FLAG_OR,
                      RT_WAITING_NO, &e) != RT_EOK)
    {
        rt_kprintf("sp1 is not ready\n");
        return 1;
    }
    sp1.motor_dir = num > sp1.current_pos ? CW : CCW;
    sp1.target_pos = num;

    rt_pin_write(SP1_DIR, (sp1.motor_dir == CW) ? PIN_HIGH : PIN_LOW);    //设置方向脚

    rt_hwtimerval_t timeout_s;               /* 定时器超时值 */
    timeout_s.sec = 0;                       /* 秒 */
    timeout_s.usec = 500000 / frequency - 1; /* 微秒 */
    if (rt_device_write(sp1_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    rt_kprintf("sp1 begin run,freq=%d,target=%d,dir=%s\n", frequency, sp1.target_pos, sp1.motor_dir ? "CW" : "CCW");
    return 0;
}



int sp2_locate_rle(rt_uint32_t frequency, rt_int32_t num, DIR_Type dir)
{
    rt_uint32_t e;
    if (rt_event_recv(sp2_event, SP_EVENT_READY,
                      RT_EVENT_FLAG_OR,
                      RT_WAITING_NO, &e) != RT_EOK)
    {
        rt_kprintf("sp2 is not ready\n");
        return 1;
    }

    sp2.motor_dir = dir;
    sp2.target_pos = sp2.motor_dir ? sp2.target_pos + num : sp2.target_pos - num;

    rt_pin_write(SP2_DIR, sp2.motor_dir == CW ? PIN_HIGH : PIN_LOW);    //设置方向脚

    rt_hwtimerval_t timeout_s;               /* 定时器超时值 */
    timeout_s.sec = 0;                       /* 秒 */
    timeout_s.usec = 500000 / frequency - 1; /* 微秒 */
    if (rt_device_write(sp2_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    rt_kprintf("sp2 begin run,freq=%d,target=%d,dir=%s\n", frequency, sp2.target_pos, sp2.motor_dir ? "CW" : "CCW");
    return 0;
}

int sp2_locate_abs(rt_uint32_t frequency, rt_int32_t num)
{
    rt_uint32_t e;
    if (rt_event_recv(sp2_event, SP_EVENT_READY,
                      RT_EVENT_FLAG_OR,
                      RT_WAITING_NO, &e) != RT_EOK)
    {
        rt_kprintf("sp2 is not ready\n");
        return 1;
    }
    sp2.motor_dir = num > sp2.current_pos ? CW : CCW;
    sp2.target_pos = num;

    rt_pin_write(SP2_DIR, (sp2.motor_dir == CW) ? PIN_HIGH : PIN_LOW);    //设置方向脚

    rt_hwtimerval_t timeout_s;               /* 定时器超时值 */
    timeout_s.sec = 0;                       /* 秒 */
    timeout_s.usec = 500000 / frequency - 1; /* 微秒 */
    if (rt_device_write(sp2_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    rt_kprintf("sp2 begin run,freq=%d,target=%d,dir=%s\n", frequency, sp2.target_pos, sp2.motor_dir ? "CW" : "CCW");
    return 0;
}




static void test_therad_entry(void *parameter)
{
    while (1)
    {
        sp1_locate_rle(100, 50, CW);
        sp2_locate_rle(2000, 500, CW);
        rt_thread_delay(1000);
        sp1_locate_rle(100, 50, CCW);
        sp2_locate_rle(5000, 500, CCW);
        rt_thread_delay(1000);
    }

}


static int autorun(int argc, char *argv[])
{

    static rt_thread_t tid1 = RT_NULL;//负责处理IO翻转的线程
    tid1 = rt_thread_create("tid1",
                            test_therad_entry, RT_NULL,
                            256,
                            10, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);


    return 0;
}

MSH_CMD_EXPORT(autorun, autorun);

