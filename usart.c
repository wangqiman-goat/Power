#include "n32g031.h"
#include "n32g031_usart.h"
#include "usart.h"
#include "main.h"
#include <string.h>
#include "crc16.h"

struct	UART_Data_Struct	UART1_STRUCT;

//#define 	__NULL 	((void*) 0)

/*
======================================================

			RS485 related macros

======================================================
*/
#define		RS485_GPIOx_CLK		RCC_APB2_PERIPH_GPIOB
#define		RS485_GPIO_PIN		GPIO_PIN_10
#define 	RS485_GPIOx        	GPIOB
#define		RS485_L()			GPIO_ResetBits(RS485_GPIOx, RS485_GPIO_PIN)
#define		RS485_H()			GPIO_SetBits(RS485_GPIOx, RS485_GPIO_PIN)

uint8_t		RS485_Delay;		//the switching time between transmit mode and receive mode of RS485
uint8_t		object_num=0;		//object numbers

const char module_type[]="CHINA TELECOM PSU";//module name
const char software_version[]="V1.0.1";//software version
volatile uint8_t rcv_timeout=0;


/**
*@name: RS485_Configuration
*@description: RS485 configuration which is not called
*@params: none
*@return: none
*/
void RS485_Configuration(void)
{
	GPIO_InitType GPIO_InitStructure;
	RCC_EnableAPB2PeriphClk(RS485_GPIOx_CLK, ENABLE);

    /* -2- Configure GPIOx_PIN in output push-pull mode */
    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin = RS485_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_HIGH;
    GPIO_InitPeripheral(RS485_GPIOx, &GPIO_InitStructure);
	GPIO_ResetBits(RS485_GPIOx, RS485_GPIO_PIN);
}


/**
 * @brief  Configures the different GPIO ports of USART.
 */
void USART_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;
	USART_InitType USART_InitStructure;

    //Initialize GPIO_InitStructure
    GPIO_InitStruct(&GPIO_InitStructure);
   
    //Configure USARTy Tx as alternate function push-pull
    GPIO_InitStructure.Pin            = USARTy_TxPin;    
    GPIO_InitStructure.GPIO_Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = USARTy_Tx_GPIO_AF;//alternate function
    GPIO_InitPeripheral(USARTy_GPIO, &GPIO_InitStructure);

    //Configure USARTx Rx as alternate function push-pull
    GPIO_InitStructure.Pin            = USARTy_RxPin;
    GPIO_InitStructure.GPIO_Alternate = USARTy_Rx_GPIO_AF;//alternate function
    GPIO_InitPeripheral(USARTy_GPIO, &GPIO_InitStructure);

	USART_NVIC_Configuration();
	
	 //USARTy and USARTz configuration ------------------------------------------------------
    USART_InitStructure.BaudRate            = USART_BAUD_RATE;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

    //Configure USART1
    USART_Init(USARTy, &USART_InitStructure);

    //Enable USARTy Receive and Transmit interrupts
    USART_ConfigInt(USARTy, USART_INT_RXDNE, ENABLE);
    USART_ConfigInt(USARTy, USART_INT_TXDE, ENABLE);
	 USART_ConfigInt(USARTy, USART_INT_ERRF, ENABLE);
	//USART_ConfigInt(USARTy, USART_INT_IDLEF, ENABLE);

    //Enable the USART1
    USART_Enable(USARTy, ENABLE);
	
	UART1_STRUCT.R_InPtr = 0;
	UART1_STRUCT.R_Flag = 0;
    UART1_STRUCT.R_Detransferred_Ptr=0;
	UART1_STRUCT.R_DTSime = 125;
	UART1_STRUCT.T_Length = 0;
	UART1_STRUCT.T_OutPtr = 0;
    UART1_STRUCT.T_Transferred_OutPtr=0;
	UART1_STRUCT.T_Flag = 0;
	UART1_STRUCT.T_DTSime = 0;
	UART1_STRUCT.T_Time = 0;
	
	//RS485_L();//receive mode
	//RS485_Delay = 0;
}

