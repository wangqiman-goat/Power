#include "n32g031.h"
#include "timer.h"

uint16_t PrescalerValue = 0;	//prescaler coeffiecient of timer1


/**
*@name: TIM3_NVIC_Configuratio
*@description: timer3 interrupt priority configuration
*@params: none
*@return: none
*/
static void TIM3_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    /* Enable the TIM3 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority           = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

/**
*@name: TIM3_Configuration
*@description: timer3 configuration
*@params: none
*@return: none
*/
void TIM3_Configuration(void)
{
	TIM_TimeBaseInitType TIM_TimeBaseStructure;
	
	TIM3_NVIC_Configuration();
	
    // Compute the prescaler value
    PrescalerValue = 0; //(uint16_t) (SystemCoreClock / 12000000) - 1;

    // Time base configuration
    TIM_TimeBaseStructure.Period    = TIMER_1MS;
    TIM_TimeBaseStructure.Prescaler = 0;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;

    TIM_InitTimeBase(TIM3, &TIM_TimeBaseStructure);

    // Prescaler configuration
    TIM_ConfigPrescaler(TIM3, PrescalerValue, TIM_PSC_RELOAD_MODE_IMMEDIATE);

    // TIM3 enable update irq
    TIM_ConfigInt(TIM3, TIM_INT_UPDATE, ENABLE);

    // TIM3 enable counter
    TIM_Enable(TIM3, ENABLE);	
}

/**
*@name: TIM6_NVIC_Configuration
*@description: timer6 interrupt configuration
*@params: none
*@return: none
*/
static void TIM6_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    /* Enable the TIM6 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = LPTIM_TIM6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority           = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

/**
*@name: TIM6_Configuration
*@description: timer6 configuration
*@params: none
*@return: none
*/
void TIM6_Configuration(void)
{
	TIM_TimeBaseInitType TIM_TimeBaseStructure;
	
	TIM6_NVIC_Configuration();
	
    // Compute the prescaler value
    PrescalerValue = 0; //(uint16_t) (SystemCoreClock / 12000000) - 1;

    // Time base configuration
    TIM_TimeBaseStructure.Period    = TIMER_1MS;
    TIM_TimeBaseStructure.Prescaler = 0;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;

    TIM_InitTimeBase(TIM6, &TIM_TimeBaseStructure);

    // Prescaler configuration
    TIM_ConfigPrescaler(TIM6, PrescalerValue, TIM_PSC_RELOAD_MODE_IMMEDIATE);

    // TIM6 enable update irq
    TIM_ConfigInt(TIM6, TIM_INT_UPDATE, ENABLE);

    // TIM6 enable counter
    TIM_Enable(TIM6, ENABLE);	
}
