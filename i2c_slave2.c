#include "n32g031_i2c.h"
#include "n32g031.h"
#include "i2c_slave2.h"

volatile _iicMaster2 iicMaster2;  // useIT
volatile _iicSlave2 iicSlave2;    // useIT

/**
 * =======================================================================================
 * =======================================================================================
 */

void I2C2_ResetBusy()
{
    I2C2->CTRL1 |= 0x8000;  // Reset Busy
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    I2C2->CTRL1 &= ~0x8000;
}

void I2C2_Config(void)
{
    I2C_InitType I2C_InitStructure;
    //I2C_InitType i2c1_slave;
    GPIO_InitType i2c1_gpio;
    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    
    GPIO_InitStruct(&i2c1_gpio);
    //PA10 -- SCL; PA9 -- SDA
    i2c1_gpio.Pin        = I2C2_SCL_PIN | I2C2_SDA_PIN;
    i2c1_gpio.GPIO_Speed = GPIO_SPEED_HIGH;
    i2c1_gpio.GPIO_Mode  = GPIO_MODE_AF_OD;//alternate open-drain
    i2c1_gpio.GPIO_Alternate = GPIO_AF_I2C2;
    GPIO_InitPeripheral(GPIOx, &i2c1_gpio);    
    
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C2, ENABLE);
    I2C_InitStructure.OwnAddr1 = I2C_SLAVE_ADDR;     
    I2C_DeInit(I2C2);
    I2C2_ResetBusy();
    I2C_InitStructure.BusMode     = I2C_BUSMODE_I2C;
    I2C_InitStructure.FmDutyCycle = I2C_FMDUTYCYCLE_2;
    I2C_InitStructure.AckEnable   = I2C_ACKEN;
    I2C_InitStructure.AddrMode    = I2C_ADDR_MODE_7BIT;
    I2C_InitStructure.ClkSpeed    = 100000;  // 100K
    I2C_Init(I2C2, &I2C_InitStructure);      // Initial and Enable I2Cx
    I2C_Enable(I2C2, ENABLE);
}

void I2C2_NVIC_Config(void)
{
    NVIC_InitType NVIC_InitStructure;
    
	NVIC_InitStructure.NVIC_IRQChannel    = I2C2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;    
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x01;    
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel++;  // I2Cx_ER_IRQn
    NVIC_Init(&NVIC_InitStructure);
    I2C_ConfigInt(I2C2, I2C_INT_EVENT | I2C_INT_BUF | I2C_INT_ERR, NVIC_InitStructure.NVIC_IRQChannelCmd);
}

void I2C2_ResetInit()
{
    I2C2_Config();
    I2C2_NVIC_Config();  // #define I2Cx_UseIT
}

/**
 * =======================================================================================
 * =======================================================================================
 */
void I2C2_Initial(void)
{
    I2C2_ResetInit();
}