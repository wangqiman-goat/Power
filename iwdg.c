#include "main.h"

__IO uint32_t LsiFreq     = 30000;//LSI时钟频率


/**
*@name: IWDG_Init
*@description: 看门狗初始化，使用LSI时钟的128分频作为独立看门狗的时钟
			   定时约1秒复位，必须要此时间内喂狗
*@params: None
*@return: None
*/
void IWDG_Init(void)
{
	RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    DBG_ConfigPeriph(DBG_IWDG_STOP, ENABLE);//调试时停止IWDG功能
	
	//Check if the system has resumed from IWDG reset
    if (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_IWDGRSTF) != RESET)
    {
        /* IWDGRST flag set */
        /* Clear reset flags */
        RCC_ClrFlag();
    }
	
	/* IWDG timeout equal to 250 ms (the timeout may varies due to LSI frequency
       dispersion) */
    //Enable write access to IWDG_PR and IWDG_RLR registers
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);

    //IWDG counter clock: LSI/128
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV128);

    /* Set counter reload value to obtain 1000ms IWDG TimeOut.
       Counter Reload Value = 1000ms/IWDG counter clock period
                            = 1000ms / (LSI/128)
                            = 4.27ms / (LsiFreq/128)
     */
    //log_debug("LsiFreq is: %d\n", LsiFreq);
    IWDG_CntReload(234);//234*4.27~=1000(ms)
    
	//Reload IWDG counter
    IWDG_ReloadKey();//每当向 IWDG_KEY 寄存器写入 0xAAAA 时，重装载值会被传送到计数器中

    //Enable IWDG (the LSI oscillator will be enabled by hardware)
    IWDG_Enable();//向IWDG_KEY 寄存器写入 0xCCCC，启动看门狗工作
}
