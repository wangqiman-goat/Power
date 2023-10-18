
#include "n32g031_i2c.h"
#include "n32g031.h"
#include "i2c_slave.h"
#include "main.h"
#include "crc16.h"
#include <string.h>

#define     NO_ERROR        0
#define     UNKNOWN_ERROR   1
#define     UNKNOWN_ID      2
#define     M_TYPE          0x01B0
#define     SW_VERSION      0x01B1
#define     R_CURRENT       0x05C0 //0x05C0~0x05C7
#define     R_OCP           0x03B1 //0x03B1~0x03B8
#define     R_OUT_ERROR     0x03B9 //0x03B9~0x03C0
#define     R_OTP           0x03C1
#define     W_SWITCH        0x0620 //0x0620~0x0627
#define     R_TEMP          0x0501 //temperature data
#define     R_PIN_STATUS    0x0D00 //0x0D00~0x0D07, for test purpose

//const char m_type[] = "PSE-D48P8S200A"; //module name, POWER: 1600W
const char m_type[] = "PSE-D48P8S75A"; //module name, POWER: 600W
const char sw_version[] = "V1.0"; //software version

uint8_t data_rx_buf[BUFFER_SIZE] = {0};//data buffer
uint8_t data_tx_buf[BUFFER_SIZE] = {0};//data buffer for I2C2
static __IO uint32_t I2CTimeout;
//volatile uint8_t flag_slave_recv_finish = 0;//received flag
volatile uint8_t flag_slave_send_finish = 0;//transmitted flag
//volatile uint8_t req_is_processed=0;//the request from the master has been processed
//static uint8_t rxDataNum = 0; //data length received
//static uint8_t txDataNum = 0; //data length transmitted

//static uint8_t RCC_RESET_Flag = 0;
//static uint8_t fb_status = 0x00;//status

volatile uint8_t iic_send_buffer[BUFFER_SIZE];

void CommTimeOut_CallBack(ErrCode_t errcode);

volatile _iicMaster iicMaster;  // useIT for master
volatile _iicSlave iicSlave;    // useIT for I2C1
volatile _iicSlave iicSlave2;   // useIT for I2C2

/**
 * =======================================================================================
 * =======================================================================================
 */

void I2C1_ResetBusy()
{
    I2C1->CTRL1 |= 0x8000;  // Reset Busy
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    I2C1->CTRL1 &= ~0x8000;
}

void I2C1_Config(void)
{
    I2C_InitType I2C_InitStructure;
    //I2C_InitType i2c1_slave;
    GPIO_InitType i2c1_gpio;
    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);

    GPIO_InitStruct(&i2c1_gpio);
    //PB6 -- SCL; PB7 -- SDA
    i2c1_gpio.Pin        = I2Cx_SCL_PIN | I2Cx_SDA_PIN;
    i2c1_gpio.GPIO_Speed = GPIO_SPEED_HIGH;
    i2c1_gpio.GPIO_Mode  = GPIO_MODE_AF_OD;//alternate open-drain
    i2c1_gpio.GPIO_Alternate = GPIO_AF_I2C;
    GPIO_InitPeripheral(GPIOx, &i2c1_gpio);

    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
    I2C_InitStructure.OwnAddr1 = I2C_SLAVE_ADDR;
    I2C_DeInit(I2C1);
    I2C1_ResetBusy();
    I2C_InitStructure.BusMode     = I2C_BUSMODE_I2C;
    I2C_InitStructure.FmDutyCycle = I2C_FMDUTYCYCLE_2;
    I2C_InitStructure.AckEnable   = I2C_ACKEN;
    I2C_InitStructure.AddrMode    = I2C_ADDR_MODE_7BIT;
    I2C_InitStructure.ClkSpeed    = 100000;  // 100K
    I2C_Init(I2C1, &I2C_InitStructure);      // Initial and Enable I2Cx
    I2C_Enable(I2C1, ENABLE);
}

void I2C1_NVIC_Config(void)
{
    NVIC_InitType NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel    = I2C1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x01;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel++;  // I2Cx_ER_IRQn
    NVIC_Init(&NVIC_InitStructure);
    I2C_ConfigInt(I2C1, I2C_INT_EVENT | I2C_INT_BUF | I2C_INT_ERR, NVIC_InitStructure.NVIC_IRQChannelCmd);
}

void I2C1_ResetInit()
{
    I2C1_Config();
    I2C1_NVIC_Config();  // #define I2Cx_UseIT
}

/**
 * =======================================================================================
 * =======================================================================================
 */
void I2C1_Initial(void)
{
    I2C1_ResetInit();
}

/**
 * @brief  I2Cx interrupt callback function
 * @param I2Cx I2C1
 */
