/**
 * @file main.c
 * @author Power
 * @version v1.0.1
 *
 * @copyright Copyright (c) 2022, DS.
 */
 /*
	MIPS的全称是Million Instructions Per Second，
	每秒百万指令（西方或者国际上的计量体系中1M(兆)=100万=1000000）；Mhz，是指单片机CPU的主频兆赫兹。
	单条指令执行时间：STM32F10X单片机在主频为72MHz下，C语言程序执行一条指令需要的时间可认为10ns~100ns。
	国民技术系列N32G031 MCU，以主频48MHz为例，这里估算的C语言执行一条指令的时间约为20ns-200ns
*/
 
#include "main.h"


//LED to test system clock frequency
#define		LED_ON()			GPIO_ResetBits(GPIOF, GPIO_PIN_7)
#define		LED_OFF()			GPIO_SetBits(GPIOF, GPIO_PIN_7)

//uint16_t 	led_cnt=0;          //for LED toggle
volatile uint8_t Flag_1ms;		//1 milli-second overflow flag
volatile uint8_t cDelay_80ms=0;	//80 milli-seconds overflow flag
volatile uint8_t cDelay_10ms=0; //10 milli-seconds overflow flag
uint16_t run_time=0;//to avoid the malfunction when powered-on, we need to delay some time

extern volatile	struct UART_Data_Struct	UART1_STRUCT;//UART interface, defined in the usart.c file
ALARM_STATUS AlarmStatus;
volatile PS_Open EnableOpen;
uint8_t out_err_filter_cnt=0;
uint8_t ocp_filter_cnt=0;
uint8_t adc_filter_cnt=0;
uint8_t off_filter_cnt=0;

/*===================================================================
					
					Hiccup related variables

====================================================================*/
uint8_t cBurpOnDelay1=0;
//uint8_t cDelay1On=0;
uint16_t cDelay1Burp=0;
uint8_t cBurpOn1Flag=0;
uint8_t cBurpOnDelay2=0;
uint16_t cDelay2Burp=0;
uint8_t cBurpOn2Flag=0;
uint8_t cBurpOnDelay3=0;
uint16_t cDelay3Burp=0;
uint8_t cBurpOn3Flag=0;
uint8_t cBurpOnDelay4=0;
uint16_t cDelay4Burp=0;
uint8_t cBurpOn4Flag=0;
uint8_t cBurpOnDelay5=0;
uint16_t cDelay5Burp=0;
uint8_t cBurpOn5Flag=0;
uint8_t cBurpOnDelay6=0;
uint16_t cDelay6Burp=0;
uint8_t cBurpOn6Flag=0;
uint8_t cBurpOnDelay7=0;
uint16_t cDelay7Burp=0;
uint8_t cBurpOn7Flag=0;
uint8_t cBurpOnDelay8=0;
uint16_t cDelay8Burp=0;
uint8_t cBurpOn8Flag=0;
uint8_t cBurp1_cnt = 0; //counter, if the burp lasts 3 periods, we then set the interval to 10 seconds.
uint8_t cBurp2_cnt = 0;
uint8_t cBurp3_cnt = 0;
uint8_t cBurp4_cnt = 0;
uint8_t cBurp5_cnt = 0;
uint8_t cBurp6_cnt = 0;
uint8_t cBurp7_cnt = 0;
uint8_t cBurp8_cnt = 0;

/*===================================================================
					
			Other variables, including output error, OCP

====================================================================*/
//VDC and OTP check
uint8_t vdc_ovp_off=0,vdc_ovp_recovered=0;
uint8_t vdc_uvp_off=0,vdc_uvp_recovered=0;
uint8_t otp_off=0,otp_recovered=0;

//output error check
uint8_t v1_ok_H=0,v1_ok_L=0;
uint8_t v2_ok_H=0,v2_ok_L=0;
uint8_t v3_ok_H=0,v3_ok_L=0;
uint8_t v4_ok_H=0,v4_ok_L=0;
uint8_t v5_ok_H=0,v5_ok_L=0;
uint8_t v6_ok_H=0,v6_ok_L=0;
uint8_t v7_ok_H=0,v7_ok_L=0;
uint8_t v8_ok_H=0,v8_ok_L=0;

//OCP check
uint8_t ocp1_H=0,ocp1_L=0;
uint8_t ocp2_H=0,ocp2_L=0;
uint8_t ocp3_H=0,ocp3_L=0;
uint8_t ocp4_H=0,ocp4_L=0;
uint8_t ocp5_H=0,ocp5_L=0;
uint8_t ocp6_H=0,ocp6_L=0;
uint8_t ocp7_H=0,ocp7_L=0;
uint8_t ocp8_H=0,ocp8_L=0;

//switch status, that is, the actual IO levels of OFF1-OFF8
uint8_t off1_H=0,off1_L=0;
uint8_t off2_H=0,off2_L=0;
uint8_t off3_H=0,off3_L=0;
uint8_t off4_H=0,off4_L=0;
uint8_t off5_H=0,off5_L=0;
uint8_t off6_H=0,off6_L=0;
uint8_t off7_H=0,off7_L=0;
uint8_t off8_H=0,off8_L=0;

void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);

///**
//*@name: Process_80MS
//*@description: 80 milli-seconds timeout process
//*@params: none
//*@return: none
//*/
//static void Process_80MS(void)
//{	
//	if(cDelay_80ms >= 80)
//	{
//		cDelay_80ms = 0;		
//		//asm("CLRWDT");		
//		//if(cDelay5S != 0)	cDelay5S--;
//		//if(c54v_OVPOCPDelay1S != 0)	c54v_OVPOCPDelay1S--;
//        //if(cBAT_ONDelay1S != 0)	cBAT_ONDelay1S--;			
//		if(UART1_STRUCT.R_DTSime != 0)//delay about ten seconds
//		{
//			if(--UART1_STRUCT.R_DTSime == 0)
//				USART_Configuration();
//		}
//		
//		if(UART1_STRUCT.T_DTSime != 0) //delay about 250 milli-seconds
//		{
//			if(--UART1_STRUCT.T_DTSime == 0)
//			{
//				USART_Configuration();
//			}
//		}
//	}
//}

