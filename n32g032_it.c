/*****************************************************************************
 * Copyright (c) 2019, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32g032_it.c
 * @author Nations Solution Team
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#include "n32g032_it.h"
#include "main.h"

/** @addtogroup N32G032_StdPeriph_Template
 * @{
 */

//暂时不使用注入式通道
//extern __IO uint16_t ADC_InjectedConvertedValueTab[32];
//__IO uint32_t Index;

/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles SVCall exception.
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles PendSV_Handler exception.
 */
void PendSV_Handler(void)
{
}

/**
 * @brief  This function handles SysTick Handler.
 */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 N32G032 Peripherals Interrupt Handlers                     */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_n32g032.s).                                                 */
/******************************************************************************/

/**
 * @brief  This function handles ADC global interrupts requests.
 */
void ADC_IRQHandler(void)
{
	/*
	
    // Set PA.06 pin 
    GPIO_WriteBit(GPIOA, GPIO_PIN_6, Bit_SET);
    // Get injected channel11 converted value 
    if (Index >= sizeof(ADC_InjectedConvertedValueTab) / sizeof(ADC_InjectedConvertedValueTab[0]))
    {
        Index = 0;
    }
    ADC_InjectedConvertedValueTab[Index++] = ADC_GetInjectedConversionDat(ADC, ADC_INJ_CH_1);
    // Clear ADC JEOC pending interrupt bit
    ADC_ClearIntPendingBit(ADC, ADC_INT_JENDC);
    // Reset PA.06 pin
    GPIO_WriteBit(GPIOA, GPIO_PIN_6, Bit_RESET);
	
	*/
}

/**
 * @brief  This function handles USARTy global interrupt request.
 */
void USARTy_IRQHandler(void)
{
    if (USART_GetIntStatus(USARTy, USART_INT_RXDNE) != RESET)//接收中断，中断标志位会由USART_ReceiveData清除(读数据寄存器)
    {
		UART1_Data_Receive();
		
        //Read one byte from the receive data register
        //RxBuffer1[RxCounter1++] = USART_ReceiveData(USARTy);

        //if (RxCounter1 == NbrOfDataToRead1)
        //{
            //Disable the USARTy Receive interrupt
            //USART_ConfigInt(USARTy, USART_INT_RXDNE, DISABLE);
        //}
		
    }

    if (USART_GetIntStatus(USARTy, USART_INT_TXDE) != RESET)//发送中断，中断标志位会由USART_SendData清除(写数据寄存器)
    {
		UART1_Data_Send();
		
        //Write one byte to the transmit data register */
        //USART_SendData(USARTy, TxBuffer1[TxCounter1++]);

        //if (TxCounter1 == NbrOfDataToTransfer1)
        //{
            // Disable the USARTy Transmit interrupt
            //USART_ConfigInt(USARTy, USART_INT_TXDE, DISABLE);
        //}
    }
}

extern volatile uint8_t Flag_1ms;//在main.c中定义
extern volatile uint8_t cDelay_80ms;//在main.c中定义

/**
*@name: TIM3_IRQHandler
*@description: timer3中断处理程序
*@params: none
*@return: none
*/
void TIM3_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM3, TIM_INT_UPDATE) != RESET)
    {
        TIM_ClrIntPendingBit(TIM3, TIM_INT_UPDATE);//清标志
		Flag_1ms = 1;  //1毫秒定时已到标志
        ++cDelay_80ms;//80毫秒计时自增
    }
}



/**
 * @}
 */