/**
 * @brief  Configures the nested vectored interrupt controller.
 */
static void USART_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    // Enable the USARTy Interrupt
    NVIC_InitStructure.NVIC_IRQChannel                   = USARTy_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority           = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


/**
*@name: Analyze_Receive_Data
*@description: check the received frame
*@params: none
*@return: error code
*/
//uint16_t crc16_t=0;

static uint8_t Analyze_Receive_Data(void)
{
	uint16_t crc16=0;
	int8_t load_data_len=0;
	uint8_t len=0;
	uint8_t ttl_len=0;
	uint8_t *pos=__NULL;
	uint16_t i=0;	
	object_num=0;
	//ttl_len=0;load_data_len=0;	
	
	while(i<UART1_STRUCT.R_InPtr)//transfer process
	{
		if(UART1_STRUCT.R_Buffer[i]!=0x5E)
		{
			UART1_STRUCT.R_Detransferred[UART1_STRUCT.R_Detransferred_Ptr++]=UART1_STRUCT.R_Buffer[i];
			i++;
		}
		else//If the current byte is 0x5E, we then need to check the next byte. If the frame is valid, the next byte can't be any other value except 0x5D or 0x7D
		{
			if(UART1_STRUCT.R_Buffer[i+1]==0x5D)//5E and 5D stand for 5E
			{
				UART1_STRUCT.R_Detransferred[UART1_STRUCT.R_Detransferred_Ptr++]=0x5E;
			}
			else if(UART1_STRUCT.R_Buffer[i+1]==0x7D)//5E and 7D stand for 7E
			{
				UART1_STRUCT.R_Detransferred[UART1_STRUCT.R_Detransferred_Ptr++]=0x7E;
			}
			else
			{
				return (DATA_ERROR);//any other value but 0x5D or 0x7D
			}
			i+=2;
		}
	}
	
	//check the ID address
	if((UART1_STRUCT.R_Detransferred[3] != ID_Address) || (UART1_STRUCT.R_Detransferred[4] != MODULE_TYPE))
	{
		return(NO_BASE_ID);
	}
	
	//check command
	if((UART1_STRUCT.R_Detransferred[6]!=CMD_QUERY) && (UART1_STRUCT.R_Detransferred[6]!=CMD_CONFIGURE) &&(UART1_STRUCT.R_Detransferred[6]!=CMD_SYS_RESET))
	{
		return (CMD_ERROR);
	}
    
    //load_data_len=UART1_STRUCT.R_InPtr-DATA_EXCLUDED_LEN;
    //if(load_data_len<=0)
    //{
        //return(DATA_ERROR);
    //}

	//check CRC
	pos=UART1_STRUCT.R_Detransferred+1;//starting byte, check sum bytes and ending byte are all excluded
	crc16=CRC16_CCITT(pos,UART1_STRUCT.R_Detransferred_Ptr-4);
    //crc16=CRC16(pos,UART1_STRUCT.R_InPtr-4);
	if(crc16 != (UART1_STRUCT.R_Detransferred[UART1_STRUCT.R_Detransferred_Ptr - 2]<<8 | UART1_STRUCT.R_Detransferred[UART1_STRUCT.R_Detransferred_Ptr - 3]))
	{
		return(CRC_ERROR);
	}
	
	load_data_len=UART1_STRUCT.R_Detransferred_Ptr-DATA_EXCLUDED_LEN;
	pos=UART1_STRUCT.R_Detransferred+8;//locate the first object	
	while(load_data_len>0)
	{
		len=*pos;
		//if(len<4 || len>5)
        if(len!=4 && len!=5 && len!=23)//the fixed data length
		{
			//object_num=0;
			return (DATA_ERROR);
		}
		object_num++;
		load_data_len-=len;
		pos+=len;
		ttl_len+=len;
	}
	
	if(load_data_len!=0)
	{
		return (DATA_ERROR);
	}
	
	load_data_len=UART1_STRUCT.R_Detransferred_Ptr-DATA_EXCLUDED_LEN;
	if(load_data_len!=ttl_len)
	{
		return(DATA_ERROR);
	}
	/*
	if((UART1_STRUCT.R_Buffer[0] != SOI) || (UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr - 1] != EOI))
	{
		return(CONFINED_EXECUTED);
	}
	*/
		
	return(NORMAL);
}