//void I2C_EV_IRQ_CallBack(volatile _iicMaster* iicMaster, volatile _iicSlave* iicSlave)
void I2C1_IRQHandler(void)
{
    uint32_t last_event = I2C_GetLastEvent(I2C1);
    if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER)
    {
        switch (last_event) {
            /* Slave Tx */
            case I2C_EVT_SLAVE_SEND_ADDR_MATCHED:  // 0x00060082.EV1.EV3_1 (ADDRF TXDATE)
            case I2C_EVT_SLAVE_DATA_SENDING:       // 0x00060080.EV3 (TXDATE)
            case I2C_EVT_SLAVE_DATA_SENDED:        // 0x00060084.EV3_2 (TXDATE BSF)
                /*
                if (iicSlave.ptr < IIC_Slave_BufSize) {
                    I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
                } else {
                    I2C_SendData(I2C1, 0xFF);  // clear TXDATE
                }
                */
                switch (iicSlave.buf[0])
                {
                    case PSU_INFO:
                        if (iicSlave.ptr < IIC_Slave_BufSize) {
                            //I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
                            I2C_SendData(I2C1, iic_send_buffer[iicSlave.ptr++]);  // clear TXDATE
                        } else {
                            I2C_SendData(I2C1, 0xFF);  // clear TXDATE
                        }
                        break;
                    default:
                        data_rx_buf[0] = 0x00;
                        I2C1->DAT = 0xFF;
                        break;
                }
                break;
            /* Slave Rx */
            case I2C_EVT_SLAVE_RECV_ADDR_MATCHED:  // 0x00020002.EV1 (ADDRF)
                iicSlave.ptr = 0;
                if (IIC_Slave_BufSize > 256) {
                    iicSlave.ptrSt = 1;  //
                } else {
                    iicSlave.ptrSt = 2;  //
                }
                //I2C_RecvData(I2C1);
                break;
            case I2C_EVT_SLAVE_DATA_RECVD:  // 0x00020040.EV2 (RXDATNE)
                switch (iicSlave.ptrSt) {
                    case 1:                                                  // 实际并未执行case 1
                        iicSlave.ptr   = (uint16_t)I2C_RecvData(I2C1) << 8;  // clear RXDATNE
                        iicSlave.ptrSt = 2;                                  //
                        break;
                    case 2:                                    // write higher byte of ptr
                    //iicSlave.ptr += I2C_RecvData(I2C1);  // clear RXDATNE  //commented by Power on March 3rd, 2023
                    //iicSlave.ptrSt = 3;                  // 准备写buf数据
                    //break;
                    case 3:
                        if (iicSlave.ptr < IIC_Slave_BufSize) {
                            iicSlave.buf[iicSlave.ptr++] = I2C_RecvData(I2C1);  // clear RXDATNE
                            iicSlave.bufSt                = 1;  //data is updating now...
                        } else {
                            I2C_RecvData(I2C1);  // clear RXDATNE
                        }
                        break;
                    default:
                        I2C_RecvData(I2C1);  // clear RXDATNE
                        break;
                }
                break;
            case I2C_EVT_SLAVE_STOP_RECVD:   // 0x00000010.EV4 (STOPF)
                iicSlave.ptr = 0;
                I2C_Enable(I2C1, ENABLE);    // clear STOPF
                if (iicSlave.bufSt == 1) {
                    iicSlave.bufSt = 2;     //data updating is complete
                }
                /*======here we don't have any operation from upper machine
                switch(iicSlave.buf[0])
                {
                    case OUTPUT_CONTROL:
                        Set_Output_Switch(iicSlave.buf[1]);
                        //iicSlave.buf[1]=0xFF;
                        break;
                    default:
                        //iicSlave.buf[0]=0x00;
                        break;
                }*/
                break;
            default:
                I2C1_ResetInit();
                break;
        }
    }
}

///**
// * @brief  i2c slave Interrupt service function
// */
//void I2C1_IRQHandler_Backup(void)
//{
//    uint8_t timeout_flag = 0;
//    uint32_t last_event = 0;
//
//    last_event = I2C_GetLastEvent(I2C1);
//  if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER) // MSMODE = 0:I2C slave mode
//    {
//        switch (last_event)
//        {
//          //EV1: 主机发送的地址与本从机地址匹配（从机准备接收数据，也就是主机要发送数据给从机）
//          case I2C_EVT_SLAVE_RECV_ADDR_MATCHED: //0x00020002.EV1 Rx addr matched
//              //clear flag,ready to receive data
//              rxDataNum = 0;
//              break;
//
//          //EV1: 主机发送的地址与本从机地址匹配（主机要从从机获取数据，从机开始发送第一个数据给主机）
//          case I2C_EVT_SLAVE_SEND_ADDR_MATCHED: //0x00060082.EV1 Tx addr matched
//              txDataNum = 0;
//              rxDataNum = 0;
//              switch(data_rx_buf[0])
//              {
//                  case OUTPUT_CONTROL:
//                      //I2C1->DAT = fb_status;
//                      I2C1->DAT = 0x00;
//                      break;
//                  case OUTPUT_VOLTAGE_HIGH: //test purpose
//                      I2C1->DAT = VOS_OUT>>8;
//                      break;
//                  case OUTPUT_VOLTAGE_LOW: //test purpose
//                      I2C1->DAT = (uint8_t)VOS_OUT;
//                      break;
//
//                  default:
//                      data_rx_buf[0]=0x00;
//                      I2C1->DAT=0xFF;
//                      break;
//              }
//              break;
//
//          //EV3: SlaveTransmitter，数据正在发送中，此事件和I2C_EVT_SLAVE_DATA_SENDED类似
//          case I2C_EVT_SLAVE_DATA_SENDING:  //0x00060080. EV3 Sending data
//              break;
//
//          //EV3: 从机发送一个数据成功，收到主机回应后接着发送下一个数据（只有返回电流值时才会发送多个字节）
//          case I2C_EVT_SLAVE_DATA_SENDED:
//              break;
//
//          //EV2: SlaveReceiver，从机接收到并保存从主机发送来的数据
//          case I2C_EVT_SLAVE_DATA_RECVD: //0x00020040.EV2 one byte recved
//              if(rxDataNum < BUFFER_SIZE)
//              {
//                  data_rx_buf[rxDataNum++] = I2C1->DAT;
//              }
//              break;
//
//          //EV4: When the application is expecting the end of the communication: master sends a stop condition and data transmission is stopped.
//          //表示应用程序希望结束通信，也就是说EV4表明主机发送了一个STOP停止信号，从机接收数据完成
//          case I2C_EVT_SLAVE_STOP_RECVD: // 0x00000010 EV4
//              I2C_Enable(I2C1, ENABLE);
//              if(rxDataNum != 0)
//              {
//                  flag_slave_recv_finish = 1; // The STOPF bit is not set after a NACK reception
//              }
//              switch(data_rx_buf[0])
//              {
//                  //case OUTPUT_CONTROL:
//                      //Set_Output_Switch(data_rx_buf[1]);
//                      //data_rx_buf[1]=0xFF;
//                      //break;
//                  default:
//                      data_rx_buf[0]=0x00;
//                      break;
//              }
//              break;
//
//          default:
//              I2C_Enable(I2C1, ENABLE);
//              timeout_flag = 1;
//              break;
//        }
//    }
//
//    if (timeout_flag)//出现超时错误则根据错误代码重启I2C模块
//    {
//        if ((I2CTimeout--) == 0)
//        {
//            CommTimeOut_CallBack(SLAVE_UNKNOW);
//        }
//    }
//    else
//    {
//        I2CTimeout = I2CT_LONG_TIMEOUT;
//    }
//
//  //EV3_2: When the master sends a NACK in order to tell slave that data transmission
//  //shall end (before sending the STOP condition). In this case slave has to stop sending
//  //data bytes and expect a Stop condition on the bus.
//  //主机在发送STOP之前发送了一个NACK给从机，表示数据传输结束（主机接收完从机发送的数据，从机停止发送数据并等待主机发送STOP）
//    if(last_event == I2C_EVT_SLAVE_ACK_MISS)
//    {
//        I2C_ClrFlag(I2C1, I2C_FLAG_ACKFAIL);
//        if(txDataNum != 0)  //从机已发送最后一个数据并从主机收到NACK
//        {
//            flag_slave_send_finish = 1;//发送完成标志
//        }
//        else //还没有发送完数据就收到主机的NACK，产生EV3_2事件，表示发送出错
//        {
//
//        }
//    }
//}

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