/**
*@name: Short_Burp1
*@description: the hiccup process of the first channel
*@params: none
*@return: none
*/
static void Short_Burp1(void)
{	
	if((AlarmStatus.alarm.SHORT1==0) 
		&& (EnableOpen.open.PS1_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS1_ON();
		cDelay1Burp=0;
		cBurpOnDelay1=0;
		cBurpOn1Flag=0;
        //cBurp1_cnt=0;
	}
	else
	{
		//It is OCP or short that leads to the hiccup.
		if((AlarmStatus.alarm.SHORT1==1) 
			&& (EnableOpen.open.PS1_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp1_cnt>=3)//prepare to use 10 seconds interval for the burp
            {
                //cBurp1_cnt = 3;
                if(++cDelay1Burp>=BURP_LAST_TIME2)
                {
                    PS1_ON();//open for a short time
                    cDelay1Burp=BURP_LAST_TIME2;
                    cBurpOn1Flag=1;
                }
                else
                {
                    PS1_OFF();
                    cBurpOnDelay1=0;
                    cBurpOn1Flag=0;
                }
            }
            else
            {
                if(++cDelay1Burp>=BURP_LAST_TIME)
                {
                    PS1_ON();//open for a short time
                    cDelay1Burp=BURP_LAST_TIME;
                    cBurpOn1Flag=1;
                }
                else
                {
                    PS1_OFF();
                    cBurpOnDelay1=0;
                    cBurpOn1Flag=0;
                }
            }
		}
		else//other exceptions, then turn off the output directly
		{
			PS1_OFF();
			//cDelay1On=0;
			cDelay1Burp=0;
			cBurpOnDelay1=0;
			cBurpOn1Flag=0;
            //cBurp1_cnt=0;
		}
	}
	
	if(cBurpOn1Flag==1)
	{		
		if(++cBurpOnDelay1>=BURP_PS_ON_TIME)
		{			
            if(OCP1)//If it is still OCP, we continue the hiccup.
			{
				cDelay1Burp=0;//clear the counter
				//PS1_OFF();
			}
			else
			{
				cDelay1Burp=0;
                //cBurp1_cnt = 0;//clear the counter
				if((EnableOpen.open.PS1_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS1_ON();
				}
                AlarmStatus.alarm.SHORT1=0;
			}
			cBurpOnDelay1=0;
			cBurpOn1Flag=0;
            if(++cBurp1_cnt>2)
            {
                cBurp1_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp2
*@description: the hiccup process of the second channel			   
*@params: none
*@return: none
*/
static void Short_Burp2(void)
{	
	if((AlarmStatus.alarm.SHORT2==0) 
		&& (EnableOpen.open.PS2_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS2_ON();
		cDelay2Burp=0;
		cBurpOnDelay2=0;
		cBurpOn2Flag=0;
        //cBurp2_cnt = 0;
	}
	else
	{		
		if((AlarmStatus.alarm.SHORT2==1) 
			&& (EnableOpen.open.PS2_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp2_cnt>=3)
            {                
                if(++cDelay2Burp>=BURP_LAST_TIME2)
                {
                    PS2_ON();//open for a short time
                    cDelay2Burp=BURP_LAST_TIME2;
                    cBurpOn2Flag=1;
                }
                else
                {
                    PS2_OFF();
                    cBurpOnDelay2=0;
                    cBurpOn2Flag=0;
                }
            }
            else
            {
                if(++cDelay2Burp>=BURP_LAST_TIME)
                {
                    PS2_ON();
                    cDelay2Burp=BURP_LAST_TIME;
                    cBurpOn2Flag=1;
                }
                else
                {
                    PS2_OFF();
                    cBurpOnDelay2=0;
                    cBurpOn2Flag=0;
                }
            }
		}
		else
		{
			PS2_OFF();
			//cDelay2On=0;
			cDelay2Burp=0;
			cBurpOnDelay2=0;
			cBurpOn2Flag=0;
            //cBurp2_cnt = 0;
		}
	}
	
	if(cBurpOn2Flag==1)
	{
		//cDelay2Burp=0;
		if(++cBurpOnDelay2>=BURP_PS_ON_TIME)
		{			
            if(OCP2)
			{
				cDelay2Burp=0;
				//PS2_OFF();
			}
			else
			{
				cDelay2Burp=0;
                //cBurp2_cnt = 0;
				if((EnableOpen.open.PS2_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS2_ON();
				}
                AlarmStatus.alarm.SHORT2=0;
			}
			cBurpOnDelay2=0;
			cBurpOn2Flag=0;
            if(++cBurp2_cnt>2)
            {
                cBurp2_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp3
*@description: the hiccup process of the third channel			   
*@params: none
*@return: none
*/
static void Short_Burp3(void)
{	
	if((AlarmStatus.alarm.SHORT3==0) 
		&& (EnableOpen.open.PS3_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS3_ON();
		cDelay3Burp=0;
		cBurpOnDelay3=0;
		cBurpOn3Flag=0;
        //cBurp3_cnt = 0;
	}
	else
	{		
		if((AlarmStatus.alarm.SHORT3==1) 
			&& (EnableOpen.open.PS3_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp3_cnt>=3)
            {                
                if(++cDelay3Burp>=BURP_LAST_TIME2)
                {
                    PS3_ON();//open for a short time
                    cDelay3Burp=BURP_LAST_TIME2;
                    cBurpOn3Flag=1;
                }
                else
                {
                    PS3_OFF();
                    cBurpOnDelay3=0;
                    cBurpOn3Flag=0;
                }
            }
            else
            {
                if(++cDelay3Burp>=BURP_LAST_TIME)
                {
                    PS3_ON();
                    cDelay3Burp=BURP_LAST_TIME;
                    cBurpOn3Flag=1;
                }
                else
                {
                    PS3_OFF();
                    cBurpOnDelay3=0;
                    cBurpOn3Flag=0;
                }
            }
		}
		else
		{
			PS3_OFF();
			//cDelay3On=0;
			cDelay3Burp=0;
			cBurpOnDelay3=0;
			cBurpOn3Flag=0;
            //cBurp3_cnt = 0;
		}
	}
	
	if(cBurpOn3Flag==1)
	{		
		if(++cBurpOnDelay3>=BURP_PS_ON_TIME)
		{			
            if(OCP3)
			{
				cDelay3Burp=0;
				//PS3_OFF();
			}
			else
			{
				cDelay3Burp=0;
                //cBurp3_cnt = 0;
				if((EnableOpen.open.PS3_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS3_ON();
				}
                AlarmStatus.alarm.SHORT3=0;
			}
			cBurpOnDelay3=0;
			cBurpOn3Flag=0;
            if(++cBurp3_cnt>2)
            {
                cBurp3_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp4
*@description: the hiccup process of the fourth channel			   
*@params: none
*@return: none
*/
static void Short_Burp4(void)
{	
	if((AlarmStatus.alarm.SHORT4==0) 
		&& (EnableOpen.open.PS4_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS4_ON();
		cDelay4Burp=0;
		cBurpOnDelay4=0;
		cBurpOn4Flag=0;
        //cBurp4_cnt = 0;
	}
	else
	{		
		if((AlarmStatus.alarm.SHORT4==1) 
			&& (EnableOpen.open.PS4_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp4_cnt>=3)
            {                
                if(++cDelay4Burp>=BURP_LAST_TIME2)
                {
                    PS4_ON();//open for a short time
                    cDelay4Burp=BURP_LAST_TIME2;
                    cBurpOn4Flag=1;
                }
                else
                {
                    PS4_OFF();
                    cBurpOnDelay4=0;
                    cBurpOn4Flag=0;
                }
            }
            else
            {
                if(++cDelay4Burp>=BURP_LAST_TIME)
                {
                    PS4_ON();
                    cDelay4Burp=BURP_LAST_TIME;
                    cBurpOn4Flag=1;
                }
                else
                {
                    PS4_OFF();
                    cBurpOnDelay4=0;
                    cBurpOn4Flag=0;
                }
            }
		}
		else
		{
			PS4_OFF();
			//cDelay4On=0;
			cDelay4Burp=0;
			cBurpOnDelay4=0;
			cBurpOn4Flag=0;
            //cBurp4_cnt = 0;
		}
	}
	
	if(cBurpOn4Flag==1)
	{
		//cDelay4Burp=0;
		if(++cBurpOnDelay4>=BURP_PS_ON_TIME)
		{			
            if(OCP4)
			{
				cDelay4Burp=0;
				//PS4_OFF();
			}
			else
			{
				cDelay4Burp=0;
                //cBurp4_cnt = 0;
				if((EnableOpen.open.PS4_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS4_ON();
				}
                AlarmStatus.alarm.SHORT4=0;
			}
			cBurpOnDelay4=0;
			cBurpOn4Flag=0;
            if(++cBurp4_cnt>2)
            {
                cBurp4_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp5
*@description: the hiccup process of the fifth channel			   
*@params: none
*@return: none
*/
static void Short_Burp5(void)
{
	if((AlarmStatus.alarm.SHORT5==0) 
		&& (EnableOpen.open.PS5_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS5_ON();
		cDelay5Burp=0;
		cBurpOnDelay5=0;
		cBurpOn5Flag=0;
        //cBurp5_cnt = 0;
	}
	else
	{		
		if((AlarmStatus.alarm.SHORT5==1) 
			&& (EnableOpen.open.PS5_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp5_cnt>=3)
            {                
                if(++cDelay5Burp>=BURP_LAST_TIME2)
                {
                    PS5_ON();//open for a short time
                    cDelay5Burp=BURP_LAST_TIME2;
                    cBurpOn5Flag=1;
                }
                else
                {
                    PS5_OFF();
                    cBurpOnDelay5=0;
                    cBurpOn5Flag=0;
                }
            }
            else
            {
                if(++cDelay5Burp>=BURP_LAST_TIME)
                {
                    PS5_ON();
                    cDelay5Burp=BURP_LAST_TIME;
                    cBurpOn5Flag=1;
                }
                else
                {
                    PS5_OFF();
                    cBurpOnDelay5=0;
                    cBurpOn5Flag=0;
                }
            }
		}
		else
		{
			PS5_OFF();
			//cDelay5On=0;
			cDelay5Burp=0;
			cBurpOnDelay5=0;
			cBurpOn5Flag=0;
            //cBurp5_cnt = 0;
		}
	}
	
	if(cBurpOn5Flag==1)
	{
		//cDelay5Burp=0;
		if(++cBurpOnDelay5>=BURP_PS_ON_TIME)
		{			
            if(OCP5)
			{
				cDelay5Burp=0;
				//PS5_OFF();
			}
			else
			{
				cDelay5Burp=0;
                //cBurp5_cnt = 0;
				if((EnableOpen.open.PS5_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS5_ON();
				}
                AlarmStatus.alarm.SHORT5=0;
			}
			cBurpOnDelay5=0;
			cBurpOn5Flag=0;
            if(++cBurp5_cnt>2)
            {
                cBurp5_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp6
*@description: the hiccup process of the sixth channel			   
*@params: none
*@return: none
*/
static void Short_Burp6(void)
{
	if((AlarmStatus.alarm.SHORT6==0) 
		&& (EnableOpen.open.PS6_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS6_ON();
		cDelay6Burp=0;
		cBurpOnDelay6=0;
		cBurpOn6Flag=0;
        //cBurp6_cnt = 0;
	}
	else
	{		
		if((AlarmStatus.alarm.SHORT6==1) 
			&& (EnableOpen.open.PS6_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp6_cnt>=3)
            {                
                if(++cDelay6Burp>=BURP_LAST_TIME2)
                {
                    PS6_ON();//open for a short time
                    cDelay6Burp=BURP_LAST_TIME2;
                    cBurpOn6Flag=1;
                }
                else
                {
                    PS6_OFF();
                    cBurpOnDelay6=0;
                    cBurpOn6Flag=0;
                }
            }
            else
            {
                if(++cDelay6Burp>=BURP_LAST_TIME)
                {
                    PS6_ON();
                    cDelay6Burp=BURP_LAST_TIME;
                    cBurpOn6Flag=1;
                }
                else
                {
                    PS6_OFF();
                    cBurpOnDelay6=0;
                    cBurpOn6Flag=0;
                }
            }
		}
		else
		{
			PS6_OFF();
			//cDelay6On=0;
			cDelay6Burp=0;
			cBurpOnDelay6=0;
			cBurpOn6Flag=0;
            //cBurp6_cnt = 0;
		}
	}
	
	if(cBurpOn6Flag==1)
	{
		//cDelay6Burp=0;
		if(++cBurpOnDelay6>=BURP_PS_ON_TIME)
		{			
            if(OCP6)
			{
				cDelay6Burp=0;
				//PS6_OFF();
			}
			else
			{
				cDelay6Burp=0;
                //cBurp6_cnt = 0;
				if((EnableOpen.open.PS6_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS6_ON();
				}
                AlarmStatus.alarm.SHORT6=0;
			}
			cBurpOnDelay6=0;
			cBurpOn6Flag=0;
            if(++cBurp6_cnt>2)
            {
                cBurp6_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp7
*@description: the hiccup process of the seventh channel			   
*@params: none
*@return: none
*/
static void Short_Burp7(void)
{
	if((AlarmStatus.alarm.SHORT7==0) 
		&& (EnableOpen.open.PS7_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS7_ON();
		cDelay7Burp=0;
		cBurpOnDelay7=0;
		cBurpOn7Flag=0;
        //cBurp7_cnt = 0;
	}
	else
	{
		if((AlarmStatus.alarm.SHORT7==1) 
			&& (EnableOpen.open.PS7_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp7_cnt>=3)
            {                
                if(++cDelay7Burp>=BURP_LAST_TIME2)
                {
                    PS7_ON();//open for a short time
                    cDelay7Burp=BURP_LAST_TIME2;
                    cBurpOn7Flag=1;
                }
                else
                {
                    PS7_OFF();
                    cBurpOnDelay7=0;
                    cBurpOn7Flag=0;
                }
            }
            else
            {
                if(++cDelay7Burp>=BURP_LAST_TIME)
                {
                    PS7_ON();
                    cDelay7Burp=BURP_LAST_TIME;
                    cBurpOn7Flag=1;
                }
                else
                {
                    PS7_OFF();
                    cBurpOnDelay7=0;
                    cBurpOn7Flag=0;
                }
            }
		}
		else
		{
			PS7_OFF();
			//cDelay7On=0;
			cDelay7Burp=0;
			cBurpOnDelay7=0;
			cBurpOn7Flag=0;
            //cBurp7_cnt = 0;
		}
	}
	
	if(cBurpOn7Flag==1)
	{
		//cDelay7Burp=0;
		if(++cBurpOnDelay7>=BURP_PS_ON_TIME)
		{			
            if(OCP7)
			{
				cDelay7Burp=0;
				//PS7_OFF();
			}
			else
			{
				cDelay7Burp=0;
                //cBurp7_cnt = 0;
				if((EnableOpen.open.PS7_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS7_ON();
				}
                AlarmStatus.alarm.SHORT7=0;
			}
			cBurpOnDelay7=0;
			cBurpOn7Flag=0;
            if(++cBurp7_cnt>2)
            {
                cBurp7_cnt = 3;
            }
		}
	}
}

/**
*@name: Short_Burp8
*@description: the hiccup process of the eighth channel			   
*@params: none
*@return: none
*/
static void Short_Burp8(void)
{
	if((AlarmStatus.alarm.SHORT8==0) 
		&& (EnableOpen.open.PS8_OPEN==1) 
		&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
		&& (AlarmStatus.alarm.OTP==0))
	{
		PS8_ON();
		cDelay8Burp=0;
		cBurpOnDelay8=0;
		cBurpOn8Flag=0;
        //cBurp8_cnt = 0;
	}
	else
	{
		if((AlarmStatus.alarm.SHORT8==1) 
			&& (EnableOpen.open.PS8_OPEN==1) 
			&& (AlarmStatus.alarm.VIN_STATUS_ERR==0)
			&& (AlarmStatus.alarm.OTP==0))
		{
            if(cBurp8_cnt>=3)
            {                
                if(++cDelay8Burp>=BURP_LAST_TIME2)
                {
                    PS8_ON();//open for a short time
                    cDelay8Burp=BURP_LAST_TIME2;
                    cBurpOn8Flag=1;
                }
                else
                {
                    PS8_OFF();
                    cBurpOnDelay8=0;
                    cBurpOn8Flag=0;
                }
            }
            else
            {
                if(++cDelay8Burp>=BURP_LAST_TIME)
                {
                    PS8_ON();
                    cDelay8Burp=BURP_LAST_TIME;
                    cBurpOn8Flag=1;
                }
                else
                {
                    PS8_OFF();
                    cBurpOnDelay8=0;
                    cBurpOn8Flag=0;
                }
            }
		}
		else
		{
			PS8_OFF();
			//cDelay8On=0;
			cDelay8Burp=0;
			cBurpOnDelay8=0;
			cBurpOn8Flag=0;
            //cBurp8_cnt = 0;
		}
	}
	
	if(cBurpOn8Flag==1)
	{
		//cDelay8Burp=0;
		if(++cBurpOnDelay8>=BURP_PS_ON_TIME)
		{			
            if(OCP8)
			{
				cDelay8Burp=0;
				//PS8_OFF();
			}
			else
			{
				cDelay8Burp=0;
                //cBurp8_cnt = 0;
				if((EnableOpen.open.PS8_OPEN==1) && (AlarmStatus.alarm.VIN_STATUS_ERR==0))
				{
					//PS8_ON();
				}
                AlarmStatus.alarm.SHORT8=0;
			}
			cBurpOnDelay8=0;
			cBurpOn8Flag=0;
            if(++cBurp8_cnt>2)
            {
                cBurp8_cnt = 3;
            }
		}
	}
}

/**
*@name: OCP_Check
*@description: check the OCP status(IO check)
*@params: none
*@return: none
*/
static void OCP_Check(void)
{
    if(OCP1 == 0)//OCP
		ocp1_L++;
	else
		ocp1_H++;
	if(OCP2 == 0)
		ocp2_L++;
	else
		ocp2_H++;
	if(OCP3 == 0)
		ocp3_L++;
	else
		ocp3_H++;
	if(OCP4 == 0)
		ocp4_L++;
	else
		ocp4_H++;
	if(OCP5 == 0)
		ocp5_L++;
	else
		ocp5_H++;
	if(OCP6 == 0)
		ocp6_L++;
	else
		ocp6_H++;
	if(OCP7 == 0)
		ocp7_L++;
	else
		ocp7_H++;
	if(OCP8 == 0)
		ocp8_L++;
	else
		ocp8_H++;
	
	if(ocp_filter_cnt>=IO_FILTER_CNT)
	{
		if(ocp1_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT1==1)
        {
			//AlarmStatus.alarm.SHORT1=0;
        }
		else if(ocp1_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT1==0)
        {
			AlarmStatus.alarm.SHORT1=1;
        } 
		if(ocp2_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT2==1)
        {
			//AlarmStatus.alarm.SHORT2=0;
        }
		else if(ocp2_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT2==0)
        {
			AlarmStatus.alarm.SHORT2=1;
        }
		if(ocp3_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT3==1)
        {
			//AlarmStatus.alarm.SHORT3=0;
        }
		else if(ocp3_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT3==0)
        {
			AlarmStatus.alarm.SHORT3=1;
        }
		if(ocp4_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT4==1)
        {
			//AlarmStatus.alarm.SHORT4=0;
        }
		else if(ocp4_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT4==0)
        {
			AlarmStatus.alarm.SHORT4=1;
        }
		if(ocp5_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT5==1)
        {
			//AlarmStatus.alarm.SHORT5=0;
        }
		else if(ocp5_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT5==0)
        {
			AlarmStatus.alarm.SHORT5=1;
        } 
		if(ocp6_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT6==1)
        {
			//AlarmStatus.alarm.SHORT6=0;
        }
		else if(ocp6_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT6==0)
        {
			AlarmStatus.alarm.SHORT6=1;
        }
		if(ocp7_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT7==1)
        {
			//AlarmStatus.alarm.SHORT7=0;
        }
		else if(ocp7_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT7==0)
        {
			AlarmStatus.alarm.SHORT7=1;
        }
		if(ocp8_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT8==1)
        {
			//AlarmStatus.alarm.SHORT8=0;
        }
		else if(ocp8_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.SHORT8==0)
        {
			AlarmStatus.alarm.SHORT8=1;
        }
		ocp_filter_cnt=0;
		ocp1_H=0;
		ocp1_L=0;
		ocp2_H=0;
		ocp2_L=0;
		ocp3_H=0;
		ocp3_L=0;
		ocp4_H=0;
		ocp4_L=0;
		ocp5_H=0;
		ocp5_L=0;
		ocp6_H=0;
		ocp6_L=0;
		ocp7_H=0;
		ocp7_L=0;
		ocp8_H=0;
		ocp8_L=0;
	}
	else
	{
		ocp_filter_cnt++;
	}	
}

/**
*@name: Output_Status_Check
*@description: check the output status(IO check)
*@params: none
*@return: none
*/
static void Output_Status_Check(void)
{
    if(V1_OK == 1) v1_ok_H++;
	else v1_ok_L++;
    
	if(V2_OK == 1) v2_ok_H++;
	else v2_ok_L++;
    
	if(V3_OK == 1) v3_ok_H++;
	else v3_ok_L++;
    
	if(V4_OK == 1) v4_ok_H++;
	else v4_ok_L++;
    
	if(V5_OK == 1) v5_ok_H++;
	else v5_ok_L++;
	
	if(V6_OK == 1) v6_ok_H++;
	else v6_ok_L++;
	
	if(V7_OK == 1) v7_ok_H++;
	else v7_ok_L++;
	
	if(V8_OK == 1) v8_ok_H++;
	else v8_ok_L++;
	
	if(out_err_filter_cnt>=IO_FILTER_CNT)
	{
		if(v1_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT1_ERROR==1)
        {
			AlarmStatus.alarm.OUT1_ERROR=0;
        }
		else if(v1_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT1_ERROR==0)
        {
			AlarmStatus.alarm.OUT1_ERROR=1;
        } 
		if(v2_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT2_ERROR==1)
        {
			AlarmStatus.alarm.OUT2_ERROR=0;
        }		
		else if(v2_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT2_ERROR==0)
        {
			AlarmStatus.alarm.OUT2_ERROR=1;
        }
        if(v3_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT3_ERROR==1)
        {
			AlarmStatus.alarm.OUT3_ERROR=0;
        }
		else if(v3_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT3_ERROR==0)
        {
			AlarmStatus.alarm.OUT3_ERROR=1;
        }
		if(v4_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT4_ERROR==1)
        {
			AlarmStatus.alarm.OUT4_ERROR=0;
        }        
		else if(v4_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT4_ERROR==0)
        {
			AlarmStatus.alarm.OUT4_ERROR=1;
        }
		if(v5_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT5_ERROR==1)
        {
			AlarmStatus.alarm.OUT5_ERROR=0;
        }
		else if(v5_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT5_ERROR==0)
        {
			AlarmStatus.alarm.OUT5_ERROR=1;
        }
		if(v6_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT6_ERROR==1)
        {
			AlarmStatus.alarm.OUT6_ERROR=0;
        }		
		else if(v6_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT6_ERROR==0)
        {
			AlarmStatus.alarm.OUT6_ERROR=1;
        }
        if(v7_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT7_ERROR==1)
        {
			AlarmStatus.alarm.OUT7_ERROR=0;
        }
		else if(v7_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT7_ERROR==0)
        {
			AlarmStatus.alarm.OUT7_ERROR=1;
        }
		if(v8_ok_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT8_ERROR==1)
        {
			AlarmStatus.alarm.OUT8_ERROR=0;
        }        
		else if(v8_ok_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OUT8_ERROR==0)
        {
			AlarmStatus.alarm.OUT8_ERROR=1;
        }
		out_err_filter_cnt=0;
		v1_ok_H=0;
		v1_ok_L=0;
		v2_ok_H=0;
		v2_ok_L=0;
		v3_ok_H=0;
		v3_ok_L=0;
		v4_ok_H=0;
		v4_ok_L=0;
		v5_ok_H=0;
		v5_ok_L=0;
		v6_ok_H=0;
		v6_ok_L=0;
		v7_ok_H=0;
		v7_ok_L=0;
		v8_ok_H=0;
		v8_ok_L=0;
	}
	else
	{
		out_err_filter_cnt++;
	}	
}

/**
*@name: ON_OFF_Check
*@description: the actual output status of the GPIO
*@params: none
*@return: none
*/
static void ON_OFF_Check(void)
{
	if(PS1_STATUS())//switching status,1: OFF, 0: ON
		off1_H++;
	else
		off1_L++;	
	if(PS2_STATUS())
		off2_H++;
	else
		off2_L++;
	if(PS3_STATUS())
		off3_H++;
	else
		off3_L++;
	if(PS4_STATUS())
		off4_H++;
	else
		off4_L++;
	if(PS5_STATUS())
		off5_H++;
	else
		off5_L++;
	if(PS6_STATUS())
		off6_H++;
	else
		off6_L++;
	if(PS7_STATUS())
		off7_H++;
	else
		off7_L++;
	if(PS8_STATUS())
		off8_H++;
	else
		off8_L++;
	
	if(off_filter_cnt>=IO_FILTER_CNT)
	{
		if(off1_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_1==0)
        {
			AlarmStatus.alarm.PSON_1=1;
        }
		else if(off1_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_1==1)
        {
			AlarmStatus.alarm.PSON_1=0;
        }
		if(off2_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_2==0)
        {
			AlarmStatus.alarm.PSON_2=1;
        }
		else if(off2_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_2==1)
        {
			AlarmStatus.alarm.PSON_2=0;
        }
		if(off3_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_3==0)
        {
			AlarmStatus.alarm.PSON_3=1;
        }
		else if(off3_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_3==1)
        {
			AlarmStatus.alarm.PSON_3=0;
        }
		if(off4_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_4==0)
        {
			AlarmStatus.alarm.PSON_4=1;
        }
		else if(off4_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_4==1)
        {
			AlarmStatus.alarm.PSON_4=0;
        }
		if(off5_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_5==0)
        {
			AlarmStatus.alarm.PSON_5=1;
        }
		else if(off5_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_5==1)
        {
			AlarmStatus.alarm.PSON_5=0;
        }
		if(off6_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_6==0)
        {
			AlarmStatus.alarm.PSON_6=1;
        }
		else if(off6_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_6==1)
        {
			AlarmStatus.alarm.PSON_6=0;
        }
		if(off7_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_7==0)
        {
			AlarmStatus.alarm.PSON_7=1;
        }
		else if(off7_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_7==1)
        {
			AlarmStatus.alarm.PSON_7=0;
        }
		if(off8_L>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_8==0)
        {
			AlarmStatus.alarm.PSON_8=1;
        }
		else if(off8_H>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.PSON_8==1)
        {
			AlarmStatus.alarm.PSON_8=0;
        }
		off_filter_cnt=0;
		off1_H=0;
		off1_L=0;
		off2_H=0;
		off2_L=0;
		off3_H=0;
		off3_L=0;
		off4_H=0;
		off4_L=0;
		off5_H=0;
		off5_L=0;
		off6_H=0;
		off6_L=0;
		off7_H=0;
		off7_L=0;
		off8_H=0;
		off8_L=0;
	}
	else
	{
		off_filter_cnt++;
	}
}

/**
*@name: Input_Output_Manager
*@description: IO control according to the state flags
*@params: none
*@return: none
*/
static void Input_Output_Manager(void)
{	
	Short_Burp1();
	Short_Burp2();
	Short_Burp3();
	Short_Burp4();
	Short_Burp5();
	Short_Burp6();
	Short_Burp7();
	Short_Burp8();
}


/**
*@name: PS_OnOff
*@description: turn ON/OFF all the channels			   
*@params: flag: 1: ON，0:OFF
*@return: none
*/
void PS_OnOff(uint8_t flag)
{
	if(flag)
	{
		PS1_ON();
		PS2_ON();
		PS3_ON();
		PS4_ON();
		PS5_ON();
		PS6_ON();
		PS7_ON();
		PS8_ON();
	}
	else
	{
		PS1_OFF();
		PS2_OFF();
		PS3_OFF();
		PS4_OFF();
		PS5_OFF();
		PS6_OFF();
		PS7_OFF();
		PS8_OFF();
	}
}

/**
*@name: ADC_Status_Check
*@description: ADC filter, the input voltage and OTP check		   
*@params: none
*@return: none
*/
static void ADC_Status_Check(void)
{
	if(vdc_ad_value<=VDC_UV_OFF_AD)	//DC under/over voltage filter
		vdc_uvp_off++;
	if(vdc_ad_value>=VDC_UV_ON_AD)
		vdc_uvp_recovered++;
	if(vdc_ad_value>=VDC_OV_OFF_AD)
		vdc_ovp_off++;
	if(vdc_ad_value<=VDC_OV_ON_AD)
		vdc_ovp_recovered++;
	
	if(otp_ad_value>=OTP_OFF_AD)	//OTP
		otp_off++;
	if(otp_ad_value<=OTP_RECOVER_AD)
		otp_recovered++;
	
	if(adc_filter_cnt>=IO_FILTER_CNT)
	{
		if(vdc_uvp_off>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.VDC_UVP==0)	//VDC
			AlarmStatus.alarm.VDC_UVP=1;
		if(vdc_uvp_recovered>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.VDC_UVP==1)
			AlarmStatus.alarm.VDC_UVP=0;
		
        if(vdc_ovp_off>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.VDC_OVP==0)
			AlarmStatus.alarm.VDC_OVP=1;
		if(vdc_ovp_recovered>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.VDC_OVP==1)
			AlarmStatus.alarm.VDC_OVP=0;
		
        if(otp_off>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OTP==0)	//OTP
			AlarmStatus.alarm.OTP=1;
		if(otp_recovered>=IO_FILTER_VALID_CNT && AlarmStatus.alarm.OTP==1)
			AlarmStatus.alarm.OTP=0;
        
        if(AlarmStatus.alarm.VDC_OVP==0 && AlarmStatus.alarm.VDC_UVP==0)
        {
            AlarmStatus.alarm.VIN_STATUS_ERR=0;
        }
        else
        {
            AlarmStatus.alarm.VIN_STATUS_ERR=1;
        }
		adc_filter_cnt=0;
		vdc_uvp_off=0;
		vdc_uvp_recovered=0;
		vdc_ovp_off=0;
		vdc_ovp_recovered=0;		
		otp_off=0;
		otp_recovered=0;
	}
	else
	{
		adc_filter_cnt++;
	}
}

/**
*@name: Process_1MS
*@description: 1 milli-second timeout process
*@params: none
*@return: none
*/
static void Process_1MS(void)
{
	if(Flag_1ms == 0)
	{
		return;
	}
		
	Flag_1ms = 0;
	
    /*
	if(led_cnt%500==0)//LED Toggle Test
	{
		GPIO_WriteBit(GPIOF, GPIO_PIN_7, (Bit_OperateType)(1 - GPIO_ReadOutputDataBit(GPIOF, GPIO_PIN_7)));
	}
	led_cnt++;
	if(led_cnt>=60000)
	{
		led_cnt=0;
	}*/
	OCP_Check();
	Output_Status_Check();
    ADC_Status_Check();	
	ON_OFF_Check();
	Input_Output_Manager();
}

///**
//*@name: Process_10MS
//*@description: 10 milli-seconds timeout process used to process the OCP check
//*@params: none
//*@return: none
//*/
//static void Process_10MS(void)
//{
//	if(cDelay_10ms >= 10)
//	{
//		cDelay_10ms = 0;
//		//Input_Output_Manager();	
//	}
//}

/**
 * @brief   Main program
 *          The system clock frequency and PLL are all configured in the function SystemInit which is called in the startup_n32g032.s file.
 *			Therefore, we don't need to explicitly call the SystemInit function.
 */
int main(void)
{	
	//SystemInit();
    RCC_Configuration();
    GPIO_Configuration();
    ADC_Initial();
	#if IIC_MODE == 1	
	I2C_Master_Init();
	#else
	I2C1_Initial();
	I2C2_Initial();
	#endif
	TIM3_Configuration();
    //TIM6_Configuration();	
	EnableOpen.open.PS1_OPEN=1;//enable to be turn ON by the upper machine
	EnableOpen.open.PS2_OPEN=1;
	EnableOpen.open.PS3_OPEN=1;
	EnableOpen.open.PS4_OPEN=1;
	EnableOpen.open.PS5_OPEN=1;
	EnableOpen.open.PS6_OPEN=1;
	EnableOpen.open.PS7_OPEN=1;
	EnableOpen.open.PS8_OPEN=1;
    AlarmStatus.alarm.VIN_STATUS_ERR=1;
	while(run_time < START_UP_DELAY);
	run_time = 0;
	IWDG_Init();

#if 0
    /* Test on channel1 transfer complete flag */
    while(!DMA_GetFlagStatus(DMA_FLAG_TC1,DMA));
    /* Clear channel1 transfer complete flag */
    DMA_ClearFlag(DMA_FLAG_TC1,DMA);
    /* TIM1 counter disable */
    TIM_Enable(TIM1, DISABLE);
#endif

    while (1)
    {
		ADC_Process();
		Process_1MS();
		//Process_10MS();
        Process_I2C2();	
        IWDG_ReloadKey();
    }
	
	//return 0;
}

/**
 * @brief  Configures the different system clocks.
 */
static void RCC_Configuration(void)
{
	// PCLK1 = HCLK/4, configure the clock of APB1 and times uses the prescaled APB1
    RCC_ConfigPclk1(RCC_HCLK_DIV4);
	
    // Enable peripheral clocks ------------------------------------------------
    // Enable TIM1 clocks
    //RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, ENABLE);
    //Enable DMA clocks
    //RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);

    //Enable GPIO clocks
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB |RCC_APB2_PERIPH_GPIOC|RCC_APB2_PERIPH_GPIOF, ENABLE);
    //Enable ADC clocks
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC ,ENABLE);

    //RCC_ADCHCLK_DIV16
    ADC_ConfigClk(ADC_CTRL3_CKMOD_AHB, RCC_ADCHCLK_DIV16);

    //enable ADC1M clock
    //RCC_EnableHsi(ENABLE);
    RCC_ConfigAdc1mClk(RCC_ADC1MCLK_SRC_HSE, RCC_ADC1MCLK_DIV8);
	
	//enable USART1 clock
	RCC_EnableAPB2PeriphClk(USARTy_GPIO_CLK, ENABLE);
	RCC_EnableAPB2PeriphClk(USARTy_CLK, ENABLE);
	
	//TIM3 and TIM clock enable
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM3, ENABLE);
    //RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM6, ENABLE);
}

/**
*@name: GPIO_Configuration
*@description: GPIO configuration
*@params: none
*@return: none
*/
static void GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);
    //Configure TIM1_CH2(PA9) as alternate function push-pull
    //GPIO_InitStructure.Pin        = GPIO_PIN_9; 
    //GPIO_InitStructure.GPIO_Current = GPIO_DC_LOW;
    //GPIO_InitStructure.GPIO_Mode  = GPIO_MODE_AF_PP;
    //GPIO_InitStructure.GPIO_Alternate = GPIO_AF2_TIM1;//alternate function
    //GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
	//************************************************************************************
	//		===================OFF1-OFF8=====================
    //		PB2,3,4,5,8,9,10 and PA15 are the output switches, corresponding to OFF1-OFF8
	//************************************************************************************
    GPIO_InitStructure.Pin       = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);	
	GPIO_InitStructure.Pin       = GPIO_PIN_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
	
	//************************************************************************************
	//		=================== V1_OK - V8_OK =====================
    //		PC13,PC14,PC15-->V1_OK, V2_OK, V3_OK,
	//		PF2,PF6,PF7-->V4_OK, V7_OK, V5_OK
	//		PA13, PA14-->V8_OK, V6_OK
	//************************************************************************************
	//When IC starts up, pin PF2 is used as BOOT0 and the default mode is pull-down. Here we regard it as a GPIO.
	GPIO_InitStructure.Pin       = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_INPUT;
    GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.Pin       = GPIO_PIN_2|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_INPUT;
    GPIO_InitPeripheral(GPIOF, &GPIO_InitStructure);
	
    //LED toggle test
    //GPIO_InitStructure.Pin       = GPIO_PIN_7;
    //GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStructure.GPIO_Speed=GPIO_SPEED_HIGH;
	//GPIO_InitStructure.GPIO_Current=GPIO_DC_HIGH;
    //GPIO_InitPeripheral(GPIOF, &GPIO_InitStructure);
	
	
    
    //==========Mask PA13 and PA14 temporarily, for the two pins are the burning pin==========
	//GPIO_InitStructure.Pin       = GPIO_PIN_13|GPIO_PIN_14;
    //GPIO_InitStructure.GPIO_Mode = GPIO_MODE_INPUT;
    //GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);	
	
	
    
    //************************************************************************************
	//		===================OCP1-OCP8=====================
    //		PB11-PB15-->OCP1-OCP5	
	//		PA8, PA11, PA12-->OCP6-OCP8
	//************************************************************************************
	//PB11-PB15, OCP1-OCP5，PA8,PA11,PA12, OCP6-OCP8
	GPIO_InitStructure.Pin       = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_INPUT;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.Pin       = GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_INPUT;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
	PS_OnOff(0);
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param file pointer to the source file name
 * @param line assert_param error line source number
 */
void assert_failed(const uint8_t* expr, const uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}

#endif

/*
=============关于补码的简洁概括==============

	正数，本身就是补码。

	负数，就用它的正数，减一取反，即可得到补码。

	如，已知：＋9 的二进制是：0000 1001。

	下面求－9 补码：

	先减一：0000 1001 - 1 = 0000 1000；

	再取反：1111 0111。

	所以有：－9 补码 = 1111 0111。

	这不就完了吗！

	简不简单？　意不意外？
	
	如果把一个值赋给一个有符号类型，如果补码的最高位是1，则是负数，还原成实际的负数值的步骤是：
	1、先按位取反
	2、再加1
	例如（int表示32位有符号）：int a = -552305;(按正数552305的补码先减一再按位取反得到-552305二进制补码是：1111 1001 0010 1000 1111)
	    （short表示16位无符号）short c = (short)a;
		由于short最大只有16位，因此高4位被忽略，剩下1001 0010 1000 1111
		又由于赋值给short，所以最高位表示符号位，这里是1表示负数，则将1001 0010 1000 1111除符号位外按位取反再加1
		得到1110 1101 0111 0001，最终的结果就是除符号位外的数据：110 1101 0111 0001=-28017
=============================================*/

/**
 * @}
 */

/**
 * @}
 */

