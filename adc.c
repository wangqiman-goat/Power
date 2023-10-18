#include "n32g031_adc.h"
#include "n32g031.h"
#include "adc.h"
#include <math.h>

#define 	CURRENT_RATE		((float)2457.0)//(4095*300/500)//rate between voltage and current, 3A corresponding to 3V

#define		SAMPLE_COUNT		64

uint8_t		sample_cnt=0; //sampled counter
uint8_t		sample_flag = 0; //sampling finished flag
uint8_t		channel=0; //channel to be sampled

uint8_t		adc_channels[ADC_CH_NUM]={IS1, IS2, IS3, IS4, IS5, IS6 ,IS7, IS8, VOS, TEMP};//all channels

uint32_t	is1_sample_sum  = 0; //accumulated AD value
uint16_t	is1_sample_value_filt = 0;//average AD values
uint16_t 	IS1_OUT;//the actual current, unit: mA

//Unless stated otherwise, the following variables are similar to that of IS1
uint32_t	is2_sample_sum  = 0;
uint16_t	is2_sample_value_filt = 0;
uint16_t	IS2_OUT;
uint32_t	is3_sample_sum  = 0;
uint16_t	is3_sample_value_filt = 0;
uint16_t	IS3_OUT;
uint32_t	is4_sample_sum  = 0;
uint16_t	is4_sample_value_filt = 0;
uint16_t	IS4_OUT;
uint32_t	is5_sample_sum  = 0;
uint16_t	is5_sample_value_filt = 0;
uint16_t	IS5_OUT;
uint32_t	is6_sample_sum  = 0;
uint16_t	is6_sample_value_filt = 0;
uint16_t	IS6_OUT;
uint32_t	is7_sample_sum  = 0;
uint16_t	is7_sample_value_filt = 0;
uint16_t	IS7_OUT;
uint32_t	is8_sample_sum  = 0;
uint16_t	is8_sample_value_filt = 0;
uint16_t	IS8_OUT;

uint32_t	vos_sample_sum  = 0;//input voltage
uint16_t	vdc_ad_value = 0;
uint16_t	VOS_OUT;
uint32_t	otp_sample_sum  = 0;//temperature
uint16_t	otp_ad_value = 0;
//signed int	TMP_OUT;
signed char	TMP_OUT;

//AD values corresponding to the the NTC temperature,(-40---125)
uint16_t TTC05104_ADArray[166]={0x5F,0x66,0x6E,0x75,0x7E,0x87,0x90,0x9A,0xA5,0xB0,
							0xBC,0xC8,0xD5,0xE3,0xF2,0x101,0x111,0x122,0x133,0x146,
							0x159,0x16D,0x182,0x198,0x1AF,0x1C7,0x1E0,0x1F9,0x214,0x22F,
							0x24C,0x26A,0x288,0x2A7,0x2C8,0x2E9,0x30C,0x32F,0x353,0x378,
							0x39E,0x3C5,0x3EC,0x414,0x43D,0x467,0x492,0x4BD,0x4E8,0x514,
							0x541,0x56E,0x59B,0x5C9,0x5F7,0x626,0x654,0x683,0x6B1,0x6E0,
							0x70F,0x73D,0x76C,0x79A,0x7C8,0x7F6,0x823,0x850,0x87D,0x8A9,
							0x8D5,0x900,0x92B,0x955,0x97F,0x9A8,0x9D0,0x9F8,0xA1F,0xA45,
							0xA6B,0xA90,0xAB4,0xAD7,0xAFA,0xB1C,0xB3D,0xB5E,0xB7D,0xB9C,
							0xBBB,0xBD8,0xBF5,0xC11,0xC2C,0xC47,0xC61,0xC7A,0xC92,0xCAA,
							0xCC2,0xCD8,0xCEE,0xD04,0xD18,0xD2C,0xD40,0xD53,0xD66,0xD77,
							0xD89,0xD9A,0xDAA,0xDBA,0xDC9,0xDD8,0xDE7,0xDF5,0xE03,0xE10,
							0xE1D,0xE29,0xE35,0xE41,0xE4C,0xE57,0xE62,0xE6C,0xE76,0xE80,
							0xE8A,0xE93,0xE9C,0xEA4,0xEAD,0xEB5,0xEBD,0xEC4,0xECC,0xED3,
							0xEDA,0xEE0,0xEE7,0xEED,0xEF3,0xEF9,0xEFF,0xF05,0xF0A,0xF0F,
							0xF15,0xF1A,0xF1E,0xF23,0xF28,0xF2C,0xF30,0xF35,0xF39,0xF3D,
							0xF40,0xF44,0xF48,0xF4B,0xF4F,0xF52};