/**
*@name: I2C2_Config
*@description: for the main communication with the upper machine
*@input: none
*@output: none
**/
void I2C2_Config(void)
{
    I2C_InitType I2C_InitStructure;
    //I2C_InitType i2c1_slave;
    GPIO_InitType i2c2_gpio;
    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);

    GPIO_InitStruct(&i2c2_gpio);
    //PA9 -- SCL; PA10 -- SDA
    i2c2_gpio.Pin        = I2C2_SCL_PIN | I2C2_SDA_PIN;
    i2c2_gpio.GPIO_Speed = GPIO_SPEED_HIGH;
    i2c2_gpio.GPIO_Mode  = GPIO_MODE_AF_OD;//alternate open-drain
    i2c2_gpio.GPIO_Alternate = GPIO_AF_I2C2;
    GPIO_InitPeripheral(GPIOy, &i2c2_gpio);

    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C2, ENABLE);
    I2C_InitStructure.OwnAddr1 = I2C2_SLAVE_ADDR;
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

/**
 * @brief  Analyze the received data from upper machine
 * @param  data, the data buffer to be parsed
 * @return the error code
 */
static uint8_t Parse_Data(uint8_t* data)
{
    uint16_t id = 0;
    uint8_t ret = 0;
    uint16_t crc16 = 0;

    if (data[0] != 0 && data[0] != 1) //0:READ, 1:WRITE
    {
        //ret = UNKNOWN_ERROR;
        return UNKNOWN_ERROR;
    }
    if (data[1] > 30) //maximum data length
    {
        //ret = UNKNOWN_ERROR;
        return UNKNOWN_ERROR;
    }
    crc16 = CRC16_CCITT(data, 30); //CRC16 check
    if (crc16 == ((data[31] & 0x00FF) << 8 | data[30]))
    {
        ret = NO_ERROR;
    }
    else
    {
        ret = UNKNOWN_ERROR;
    }
    id = (data[3] & 0x00FF) << 8 | data[2];
    if ((id >= R_CURRENT && id <= R_CURRENT + 7)
            || (id >= W_SWITCH && id <= W_SWITCH + 7)
            || (id >= R_OCP && id <= R_OCP + 7)
            || (id >= R_OUT_ERROR && id <= R_OUT_ERROR + 7)
            || (id == M_TYPE)
            || (id == SW_VERSION)
            || (id == R_OTP)
            || (id == R_TEMP))
    {
        ret = NO_ERROR;
    }
    else
    {
        ret = UNKNOWN_ID;
    }

    return ret;
}

/**
 * @brief  process the request from the master device, the I2C2 interface
 * @param  none
 * @return none
 */
