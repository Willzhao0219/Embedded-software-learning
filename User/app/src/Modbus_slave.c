/*
*********************************************************************************************************
*
*	模块名称 : Modbus通信从站
*	文件名称 : Modbus_slave.c
*	版    本 : V1.0
*	说    明 : 串口modbus协议的实现（实现01H, 02H, 03H, 05H, 15H, 16H功能码）
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2019-11-02  Will     第一次进行应用层、链路层modbus协议编写
        V1.1    2019-12-24  Will     在轮询函数最后添加 g_ModS.Rxcount = 0，解决单次接收数据后，CRC校验不准确
*
*********************************************************************************************************
*/

#include "bsp.h"

/*  */

static uint8_t g_modbus_timeout = 0;

MODBUS_STRUCT g_ModS;
VAR_T g_tVar;

/*
*********************************************************************************************************
*	函 数 名: Modbus_CatchBug
*	功能说明: 显示接收缓存内的前八个字节的数据。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_CatchBug(void)
{
	uint8_t i = 0;
	
	for(i = 0; i<8; i++)
	{
		printf("g_ModS.Rxbuf[%d] = %x\r\n", i, g_ModS.RxBuf[i]);
	}
}



    // CRC 高位字节值表
static const uint8_t s_CRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;
// CRC 低位字节值表
const uint8_t s_CRCLo[] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
	0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
	0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
	0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
	0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
	0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
	0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
	0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
	0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
	0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
	0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
	0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
	0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
	0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
	0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
	0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/*
*********************************************************************************************************
*	函 数 名: CRC16_Modbus
*	功能说明: 计算CRC。 用于Modbus协议。
*	形    参: _pBuf 数据；
*			  _ucLen 数据长度（不带CRC）
*	返 回 值: 16位整数值。 对于Modbus ，此结果高字节先传送，低字节后传送。
*********************************************************************************************************
*/
uint16_t CRC16_Modbus(uint8_t *_pBuf, uint16_t _usLen)
{
	uint8_t ucCRCHi = 0xFF; /* 高CRC字节初始化 */
	uint8_t ucCRCLo = 0xFF; /* 低CRC 字节初始化 */
	uint16_t usIndex;  /* CRC循环中的索引 */

    while (_usLen--)
    {
		usIndex = ucCRCHi ^ *_pBuf++; /* 计算CRC */
		ucCRCHi = ucCRCLo ^ s_CRCHi[usIndex];
		ucCRCLo = s_CRCLo[usIndex];
    }
    return ((uint16_t)ucCRCHi << 8 | ucCRCLo);
}



