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
    CW = 1,//�ߵ�ƽ˳ʱ��
    CCW = 0,//�͵�ƽ��ʱ��  //left
} DIR_Type; //���з���


typedef struct
{
    rt_int32_t target_pos; //�з��ŷ���
    rt_int32_t current_pos; //�з��ŷ���
    DIR_Type motor_dir; //˳ʱ��
} sp_data;


#define SP1_P PBout(5)// PB5
#define SP1_D PEout(5)// PE5	

#define SP1_PULSE GET_PIN(B, 5)
#define SP1_DIR   GET_PIN(E, 5)

#define SP1_TIMER_NAME "timer4"

#define SP2_PULSE GET_PIN(B, 4)
#define SP2_DIR   GET_PIN(E, 4)

#define SP2_TIMER_NAME "timer3"



#define SP_EVENT_COMPLETE   (1<<1)  //sp1.current_pos=sp1.target_pos

sp_data sp1;
sp_data sp2;


static rt_mutex_t sp1_mutex = RT_NULL;  //��sp1���ݽ��б���
static rt_event_t sp1_event = RT_NULL;  //��¼sp1�������¼�
static rt_device_t sp1_hwtimer = RT_NULL;   /* ��ʱ���豸��� */

static rt_mutex_t sp2_mutex = RT_NULL;  //��sp1���ݽ��б���
static rt_event_t sp2_event = RT_NULL;  //��¼sp1�������¼�
static rt_device_t sp2_hwtimer = RT_NULL;   /* ��ʱ���豸��� */




static rt_err_t sp1_timeout_callback(rt_device_t dev, rt_size_t size)
{
    static rt_uint8_t io_status = 0;
    /*�����￪ʼ*/
    //rt_event_send(sp1_event, SP_EVENT_ONEPULSE);
    /*�������������Ҫ4.7us*/

    /*�����￪ʼ*/
    //io_status = rt_pin_read(SP1_PULSE);
		io_status = SP1_P;
    if (PIN_HIGH == io_status)
    {
        rt_pin_write(SP1_PULSE, PIN_LOW);
        if (sp1.target_pos == sp1.current_pos)
        {
            rt_event_send(sp1_event, SP_EVENT_COMPLETE);    //��������¼�
        }
    }
    else if (sp1.target_pos != sp1.current_pos)//PIN_LOW == io_status
    {
        if (CW == sp1.motor_dir)sp1.current_pos++;
        else sp1.current_pos--;
        rt_pin_write(SP1_PULSE, PIN_HIGH);
    }
    /*�������������Ҫ7us*/

    return RT_EOK;
}

static rt_err_t sp2_timeout_callback(rt_device_t dev, rt_size_t size)
{
    static rt_uint8_t io_status = 0;
    //rt_event_send(sp2_event, SP_EVENT_ONEPULSE);
    io_status = rt_pin_read(SP2_PULSE);
    if (PIN_HIGH == io_status)
    {
        rt_pin_write(SP2_PULSE, PIN_LOW);
        if (sp2.target_pos == sp2.current_pos)
        {
            rt_event_send(sp2_event, SP_EVENT_COMPLETE);    //��������¼�
        }
    }
    else if (sp2.target_pos != sp2.current_pos)//PIN_LOW == io_status
    {
        if (CW == sp2.motor_dir)sp2.current_pos++;
        else sp2.current_pos--;
        rt_pin_write(SP2_PULSE, PIN_HIGH);
    }
    return RT_EOK;
}