void Process_I2C2_Request(void)
{
    uint16_t id = 0;
    uint8_t ptr = 0;
    //uint16_t value = 0; //result
    int16_t value = 0;
    uint8_t len = 0;
    uint8_t i = 0;
    uint16_t crc16 = 0;
    uint8_t temp = 0;
    uint8_t ret = Parse_Data(data_rx_buf);
    if (ret == 0) //no error
    {
        id = (data_rx_buf[3] & 0x00FF) << 8 | data_rx_buf[2];
        switch (id)
        {
            case W_SWITCH+0:        //ON/OFF Control
            case W_SWITCH+1:
            case W_SWITCH+2:
            case W_SWITCH+3:
            case W_SWITCH+4:
            case W_SWITCH+5:
            case W_SWITCH+6:
            case W_SWITCH+7:
                value = Set_IOUT_Switch(id - W_SWITCH, data_rx_buf[4]); //data_rx_buf[4] is ON/OFF byte
                len = 5;
                temp = 0;
                break;
            case R_OCP+0:           //OCP
            case R_OCP+1:
            case R_OCP+2:
            case R_OCP+3:
            case R_OCP+4:
            case R_OCP+5:
            case R_OCP+6:
            case R_OCP+7:
                value = Get_OCP_Status(id - R_OCP);
                len = 5;
                temp = 0;
                break;
            case R_CURRENT+0:       //output current
            case R_CURRENT+1:
            case R_CURRENT+2:
            case R_CURRENT+3:
            case R_CURRENT+4:
            case R_CURRENT+5:
            case R_CURRENT+6:
            case R_CURRENT+7:
                value = Get_IOUT_Value(id - R_CURRENT);
                len = 6;
                temp = 0;
                break;
            case R_OUT_ERROR+0:     //Output Error
            case R_OUT_ERROR+1:
            case R_OUT_ERROR+2:
            case R_OUT_ERROR+3:
            case R_OUT_ERROR+4:
            case R_OUT_ERROR+5:
            case R_OUT_ERROR+6:
            case R_OUT_ERROR+7:
                value = Get_VOUT_Status(id - R_OUT_ERROR);
                len = 5;
                temp = 0;
                break;
            case R_TEMP:            //actual temperature rounded
                value = TMP_OUT;
                len = 5;
                temp = 0;
                break;
            case R_PIN_STATUS+0:    //Pin status
            case R_PIN_STATUS+1:
            case R_PIN_STATUS+2:
            case R_PIN_STATUS+3:
            case R_PIN_STATUS+4:
            case R_PIN_STATUS+5:
            case R_PIN_STATUS+6:
            case R_PIN_STATUS+7:
                value = Get_IOUT_Switch(id - R_PIN_STATUS);
                len = 5;
                temp = 0;
                break;
            case M_TYPE:            //module type
                len = 24;
                temp = strlen(m_type);
                break;
            case SW_VERSION:        //software version
                len = 24;
                temp = strlen(sw_version);
                break;
            case R_OTP:             //over temperature
                value = AlarmStatus.alarm.OTP;
                len = 5;
                temp = 0;
                break;
            default:
                break;
        }
        data_tx_buf[ptr++] = NO_ERROR;
        data_tx_buf[ptr++] = len;           //valid data length = ACK+LENGTH+ID+Valid Value
        data_tx_buf[ptr++] = data_rx_buf[2]; //ID
        data_tx_buf[ptr++] = data_rx_buf[3];
        if (len == 5 || len == 6)           //valid value of current, OCP or output status
        {
            data_tx_buf[ptr++] = value % 256;
            data_tx_buf[ptr++] = value >> 8;
            //for (i = len; i < 30; i++)
            for (i = 6; i < 30; i++)      //bytes to be filled with 0
            {
                data_tx_buf[ptr++] = 0;
            }
        }
        else                                //valid value of module type or software version
        {
            if (id == M_TYPE)
            {
                for (i = 0; i < strlen(m_type); i++)
                {
                    data_tx_buf[ptr++] = m_type[i];
                }
            }
            else
            {
                for (i = 0; i < strlen(sw_version); i++)
                {
                    data_tx_buf[ptr++] = sw_version[i];
                }
            }
            for (i = 4 + temp; i < 30; i++) //bytes to be filled with 0 at the end of the module type or software version
            {
                data_tx_buf[ptr++] = 0;
            }
        }
        //calculate the CRC16
        crc16 = CRC16_CCITT(data_tx_buf, ptr);
        data_tx_buf[ptr++] = crc16 % 256;
        data_tx_buf[ptr++] = crc16 >> 8;
    }
    else
    {
        data_tx_buf[ptr++] = ret; //ACK
        data_tx_buf[ptr++] = 4; //no valid value, ACK+LENGTH+ID
        data_tx_buf[ptr++] = data_rx_buf[2]; //ID
        data_tx_buf[ptr++] = data_rx_buf[3];
        for (i = 4; i < 30; i++) //bytes to be filled with 0
        {
            data_tx_buf[ptr++] = 0;
        }
        crc16 = CRC16_CCITT(data_tx_buf, ptr);
        data_tx_buf[ptr++] = crc16 % 256;
        data_tx_buf[ptr++] = crc16 >> 8;
    }
}

/**
 * @brief  I2Cx main process
 * @param none
 */
void Process_I2C2(void)
{
    if (AlarmStatus.alarm.I2C2_NEW_REQ)
    {
        Process_I2C2_Request();
        AlarmStatus.alarm.I2C2_NEW_REQ = 0;
        AlarmStatus.alarm.I2C2_REQ_PROCESSED = 1;
    }
}

/**
 * @brief  I2Cx interrupt callback function
 * @param I2Cx I2C2
 */
