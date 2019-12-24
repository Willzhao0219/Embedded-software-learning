/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 : 学习串口与PC通信、AD7606数据采集
*              注意事项：
*                1. 本实验推荐使用串口软件SecureCRT查看打印信息，波特率115200，数据位8，奇偶校验位无，停止位1。
*                2. 务必将编辑器的缩进参数和TAB设置为4来阅读本文件，要不代码显示不整齐。
*
*	修改记录 :
*		版本号   日期         作者        说明
*		V1.0    2019-10-20   Will     1. CMSIS软包版本 V5.5.0
*                                         2. HAL库版本 V2.4.0
*
*********************************************************************************************************
*/	
#include "bsp.h"			/* 底层硬件驱动 */


static void AD7606_Mak(void);
static void AD7606_Disp(void);
static void SampleDataDealing(void);


static int16_t s_volt[8];
static int16_t s_dat[8];
int8_t g_DataBuf[4096];

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: c程序入口
*	形    参: 无
*	返 回 值: 错误代码(无需处理)
*********************************************************************************************************
*/
int main(void)
{
	uint8_t ucKeyCode;
	uint8_t ucRefresh = 0;
	uint8_t ucFifoMode;
	uint8_t data[3];
	const char buf1[] = "接收到串口命令1\r\n";
	const char buf2[] = "接收到串口命令2\r\n";
	const char buf3[] = "接收到串口命令3\r\n";
	const char buf4[] = "接收到串口命令4\r\n";
    char buf5[] = "Hello World!!!\r\n";
	
	bsp_Init();		/* 硬件初始化 */
	
	PrintfLogo();	/* 打印例程名称和版本等信息 */
	PrintfHelp();	/* 打印操作提示 */
	
	ucFifoMode = 0;	 	/* AD7606进入普通工作模式 */
	AD7606_StartRecord(1000);		/* 进入自动采集模式，采样频率1KHz，数据存放在全局FIFO */
	AD7606_SetOS(AD_OS_NO);		/* 无过采样 */
	AD7606_SetInputRange(0);	/* 0表示输入量程为正负5V, 1表示正负10V */
	AD7606_StartConvst();		/* 启动1次转换 */

	bsp_StartAutoTimer(0, 100);	/* 启动1个100ms的自动重装的定时器 */
	ucRefresh = 0;
	
	/* 主程序大循环 */
	while (1)
	{

		bsp_Idle();		/* 空闲时执行的函数,比如喂狗. 在bsp.c中 */

		if (ucRefresh == 1)
		{
			ucRefresh = 0;

			/* 处理数据 */
			AD7606_Mak();
										 
			/* 打印ADC采样结果 */
//			AD7606_Disp();		
		}

		if (ucFifoMode == 0)	/* AD7606 普通工作模式 */
		{
			if (bsp_CheckTimer(0))
			{
				/* 每隔500ms 进来一次. 由软件启动转换 */
				AD7606_ReadNowAdc();		/* 读取采样结果 */
				AD7606_StartConvst();		/* 启动下次转换 */

				ucRefresh = 1;	/* 刷新显示 */
			}
		}
		else
		{
			/*
				在FIFO工作模式，bsp_AD7606自动进行采集，数据存储在FIFO缓冲区。
				结果可以通过下面的函数读取:
				uint8_t AD7606_ReadFifo(uint16_t *_usReadAdc)

				你可以将数据保存到SD卡，或者保存到外部SRAM。

				本例未对FIFO中的数据进行处理，进行打印当前最新的样本值。

				如果主程序不能及时读取FIFO数据，那么 AD7606_FifoFull() 将返回真。

				8通道200K采样时，数据传输率 = 200 000 * 2 * 8 = 3.2MB/S
			*/

			if (bsp_CheckTimer(0))
			{
				ucRefresh = 1;	/* 刷新显示 */
			}
		}

		/* 按键检测由后台systick中断服务程序实现，我们只需要调用bsp_GetKey读取键值即可。这个函数不会
		等待按键按下，这样我们可以在while循环内做其他的事情 */
		ucKeyCode = bsp_GetKey();	/* 读取键值, 无键按下时返回 KEY_NONE = 0 */
		if (ucKeyCode != KEY_NONE)
		{
			/*
				对于按键弹起事件，缺省的bsp_button.c 仅检测了TAMPER、WAKEUP、USER键、摇杆OK键的弹起事件
				如果您的应用程序需要其它键（比如方向键）的弹起事件，您需要简单修改一下bsp_button.c文件
			*/
			switch (ucKeyCode)
			{
				case KEY_DOWN_K1:			/* K1键按下 切换量程 */
					if (g_tAD7606.ucRange == 0)
					{
						AD7606_SetInputRange(1);
					}
					else
					{
						AD7606_SetInputRange(0);
					}
					ucRefresh = 1;
					break;

				case KEY_DOWN_K2:			/* K2键按下 */
					ucFifoMode = 1;	 				/* AD7606进入FIFO工作模式 */
					printf("\33[%dA", (int)1);  /* 光标上移n行 */	
					printf("AD7606进入FIFO工作模式 (200KHz 8通道同步采集)...\r\n");
					AD7606_StartRecord(200000);		/* 启动200kHz采样速率 */
					break;

				case KEY_DOWN_K3:			/* K3键按下 */
					AD7606_StopRecord();	/* 停止记录 */
					ucFifoMode = 0;	 		/* AD7606进入普通工作模式 */
					printf("\33[%dA", (int)1);  /* 光标上移n行 */
					printf("AD7606进入普通工作模式(0.5s定时8通道同步采集)...\r\n");
					break;

				case JOY_DOWN_U:			/* 摇杆UP键按下 */
					if (g_tAD7606.ucOS < 6)
					{
						g_tAD7606.ucOS++;
					}
					AD7606_SetOS(g_tAD7606.ucOS);
					ucRefresh = 1;
					break;

				case JOY_DOWN_D:			/* 摇杆DOWN键按下 */
					if (g_tAD7606.ucOS > 0)
					{
						g_tAD7606.ucOS--;
					}
					AD7606_SetOS(g_tAD7606.ucOS);
					ucRefresh = 1;
					break;

				case JOY_DOWN_L:			/* 摇杆LEFT键按下 */
					break;

				case JOY_DOWN_R:			/* 摇杆RIGHT键按下 */
					break;

				case JOY_DOWN_OK:			/* 摇杆OK键按下 */
					break;

				default:
					/* 其他的键值不处理 */
					break;
			}
		}
		
	}
}