/**
*@name: ADC_GPIO_Configuration
*@description: ADC GPIO configuration, they are: PA0-PA7 and PB0
*@params: none
*@return: none
*/
static void ADC_GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);
    // Configure PA0-PA7 as analog input-->IS1-->IS8
    GPIO_InitStructure.Pin       = GPIO_PIN_0 | GPIO_PIN_1|GPIO_PIN_2 | GPIO_PIN_3|GPIO_PIN_4 | GPIO_PIN_5|GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_ANALOG;
    GPIO_InitPeripheral(ADC_GPIO1, &GPIO_InitStructure);
	
	// Configure PB0 AND PB1 as analog input-->input voltage and OTP
    GPIO_InitStructure.Pin       = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_ANALOG;
    GPIO_InitPeripheral(ADC_GPIO2, &GPIO_InitStructure);	
}

/**
 * @brief  Configures NVIC and Vector Table base location.
 */
void ADC_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    //Configure and enable ADC interrupt
    NVIC_InitStructure.NVIC_IRQChannel                   = ADC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority           = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


/**
*@name: ADC_Initial
*@description: ADC initialization
*@params: none
*@return: none
*/
void ADC_Initial(void)
{
	ADC_InitType ADC_InitStructure;
	
	ADC_GPIO_Configuration();
	
    /* ADC configuration ------------------------------------------------------*/
    ADC_InitStructure.MultiChEn      = DISABLE;
    ADC_InitStructure.ContinueConvEn = DISABLE;
    ADC_InitStructure.ExtTrigSelect  = ADC_EXT_TRIGCONV_NONE;
    ADC_InitStructure.DatAlign       = ADC_DAT_ALIGN_R;
    ADC_InitStructure.ChsNumber      = 1;
    ADC_Init(ADC, &ADC_InitStructure);
	//ADC1 regular channel13 configuration
	//ADC_ConfigRegularChannel(ADC, ADC_CH_13_PC2, 1, ADC_SAMP_TIME_55CYCLES5);
	//Enable ADC DMA
	//ADC_EnableDMA(ADC, ENABLE);

    //Enable ADC
    ADC_Enable(ADC, ENABLE);
    
	// Check ADC Ready
    while(ADC_GetFlagStatusNew(ADC,ADC_FLAG_RDY) == RESET);
    while(ADC_GetFlagStatusNew(ADC,ADC_FLAG_PD_RDY));
	
	channel=IS1;
}


/**
*@name: ADC_GetData
*@description: get the converted AD value
*@params: ADC_Channel: channel to be converted
*@return: AD value we need
*/
uint16_t ADC_GetData(uint8_t ADC_Channel)
{
    uint16_t data;
	
    ADC_ConfigRegularChannel(ADC, ADC_Channel, 1, ADC_SAMP_TIME_56CYCLES5);
    // Start ADC Software Conversion
    ADC_EnableSoftwareStartConv(ADC,ENABLE);//start software conversion
    while(ADC_GetFlagStatus(ADC,ADC_FLAG_ENDC)==0)//wait for the conversion to be finished
	{
		
    }
    ADC_ClearFlag(ADC,ADC_FLAG_ENDC);//clear the end conversion flag
    ADC_ClearFlag(ADC,ADC_FLAG_STR);
    data = ADC_GetDat(ADC);
    
	return (data);
}