void I2C2_IRQHandler(void)
{
    uint32_t last_event = I2C_GetLastEvent(I2C2);
    if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER)
    {
        switch (last_event) {
            /* Slave Tx */
            case I2C_EVT_SLAVE_SEND_ADDR_MATCHED:  // 0x00060082.EV1.EV3_1 (ADDRF TXDATE), the slave address(read) sent by the master is matched, then we can send out data to the master.
                iicSlave2.ptr2 = 0;
                if (AlarmStatus.alarm.I2C2_REQ_PROCESSED)
                {
                    I2C_SendData(I2C2, data_tx_buf[iicSlave2.ptr2++]);//send the first byte
                }
                else
                {
                    I2C_SendData(I2C2, 0xFF);  //clear TXDATE flag
                }
                break;

            //the master wants to read the next byte from slave device
            case I2C_EVT_SLAVE_DATA_SENDING:       // 0x00060080.EV3 (TXDATE)
            case I2C_EVT_SLAVE_DATA_SENDED:        // 0x00060084.EV3_2 (TXDATE BSF)
                if (AlarmStatus.alarm.I2C2_REQ_PROCESSED) //the received request has been processed and now we send out the result
                {
                    if (iicSlave2.ptr2 < IIC_Slave_BufSize)
                    {
                        I2C_SendData(I2C2, data_tx_buf[iicSlave2.ptr2++]); //send data and clear TXDATE flag
                    }
                    else
                    {
                        AlarmStatus.alarm.I2C2_REQ_PROCESSED = 0;
                        iicSlave2.ptr2 = 0;
                        I2C_SendData(I2C2, 0xFF);  //clear TXDATE flag
                    }
                }
                else
                {
                    I2C_SendData(I2C2, 0xFF);
                }
                break;

            /* Slave Rx */
            //The slave address(write) sent by the master is matched, here we are prepare for the coming data.
            case I2C_EVT_SLAVE_RECV_ADDR_MATCHED:  // 0x00020002.EV1 (ADDRF),
                iicSlave2.ptr = 0;
                //I2C_RecvData(I2C2);
                break;

            //The slave has received data from the master and all here we need to do is to save all of them.
            case I2C_EVT_SLAVE_DATA_RECVD:  // 0x00020040.EV2 (RXDATNE)
                if (iicSlave2.ptr < IIC_Slave_BufSize)
                {
                    iicSlave2.buf[iicSlave2.ptr++] = I2C_RecvData(I2C2);  //receive data and clear RXDATNE flag
                    //iicSlave2.bufSt                = 1;   //data is updating now...
                }
                else
                {
                    I2C_RecvData(I2C2);  // clear RXDATNE flag
                }
                break;

            //The master sends out a STOP condition to end the communication and release the BUS.
            case I2C_EVT_SLAVE_STOP_RECVD:   // 0x00000010.EV4 (STOPF)
                I2C_Enable(I2C2, ENABLE);    // clear STOPF
                if (iicSlave2.ptr >= IIC_Slave_BufSize) //write command from the master, a package of 32 bytes
                {
                    uint8_t i = 0;
                    AlarmStatus.alarm.I2C2_NEW_REQ = 1; // The STOPF bit is not set after a NACK reception
                    for (i = 0; i < IIC_Slave_BufSize; i++)
                    {
                        data_rx_buf[i] = iicSlave2.buf[i];
                    }
                    for (i = 0; i < IIC_Slave_BufSize; i++) //clear the receive buffer
                    {
                        iicSlave2.buf[i] = 0x00;
                    }
                }
                AlarmStatus.alarm.I2C2_REQ_PROCESSED = 0;
                iicSlave2.ptr2 = 0;
                iicSlave2.ptr = 0;
                break;
            default:
                I2C2_ResetInit();
                break;
        }
    }
    //EV3_2: When the master sends a NACK in order to tell slave that data transmission
    //shall end (before sending the STOP condition). In this case slave has to stop sending
    //data bytes and expect a Stop condition on the bus.
    //主机在发送STOP之前发送了一个NACK给从机，表示数据传输结束（主机接收完从机发送的数据，从机停止发送数据并等待主机发送STOP）
    if (last_event == I2C_EVT_SLAVE_ACK_MISS)
    {
        I2C_ClrFlag(I2C2, I2C_FLAG_ACKFAIL);
        if (iicSlave2.ptr2 != 0) //从机已发送最后一个数据并从主机收到NACK
        {
            flag_slave_send_finish = 1;//发送完成标志
        }
        else //还没有发送完数据就收到主机的NACK，产生EV3_2事件，表示发送出错
        {

        }
    }
}

void CommTimeOut_CallBack(ErrCode_t errcode)
{
    //log_info("...ErrCode:%d\r\n", errcode);

#if (COMM_RECOVER_MODE == MODULE_SELF_RESET)
    IIC_SWReset();
#elif (COMM_RECOVER_MODE == MODULE_RCC_RESET)
    IIC_RCCReset();
#elif (COMM_RECOVER_MODE == SYSTEM_NVIC_RESET)
    SystemNVICReset();
#endif
}




//**************************** CAN BE EXECUTED CORRECTLY*************************
//#include "n32g031_i2c.h"
//#include "n32g031.h"
//#include "i2c_slave.h"
//#include "main.h"

//uint8_t data_rx_buf[BUFFER_SIZE] = {0};//data buffer
//uint8_t data_tx_buf[BUFFER_SIZE] = {0};//data buffer for I2C2
//static __IO uint32_t I2CTimeout;
//uint8_t flag_slave_recv_finish = 0;//received flag
//uint8_t flag_slave_send_finish = 0;//transmitted flag
////static uint8_t rxDataNum = 0; //data length received
////static uint8_t txDataNum = 0; //data length transmitted

////static uint8_t RCC_RESET_Flag = 0;
////static uint8_t fb_status = 0x00;//status

//volatile uint8_t iic_send_buffer[BUFFER_SIZE];

//void CommTimeOut_CallBack(ErrCode_t errcode);

//volatile _iicMaster iicMaster;  // useIT for master
//volatile _iicSlave iicSlave;    // useIT for I2C1
//volatile _iicSlave iicSlave2;	// useIT for I2C2

///**
// * =======================================================================================
// * =======================================================================================
// */

//void I2C1_ResetBusy()
//{
//    I2C1->CTRL1 |= 0x8000;  // Reset Busy
//    __NOP();
//    __NOP();
//    __NOP();
//    __NOP();
//    __NOP();
//    I2C1->CTRL1 &= ~0x8000;
//}

//void I2C1_Config(void)
//{
//    I2C_InitType I2C_InitStructure;
//    //I2C_InitType i2c1_slave;
//    GPIO_InitType i2c1_gpio;
//    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
//    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
//    
//    GPIO_InitStruct(&i2c1_gpio);
//    //PB6 -- SCL; PB7 -- SDA
//    i2c1_gpio.Pin        = I2Cx_SCL_PIN | I2Cx_SDA_PIN;
//    i2c1_gpio.GPIO_Speed = GPIO_SPEED_HIGH;
//    i2c1_gpio.GPIO_Mode  = GPIO_MODE_AF_OD;//alternate open-drain
//    i2c1_gpio.GPIO_Alternate = GPIO_AF_I2C;
//    GPIO_InitPeripheral(GPIOx, &i2c1_gpio);    
//    
//    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
//    I2C_InitStructure.OwnAddr1 = I2C_SLAVE_ADDR;     
//    I2C_DeInit(I2C1);
//    I2C1_ResetBusy();
//    I2C_InitStructure.BusMode     = I2C_BUSMODE_I2C;
//    I2C_InitStructure.FmDutyCycle = I2C_FMDUTYCYCLE_2;
//    I2C_InitStructure.AckEnable   = I2C_ACKEN;
//    I2C_InitStructure.AddrMode    = I2C_ADDR_MODE_7BIT;
//    I2C_InitStructure.ClkSpeed    = 100000;  // 100K
//    I2C_Init(I2C1, &I2C_InitStructure);      // Initial and Enable I2Cx
//    I2C_Enable(I2C1, ENABLE);
//}