/**
*@name: Get_OCP_Status
*@description: get the OCP status of the designated channel
*@params: channel: channel
*@return: 1: OCP, 0:OK
*/
uint8_t Get_OCP_Status(uint8_t channel)
{
	uint8_t result=0;
	
	switch(channel)
	{
		case 0:
			//result=OCP1;
			result=AlarmStatus.alarm.SHORT1;
			break;
		case 1:
			//result=OCP2;
			result=AlarmStatus.alarm.SHORT2;
			break;
		case 2:
			//result=OCP3;
			result=AlarmStatus.alarm.SHORT3;
			break;
		case 3:
			//result=OCP4;
			result=AlarmStatus.alarm.SHORT4;
			break;
		case 4:
			//result=OCP5;
			result=AlarmStatus.alarm.SHORT5;
			break;
		case 5:
			//result=OCP6;
			result=AlarmStatus.alarm.SHORT6;
			break;
		case 6:
			//result=OCP7;
			result=AlarmStatus.alarm.SHORT7;
			break;
		case 7:
			//result=OCP8;
			result=AlarmStatus.alarm.SHORT8;
			break;
		default:
			break;
	}
	
	return (result);
}

/**
*@name: Get_VOUT_Status
*@description: get the output status of the designated channel
*@params: channel: channel number
*@return: 0:OK，1:abnormal
*/
uint8_t Get_VOUT_Status(uint8_t channel)
{
	uint8_t result=0;
	
	switch(channel)
	{
		case 0:
			//result=V1_OK==1?0:1;
			result=AlarmStatus.alarm.OUT1_ERROR;
			break;
		case 1:
			//result=V2_OK==1?0:1;
			result=AlarmStatus.alarm.OUT2_ERROR;
			break;
		case 2:
			//result=V3_OK==1?0:1;
			result=AlarmStatus.alarm.OUT3_ERROR;
			break;
		case 3:
			//result=V4_OK==1?0:1;
			result=AlarmStatus.alarm.OUT4_ERROR;
			break;
		case 4:
			//result=V5_OK==1?0:1;
			result=AlarmStatus.alarm.OUT5_ERROR;
			break;
		case 5:
			//result=V6_OK==1?0:1;
			result=AlarmStatus.alarm.OUT6_ERROR;
			break;
		case 6:
			//result=V7_OK==1?0:1;
			result=AlarmStatus.alarm.OUT7_ERROR;
			break;
		case 7:
			//result=V8_OK==1?0:1;
			result=AlarmStatus.alarm.OUT8_ERROR;
			break;
		default:
			break;
	}
	
	return (result);
}

/**
*@name: Get_IOUT_Value
*@description: get the current value of the designated channel
*@params: channel: channel number
*@return: current value: mA
*/
//extern uint16_t is1_sample_value_filt;
uint16_t Get_IOUT_Value(uint8_t channel)
{
	uint16_t result=0;
	
	switch(channel)
	{
		case 0:			
			result=IS1_OUT;			
			break;
		case 1:
			result=IS2_OUT;
			break;
		case 2:
			result=IS3_OUT;
			break;
		case 3:
			result=IS4_OUT;
			break;
		case 4:
			result=IS5_OUT;
			break;
		case 5:
			result=IS6_OUT;
			break;
		case 6:
			result=IS7_OUT;
			break;
		case 7:
			result=IS8_OUT;
			break;
		default:
			break;
	}
	
	return (result);
}