/* 将16有符号整形转换为8为有符号整形进行传输 */
void SampleDataDealing(void)
{
	uint16_t i;
	
	for(i = 0;i <2048; i++)
	{
		g_DataBuf[2*i+0] = (g_tAdcFifo.sBuf[i]&0xFF);
		g_DataBuf[2*i+1] = ((g_tAdcFifo.sBuf[i]&0xFF00)>>8);
				
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: AD7606_Mak
*	功能说明: 处理采样后的数据
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void AD7606_Mak(void)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{		
	/* 
		32767 = 5V , 这是理论值，实际可以根据5V基准的实际值进行公式矫正 
		volt[i] = ((int16_t)dat[i] * 5000) / 32767;	计算实际电压值（近似估算的），如需准确，请进行校准            
		volt[i] = dat[i] * 0.3051850947599719
	*/	
		s_dat[i] = g_tAD7606.sNowAdc[i];
		if (g_tAD7606.ucRange == 0)
		{
			s_volt[i] = (g_tAD7606.sNowAdc[i] * 5000) / 32767;
		}
		else
		{
			s_volt[i] = (g_tAD7606.sNowAdc[i] * 10000) / 32767;
		}
	}
}

void AD7606_Disp(void)
{
	int16_t i;	
	int16_t iTemp;

	/* 打印采集数据 */
	printf(" OS  =  %d \r\n", g_tAD7606.ucOS);
	
	for (i = 0; i < 8; i++)
	{                
   		iTemp = s_volt[i];	/* uV  */
		
		if (s_dat[i] < 0)
		{
			iTemp = -iTemp;
            printf(" CH%d = %6d,0x%04X (-%d.%d%d%d V) \r\n", i+1, s_dat[i], (uint16_t)s_dat[i], iTemp /1000, (iTemp%1000)/100, (iTemp%100)/10,iTemp%10);
		}
		else
		{
         	printf(" CH%d = %6d,0x%04X ( %d.%d%d%d V) \r\n", i+1, s_dat[i], (uint16_t)s_dat[i] , iTemp /1000, (iTemp%1000)/100, (iTemp%100)/10,iTemp%10);                    
		}
	}
	printf("\33[%dA", (int)9);  /* 光标上移n行 */		
}

/***************************** (END OF FILE) *********************************/
