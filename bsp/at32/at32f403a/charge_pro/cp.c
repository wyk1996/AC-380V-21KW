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
#include "chtask.h"

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

#define HWTIMER_DEV_NAME   "timer3"     /* 定时器名称 */
extern CH_TASK_T stChTcb;
uint32_t ADCConvertedValue[100];
uint32_t count = 0;
uint16_t Feilvcount = 0;  //费率计算统计

/* 定时器超时时回调执行函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
    ADCConvertedValue[count] = rt_adc_read(adc_dev, CP_ADC_CHANNEL);;
    count++;
    if(count >= 100)
    {
        count = 0;
        //10us进入一次，100次就是1ms（因为频率1000HZ,周期T=1ms）

        if(++Feilvcount>=2000)  //进入1次是1ms，2000次=2000ms
        {
            Feilvcount=0;
            if(stChTcb.stRn8209.usCurrent > 20) //大于0.2A才计算
            {
                stChTcb.stCh.jsviDChQ[0] +=  stChTcb.stRn8209.usCurrent * stChTcb.stRn8209.usVolt;
                stChTcb.stCh.viDChQ[0] = stChTcb.stCh.jsviDChQ[0] /1000/1800;	//开始充电初始化清零 2s计算一次   0.001度
            }
            if(stChTcb.stRn8209.usCurrentb > 20) //大于0.2A才计算
            {
                stChTcb.stCh.jsviDChQ[1] +=  stChTcb.stRn8209.usCurrentb * stChTcb.stRn8209.usVoltb;
                stChTcb.stCh.viDChQ[1] = stChTcb.stCh.jsviDChQ[1] /1000/1800;	//开始充电初始化清零 2s计算一次   0.001度
            }
            if(stChTcb.stRn8209.usCurrentc > 20) //大于0.2A才计算
            {
                stChTcb.stCh.jsviDChQ[2] +=  stChTcb.stRn8209.usCurrentc * stChTcb.stRn8209.usVoltc;
                stChTcb.stCh.viDChQ[2] = stChTcb.stCh.jsviDChQ[2] /1000/1800;	//开始充电初始化清零 2s计算一次   0.001度
            }
        }
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
    timer1_pwm_init();   //初始化设置
    hwtimer_sample();     //超时进执行回调函数
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

void Swap(uint32_t *a,uint32_t *b)
{
    int t;
    t=*a;
    *a=*b;
    *b=t;
}

void BubbleSort(uint32_t * a,uint8_t n)
{
    int i,j;
    for(i=0; i<(n-1); i++)      //外层循环 最后一个只有一个元素（修改正确）
    {
        int flag=0;
        for(j=0; j<(n-1)-i; j++)   //内层循环
        {
            if(a[j]>a[j+1])
            {
                Swap(&a[j],&a[j+1]);
                flag=1;
            }
        }
        if(flag==0) break;
    }
}




/**
 * @brief 获取cp电压采样值
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint32_t adcbuf[100];
uint32_t ch_get_cp_volt(uint32_t *puiCpVolt)
{
    uint8_t i;
    uint8_t  count = 0;
    uint32_t alladc = 0;
    uint8_t  adccount = 0;

    for(i = 0; i < 100; i++)
    {
        /* 读取采样值 */
        if(ADCConvertedValue[i] < CP_ADC_DIFF)
        {
            continue;
        }
        adcbuf[count] = ADCConvertedValue[i];

        count++;
    }
    if(count < 5)
    {
        *puiCpVolt = 0;
        return 1;
    }
    BubbleSort(adcbuf,count);
    if(count > 40)
    {
        for(i = 10; i<count-10; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    else if(count > 20)
    {
        for(i = 5; i<count-5; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    else
    {
        for(i = 0; i<count; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    *puiCpVolt = alladc * 3.117 + 700;
    return count;
}