/**
*@name: Set_IOUT_Switch
*@description: switching control
*@params: channel: channel number
		  value: 1: ON，0:OFF
*@return: IO status
*/
uint8_t Set_IOUT_Switch(uint8_t channel, uint8_t value)
{
	uint8_t result=0;
	uint8_t cnt=3;
	
	switch(channel)
	{
		case 0:
			if(value)
			{
				if((AlarmStatus.alarm.SHORT1==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS1_ON();
				}
				EnableOpen.open.PS1_OPEN=1;
			}
			else
			{				
				PS1_OFF();
				EnableOpen.open.PS1_OPEN=0;
			}
			while(cnt--);
			result=PS1_STATUS();
			break;
		case 1:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT2==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS2_ON();
				}
				EnableOpen.open.PS2_OPEN=1;
			}
			else
			{
				PS2_OFF();
				EnableOpen.open.PS2_OPEN=0;
			}
			while(cnt--);
			result=PS2_STATUS();
			break;
		case 2:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT3==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS3_ON();
				}
				EnableOpen.open.PS3_OPEN=1;
			}
			else
			{
				PS3_OFF();
				EnableOpen.open.PS3_OPEN=0;
			}
			while(cnt--);
			result=PS3_STATUS();
			break;
		case 3:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT4==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS4_ON();
				}
				EnableOpen.open.PS4_OPEN=1;
			}
			else
			{
				PS4_OFF();
				EnableOpen.open.PS4_OPEN=0;
			}
			while(cnt--);
			result=PS4_STATUS();
			break;
		case 4:
			if(value)	
			{				
				if((AlarmStatus.alarm.SHORT5==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS5_ON();
				}
				EnableOpen.open.PS5_OPEN=1;
			}
			else
			{
				PS5_OFF();
				EnableOpen.open.PS5_OPEN=0;
			}
			while(cnt--);
			result=PS5_STATUS();
			break;
		case 5:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT6==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS6_ON();
				}
				EnableOpen.open.PS6_OPEN=1;
			}
			else
			{
				PS6_OFF();
				EnableOpen.open.PS6_OPEN=0;
			}
			while(cnt--);
			result=PS6_STATUS();
			break;
		case 6:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT7==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS7_ON();
				}
				EnableOpen.open.PS7_OPEN=1;
			}
			else
			{
				PS7_OFF();
				EnableOpen.open.PS7_OPEN=0;
			}
			while(cnt--);
			result=PS7_STATUS();
			break;
		case 7:
			if(value)
			{				
				if((AlarmStatus.alarm.SHORT8==0) 
					&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
					&& (AlarmStatus.alarm.OTP==0))
				{
					PS8_ON();
				}
				EnableOpen.open.PS8_OPEN=1;
			}
			else
			{
				PS8_OFF();
				EnableOpen.open.PS8_OPEN=0;
			}
			while(cnt--);
			result=PS8_STATUS();
			break;
		default:
			break;
	}//end switch
	
	if(result)
	{
		result=0;
	}
	else
	{
		result=1;
	}
	
	return (result);
}

/**
*@name: Get_IOUT_Switch
*@description: get the switching status
*@params: channel: channel number		  
*@return: 1:ON, 0:OFF
*/
uint8_t Get_IOUT_Switch(uint8_t channel)
{
	uint8_t result=0;
	
	switch(channel)
	{
		case 0:			
			//result=PS1_STATUS();
			result=AlarmStatus.alarm.PSON_1;
			break;
		
		case 1:			
			//result=PS2_STATUS();
			result=AlarmStatus.alarm.PSON_2;
			break;
		
		case 2:			
			//result=PS3_STATUS();
			result=AlarmStatus.alarm.PSON_3;
			break;
		
		case 3:			
			//result=PS4_STATUS();
			result=AlarmStatus.alarm.PSON_4;
			break;
		
		case 4:			
			//result=PS5_STATUS();
			result=AlarmStatus.alarm.PSON_5;
			break;
		
		case 5:			
			//result=PS6_STATUS();
			result=AlarmStatus.alarm.PSON_6;
			break;
		
		case 6:			
			//result=PS7_STATUS();
			result=AlarmStatus.alarm.PSON_7;
			break;
		
		case 7:			
			//result=PS8_STATUS();
			result=AlarmStatus.alarm.PSON_8;
			break;
		
		default:
			break;
	}
	
	return (result);
}