//void I2C1_NVIC_Config(void)
//{
//    NVIC_InitType NVIC_InitStructure;
//    
//	NVIC_InitStructure.NVIC_IRQChannel    = I2C1_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;    
//    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x01;    
//    NVIC_Init(&NVIC_InitStructure);
//    NVIC_InitStructure.NVIC_IRQChannel++;  // I2Cx_ER_IRQn
//    NVIC_Init(&NVIC_InitStructure);
//    I2C_ConfigInt(I2C1, I2C_INT_EVENT | I2C_INT_BUF | I2C_INT_ERR, NVIC_InitStructure.NVIC_IRQChannelCmd);
//}

//void I2C1_ResetInit()
//{
//    I2C1_Config();
//    I2C1_NVIC_Config();  // #define I2Cx_UseIT
//}

///**
// * =======================================================================================
// * =======================================================================================
// */
//void I2C1_Initial(void)
//{
//    I2C1_ResetInit();
//}

///**
// * @brief  I2Cx interrupt callback function
// * @param I2Cx I2C1
// */
////void I2C_EV_IRQ_CallBack(volatile _iicMaster* iicMaster, volatile _iicSlave* iicSlave)
//void I2C1_IRQHandler(void)
//{
//    uint32_t last_event = I2C_GetLastEvent(I2C1);
//    if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER)
//    {
//        switch (last_event) {
//                /* Slave Tx */
//            case I2C_EVT_SLAVE_SEND_ADDR_MATCHED:  // 0x00060082.EV1.EV3_1 (ADDRF TXDATE)
//            case I2C_EVT_SLAVE_DATA_SENDING:       // 0x00060080.EV3 (TXDATE)
//            case I2C_EVT_SLAVE_DATA_SENDED:        // 0x00060084.EV3_2 (TXDATE BSF)
//                /*
//                if (iicSlave.ptr < IIC_Slave_BufSize) {
//                    I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
//                } else {
//                    I2C_SendData(I2C1, 0xFF);  // clear TXDATE
//                }
//                */
//                switch(iicSlave.buf[0])
//				{
//					case PSU_INFO:
//						if (iicSlave.ptr < IIC_Slave_BufSize) {
//							//I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
//							I2C_SendData(I2C1, iic_send_buffer[iicSlave.ptr++]);  // clear TXDATE
//						} else {
//							I2C_SendData(I2C1, 0xFF);  // clear TXDATE
//						}
//						break;
//					default:
//						data_rx_buf[0]=0x00;
//						I2C1->DAT=0xFF;
//						break;
//                }
//                break;
//                /* Slave Rx */
//            case I2C_EVT_SLAVE_RECV_ADDR_MATCHED:  // 0x00020002.EV1 (ADDRF)
//                iicSlave.ptr = 0;
//                if (IIC_Slave_BufSize > 256) {
//                    iicSlave.ptrSt = 1;  //
//                } else {
//                    iicSlave.ptrSt = 2;  //
//                }
//				//I2C_RecvData(I2C1);
//                break;
//            case I2C_EVT_SLAVE_DATA_RECVD:  // 0x00020040.EV2 (RXDATNE)
//                switch (iicSlave.ptrSt) {
//                    case 1:                                                  //实际并未执行case 1
//                        iicSlave.ptr   = (uint16_t)I2C_RecvData(I2C1) << 8;  // clear RXDATNE
//                        iicSlave.ptrSt = 2;                                  //
//                        break;
//                    case 2:                                   // 写ptr高字节
//                        //iicSlave.ptr += I2C_RecvData(I2C1);  // clear RXDATNE  //commented by Power on March 3rd, 2023
//                        //iicSlave.ptrSt = 3;                  //准备写buf数据
//                        //break;
//                    case 3: 
//                        if (iicSlave.ptr < IIC_Slave_BufSize) {
//                            iicSlave.buf[iicSlave.ptr++] = I2C_RecvData(I2C1);  // clear RXDATNE
//                            iicSlave.bufSt                = 1;	//data is updating now...
//                        } else {
//                            I2C_RecvData(I2C1);  // clear RXDATNE
//                        }                        
//                        break;
//                    default:
//                        I2C_RecvData(I2C1);  // clear RXDATNE
//                        break;
//                }
//                break;
//            case I2C_EVT_SLAVE_STOP_RECVD:   // 0x00000010.EV4 (STOPF)
//				iicSlave.ptr=0;
//                I2C_Enable(I2C1, ENABLE);    // clear STOPF
//                if (iicSlave.bufSt == 1) { 
//                    iicSlave.bufSt = 2;		//data updating is complete
//                }
//				/*======here we don't have any operation from upper machine
//				switch(iicSlave.buf[0])
//				{
//					case OUTPUT_CONTROL:
//						Set_Output_Switch(iicSlave.buf[1]);
//						//iicSlave.buf[1]=0xFF;
//						break;
//					default:
//						//iicSlave.buf[0]=0x00;
//						break;
//				}*/
//                break;
//            default:
//                I2C1_ResetInit();
//                break;
//        }
//    }
//}

