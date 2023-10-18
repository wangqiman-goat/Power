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
 * @file n32g031_it.c
 * @author Nations 
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#include "n32g031_it.h"
#include "main.h"

/** @addtogroup N32G031_StdPeriph_Template
 * @{
 */

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

extern volatile	struct UART_Data_Struct	UART1_STRUCT;//UART interface defined in the usart.c

/**
 * @brief  This function handles USARTy global interrupt request.
 */
void USARTy_IRQHandler(void)
{
	//receive data interrupt service routine
	//the USART_INT_RXDNE flag will be cleared once we call the USART_ReceiveData function
    if (USART_GetIntStatus(USARTy, USART_INT_RXDNE) != RESET)
    {
		USART1_Data_Receive();
		
        //Read one byte from the receive data register
        //RxBuffer1[RxCounter1++] = USART_ReceiveData(USARTy);

        //if (RxCounter1 == NbrOfDataToRead1)
        //{
            //Disable the USARTy Receive interrupt
            //USART_ConfigInt(USARTy, USART_INT_RXDNE, DISABLE);
        //}		
    }
	
	//ORE overflow check and clear this flag
    if(USART_GetIntStatus(USARTy, USART_INT_OREF) != RESET)
    {
        USART_GetIntStatus(USARTy, USART_INT_OREF);        
        USART_ReceiveData(USARTy);
    }
	
	/*
	else if (USART_GetIntStatus(USARTy, USART_INT_IDLEF) != RESET)
	{
		//USARTy->DAT;
		USART_GetIntStatus(USARTy,USART_INT_IDLEF);//clear USART_INT_IDLEF flag
		USART_ReceiveData(USARTy);		
		if((UART1_STRUCT.R_InPtr >= MINUMUM_DATA_LEN) && (UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr - 1] == EOI))						
		{			
			UART1_STRUCT.R_Flag = 1;//get a complete frame
		}
	}*/
    
	//transmit data interrupt service routine.
	//the USART_INT_TXDE flag will be cleared once we call the USART_SendData function
    if (USART_GetIntStatus(USARTy, USART_INT_TXDE) != RESET)
    {
		USART1_Data_Send();
		
        //Write one byte to the transmit data register
        //USART_SendData(USARTy, TxBuffer1[TxCounter1++]);

        //if (TxCounter1 == NbrOfDataToTransfer1)
        //{
            // Disable the USARTy Transmit interrupt
            //USART_ConfigInt(USARTy, USART_INT_TXDE, DISABLE);
        //}
    }
}

//the following variables are defined in the main.c file
extern volatile uint8_t Flag_1ms;
extern volatile uint8_t cDelay_80ms;
extern volatile uint8_t cDelay_10ms;
extern volatile uint8_t rcv_timeout;

/**
*@name: TIM3_IRQHandler
*@description: timer3 overflow interrupt service routine
*@params: none
*@return: none
*/
void TIM3_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM3, TIM_INT_UPDATE) != RESET)
    {
        TIM_ClrIntPendingBit(TIM3, TIM_INT_UPDATE);//clear the flag
		Flag_1ms = 1;
        //++cDelay_80ms;
		//++cDelay_10ms;
		if(++run_time>=START_UP_DELAY)
		{
			run_time=START_UP_DELAY;
		}
		/*
		if(++rcv_timeout>=5)
		{
			rcv_timeout=5;
			if((UART1_STRUCT.R_InPtr >= MINUMUM_DATA_LEN) && (UART1_STRUCT.R_Buffer[0] == SOI) && (UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr - 1] == EOI))						
			{			
				UART1_STRUCT.R_Flag = 1;
			}
			else
			{
				UART1_STRUCT.R_InPtr=0;
				UART1_STRUCT.R_Flag = 0;
			}
		}*/
    }
}

/**
*@name: LPTIM_TIM3_IRQHandler
*@description: timer6 overflow interrupt service routine
*@params: none
*@return: none
*/
void LPTIM_TIM6_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM6, TIM_INT_UPDATE) != RESET)
    {
        TIM_ClrIntPendingBit(TIM6, TIM_INT_UPDATE);
        /*
		if(++rcv_timeout>=5)//used to check whether a frame is received
		{
			rcv_timeout=5;
			if((UART1_STRUCT.R_InPtr >= MINUMUM_DATA_LEN) && (UART1_STRUCT.R_Buffer[0] == SOI) && (UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr - 1] == EOI))						
			{			
				UART1_STRUCT.R_Flag = 1;//get a complete frame
                TIM_Enable(TIM6, DISABLE);//disable the timer
			}
			else//no coming data or an error occurred
			{
				UART1_STRUCT.R_InPtr=0;
				UART1_STRUCT.R_Flag = 0;
			}
		}*/
    }
}

/******************************************************************************/
/*                 N32G031 Peripherals Interrupt Handlers                     */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_n32g031.s).                                                 */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 */
/*void PPP_IRQHandler(void)
{
}*/

/**
 * @}
 */