/**
*@name: UART1_Comm_Data_Complete
*@description: send out response to the upper machine if the frame received is valid
*@params: command: command
*@return: none
*/
static void UART1_Comm_Data_Complete(uint8_t command)
{
	uint8_t	cTemp = 10;
	uint8_t i=0;
	uint8_t *pos=__NULL;
	uint8_t tlv_len=0;//TLV=1+2+(1 or 2)
	uint8_t data_len=data_len;
	uint16_t id=0;
	uint16_t iout=0;
	uint16_t crc16=0;
	
	UART1_STRUCT.T_OutPtr = 0;
	UART1_STRUCT.T_Transferred_OutPtr = 0;
	
	//UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++] = SOI;
	for(i=0;i<UART1_STRUCT.R_Detransferred_Ptr;i++)
	{
		UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr++]=UART1_STRUCT.R_Detransferred[i];
	}	
	UART1_STRUCT.T_Transferred[7]=NORMAL;
	pos=UART1_STRUCT.T_Transferred+8;//point to the first object
	if(object_num>0)
	{
		while(object_num--)
		{
			tlv_len=*pos;
			data_len=tlv_len-2-1;//object data length, please refer to the TLV
			/*if(data_len>2){break;}*/
			id=*(pos+2)<<8|*(pos+1);//object id
			switch(id)
			{
				case TLV_CHL_OCP+0://OCP
				case TLV_CHL_OCP+1:
				case TLV_CHL_OCP+2:
				case TLV_CHL_OCP+3:
				case TLV_CHL_OCP+4:
				case TLV_CHL_OCP+5:
				case TLV_CHL_OCP+6:
				case TLV_CHL_OCP+7:
					id-=TLV_CHL_OCP;
					*(pos+3)=Get_OCP_Status(id);
					break;
				
				case TLV_CHL_OUTPUT_ERR+0://Output Error
				case TLV_CHL_OUTPUT_ERR+1:
				case TLV_CHL_OUTPUT_ERR+2:
				case TLV_CHL_OUTPUT_ERR+3:
				case TLV_CHL_OUTPUT_ERR+4:
				case TLV_CHL_OUTPUT_ERR+5:
				case TLV_CHL_OUTPUT_ERR+6:
				case TLV_CHL_OUTPUT_ERR+7:
					id-=TLV_CHL_OUTPUT_ERR;
					*(pos+3)=Get_VOUT_Status(id);
					break;
				
				case TLV_CHL_CURRENT+0://Current
				case TLV_CHL_CURRENT+1:
				case TLV_CHL_CURRENT+2:
				case TLV_CHL_CURRENT+3:
				case TLV_CHL_CURRENT+4:
				case TLV_CHL_CURRENT+5:
				case TLV_CHL_CURRENT+6:
				case TLV_CHL_CURRENT+7:
					id-=TLV_CHL_CURRENT;
					iout=Get_IOUT_Value(id);
					*(pos+3)=iout%256;
					*(pos+4)=iout>>8;
					break;
				
				case TLV_CHL_ON_OFF+0://ON/OFF Control
				case TLV_CHL_ON_OFF+1:
				case TLV_CHL_ON_OFF+2:
				case TLV_CHL_ON_OFF+3:
				case TLV_CHL_ON_OFF+4:
				case TLV_CHL_ON_OFF+5:
				case TLV_CHL_ON_OFF+6:
				case TLV_CHL_ON_OFF+7:
					id-=TLV_CHL_ON_OFF;
					*(pos+3)=Set_IOUT_Switch(id, *(pos+3));
					break;
				
				case 0x0C00://Input voltage
					id-=0x0C00;					
					*(pos+3)=VOS_OUT%256;
					*(pos+4)=VOS_OUT>>8;
					break;
				
				case 0x0D00://Pin status
					id-=0x0D00;
					*(pos+3)=Get_IOUT_Switch(id);
					break;
				
				case TLV_MODULE_TYPE://Module type
					for(i=0;i<strlen(module_type);i++)
					{
						*((pos+3)+i)=module_type[i];
					}
					break;
					
				case TLV_SW_VERSION://Software version
					for(i=0;i<strlen(software_version);i++)
					{
						*((pos+3)+i)=software_version[i];
					}
					break;
					
				default:
					//TODO
					break;
			}			
			
			/*
			tlv_len=*pos;
			data_len=tlv_len-2-1;//object data length, please refer to the TLV
			//if(data_len>2)
			//{
				//break;
			//}
			id=*(pos+2)<<8|*(pos+1);//object id
			if(id>=TLV_CHL_OCP && id<=TLV_CHL_OCP+7)//OCP warning
			{
				id-=TLV_CHL_OCP;
				*(pos+3)=Get_OCP_Status(id);
			}
			else if(id>=TLV_CHL_OUTPUT_ERR && id<=TLV_CHL_OUTPUT_ERR+7)//output warning
			{
				id-=TLV_CHL_OUTPUT_ERR;
				*(pos+3)=Get_VOUT_Status(id);
			}
			else if(id>=TLV_CHL_CURRENT && id<=TLV_CHL_CURRENT+7)//current
			{
				id-=TLV_CHL_CURRENT;
				iout=Get_IOUT_Value(id);
				*(pos+3)=iout%256;
				*(pos+4)=iout>>8;
			}			
			else if(id>=TLV_CHL_ON_OFF && id<=TLV_CHL_ON_OFF+7)//switching control
			{
				id-=TLV_CHL_ON_OFF;
				*(pos+3)=Set_IOUT_Switch(id, *(pos+3));				
			}
			else if(id>=0x0C00&&id<=0x0C07)//input voltage, test purpose
			{
				id-=0x0C00;
				//iout=Get_IOUT_Value(id);
				*(pos+3)=VOS_OUT%256;
				*(pos+4)=VOS_OUT>>8;
			}
			else if(id>=0x0D00&&id<=0x0D07)//switching status
			{
				id-=0x0D00;
				*(pos+3)=Get_IOUT_Switch(id);
			}
			else
			{
				if(id==TLV_MODULE_TYPE)
				{
					for(i=0;i<strlen(module_type);i++)
					{
						*((pos+3)+i)=module_type[i];
					}
				}
				else if(id==TLV_SW_VERSION)
				{
					for(i=0;i<strlen(software_version);i++)
					{
						*((pos+3)+i)=software_version[i];
					}
				}
				else
				{
					//TODO
				}
			}*/
			
			pos+=tlv_len;//next object
		}
	}	
	
	//CRC information
	crc16=CRC16_CCITT(UART1_STRUCT.T_Transferred+1,UART1_STRUCT.T_Transferred_OutPtr -4);
    //crc16=CRC16(UART1_STRUCT.T_Buffer+1,UART1_STRUCT.T_OutPtr -4);
	UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr-3]=crc16%256;
	UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr-2]=crc16>>8;	
	
	i = 0;
	UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
	for(i=1;i<UART1_STRUCT.T_Transferred_OutPtr;i++)
	{
		if(UART1_STRUCT.T_Transferred[i]!=0x7E && UART1_STRUCT.T_Transferred[i]!=0x5E)
		{
			UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
		}
		else
		{
			if(UART1_STRUCT.T_Transferred[i]==0x5E)
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5E;
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5D;
			}
			else if((UART1_STRUCT.T_Transferred[i]==0x7E) && (i!=UART1_STRUCT.T_Transferred_OutPtr-1))
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5E;
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x7D;
			}
			else
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
			}
		}				
	}
	
	//RS485_H();
	//RS485_Delay = 3;
	while(cTemp-- != 0);
	UART1_STRUCT.T_Length = (UART1_STRUCT.T_OutPtr-1);
	UART1_STRUCT.T_OutPtr = 0;
	USART_SendData(USARTy, UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]);
	UART1_STRUCT.T_DTSime = 3;
	USART_ConfigInt(USARTy, USART_INT_TXDE, ENABLE);
	
	/*
	//UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++] = EOI;
	RS485_H();
	RS485_Delay = 3;
	while(cTemp-- != 0);
	UART1_STRUCT.T_Length = (UART1_STRUCT.T_OutPtr);
	UART1_STRUCT.T_OutPtr = 0;
	//TXREG2 = UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++];
	USART_SendData(USARTy, UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]);
	UART1_STRUCT.T_DTSime = 3;
	//TX2IE = 1;
	USART_ConfigInt(USARTy, USART_INT_TXDE, ENABLE);
	*/
}