/////**
//// * @brief  i2c slave Interrupt service function
//// */
////void I2C1_IRQHandler_Backup(void)
////{
////    uint8_t timeout_flag = 0;
////    uint32_t last_event = 0;	
////    
////    last_event = I2C_GetLastEvent(I2C1);
////	if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER) // MSMODE = 0:I2C slave mode
////    {
////        switch (last_event)
////        {
////			//EV1: 主机发送的地址与本从机地址匹配（从机准备接收数据，也就是主机要发送数据给从机）
////			case I2C_EVT_SLAVE_RECV_ADDR_MATCHED: //0x00020002.EV1 Rx addr matched				
////				//clear flag,ready to receive data
////				rxDataNum = 0;
////				break;
////			
////			//EV1: 主机发送的地址与本从机地址匹配（主机要从从机获取数据，从机开始发送第一个数据给主机）
////			case I2C_EVT_SLAVE_SEND_ADDR_MATCHED: //0x00060082.EV1 Tx addr matched				
////				txDataNum = 0;
////				rxDataNum = 0;
////				switch(data_rx_buf[0])
////				{
////					case OUTPUT_CONTROL:
////						//I2C1->DAT = fb_status;
////						I2C1->DAT = 0x00;
////						break;					
////					case OUTPUT_VOLTAGE_HIGH: //test purpose
////						I2C1->DAT = VOS_OUT>>8;
////						break;
////					case OUTPUT_VOLTAGE_LOW: //test purpose
////						I2C1->DAT = (uint8_t)VOS_OUT;
////						break;
////					
////					default:
////						data_rx_buf[0]=0x00;
////						I2C1->DAT=0xFF;
////						break;
////				}				
////				break;
////			
////			//EV3: SlaveTransmitter，数据正在发送中，此事件和I2C_EVT_SLAVE_DATA_SENDED类似	
////			case I2C_EVT_SLAVE_DATA_SENDING:  //0x00060080. EV3 Sending data			   
////				break;
////			
////			//EV3: 从机发送一个数据成功，收到主机回应后接着发送下一个数据（只有返回电流值时才会发送多个字节）
////			case I2C_EVT_SLAVE_DATA_SENDED:						
////				break;
////			
////			//EV2: SlaveReceiver，从机接收到并保存从主机发送来的数据
////			case I2C_EVT_SLAVE_DATA_RECVD: //0x00020040.EV2 one byte recved
////				if(rxDataNum < BUFFER_SIZE)
////				{
////					data_rx_buf[rxDataNum++] = I2C1->DAT;
////				}
////				break; 
////			
////			//EV4: When the application is expecting the end of the communication: master sends a stop condition and data transmission is stopped.
////			//表示应用程序希望结束通信，也就是说EV4表明主机发送了一个STOP停止信号，从机接收数据完成
////			case I2C_EVT_SLAVE_STOP_RECVD: // 0x00000010 EV4
////				I2C_Enable(I2C1, ENABLE);   
////				if(rxDataNum != 0)
////				{
////					flag_slave_recv_finish = 1; // The STOPF bit is not set after a NACK reception
////				}
////				switch(data_rx_buf[0])
////				{
////					//case OUTPUT_CONTROL:
////						//Set_Output_Switch(data_rx_buf[1]);
////						//data_rx_buf[1]=0xFF;
////						//break;
////					default:
////						data_rx_buf[0]=0x00;
////						break;
////				}
////				break;
////			
////			default:
////				I2C_Enable(I2C1, ENABLE);
////				timeout_flag = 1;
////				break;
////        }
////    }
////    
////    if (timeout_flag)//出现超时错误则根据错误代码重启I2C模块
////    {
////        if ((I2CTimeout--) == 0)
////        {
////            CommTimeOut_CallBack(SLAVE_UNKNOW);
////        }
////    }
////    else
////    {
////        I2CTimeout = I2CT_LONG_TIMEOUT;
////    }
////	
////	//EV3_2: When the master sends a NACK in order to tell slave that data transmission
////	//shall end (before sending the STOP condition). In this case slave has to stop sending
////	//data bytes and expect a Stop condition on the bus.
////	//主机在发送STOP之前发送了一个NACK给从机，表示数据传输结束（主机接收完从机发送的数据，从机停止发送数据并等待主机发送STOP）
////    if(last_event == I2C_EVT_SLAVE_ACK_MISS)   
////    {   
////        I2C_ClrFlag(I2C1, I2C_FLAG_ACKFAIL);
////        if(txDataNum != 0)  //从机已发送最后一个数据并从主机收到NACK
////        {
////            flag_slave_send_finish = 1;//发送完成标志
////        }
////        else //还没有发送完数据就收到主机的NACK，产生EV3_2事件，表示发送出错
////        {
////			
////        }
////    }
////}

///**
// * =======================================================================================
// * =======================================================================================
// */

//void I2C2_ResetBusy()
//{
//    I2C2->CTRL1 |= 0x8000;  // Reset Busy
//    __NOP();
//    __NOP();
//    __NOP();
//    __NOP();
//    __NOP();
//    I2C2->CTRL1 &= ~0x8000;
//}

///**
//*@name: I2C2_Config
//*@description: for the main communication with the upper machine
//*@input: none
//*@output: none
//**/
//void I2C2_Config(void)
//{
//    I2C_InitType I2C_InitStructure;
//    //I2C_InitType i2c1_slave;
//    GPIO_InitType i2c1_gpio;
//    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
//    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
//    
//    GPIO_InitStruct(&i2c1_gpio);
//    //PA9 -- SCL; PA10 -- SDA
//    i2c1_gpio.Pin        = I2C2_SCL_PIN | I2C2_SDA_PIN;
//    i2c1_gpio.GPIO_Speed = GPIO_SPEED_HIGH;
//    i2c1_gpio.GPIO_Mode  = GPIO_MODE_AF_OD;//alternate open-drain
//    i2c1_gpio.GPIO_Alternate = GPIO_AF_I2C2;
//    GPIO_InitPeripheral(GPIOx, &i2c1_gpio);    
//    
//    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C2, ENABLE);
//    I2C_InitStructure.OwnAddr1 = I2C2_SLAVE_ADDR;     
//    I2C_DeInit(I2C2);
//    I2C2_ResetBusy();
//    I2C_InitStructure.BusMode     = I2C_BUSMODE_I2C;
//    I2C_InitStructure.FmDutyCycle = I2C_FMDUTYCYCLE_2;
//    I2C_InitStructure.AckEnable   = I2C_ACKEN;
//    I2C_InitStructure.AddrMode    = I2C_ADDR_MODE_7BIT;
//    I2C_InitStructure.ClkSpeed    = 100000;  // 100K
//    I2C_Init(I2C2, &I2C_InitStructure);      // Initial and Enable I2Cx
//    I2C_Enable(I2C2, ENABLE);
//}

