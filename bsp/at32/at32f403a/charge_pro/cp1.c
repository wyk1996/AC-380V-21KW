/*******************************************************************************
 * @file
 * @note
 * @brief
 *
 * @author
 * @date     2021-05-02
 * @version  V1.0.0
 *
 * @Description cp采样
 *
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning
 *******************************************************************************/
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <board.h>
#include <stdlib.h>
#include <board.h>
#include "common.h"
#include "drv_hwtimer.h"

/* 引脚支持adc1通道10 */
#define   CP_ADC_NAME          "adc1"
#define   CP_ADC_CHANNEL       ADC_CHANNEL_1

/* 1ms */
#define   CP_PERIOD             1000000
/* 533333 */
#define   CP_CH_PULSE           467000    //467 877                     
#define   CP_CH_NULL            0
/* 3300mv */
#define   CP_ADC_REFER_VOLT     3300
/* 转换位数为12位 */
#define   CP_ADC_CONVERT_BITS   (1ul << 12)
#define   CP_ADC_MIN_CONVERT_V  1300
#define   CP_ADC_DIFF            700
/* defined the PWM pin: PA0 */
#define PWM_RUN_PIN             0

#define PWM_DEV_NAME        "pwm2"  /* PWM设备名称 */
#define PWM_DEV_CHANNEL     1       /* PWM通道 */

struct rt_device_pwm *pwm_dev;      /* PWM设备句柄 */
rt_adc_device_t adc_dev;

/**
 * @brief 通用定时器1初始化PWM函数
 * @param[in] psr:时钟预分频系数,预分频值=psr+1
 *            arr:自动重装载值,计数次数=arr+1
 * @param[out]
 * @return
 * @note
 */
void timer1_pwm_init()
{
    rt_uint32_t period, pulse, dir;

    period = 1000000;    /* 周期为0.5ms，单位为纳秒ns */
    dir = 1;            /* PWM脉冲宽度值的增减方向 */
    pulse = 0;          /* PWM脉冲宽度值，单位为纳秒ns */

    /* 查找设备 */
    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (pwm_dev == RT_NULL)
    {
        rt_kprintf("pwm sample run failed! can't find %s device!\n", PWM_DEV_NAME);
        return;
    }

    /* 设置PWM周期和脉冲宽度默认值 */
    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, period, pulse);
    /* 使能设备 */
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);

}

///**
// * @brief pwm 占空比100%输出
// * @param[in]
// * @param[out]
// * @return
// * @note
// */
//void cp_pwm_full(void)
//{
//    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, CP_PERIOD, CP_CH_NULL);
//    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
//}

///**
// * @brief pwm 制定的 CP_CH_PULSE 输出
// * @param[in]
// * @param[out]
// * @return
// * @note
// */
//void cp_pwm_ch_puls(void)
//{
//    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, CP_PERIOD, CP_CH_PULSE);
//    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
//}

///**
// * @brief pwm 输出初始化
// * @param[in]
// * @param[out]
// * @return
// * @note
// */
//int32_t cp_pwm_init(void)
//{
//    /* PWM管脚配置 */
////    rt_pin_mode(PWM_RUN_PIN, PIN_MODE_OUTPUT);
////    rt_pin_write(PWM_RUN_PIN,PIN_LOW);
//    timer1_pwm_init();
//    //cp_pwm_ch_puls();
//    cp_pwm_full();

//    return RT_EOK;
//}


///**
// * @brief cp adc采样初始化
// * @param[in]
// * @param[out]
// * @return
// * @note
// */
///* 引脚支持adc1通道10 */

//rt_adc_device_t adc_dev;
//int32_t cp_adc_init(void)
//{
//    /* 查找设备 */
//    adc_dev = (rt_adc_device_t)rt_device_find(CP_ADC_NAME);
//    if (adc_dev == RT_NULL)
//    {
//        rt_kprintf("adc sample run failed! can't find %s device!\n", CP_ADC_NAME);
//        return RT_ERROR;
//    }

//    /* 使能设备 */
//    rt_adc_enable(adc_dev, CP_ADC_CHANNEL);

//    return RT_EOK;
//}


//======================最新
#define HWTIMER_DEV_NAME   "timer3"     /* 定时器名称 */

uint32_t ADCConvertedValue[100];
uint32_t	count = 0;

/* 定时器超时回调函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
	ADCConvertedValue[count] = rt_adc_read(adc_dev, CP_ADC_CHANNEL);;
	count++;
	if(count >= 100)
	{
		count = 0;
	}

    return 0;
}

static int hwtimer_sample()
{
    rt_err_t ret = RT_EOK;
    rt_hwtimerval_t timeout_s;      /* 定时器超时值 */
    rt_device_t hw_dev = RT_NULL;   /* 定时器设备句柄 */
    rt_hwtimer_mode_t mode;         /* 定时器模式 */
    rt_uint32_t freq = 1000000;               /* 计数频率 */

    /* 查找定时器设备 */
    hw_dev = rt_device_find(HWTIMER_DEV_NAME);
    if (hw_dev == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", HWTIMER_DEV_NAME);
        return RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", HWTIMER_DEV_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_cb);

    /* 设置计数频率(若未设置该项，默认为1Mhz 或 支持的最小计数频率) */
    rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
    /* 设置模式为周期性定时器（若未设置，默认是HWTIMER_MODE_ONESHOT）*/
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }

    /* 设置定时器超时值为5s并启动定时器 */
    timeout_s.sec = 0;      /* 秒 */
    timeout_s.usec = 10;     /* 微秒 */
    if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }

    /* 延时3500ms */