/**
*@name: UART1_Send_Error
*@description: send out error message when the frame is invalid
*@params: data: package
		  len: data length
		  code: error code
*@return: none
*/
static void UART1_Send_Error(uint8_t *data, uint8_t len, uint8_t code)
{
	uint8_t	cTemp = 10;
	uint8_t i=0;
	uint16_t crc16=0;
	
	UART1_STRUCT.T_OutPtr = 0;
	UART1_STRUCT.T_Transferred_OutPtr=0;
	//UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++] = SOI;	
	//UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++] = EOI;
	for(i=0;i<len;i++)
	{
		UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr++]=data[i];
	}	
	UART1_STRUCT.T_Transferred[7]=code;
	crc16=CRC16_CCITT(data+1,UART1_STRUCT.T_Transferred_OutPtr -4);
    //crc16=CRC16(data+1,UART1_STRUCT.T_OutPtr -4);
	UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr-3]=crc16%256;
	UART1_STRUCT.T_Transferred[UART1_STRUCT.T_Transferred_OutPtr-2]=crc16>>8;
	
	i = 0;
	UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
	for(i=1;i<UART1_STRUCT.T_Transferred_OutPtr;i++)
	{
		if(UART1_STRUCT.T_Transferred[i]!=0x7E && UART1_STRUCT.T_Transferred[i]!=0x5E)
		{
			UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
		}
		else
		{
			if(UART1_STRUCT.T_Transferred[i]==0x5E)//needs to be transferred to 0x5E and 0x5D
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5E;
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5D;
			}
			else if((UART1_STRUCT.T_Transferred[i]==0x7E) && (i!=UART1_STRUCT.T_Transferred_OutPtr-1))//needs to be transferred to 0x5E and 0x7D
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x5E;
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=0x7D;
			}
			else//the last byte, that is, the end flag, 0x7E
			{
				UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]=UART1_STRUCT.T_Transferred[i];
			}
		}				
	}
	
	//RS485_H();
	//RS485_Delay = 3;
	while(cTemp-- != 0);//delay some time when switching RS485 mode
	UART1_STRUCT.T_Length = (UART1_STRUCT.T_OutPtr-1);
	UART1_STRUCT.T_OutPtr = 0;	
	USART_SendData(USARTy, UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]);
	UART1_STRUCT.T_DTSime = 3;
	USART_ConfigInt(USARTy, USART_INT_TXDE, ENABLE);
	
	/*
	RS485_H();
	RS485_Delay = 3;
	while(cTemp-- != 0);
	UART1_STRUCT.T_Length = (UART1_STRUCT.T_OutPtr);
	UART1_STRUCT.T_OutPtr = 0;
	//TXREG2 = UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++];
	USART_SendData(USARTy, UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]);
	UART1_STRUCT.T_DTSime = 3;
	//TX2IE = 1;
	USART_ConfigInt(USARTy, USART_INT_TXDE, ENABLE);
	*/
}