//void I2C2_NVIC_Config(void)
//{
//    NVIC_InitType NVIC_InitStructure;
//    
//	NVIC_InitStructure.NVIC_IRQChannel    = I2C2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;    
//    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x01;    
//    NVIC_Init(&NVIC_InitStructure);
//    NVIC_InitStructure.NVIC_IRQChannel++;  // I2Cx_ER_IRQn
//    NVIC_Init(&NVIC_InitStructure);
//    I2C_ConfigInt(I2C2, I2C_INT_EVENT | I2C_INT_BUF | I2C_INT_ERR, NVIC_InitStructure.NVIC_IRQChannelCmd);
//}

//void I2C2_ResetInit()
//{
//    I2C2_Config();
//    I2C2_NVIC_Config();  // #define I2Cx_UseIT
//}

///**
// * =======================================================================================
// * =======================================================================================
// */
//void I2C2_Initial(void)
//{
//    I2C2_ResetInit();
//}


//static void Parse_Data()
//{
//	
//}

///**
// * @brief  I2Cx interrupt callback function
// * @param I2Cx I2C2
// */
//void I2C2_IRQHandler(void)
//{
//    uint32_t last_event = I2C_GetLastEvent(I2C2);
//    if ((last_event & I2C_ROLE_MASTER) != I2C_ROLE_MASTER)
//    {
//        switch (last_event) {
//                /* Slave Tx */
//            case I2C_EVT_SLAVE_SEND_ADDR_MATCHED:  // 0x00060082.EV1.EV3_1 (ADDRF TXDATE)
//            case I2C_EVT_SLAVE_DATA_SENDING:       // 0x00060080.EV3 (TXDATE)
//            case I2C_EVT_SLAVE_DATA_SENDED:        // 0x00060084.EV3_2 (TXDATE BSF)
//                /*
//                if (iicSlave.ptr < IIC_Slave_BufSize) {
//                    I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
//                } else {
//                    I2C_SendData(I2C1, 0xFF);  // clear TXDATE
//                }
//                */
//                switch(iicSlave.buf[0])
//				{
//					case PSU_INFO:
//						if (iicSlave.ptr < IIC_Slave_BufSize) {
//							//I2C_SendData(I2C1, iicSlave.buf[iicSlave.ptr++]);  // clear TXDATE
//							I2C_SendData(I2C2, iic_send_buffer[iicSlave.ptr++]);  // clear TXDATE
//						} else {
//							I2C_SendData(I2C2, 0xFF);  // clear TXDATE
//						}
//						break;
//					default:
//						data_rx_buf[0]=0x00;
//						I2C2->DAT=0xFF;
//						break;
//                }
//                break;
//                /* Slave Rx */
//            case I2C_EVT_SLAVE_RECV_ADDR_MATCHED:  // 0x00020002.EV1 (ADDRF)
//                iicSlave.ptr = 0;
//                if (IIC_Slave_BufSize > 256) {
//                    iicSlave.ptrSt = 1;  //
//                } else {
//                    iicSlave.ptrSt = 2;  //
//                }
//				//I2C_RecvData(I2C1);
//                break;
//            case I2C_EVT_SLAVE_DATA_RECVD:  // 0x00020040.EV2 (RXDATNE)
//                switch (iicSlave.ptrSt) {
//                    case 1:                                                  //实际并未执行case 1
//                        iicSlave.ptr   = (uint16_t)I2C_RecvData(I2C2) << 8;  // clear RXDATNE
//                        iicSlave.ptrSt = 2;                                  //
//                        break;
//                    case 2:                                   // 写ptr高字节
//                        //iicSlave.ptr += I2C_RecvData(I2C1);  // clear RXDATNE  //commented by Power on March 3rd, 2023
//                        //iicSlave.ptrSt = 3;                  //准备写buf数据
//                        //break;
//                    case 3: 
//                        if (iicSlave.ptr < IIC_Slave_BufSize) {
//                            iicSlave.buf[iicSlave.ptr++] = I2C_RecvData(I2C2);  // clear RXDATNE
//                            iicSlave.bufSt                = 1;	//data is updating now...
//                        } else {
//                            I2C_RecvData(I2C2);  // clear RXDATNE
//                        }                        
//                        break;
//                    default:
//                        I2C_RecvData(I2C2);  // clear RXDATNE
//                        break;
//                }
//                break;
//            case I2C_EVT_SLAVE_STOP_RECVD:   // 0x00000010.EV4 (STOPF)
//				iicSlave.ptr=0;
//                I2C_Enable(I2C2, ENABLE);    // clear STOPF
//                if (iicSlave.bufSt == 1) { 
//                    iicSlave.bufSt = 2;		//data updating is complete
//                }				
//                break;
//            default:
//                I2C2_ResetInit();
//                break;
//        }
//    }
//}

//void CommTimeOut_CallBack(ErrCode_t errcode)
//{
//    //log_info("...ErrCode:%d\r\n", errcode);
//    
//#if (COMM_RECOVER_MODE == MODULE_SELF_RESET)
//    IIC_SWReset();
//#elif (COMM_RECOVER_MODE == MODULE_RCC_RESET)
//    IIC_RCCReset();
//#elif (COMM_RECOVER_MODE == SYSTEM_NVIC_RESET)
//    SystemNVICReset();
//#endif
//}


