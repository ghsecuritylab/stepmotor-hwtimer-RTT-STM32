# 使用rt-thread hwtimer+thread分层方式驱动步进电机

## 简介

本文档以 野火霸道 STM32F103 开发板的 BSP (板级支持包) 为基础，编程实现了一种步进电机驱动的方式。

主要内容如下：

- 实现原理
- 函数说明
- 运行结果


## 实现原理

步进电机的驱动是通过脉冲控制，每接收一个脉冲，就会转动一步。

如我们需要产生一个1kHz的脉冲，通过启动一个500us周期的硬件定时器，时间到了之后在中断服务函数中进行IO翻转、计算剩余步骤等操作，就可以实现简单的脉冲输出。



## 函数说明
+ `int sp1_locate_abs(rt_uint32_t frequency, rt_int32_t num)`:控制1#步进电机，使用frequency频率，转动到距离原点num个脉冲处。
+ `int sp1_locate_rle(rt_uint32_t frequency, rt_int32_t num, DIR_Type dir)`:控制1#步进电机，使用frequency频率，向dir方向运动num个脉冲。
+ `int sp2_locate_abs(rt_uint32_t frequency, rt_int32_t num)`:控制2#步进电机，使用frequency频率，转动到距离原点num个脉冲处。
+ `int sp2_locate_rle(rt_uint32_t frequency, rt_int32_t num, DIR_Type dir)`:控制2#步进电机，使用frequency频率，向dir方向运动num个脉冲。
+ `static int autorun(int argc, char *argv[])`：自动运行函数；控制1#步进电机以100Hz的频率前进50个脉冲，同时控制2#步进电机以5000Hz的频率前进500个脉冲；指令发送完成的1s后，再控制两个电机回到原点。

## 运行结果

下载程序成功之后，系统会自动运行，连接开发板对应串口到 PC , 在终端工具里打开相应的串口（115200-8-1-N），复位设备后，可以看到 RT-Thread 的输出信息:

```bash
 \ | /
- RT -     Thread Operating System
 / | \     4.0.0 build Oct 19 2019
 2006 - 2018 Copyright by rt-thread team
msh > 
```

输入 `autorun`，通过示波器可以看到相应频率的方波输出，同时MSH也有相应的输出：
```bash
 \ | /
- RT -     Thread Operating System
 / | \     4.0.0 build Oct 20 2019
 2006 - 2018 Copyright by rt-thread team
msh >autorun
sp1 begin run,freq=100,target=50,dir=CW
sp2 begin run,freq=2000,target=500,dir=CW
msh >sp1 begin run,freq=100,target=0,dir=CCW
sp2 begin run,freq=5000,target=0,dir=CCW
sp1 begin run,freq=100,target=50,dir=CW
sp2 begin run,freq=2000,target=500,dir=CW
sp1 begin run,freq=100,target=0,dir=CCW
sp2 begin run,freq=5000,target=0,dir=CCW
```


## 联系人信息

维护人:

- [ShineRoyal](https://github.com/ShineRoyal) 