/**
*@name: Calc_IOUT
*@description: get the actual current according to the AD value(voltage), unit: mA
*@params: ad_value: AD value
*@return: the actual current value
*/
uint16_t Calc_IOUT(uint16_t ad_value)
{
	float val=0.00;	
	uint32_t l2=0;	
	
	l2 = ad_value;
	l2 *= 300;//3A:3V, multiplied by 100
	val=(float)l2;
	val /=CURRENT_RATE;
	if(l2 <= 0)
		return 0;
	else
		return (val*10);//mA
	
	//val=is1_sample_value_filt/ADCFS;		
	//IS1_OUT=val*5000;
}

/**
*@name: TTC05104_ADBinary_Search
*@description: get the index of the table NTC_ADArray according to the ADC value
			   order by ASC
*@params: ad_value: the sampled ADC value
*@return: the final AD value
*/
static uint8_t  TTC05104_ADBinary_Search(uint16_t ad_value)
{	
   int start=0; //start index
   int end = 165; //max index
   int mid = 0;  //the index we need
   while(start<=end)
   {
      mid=(start+end)/2;//get the middle index
	  if(mid==165) break;//the maximum index
      if(ad_value==TTC05104_ADArray[mid]) break;  //we get the right index  
      if((ad_value>TTC05104_ADArray[mid])&&(ad_value<TTC05104_ADArray[mid+1]))  break;
			
      if(ad_value>TTC05104_ADArray[mid])  //the index we need is in the second part
		start = mid+1; 
		
      else if(ad_value<TTC05104_ADArray[mid])
		end = mid-1;
   }
   return mid;//get the index
}

///*
//* function name: Cal_Temperature
//* description: convert the ADC value to temperature
//* params: ad_value: NTC AD value
//* return: the actual temperature multiplied by 100
//*/
//signed int Cal_Temperature(uint16_t ad_value)
//{
//	float temperature=0.00;
//	signed short search_temperature=0;
//	uint8_t temp=TTC05104_ADBinary_Search(ad_value);
//	if(temp<165)
//	{
//		search_temperature=temp-40;
//		temperature=search_temperature+(float)(TTC05104_ADArray[temp]-ad_value)/(float)(TTC05104_ADArray[temp]-TTC05104_ADArray[temp+1]);
//		//temperature=search_temperature+(float)(NTC_ADArray[temp+1]-ad_value)/(float)(NTC_ADArray[temp+1]-NTC_ADArray[temp]);
//		temperature*=100;
//	}
//	else
//	{
//		return (125*100);//maximum temperature is 105
//	}	
//	
//	return ((signed int)temperature);
//}

/*
* function name: Cal_Temperature
* description: convert the ADC value to temperature
* params: ad_value: NTC AD value
* return: the actual temperature multiplied by 100
*/
signed char Cal_Temperature(uint16_t ad_value)
{
	float temperature=0.00;
	signed short search_temperature=0;
	uint8_t temp=TTC05104_ADBinary_Search(ad_value);
	if(temp<165)
	{
		search_temperature=temp-40;
		temperature=search_temperature+(float)(TTC05104_ADArray[temp]-ad_value)/(float)(TTC05104_ADArray[temp]-TTC05104_ADArray[temp+1]);
		//temperature*=100;
	}
	else
	{
		return (125);//maximum temperature is 105
	}	
	
	return (signed int)(round(temperature));
}

/**
*@name: ADC_Process
*@description: ADC main process
*@params: none
*@return: none
*/
void ADC_Process(void)
{
	ADC_Sample();
	if(sample_flag)
	{
		IS1_OUT=CURRENT_CALCULATE(is1_sample_value_filt);//Calc_IOUT(is1_sample_value_filt);
		IS2_OUT=CURRENT_CALCULATE(is2_sample_value_filt);//Calc_IOUT(is2_sample_value_filt);
		IS3_OUT=CURRENT_CALCULATE(is3_sample_value_filt);//Calc_IOUT(is3_sample_value_filt);
		IS4_OUT=CURRENT_CALCULATE(is4_sample_value_filt);//Calc_IOUT(is4_sample_value_filt);
		IS5_OUT=CURRENT_CALCULATE(is5_sample_value_filt);//Calc_IOUT(is5_sample_value_filt);
		IS6_OUT=CURRENT_CALCULATE(is6_sample_value_filt);//Calc_IOUT(is6_sample_value_filt);
		IS7_OUT=CURRENT_CALCULATE(is7_sample_value_filt);//Calc_IOUT(is7_sample_value_filt);
		IS8_OUT=CURRENT_CALCULATE(is8_sample_value_filt);//Calc_IOUT(is8_sample_value_filt);
		VOS_OUT= vdc_ad_value;
		TMP_OUT=Cal_Temperature(otp_ad_value);
		sample_flag=0;
	}
}

