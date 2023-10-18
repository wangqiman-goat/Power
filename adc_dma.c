/** @addtogroup ADC_TIMTrigger_AutoInjection
 * @{
 */
 
#include "adc_dma.h"


//原来数组定义
//__IO uint16_t ADC_RegularConvertedValueTab[32], ADC_InjectedConvertedValueTab[32];

//要转换的通道数是9个，每个通道转换256次，暂不使用注入式通道，使用规则通道
__IO uint16_t ADC_RegularConvertedValueTab[256][9];

void adc_dma_init(void)
{
	ADC_InitType ADC_InitStructure;
	DMA_InitType DMA_InitStructure;
	TIM_TimeBaseInitType TIM_TimeBaseStructure;
	OCInitType TIM_OCInitStructure;

	/* TIM1 configuration ------------------------------------------------------*/
    /* Time Base configuration */
    TIM_InitTimBaseStruct(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.Period    = 0x8FF; // 0xFF;//定时器的周期，到这个值后会溢出
    TIM_TimeBaseStructure.Prescaler = 0x4;	//时钟预分频，如果主频是48M，则定时器1为12M
    TIM_TimeBaseStructure.ClkDiv    = 0x0; //这个主要和滤波器的采样频率有关，暂不用理会
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;//向上计数模式
    TIM_InitTimeBase(TIM1, &TIM_TimeBaseStructure);
    /* TIM1 channel1 configuration in PWM mode */
    TIM_OCInitStructure.OcMode      = TIM_OCMODE_PWM1;//输出模式配置，PWM1是向上计数模式，TIM1_CNT<TIM1_CCR时，TIM1的通道1输出有效电平，否则为无效电平
    TIM_OCInitStructure.OutputState = TIM_OUTPUT_STATE_ENABLE;//配置输出模式的状态，能使或关闭输出
    TIM_OCInitStructure.Pulse       = 0x7F;//该值为比较寄存器TIM1_CCR的值，当TIM1计数器TIM_CNT的比较结果发生变化时，输出脉冲将发生跳变
    TIM_OCInitStructure.OcPolarity  = TIM_OC_POLARITY_LOW;//把PWM模式中的有效电平设置为低电平
    TIM_InitOc2(TIM1, &TIM_OCInitStructure);

    /* DMA Channel1 Configuration ----------------------------------------------*/
    DMA_DeInit(DMA_CH1);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&ADC->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)ADC_RegularConvertedValueTab;//DMA要搬运数据目的内存地址
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = 256*9;//总共256*9个数据，9个通道，每个通道采256次
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;//DMA外设地址不自增
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;//内存地址自增
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_HALFWORD;//数据长度，半字，32的情况下就是两个byte
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;//回环模式搬运256*9个半字的ADC1转换结果到ADC_RegularConvertedValueTab数组
    DMA_InitStructure.Priority       = DMA_PRIORITY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH1, &DMA_InitStructure);
    DMA_RequestRemap(DMA_REMAP_ADC, DMA, DMA_CH1, ENABLE);
    /* Enable DMA channel1 */
    DMA_EnableChannel(DMA_CH1, ENABLE);

    /* ADC configuration ------------------------------------------------------*/
    ADC_InitStructure.MultiChEn      = ENABLE;//允许多通道扫描和连续转换
    ADC_InitStructure.ContinueConvEn = ENABLE;
    ADC_InitStructure.ExtTrigSelect  = ADC_EXT_TRIGCONV_T1_CC2;//选择AD转换的外部触发源，这里是TIM1的通道2
    ADC_InitStructure.DatAlign       = ADC_DAT_ALIGN_R;//数据右对齐
    ADC_InitStructure.ChsNumber      = 9;//通道数量
    ADC_Init(ADC, &ADC_InitStructure);

	//注入式通道相关设置
    // Set injected sequencer length
	/*
    ADC_ConfigInjectedSequencerLength(ADC, 1);
    // ADC injected channel Configuration
    ADC_ConfigInjectedChannel(ADC, ADC_CH_4_PA4, 1, ADC_SAMP_TIME_72CYCLES5);
    // ADC injected external trigger configuration
    ADC_ConfigExternalTrigInjectedConv(ADC, ADC_EXT_TRIG_INJ_CONV_NONE);//通过软件触发
    // Enable automatic injected conversion start after regular one 
    ADC_EnableAutoInjectedConv(ADC, ENABLE);
    // Enable JEOC interrupt
    ADC_ConfigInt(ADC, ADC_INT_JENDC, ENABLE);
	*/

	//规则通道相关配置
    /* ADC regular channel configuration */
    ADC_ConfigRegularChannel(ADC, ADC_CH_5_PA5, 1, ADC_SAMP_TIME_14CYCLES5);
    /* Enable ADC DMA */
    ADC_EnableDMA(ADC, ENABLE);
    /* Enable ADC external trigger */ //允许外部触发源来启动AD转换
    ADC_EnableExternalTrigConv(ADC, ENABLE);

    /* Enable ADC */
    ADC_Enable(ADC, ENABLE);

    /*wait ADC is ready to use*/
    while(!ADC_GetFlagStatusNew(ADC, ADC_FLAG_RDY))
        ;
    /*wait ADC is powered on*/
    while(ADC_GetFlagStatusNew(ADC, ADC_FLAG_PD_RDY))
        ;

    /* TIM1 counter enable */
    TIM_Enable(TIM1, ENABLE);
    /* TIM1 main Output Enable */
    TIM_EnableCtrlPwmOutputs(TIM1, ENABLE);
}
