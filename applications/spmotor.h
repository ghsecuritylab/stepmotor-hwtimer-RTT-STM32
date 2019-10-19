#ifndef __SPMOTOR_H__
#define __SPMOTOR_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_gpio.h"



//struct stepmotor_data
//{
//    rt_int32_t target_pos; //有符号方向
//    rt_int32_t current_pos; //有符号方向
//    DIR_Type motor_dir; //顺时针
//};


//struct stepmotor_device
//{
//    rt_base_t pulse_pin;
//    rt_base_t enable_pin;
//    rt_base_t dir_pin;

//    char hw_name[RT_NAME_MAX];
//    rt_device_t hw_dev;
//    rt_hwtimerval_t timeout;
//};

//typedef struct stepmotor_device *stepmotor_device_t;

//stepmotor_device_t stepmotor_init(const char *hw_name, rt_base_t pulse_pin, rt_base_t enable_pin, rt_base_t dir_pin);
//void stepmotor_deinit(stepmotor_device_t dev);

//rt_err_t stepmotor_locate_rle(stepmotor_device_t stepmotor, rt_uint32_t frequency, rt_int32_t num, DIR_Type dir);
//rt_err_t stepmotor_locate_abs(stepmotor_device_t stepmotor, rt_uint32_t frequency, rt_int32_t num);

//rt_int32_t stepmotor_read_current_pos(stepmotor_device_t stepmotor);
//rt_int32_t stepmotor_write_current_pos(stepmotor_device_t stepmotor);

#endif

