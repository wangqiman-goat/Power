#include "n32g031.h"
#include "main.h"

/**
 * @param data byte array
 * @return  the decimal value of CRC16
 */
 /*
public static int getCrc16(byte[] data){
	System.out.println("\nCRC 16 calculation progress:\n");
	int current_crc_value = PRESET_VALUE;
	for (int i = 0; i < data.length; i++){
		current_crc_value ^= data[i] & 0xFF;
		for (int j = 0; j < 8; j++ ) {
			if ((current_crc_value & 1) != 0) {
				current_crc_value = (current_crc_value >>> 1) ^ POLYNOMIAL;
			}
			else{
				current_crc_value = current_crc_value >>> 1;
			}
		}
	}
	return current_crc_value & 0xFFFF;
}
*/

#define	 PRESET_VALUE  	0xFFFF
#define	 POLYNOMIAL 	0xA001


//private static final int POLYNOMIAL = /*0xA053;*//*0x0589*/0xA001;
/*    02 05 00 03 FF 00 的不同crc计算值：
CRC-16 0x127C
CRC-16 (Modbus)    0x097C 对应的多项式为 0xA001
CRC-16 (Sick)  0xE2F0
CRC-CCITT (XModem) 0xF2B8
CRC-CCITT (0xFFFF) 0xFCA8
CRC-CCITT (0x1D0F) 0xC386
CRC-CCITT (Kermit) 0xA63E
CRC-DNP    0x6E28*/


/*==================================================================================

		******校验码的计算多项式(X16 + X15 + X2 + 1)算法原理******

        1．预置1个16位的寄存器为十六进制FFFF（即全为1）；称此寄存器为CRC寄存器；
        2．把第一个8位二进制数据 （既通讯信息帧的第一个字节）与16位的CRC寄存器的低8位相异或，把结果放于CRC寄存器；
        3．把CRC寄存器的内容右移一 位（朝低位）用0填补最高位，并检查右移后的移出位；
        4．如果移出位为0：重复第3步（再次右移一位）；
		   如果移出位为1：CRC寄存器与多项式A001（1010 0000 0000 0001）进行异或；(Modbus)
        5．重复步骤3和4，直到右移8次，这样整个8位数据全部进行了处理；
        6．重复步骤2到步骤5，进行通讯信息帧下一个字节的处理；
        7．将该通讯信息帧所有字节按上述步骤计算完成后，得到的16位CRC寄存器的高、低字节进行交换；
        8．最后得到的CRC寄存器内容即为：CRC码。

		
===================================================================================*/


/**
 * @name  getCRC16
 * @descrption: 将数组对象转换为对应的CRC16(CCITT 0xA001标准)
 * @param data byte数组
 * @return  CRC16校验得到的十进制int
 */
uint16_t getCRC16(uint8_t *data, uint8_t size)
{
	int current_crc_value = PRESET_VALUE;
	uint8_t i=0;
	while(size--)
	{		
		current_crc_value ^= (*data++ & 0xFF);
		for (i = 0; i < 8; i++ ) 
		{
			if ((current_crc_value & 1) != 0) {
				current_crc_value = (current_crc_value >> 1) ^ POLYNOMIAL;
			}
			else{
				current_crc_value = current_crc_value >> 1;
			}
		}
	}
	
	return current_crc_value & 0xFFFF;//the low 16-bit we want
}

/****************************Info********************************************** 
 * Name:    InvertUint8 
 * Note: 	把字节颠倒过来，如0x12变成0x48
			0x12: 0001 0010
			0x48: 0100 1000
 *****************************************************************************/
/*
static void InvertUint8(uint8_t *dBuf,uint8_t *srcBuf)
{
	int i;
	uint8_t tmp[4]={0};
 
	for(i=0;i< 8;i++)
	{
		if(srcBuf[0]& (1 << i))
		tmp[0]|=1<<(7-i);
	}
	dBuf[0] = tmp[0];	
}

static void InvertUint16(unsigned short *dBuf,unsigned short *srcBuf)
{
	int i;
	unsigned short tmp[4]={0};
 
	for(i=0;i< 16;i++)
	{
		if(srcBuf[0]& (1 << i))
		tmp[0]|=1<<(15 - i);
	}
	dBuf[0] = tmp[0];
}
*/

/*==================================================================================

		******CRC16 CCITT x16+x12+x5+1(0x11021)的算法原理******

		1.根据CRC16的标准选择初值CRCIn的值。

		2.将数据的第一个字节与CRCIn高8位异或。

		3.判断最高位，若该位为 0 左移一位，若为 1 左移一位再与多项式Hex码异或。

		4.重复3直至8位全部移位计算结束。

		5.重复将所有输入数据操作完成以上步骤，所得16位数即16位CRC校验码。
		
===================================================================================*/

/************************************************************************************************
 @* Name:    CRC-16/CCITT        x16+x12+x5+1 
 @* Width:	16
 @* Poly:    0x11021 
 @* Init:    0xFFFF 
 @* Refin:   false，输入不用反转 
 @* Refout:  false, 输出不用反转 
 @* Xorout:  0x0000 
 @* Alias:   CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT
 @* CRC16_CCITT：多项式x16+x12+x5+1（0x1021），初始值0xFFFF，高位在前，低位在后，结果与0xFFFF异或
 ***********************************************************************************************/
uint16_t CRC16_CCITT(uint8_t *data, uint8_t datalen)
{
	//unsigned short wCRCin = 0xFFFF;
	unsigned short wCRCin = 0x0000;//0xFFFF;使用XMODEM的CRC16算法，区别是初值不同
	unsigned short wCPoly = 0x1021;//本来是0x11021，但只取16位，最高位1去掉
	uint8_t wChar = 0;
	uint8_t i=0;
	
	while (datalen--) 	
	{
		wChar = *(data++);
		//InvertUint8(&wChar,&wChar);//反转时用
		wCRCin ^= (wChar << 8);
		for(i = 0;i < 8;i++)
		{
			if(wCRCin & 0x8000)//如果最高位是1则左移1位并与多项式wCPoly相异或
				wCRCin = (wCRCin << 1) ^ wCPoly;
			else
				wCRCin = wCRCin << 1;//否则直接左移1位
		}
	}
	//InvertUint16(&wCRCin,&wCRCin);//反转时用
	return (wCRCin);
}

/**
*@name: CRC16
*@description: CRC计算，经测试：这种方法也是可以正常使用的			   
*@params: buffer: 要计算的数组
          size: 数据长度
*@return: 返回相应的16位结果
*/
uint16_t CRC16(const uint8_t *buffer, uint8_t size)
{
	uint16_t crc=0xFFFF;//initial value, can also be 0x0000
    if(__NULL!=buffer && size>0)
    {
        while(size--)
        {
            crc=(crc>>8)|(crc<<8);//高低字节交换
            crc^=*buffer++;
            crc^=((uint8_t)crc)>>4;
            crc^=crc<<12;
            crc^=(crc&0xFF)<<5;
        }
    }
    
    return crc;
}