static int sp1_sample(int argc, char *argv[])
{
    rt_pin_write(SP1_PULSE, PIN_LOW);
    rt_pin_mode(SP1_PULSE, PIN_MODE_OUTPUT);
    rt_pin_mode(SP1_DIR, PIN_MODE_OUTPUT);

    //�����¼�
    if (sp1_event == NULL)
    {
        sp1_event = rt_event_create("ev_sp1", RT_IPC_FLAG_FIFO);
        if (sp1_event == RT_NULL)
        {
            rt_kprintf("create sp1_event failed.\n");
            return -1;
        }
    }

    //����������
    if (sp1_mutex == RT_NULL)
    {
        sp1_mutex = rt_mutex_create("mx_sp1", RT_IPC_FLAG_FIFO);
        if (sp1_mutex == RT_NULL)
        {
            rt_kprintf("create mx_sp1 failed.\n");
            return -1;
        }
    }


    rt_err_t ret = RT_EOK;
    rt_hwtimerval_t timeout_s;      /* ��ʱ����ʱֵ */
    rt_hwtimer_mode_t mode;         /* ��ʱ��ģʽ */

    /* ���Ҷ�ʱ���豸 */
    sp1_hwtimer = rt_device_find(SP1_TIMER_NAME);
    if (sp1_hwtimer == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", SP1_TIMER_NAME);
        return RT_ERROR;
    }

    /* �Զ�д��ʽ���豸 */
    ret = rt_device_open(sp1_hwtimer, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", SP1_TIMER_NAME);
        return ret;
    }

    /* ���ó�ʱ�ص����� */
    rt_device_set_rx_indicate(sp1_hwtimer, sp1_timeout_callback);

    /* ����ģʽΪ�����Զ�ʱ�� */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(sp1_hwtimer, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }

    /* ���ö�ʱ����ʱֵΪ5s��������ʱ�� */
    timeout_s.sec = 0;           /* �� */
    timeout_s.usec = 9;     /* ΢�� */

    if (rt_device_write(sp1_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    return ret;
}

MSH_CMD_EXPORT(sp1_sample, sp1_sample);


static int sp2_sample(int argc, char *argv[])
{
    //�����¼�
    if (sp2_event == NULL)
    {
        sp2_event = rt_event_create("ev_sp2", RT_IPC_FLAG_FIFO);
        if (sp2_event == RT_NULL)
        {
            rt_kprintf("create sp2_event failed.\n");
            return -1;
        }
    }

    //����������
    if (sp2_mutex == RT_NULL)
    {
        sp2_mutex = rt_mutex_create("mx_sp2", RT_IPC_FLAG_FIFO);
        if (sp2_mutex == RT_NULL)
        {
            rt_kprintf("create mx_sp2 failed.\n");
            return -1;
        }
    }

    rt_err_t ret = RT_EOK;
    rt_hwtimerval_t timeout_s;      /* ��ʱ����ʱֵ */
    rt_hwtimer_mode_t mode;         /* ��ʱ��ģʽ */

    /* ���Ҷ�ʱ���豸 */
    sp2_hwtimer = rt_device_find(SP2_TIMER_NAME);
    if (sp2_hwtimer == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", SP2_TIMER_NAME);
        return RT_ERROR;
    }

    /* �Զ�д��ʽ���豸 */
    ret = rt_device_open(sp2_hwtimer, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", SP2_TIMER_NAME);
        return ret;
    }

    /* ���ó�ʱ�ص����� */
    rt_device_set_rx_indicate(sp2_hwtimer, sp2_timeout_callback);

    /* ����ģʽΪ�����Զ�ʱ�� */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(sp2_hwtimer, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }

    /* ���ö�ʱ����ʱֵΪ5s��������ʱ�� */
    timeout_s.sec = 0;           /* �� */
    timeout_s.usec = 999999;     /* ΢�� */

    if (rt_device_write(sp2_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }
    return ret;
}

MSH_CMD_EXPORT(sp2_sample, sp2_sample);



static void test1_therad_entry(void *parameter)
{
    rt_hwtimerval_t timeout_s;      /* ��ʱ����ʱֵ */
    timeout_s.sec = 0;        /* �� */
    timeout_s.usec = 199;     /* ΢�� */
    rt_event_send(sp1_event, SP_EVENT_COMPLETE);
    while (1)
    {
        static rt_err_t result;
        rt_uint32_t e;
        result = rt_event_recv(sp1_event, SP_EVENT_COMPLETE,
                               RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                               RT_WAITING_FOREVER, &e);

        //timeout_s.usec += 5000;     /* ΢�� */
        if (rt_device_write(sp1_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
        {
            rt_kprintf("set timeout value failed\n");
            return ;
        }

        rt_mutex_take(sp1_mutex, RT_WAITING_FOREVER);
        sp1.target_pos += 1 ;
        if (sp1.target_pos > sp1.current_pos)       sp1.motor_dir = CW;
        else if (sp1.target_pos < sp1.current_pos)  sp1.motor_dir = CCW;
        rt_mutex_release(sp1_mutex);
    }
}
MSH_CMD_EXPORT(test1_therad_entry, test_therad_entry);


static void test2_therad_entry(void *parameter)
{
    rt_hwtimerval_t timeout_s;      /* ��ʱ����ʱֵ */
    timeout_s.sec = 0;        /* �� */
    timeout_s.usec = 199;     /* ΢�� */
    rt_event_send(sp2_event, SP_EVENT_COMPLETE);
    while (1)
    {
        static rt_err_t result;
        rt_uint32_t e;
        result = rt_event_recv(sp2_event, SP_EVENT_COMPLETE,
                               RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                               RT_WAITING_FOREVER, &e);

        //timeout_s.usec += 5000;     /* ΢�� */
        if (rt_device_write(sp2_hwtimer, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
        {
            rt_kprintf("set timeout value failed\n");
            return ;
        }

        rt_mutex_take(sp2_mutex, RT_WAITING_FOREVER);
        sp2.target_pos += 1 ;
        if (sp2.target_pos > sp2.current_pos)       sp2.motor_dir = CW;
        else if (sp2.target_pos < sp2.current_pos)  sp2.motor_dir = CCW;
        rt_mutex_release(sp2_mutex);
    }
}
MSH_CMD_EXPORT(test2_therad_entry, test_therad_entry);


static int set_target(int argc, char *argv[])
{
    if (argc == 2)
    {
        rt_mutex_take(sp1_mutex, RT_WAITING_FOREVER);
        sp1.target_pos = atoi(argv[1]);

        if (sp1.target_pos > sp1.current_pos)       sp1.motor_dir = CW;
        else if (sp1.target_pos < sp1.current_pos)  sp1.motor_dir = CCW;


        rt_mutex_release(sp1_mutex);

        rt_kprintf("set ok\n");
    }
    return 0;
}

MSH_CMD_EXPORT(set_target, set_target);


static int get_value(int argc, char *argv[])
{

    rt_mutex_take(sp1_mutex, RT_WAITING_FOREVER);
    rt_kprintf("target:%d\t current:%d\r\n", sp1.target_pos, sp1.current_pos);
    rt_mutex_release(sp1_mutex);

    return 0;
}

MSH_CMD_EXPORT(get_value, get_value);


static int test_run(int argc, char *argv[])
{

    static rt_thread_t tid1 = RT_NULL;//������IO��ת���߳�
    tid1 = rt_thread_create("tid1",
                            test1_therad_entry, RT_NULL,
                            1024,
                            4, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);



    static rt_thread_t tid2 = RT_NULL;//������IO��ת���߳�
    tid2 = rt_thread_create("tid2",
                            test2_therad_entry, RT_NULL,
                            1024,
                            4, 10);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    return 0;
}

MSH_CMD_EXPORT(test_run, test_run);

