/*
*********************************************************************************************************
*
*	模块名称 : Modbus通信从站
*	文件名称 : Modbus_slave.h
*	版    本 : V1.0
*	说    明 : 串口modbus协议的实现（实现01H, 02H, 03H, 05H, 15H, 16H功能码）
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2019-11-02  Will     第一次进行应用层、链路层modbus协议编写
*
*********************************************************************************************************
*/
#ifndef __MODBUS_SLAVE_H
#define __MODBUS_SLAVE_H

#define Addr485 1
#define Baund485 UART3_BAUD

/* 01H 读强制单线圈 */
/* 05H 写强制单线圈 */
#define REG_D01		0x0101
#define REG_D02		0x0102
#define REG_D03		0x0103
#define REG_D04		0x0104
#define REG_DXX 	REG_D04

/* 02H 读取输入状态 */
#define REG_T01		0x0201
#define REG_T02		0x0202
#define REG_T03		0x0203
#define REG_TXX		REG_T03

/* 03H 读保持寄存器 */
/* 06H 写保持寄存器 */
/* 10H 写多个保存寄存器 */
#define SLAVE_REG_P01		0x0301
#define SLAVE_REG_P02		0x0302

/* 04H 读取输入寄存器(模拟信号) */
#define REG_A01		0x0401
#define REG_AXX		REG_A01

/* RTU 应答代码 */
#define RSP_OK				0		/* 成功 */
#define RSP_ERR_CMD			0x01	/* 不支持的功能码 */
#define RSP_ERR_REG_ADDR	0x02	/* 寄存器地址错误 */
#define RSP_ERR_VALUE		0x03	/* 数据值域错误 */
#define RSP_ERR_WRITE		0x04	/* 写入失败 */

#define S_RX_BUF_SIZE  30
#define S_TX_BUF_SIZE  30

typedef struct
{
	uint8_t RxBuf[S_RX_BUF_SIZE];
	uint8_t RxCount;
	uint8_t RxStatus;
	uint8_t RxNewFlag;
	
	uint8_t RspCode;
	
	uint8_t TxBuf[S_TX_BUF_SIZE];
	uint8_t TxCount;
}MODBUS_STRUCT;

typedef struct
{
	/* 03H 06H 读写保持寄存器 */
	uint16_t P01;
	uint16_t P02;

	/* 04H 读取模拟量寄存器 */
	uint16_t A01;

	/* 01H 05H 读写单个强制线圈 */
	uint16_t D01;
	uint16_t D02;
	uint16_t D03;
	uint16_t D04;

}VAR_T;

extern MODBUS_STRUCT g_ModS;
extern VAR_T g_tVar;

void Modbus_Poll(void);

#endif
/***************************** (END OF FILE) *********************************/