/**
*@name: USART1_Process
*@description: UART main process called by main
*@params: none
*@return: none
*/
void USART1_Process(void)
{
	uint8_t	cTemp;
		
	if(UART1_STRUCT.R_Flag == 1)
	{
        rcv_timeout=0;
        UART1_STRUCT.R_Flag=0;
		cTemp = Analyze_Receive_Data();			
		if(cTemp != NO_BASE_ID)
		{
			if(cTemp == NORMAL)
			{
				UART1_STRUCT.R_DTSime = 125;
				UART1_Comm_Data_Complete(UART1_STRUCT.R_Buffer[6]);
			}
			else
			{
				UART1_STRUCT.R_DTSime = 125;
				UART1_Send_Error(UART1_STRUCT.R_Detransferred, UART1_STRUCT.R_Detransferred_Ptr, cTemp);
			}
		}
		
		//LED_FLASH();
		UART1_STRUCT.R_InPtr = 0;//prepare for the next frame
		//UART1_STRUCT.R_Flag = 0;
		UART1_STRUCT.R_Detransferred_Ptr=0;
		UART1_STRUCT.T_Transferred_OutPtr=0;
		//rcv_timeout=0;
        //TIM_Enable(TIM6, ENABLE);
	}
}


/**
*@name: UART1_Data_Receive
*@description: receive interrupt service routine
*@params: none
*@return: none
*/
void USART1_Data_Receive(void)
{
	uint8_t	cTemp=cTemp;//avoid the compiler warning
	
	if(UART1_STRUCT.R_Flag != 0)
	{
		cTemp = USART_ReceiveData(USARTy);
		return;
	}
	
	if(UART1_STRUCT.R_InPtr >= RECEIVE_DATA_LEN)//maximum data length
	{
  		UART1_STRUCT.R_InPtr = 0;
	}
	
	/*
	if(UART1_STRUCT.R_InPtr >= (RECEIVE_DATA_LEN-1))
	{
  		UART1_STRUCT.R_InPtr = 0;
	}*/
  	
  	UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr++] = USART_ReceiveData(USARTy);//save the byte
	rcv_timeout=0;//once we received a byte, clear the timer to prepare for the next byte
  	
   	if(UART1_STRUCT.R_Buffer[0] != SOI)
  	{
  		UART1_STRUCT.R_InPtr = 0;
  		return;
  	}
  	else if(UART1_STRUCT.R_InPtr <= 1)
  	{
  		return;
  	}
  	
    //receive a frame
  	if((UART1_STRUCT.R_InPtr >= MINUMUM_DATA_LEN) && (UART1_STRUCT.R_Buffer[UART1_STRUCT.R_InPtr - 1] == EOI))						
	{			
		UART1_STRUCT.R_Flag = 1;
	}
}


/**
*@name: UART1_Data_Send
*@description: transmit interrupt service routine
*@params: none
*@return: none
*/
void USART1_Data_Send(void)
{
	if(UART1_STRUCT.T_Length != 0)
	{
		//RS485_Delay = 3;
		UART1_STRUCT.T_Length--;
		USART_SendData(USARTy, UART1_STRUCT.T_Buffer[UART1_STRUCT.T_OutPtr++]);
		while (USART_GetFlagStatus(USARTy, USART_FLAG_TXDE) == RESET);
	}
	else//last byte
	{
		UART1_STRUCT.T_DTSime = 0;
        
        //wait for the bit-shift register to be empty
        while(USART_GetFlagStatus(USARTy, USART_FLAG_TXDE)==RESET)
            ;
		//wait for the last byte to be transmitted
		while(USART_GetFlagStatus(USARTy, USART_FLAG_TXC)==RESET)
            ;
        //RS485_L();//receive mode
		USART_ConfigInt(USARTy, USART_INT_TXDE, DISABLE);//disable the transmit interrupt
	}	
}