/*
*********************************************************************************************************
*	函 数 名: Modbus_SendWithCRC
*	功能说明: 发送一串数据，自动追加2字节CRC
*	形    参: _pBuf 数据；
*			  _ucLen 数据长度（不带CRC）
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen)
{
	uint16_t crc;
	uint8_t buf[S_TX_BUF_SIZE];
	
	memcpy(buf, _pBuf, _ucLen);
	crc = CRC16_Modbus(_pBuf, _ucLen);
	buf[_ucLen++] = crc >> 8;
	buf[_ucLen++] = crc;
	
	RS485_SendBuf(buf, _ucLen);
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_SendAckErr
*	功能说明: 发送错误应答
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_SendAckErr(uint8_t _ucErrCode)
{
	uint8_t txbuf[3];
	txbuf[0] = g_ModS.RxBuf[0];              /* 485地址 */
	txbuf[1] = g_ModS.RxBuf[1] | 0x80;       /* 异常的功能码，原功能码最高位置1 */
	txbuf[0] = _ucErrCode;                   /* 错误代码 */
	
	Modbus_SendWithCRC(txbuf, 3);
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_SendAckOk
*	功能说明: 发送正确应答
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_SendAckOk(void)
{
	uint8_t txbuf[6];
	uint8_t i;

	for (i = 0; i < 6; i++)
	{
		txbuf[i] = g_ModS.RxBuf[i];
	}
	
	Modbus_SendWithCRC(txbuf, 6);
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_ReadRegValue
*	功能说明: 读取保持寄存器的值，
*	形    参: reg_addr 寄存器地址
*             reg_value 存放寄存器结果
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t Modbus_ReadRegValue(uint16_t reg_addr, uint8_t *reg_value)
{
	uint16_t value;
	
	switch(reg_addr)
	{
		case SLAVE_REG_P01:
		{
			value = g_tVar.P01;
			break;
		}
		case SLAVE_REG_P02:
		{
			value = g_tVar.P02;
			break;
		}
		
		default:
		{
			return 0;        /* 参数异常返回0 */
		}
	}
	
	reg_value[0] = value >> 8;
	reg_value[1] = value;
	
	return 1;                /* 读取成功 */
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_01H
*	功能说明: 读取线圈状态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_01H(void)
{
	/*
	 举例：
		主机发送:
			11 从机地址
			01 功能码
			00 寄存器起始地址高字节
			13 寄存器起始地址低字节
			00 寄存器数量高字节
			25 寄存器数量低字节
			0E CRC校验高字节
			84 CRC校验低字节

		从机应答: 	1代表ON，0代表OFF。若返回的线圈数不为8的倍数，则在最后数据字节未尾使用0代替. BIT0对应第1个
			11 从机地址
			01 功能码
			05 返回字节数
			CD 数据1(线圈0013H-线圈001AH)
			6B 数据2(线圈001BH-线圈0022H)
			B2 数据3(线圈0023H-线圈002AH)
			0E 数据4(线圈0032H-线圈002BH)
			1B 数据5(线圈0037H-线圈0033H)
			45 CRC校验高字节
			E6 CRC校验低字节

		例子:
			01 01 10 01 00 03   29 0B	--- 查询D01开始的3个继电器状态
			01 01 10 03 00 01   09 0A   --- 查询D03继电器的状态
	*/
	uint16_t reg;
	uint16_t num;
	uint16_t i;
	uint16_t m;
	uint8_t status[10];
	

	
	g_ModS.RspCode = RSP_OK;      /* 对响应代码进行初始化 */
	
	if(g_ModS.RxCount != 8)
	{
		g_ModS.RspCode = RSP_ERR_VALUE;       /* PDU帧结构错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	reg = (((uint16_t)g_ModS.RxBuf[2] << 8) | g_ModS.RxBuf[3]);     /* 计算寄存器号 */
	num = (((uint16_t)g_ModS.RxBuf[4] << 8) | g_ModS.RxBuf[5]);     /* 计算寄存器个数 */
	
	m = (num + 7) / 8;

	if((reg >= REG_D01) && (num > 0) && (reg + num <= REG_DXX + 1))
	{
		/* 在进行程序移植时，主要是对本部分代码进行修改 */
		for (i = 0; i < m; i++)
		{
			status[i] = 0;
		}
		for (i = 0; i < num; i++)
		{
			if (bsp_IsLedOn(i + 1 + reg - REG_D01))		/* 读LED的状态，写入状态寄存器的每一位 */
			{  
				status[i / 8] |= (1 << (i % 8));
			}
		}
		/* ******************************************** */
	}
	else
	{

		g_ModS.RspCode = RSP_ERR_REG_ADDR;    /* 寄存器寻址错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	if(g_ModS.RspCode == RSP_OK)
	{
		g_ModS.TxCount = 0;
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[0];
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[1];
		g_ModS.TxBuf[g_ModS.TxCount++] = m;			/* 返回字节数 */
		
		for (i = 0; i < m; i++)
		{
			g_ModS.TxBuf[g_ModS.TxCount++] = status[i];	/* 继电器状态 */
		}
		Modbus_SendWithCRC(g_ModS.TxBuf, g_ModS.TxCount);
	}
	else
	{
		Modbus_SendAckErr(g_ModS.RspCode);				/* 告诉主机命令错误 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_02H
*	功能说明: 读取输入状态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_02H(void)
{
	/*
		主机发送:
			11 从机地址
			02 功能码
			00 寄存器地址高字节
			C4 寄存器地址低字节
			00 寄存器数量高字节
			16 寄存器数量低字节
			BA CRC校验高字节
			A9 CRC校验低字节

		从机应答:  响应各离散输入寄存器状态，分别对应数据区中的每位值，1 代表ON；0 代表OFF。
		           第一个数据字节的LSB(最低字节)为查询的寻址地址，其他输入口按顺序在该字节中由低字节
		           向高字节排列，直到填充满8位。下一个字节中的8个输入位也是从低字节到高字节排列。
		           若返回的输入位数不是8的倍数，则在最后的数据字节中的剩余位至该字节的最高位使用0填充。
			11 从机地址
			02 功能码
			03 返回字节数
			AC 数据1(00C4H-00CBH)
			DB 数据2(00CCH-00D3H)
			35 数据3(00D4H-00D9H)
			20 CRC校验高字节
			18 CRC校验低字节

		例子:
		01 02 20 01 00 08  23CC  ---- 读取T01-08的状态
		01 02 20 04 00 02  B3CA  ---- 读取T04-05的状态
		01 02 20 01 00 12  A207   ---- 读 T01-18
	*/
	uint16_t reg;
	uint16_t num;
	uint16_t i;
	uint16_t m;
	uint8_t status[10];
	
	g_ModS.RspCode = RSP_OK;      /* 对响应代码进行初始化 */
	
	if(g_ModS.RxCount != 8)
	{
		g_ModS.RspCode = RSP_ERR_VALUE;       /* PDU帧结构错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	reg = (((uint16_t)g_ModS.RxBuf[2] << 8) | g_ModS.RxBuf[3]);     /* 计算寄存器号 */
	num = (((uint16_t)g_ModS.RxBuf[4] << 8) | g_ModS.RxBuf[5]);     /* 计算寄存器个数 */
	
	m = (num + 7) / 8;
	
	if((reg >= REG_T01) && (num > 0) && (reg + num <= REG_TXX + 1))
	{
		/* 在进行程序移植时，主要是对本部分代码进行修改 */
		for (i = 0; i < m; i++)
		{
			status[i] = 0;
		}
		for (i = 0; i < num; i++)
		{
			if (bsp_GetKeyState((KEY_ID_E)(KID_K1 + reg - REG_T01 + i)))		/* 读取按键状态 */
			{  
				status[i / 8] |= (1 << (i % 8));
			}
		}
		/* ******************************************** */
	}
	else
	{
		g_ModS.RspCode = RSP_ERR_REG_ADDR;    /* 寄存器寻址错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	if(g_ModS.RspCode == RSP_OK)
	{
		g_ModS.TxCount = 0;
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[0];
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[1];
		g_ModS.TxBuf[g_ModS.TxCount++] = m;			/* 返回字节数 */
		
		for (i = 0; i < m; i++)
		{
			g_ModS.TxBuf[g_ModS.TxCount++] = status[i];	/* T01-T02状态 */
		}
		Modbus_SendWithCRC(g_ModS.TxBuf, g_ModS.TxCount);
	}
	else
	{
		Modbus_SendAckErr(g_ModS.RspCode);				/* 告诉主机命令错误 */
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_03H
*	功能说明: 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_03H(void)
{
	/*
		从机地址为11H。保持寄存器的起始地址为006BH，结束地址为006DH。该次查询总共访问3个保持寄存器。

		主机发送:
			11 从机地址
			03 功能码
			00 寄存器地址高字节
			6B 寄存器地址低字节
			00 寄存器数量高字节
			03 寄存器数量低字节
			76 CRC高字节
			87 CRC低字节

		从机应答: 	保持寄存器的长度为2个字节。对于单个保持寄存器而言，寄存器高字节数据先被传输，
					低字节数据后被传输。保持寄存器之间，低地址寄存器先被传输，高地址寄存器后被传输。
			11 从机地址
			03 功能码
			06 字节数
			00 数据1高字节(006BH)
			6B 数据1低字节(006BH)
			00 数据2高字节(006CH)
			13 数据2 低字节(006CH)
			00 数据3高字节(006DH)
			00 数据3低字节(006DH)
			38 CRC高字节
			B9 CRC低字节

		例子:
			01 03 30 06 00 01  6B0B      ---- 读 3006H, 触发电流
			01 03 4000 0010 51C6         ---- 读 4000H 倒数第1条浪涌记录 32字节
			01 03 4001 0010 0006         ---- 读 4001H 倒数第1条浪涌记录 32字节

			01 03 F000 0008 770C         ---- 读 F000H 倒数第1条告警记录 16字节
			01 03 F001 0008 26CC         ---- 读 F001H 倒数第2条告警记录 16字节

			01 03 7000 0020 5ED2         ---- 读 7000H 倒数第1条波形记录第1段 64字节
			01 03 7001 0020 0F12         ---- 读 7001H 倒数第1条波形记录第2段 64字节

			01 03 7040 0020 5F06         ---- 读 7040H 倒数第2条波形记录第1段 64字节
	*/
	uint16_t reg;
	uint16_t num;
	uint16_t i;
	uint8_t reg_value[64];
	
	g_ModS.RspCode = RSP_OK;
	
	if (g_ModS.RxCount != 8)								/* 03H命令必须是8个字节 */
	{
		g_ModS.RspCode = RSP_ERR_VALUE;					/* 数据值域错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	reg = (((uint16_t)g_ModS.RxBuf[2] << 8) | g_ModS.RxBuf[3]);     /* 计算寄存器号 */
	num = (((uint16_t)g_ModS.RxBuf[4] << 8) | g_ModS.RxBuf[5]);     /* 计算寄存器个数 */
	
	if (num > sizeof(reg_value) / 2)
	{
		g_ModS.RspCode = RSP_ERR_VALUE;					/* 数据值域错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	for (i = 0; i < num; i++)
	{
		if (Modbus_ReadRegValue(reg, &reg_value[2 * i]) == 0)	/* 读出寄存器值放入reg_value */
		{
			g_ModS.RspCode = RSP_ERR_REG_ADDR;				/* 寄存器地址错误 */
			Modbus_SendAckErr(g_ModS.RspCode);
			return;
		}
		reg++;
	}
	
	if(g_ModS.RspCode == RSP_OK)
	{
		g_ModS.TxCount = 0;
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[0];
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[1];
		g_ModS.TxBuf[g_ModS.TxCount++] = num * 2;			/* 返回字节数 */
		
		for (i = 0; i < num; i++)
		{
			g_ModS.TxBuf[g_ModS.TxCount++] = reg_value[2 * i];
			g_ModS.TxBuf[g_ModS.TxCount++] = reg_value[2 * i + 1];
		}
		Modbus_SendWithCRC(g_ModS.TxBuf, g_ModS.TxCount);  /* 发送正确应答 */
	}
	else
	{
		Modbus_SendAckErr(g_ModS.RspCode);				/* 告诉错误应答 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_04H
*	功能说明: 读取输入寄存器（对应A01/A02） SMA
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_04H(void)
{
	/*
		主机发送:
			11 从机地址
			04 功能码
			00 寄存器起始地址高字节
			08 寄存器起始地址低字节
			00 寄存器个数高字节
			02 寄存器个数低字节
			F2 CRC高字节
			99 CRC低字节

		从机应答:  输入寄存器长度为2个字节。对于单个输入寄存器而言，寄存器高字节数据先被传输，
				低字节数据后被传输。输入寄存器之间，低地址寄存器先被传输，高地址寄存器后被传输。
			11 从机地址
			04 功能码
			04 字节数
			00 数据1高字节(0008H)
			0A 数据1低字节(0008H)
			00 数据2高字节(0009H)
			0B 数据2低字节(0009H)
			8B CRC高字节
			80 CRC低字节

		例子:

			01 04 2201 0006 2BB0  --- 读 2201H A01通道模拟量 开始的6个数据
			01 04 2201 0001 6A72  --- 读 2201H

	*/
	uint16_t reg;
	uint16_t num;
	uint16_t i;
	uint16_t status[10];
	

	memset(status, 0, 10);

	g_ModS.RspCode = RSP_OK;
	
	if (g_ModS.RxCount != 8)								/* 03H命令必须是8个字节 */
	{
		g_ModS.RspCode = RSP_ERR_VALUE;					/* 数据值域错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	reg = (((uint16_t)g_ModS.RxBuf[2] << 8) | g_ModS.RxBuf[3]);    /* 计算寄存器号 */
	num = (((uint16_t)g_ModS.RxBuf[4] << 8) | g_ModS.RxBuf[5]);     /* 计算寄存器个数 */
	
	if((reg >= REG_A01) && (num > 0) && (reg + num <= REG_AXX + 100))
	{
		/* 在进行程序移植时，主要是对本部分代码进行修改 */
		for (i = 0; i < num; i++)
		{
			switch (reg)
			{
				/* 测试参数 */
				case REG_A01:
					status[i] = g_tVar.A01;
					break;
					
				default:
					status[i] = 0;
					break;
			}
			reg++;
		}
		/* ******************************************** */
	}
	else
	{
		g_ModS.RspCode = RSP_ERR_REG_ADDR;    /* 寄存器寻址错误 */
		Modbus_SendAckErr(g_ModS.RspCode);
		return;
	}
	
	if(g_ModS.RspCode == RSP_OK)
	{
		g_ModS.TxCount = 0;
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[0];
		g_ModS.TxBuf[g_ModS.TxCount++] = g_ModS.RxBuf[1];
		g_ModS.TxBuf[g_ModS.TxCount++] = num * 2;			/* 返回字节数 */
		
		for (i = 0; i < num; i++)
		{
			g_ModS.TxBuf[g_ModS.TxCount++] = status[i] >> 8;
			g_ModS.TxBuf[g_ModS.TxCount++] = status[i] & 0xFF;
		}
		Modbus_SendWithCRC(g_ModS.TxBuf, g_ModS.TxCount);
	}
	else
	{
		Modbus_SendAckErr(g_ModS.RspCode);				/* 告诉主机命令错误 */
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_Analysis
*	功能说明: 分析应用层协议
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_Analysis(void)
{
	switch(g_ModS.RxBuf[1])   /* 第二个字节 功能码 */
	{
		case 0x01:
		{
			Modbus_01H();     /* 读取线圈状态 */
			break;
		}
		case 0x02:
		{
			Modbus_02H();     /* 读取输入状态 */
			break;
		}
		case 0x03:
		{
			Modbus_03H();     /* 读取保持寄存器 */
			break;
		}
		case 0x04:
		{
			Modbus_04H();     /* 读取输入寄存器 */
			break;
		}
//		case 0x05:
//		{
//			Modbus_05H();     /* 强制单线圈 */
//			break;
//		}
//		case 0x06:
//		{
//			Modbus_06H();     /* 写单个保存寄存器 */
//			break;
//		}
//		case 0x10:
//		{
//			Modbus_10H();     /* 写多个保存寄存器 */
//			break;
//		}
		
		default:
		{
			g_ModS.RspCode = RSP_ERR_CMD;
			Modbus_SendAckErr(g_ModS.RspCode);    /* 告诉主机命令错误 */
			break;
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_Poll
*	功能说明: 解析数据包，在主程序中轮流调用。出现 g_modbus_timeout = 1 时，表示一次接收完成。
              3.5个字符的时间间隔，只是用在RTU模式下面，因为RTU模式没有开始符和结束符，两个数据包之间只能靠时间间隔来区分。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_Poll(void)
{
	uint16_t addr;
	uint16_t crc1;
	/* 超过3.5个字符时间后执行Modbus_RxTimeOut()函数，全局变量g_rtu_timeout = 1 */
	/* g_modbus_timeout = 0 时，没有完成一个完整帧的获取，返回并继续接收，等待获取完整帧再进行数据解析 */
	if(g_modbus_timeout == 0)
	{
		return;   /* 没有超时，继续接收。不要清零 g_ModS.RxCount */
	}
	Modbus_CatchBug();

	g_modbus_timeout = 0;
	
	
	if(g_ModS.RxCount < 4)
	{
		g_ModS.RxCount = 0;
		return;
	}

	/* 计算CRC校验 */
	crc1 = CRC16_Modbus(g_ModS.RxBuf, g_ModS.RxCount);
	if(crc1 != 0)
	{
		g_ModS.RxCount = 0;
		return;
	}

	/* 站地址判断 */
	addr = g_ModS.RxBuf[0];
	if(addr != Addr485)
	{
		g_ModS.RxCount = 0;
		return;/*待修改，修改为向主站返回错误信息*/
	}
	
	/* 分析应用层协议,进行数据解析 */
	Modbus_Analysis();
	g_ModS.RxCount = 0;
	
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_RxTimeOut
*	功能说明: 超过3.5个字符时间后执行本函数。 设置全局变量 g_modbus_timeout = 1; 通知主程序开始解码。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_RxTimeOut(void)
{
	g_modbus_timeout = 1;
}

/*
*********************************************************************************************************
*	函 数 名: Modbus_ReciveNew
*	功能说明: 串口接收中断服务程序会调用本函数。当收到一个字节时，执行一次本函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Modbus_ReciveNew(uint8_t _byte)
{
	/*
		3.5个字符的时间间隔，只是用在RTU模式下面，因为RTU模式没有开始符和结束符，
		两个数据包之间只能靠时间间隔来区分，Modbus定义在不同的波特率下，间隔时间是不一样的，
		所以就是3.5个字符的时间，波特率高，这个时间间隔就小，波特率低，这个时间间隔相应就大

		4800  = 7.297ms
		9600  = 3.646ms
		19200  = 1.771ms
		38400  = 0.885ms
	*/
	uint32_t timeout;
	
	g_modbus_timeout = 0;
	
	timeout = 35000000 / Baund485;    /* 计算超时时间，单位us */

	
	/* 硬件定时中断，定时精度us 硬件定时器1用于ADC, 定时器2用于Modbus */
	bsp_StartHardTimer(1, timeout, (void *)Modbus_RxTimeOut);
	
	if(g_ModS.RxCount < S_RX_BUF_SIZE)
	{
		g_ModS.RxBuf[g_ModS.RxCount++] = _byte;
	}
}



/* ************************************************************ */