/**
*@name: ADC_Sample
*@description: sample all the channels
*@params: none
*@return: none
*/
void ADC_Sample(void)
{
 	uint16_t ad_data=0;

	if(!sample_flag)
	{
        if (sample_cnt == 0) //the first value should be discarded when switching channel
        {
            sample_cnt = 1;
            return;
        }
		switch(channel)
		{
			case IS1:
			{
                ad_data=ADC_GetData(IS1);                
                is1_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is1_sample_value_filt=is1_sample_sum>>6;//get the average AD value
					is1_sample_sum = 0;
					channel = IS2;              //next channel
				}					
				break;
			}            
			case IS2:
			{
                ad_data=ADC_GetData(IS2);                
                is2_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is2_sample_value_filt=is2_sample_sum>>6;
					is2_sample_sum = 0;
					channel = IS3;
				}					
				break;
			}
			case IS3:
			{
                ad_data=ADC_GetData(IS3);                
                is3_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is3_sample_value_filt=is3_sample_sum>>6;
					is3_sample_sum = 0;
					channel = IS4;
				}					
				break;
			}
			case IS4:
			{
                ad_data=ADC_GetData(IS4);                
                is4_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is4_sample_value_filt=is4_sample_sum>>6;
					is4_sample_sum = 0;
					channel = IS5;
				}					
				break;
			}
			case IS5:
			{
                ad_data=ADC_GetData(IS5);                
                is5_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is5_sample_value_filt=is5_sample_sum>>6;
					is5_sample_sum = 0;
					channel = IS6;
				}					
				break;
			}
			case IS6:
			{
                ad_data=ADC_GetData(IS6);                
                is6_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is6_sample_value_filt=is6_sample_sum>>6;
					is6_sample_sum = 0;
					channel = IS7;
				}					
				break;
			}
			case IS7:
			{
                ad_data=ADC_GetData(IS7);                
                is7_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is7_sample_value_filt=is7_sample_sum>>6;
					is7_sample_sum = 0;
					channel = IS8;
				}					
				break;
			}
			case IS8:
			{
                ad_data=ADC_GetData(IS8);                
                is8_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					is8_sample_value_filt=is8_sample_sum>>6;
					is8_sample_sum = 0;
					channel = TEMP;
				}					
				break;
			}
			case TEMP:
			{
                ad_data=ADC_GetData(TEMP);                
                otp_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					otp_ad_value=otp_sample_sum>>6;
					otp_sample_sum = 0;
					channel = VOS;
				}					
				break;
			}
			case VOS:
			{
                ad_data=ADC_GetData(VOS);                
                vos_sample_sum+=ad_data;
				if(++sample_cnt>SAMPLE_COUNT)
				{
					sample_cnt = 0;
					vdc_ad_value=vos_sample_sum>>6;
					vos_sample_sum = 0;					
					sample_flag = 1;//sampleing finished flag
					channel = IS1;
				}					
				break;
			}
			default:
			{
				channel=IS1;
				sample_cnt = 0;
				is1_sample_sum=0;
				is2_sample_sum=0;
				is3_sample_sum=0;
				is4_sample_sum=0;
				is5_sample_sum=0;
				is6_sample_sum=0;
				is7_sample_sum=0;
				is8_sample_sum=0;
				vos_sample_sum=0;
				otp_sample_sum=0;
				break;
			}
		}
	}
}