//    rt_thread_mdelay(3500);

    /* 读取定时器当前值 */
    rt_device_read(hw_dev, 0, &timeout_s, sizeof(timeout_s));
//    rt_kprintf("Read: Sec = %d, Usec = %d\n", timeout_s.sec, timeout_s.usec);

    return ret;
}


/**
 * @brief pwm 占空比100%输出
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
void cp_pwm_full(void)
{
    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, CP_PERIOD, CP_CH_NULL);
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
}

/**
 * @brief pwm 制定的 CP_CH_PULSE 输出
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
void cp_pwm_ch_puls(void)
{
    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, CP_PERIOD, CP_CH_PULSE);
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
}

/**
 * @brief pwm 输出初始化
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
int32_t cp_pwm_init(void)
{
    /* PWM管脚配置 */
//    rt_pin_mode(PWM_RUN_PIN, PIN_MODE_OUTPUT);
//    rt_pin_write(PWM_RUN_PIN,PIN_LOW);
    timer1_pwm_init();
	hwtimer_sample();
    //cp_pwm_ch_puls();
    cp_pwm_full();

    return RT_EOK;
}


/**
 * @brief cp adc采样初始化
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
/* 引脚支持adc1通道10 */


int32_t cp_adc_init(void)
{
    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(CP_ADC_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", CP_ADC_NAME);
        return RT_ERROR;
    }

    /* 使能设备 */
    rt_adc_enable(adc_dev, CP_ADC_CHANNEL);

    return RT_EOK;
}














































/**
 * @brief 获取cp电压采样值
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint32_t ch_get_cp_volt(uint32_t *puiCpVolt)
{
    uint32_t value0    = 0;
    uint32_t value1    = 0;
    uint32_t value     = 0;
    uint32_t uiDValue  = 0;

    uint32_t i = 0;
    uint32_t n = 0;
    uint32_t uiaBuf[8] = {0};
    uint32_t uiSum = 0;
    uint32_t uiVaule = 0;
    rt_err_t ret = RT_EOK;

    /* 使能设备 */
//    ret = rt_adc_enable(pcp_dev, CP_ADC_CHANNEL);

    if(RT_EOK != ret)
    {
        return 0;
    }


    for(i = 0; i < 100; i++)
    {
        /* 读取采样值 */
//        value = rt_adc_read(pcp_dev, CP_ADC_CHANNEL);
        value0 =  rt_adc_read(adc_dev, CP_ADC_CHANNEL);
        if(value0 < CP_ADC_MIN_CONVERT_V)
        {
            continue;
        }

        value1 = rt_adc_read(adc_dev, CP_ADC_CHANNEL);
        if(value1 < CP_ADC_MIN_CONVERT_V)
        {
            continue;
        }

        uiDValue = value0 >= value1 ? value0 - value1  : value1 - value0  ;
        if(uiDValue > CP_ADC_DIFF)
        {
            continue;
        }
        value = (value0 + value1) / 2 ;

        uiaBuf[n] = value * CP_ADC_REFER_VOLT / CP_ADC_CONVERT_BITS;
        uiSum += uiaBuf[n];

        if(uiSum > 13000)
        {
            continue;
        }

        if(++n == 4)
        {
//			rt_adc_disable(pcp_dev,  CP_ADC_CHANNEL);
            //return (uiSum / 8);
            uiVaule = uiSum / 4;

            *puiCpVolt = uiVaule * 4;
            return 0;
        }
    }

//	rt_adc_disable(pcp_dev, CP_ADC_CHANNEL);
    return 0;
}


/**
 * @brief   获取cp电压 10次平均值
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint32_t ch_get_cp_average_volt(uint32_t *average_CpVolt)
{
	  static uint32_t cp[10];
    static uint8_t count = 0;
    uint8_t i = 0;
	
	if(ch_get_cp_volt(&cp[count]))
	{
	    count++;
	}


    if(count == 10)
    {
        count = 0;
        for(i = 0; i < 9; i++)
        {
            if((cp[i] > (cp[i+1]+500)) || ((cp[i] + 500) < cp[i+1]))
            {
                break;
            }
        }
        if(i == 9)
        {
            *average_CpVolt =( cp[0] + cp[1] +cp[2] +cp[3] +cp[4] +cp[5] +cp[6] +cp[7] +cp[8] +cp[9] ) / 10;
        }
    }
}

