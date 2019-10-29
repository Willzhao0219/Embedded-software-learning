/*
*********************************************************************************************************
*
*	ģ������ : AD7606���ݲɼ�ģ��
*	�ļ����� : bsp_ad7606.c
*	��    �� : V1.0
*	˵    �� : AD7606����STM32��FMC�����ϡ�
*
*			������ʹ���� TIM3 ��ΪӲ����ʱ������ʱ����ADCת��
*
*	�޸ļ�¼ :
*		�汾��  ����        ����     ˵��
*		V1.0    2019-10-28 Will  
*
*	
*
*********************************************************************************************************
*/

/*
	STM32-V6������ + AD7606ģ�飬 ���Ʋɼ���IO:
	
	PC6/TIM3_CH1/TIM8_CH1     ----> AD7606_CONVST  (������ͷ����),  ���PWM��������ΪADC�����ź�
	PE5/DCMI_D6/AD7606_BUSY   <---- AD7606_BUSY    , CPU��BUSY�жϷ�������ж�ȡ�ɼ����
	
	�������IO��STM32-V5�������ǲ�ͬ��
*/

#include "bsp.h"

/* ���ù�������IO, ����չ��74HC574�� */
#define OS0_1()		HC574_SetPin(AD7606_OS0, 1)
#define OS0_0()		HC574_SetPin(AD7606_OS0, 0)
#define OS1_1()		HC574_SetPin(AD7606_OS1, 1)
#define OS1_0()		HC574_SetPin(AD7606_OS1, 0)
#define OS2_1()		HC574_SetPin(AD7606_OS2, 1)
#define OS2_0()		HC574_SetPin(AD7606_OS2, 0)

/* ����ADת����GPIO : PC6 */
#define CONVST_1()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET)
#define CONVST_0()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET)

/* �����������̵�GPIO, ����չ��74HC574�� */
#define RANGE_1()	HC574_SetPin(AD7606_RANGE, 1)
#define RANGE_0()	HC574_SetPin(AD7606_RANGE, 0)

/* AD7606��λ����, ����չ��74HC574�� */
#define RESET_1()	HC574_SetPin(AD7606_RESET, 1)
#define RESET_0()	HC574_SetPin(AD7606_RESET, 0)

/* AD7606 FSMC���ߵ�ַ��ֻ�ܶ�������д */
#define AD7606_RESULT()	*(__IO uint16_t *)0x64003000

AD7606_VAR_T g_tAD7606;		/* ����1��ȫ�ֱ���������һЩ���� */
AD7606_FIFO_T g_tAdcFifo;	/* ����FIFO�ṹ����� */

static void AD7606_CtrlLinesConfig(void);
static void AD7606_FSMCConfig(void);

/*
*********************************************************************************************************
*	�� �� ��: bsp_InitAD7606
*	����˵��: ���������ⲿSRAM��GPIO��FSMC
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_InitAD7606(void)
{
	AD7606_CtrlLinesConfig();
	AD7606_FSMCConfig();

	AD7606_SetOS(AD_OS_NO);		/* �޹����� */
	AD7606_SetInputRange(0);	/* 0��ʾ��������Ϊ����5V, 1��ʾ����10V */

	AD7606_Reset();

	CONVST_1();					/* ����ת����GPIOƽʱ����Ϊ�� */
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_CtrlLinesConfig
*	����˵��: ����GPIO���ߣ�FMC�ܽ�����Ϊ���ù���
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
/*
	������STM32-V6��������߷�����
	PD0/FMC_D2
	PD1/FMC_D3
	PD4/FMC_NOE		---- �������źţ�OE = Output Enable �� N ��ʾ����Ч
	PD5/FMC_NWE		-XX- д�����źţ�AD7606 ֻ�ж�����д�ź�
	PD8/FMC_D13
	PD9/FMC_D14
	PD10/FMC_D15
	PD14/FMC_D0
	PD15/FMC_D1

	PE7/FMC_D4
	PE8/FMC_D5
	PE9/FMC_D6
	PE10/FMC_D7
	PE11/FMC_D8
	PE12/FMC_D9
	PE13/FMC_D10
	PE14/FMC_D11
	PE15/FMC_D12
	
	PG0/FMC_A10		--- ����ƬѡFMC_NE2һ������
	PG1/FMC_A11		--- ����ƬѡFMC_NE2һ������
	PG9/FMC_NE2		--- ��Ƭѡ��TFT, OLED �� AD7606��	
*/

/* 
	����AD7606����������IO��������չ��74HC574��
	D13 - AD7606_OS0
	D14 - AD7606_OS1
	D15 - AD7606_OS2
	D24 - AD7606_RESET
	D25 - AD7606_RAGE	
*/
static void AD7606_CtrlLinesConfig(void)
{
	GPIO_InitTypeDef gpio_init_structure;

	/* ʹ��FMCʱ�� */
	__HAL_RCC_FMC_CLK_ENABLE();
	

	/* ʹ�� GPIOʱ�� */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	

	/* ���� GPIO ��ص�IOΪ����������� */
	gpio_init_structure.Mode = GPIO_MODE_AF_PP;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio_init_structure.Alternate = GPIO_AF12_FMC;
	
	/* ����GPIOD */
	gpio_init_structure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 |
	                          GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &gpio_init_structure);
	
	/* ����GPIOE */
	gpio_init_structure.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
	                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &gpio_init_structure);
	
	/* ����GPIOG */
	gpio_init_structure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_9;
	HAL_GPIO_Init(GPIOE, &gpio_init_structure);
	
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_FSMCConfig
*	����˵��: ����FSMC���ڷ���ʱ��
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void AD7606_FSMCConfig(void)
{
	SRAM_HandleTypeDef hsram = {0};
	FMC_NORSRAM_TimingTypeDef SRAM_Timing = {0};
	
	hsram.Instance  = FMC_NORSRAM_DEVICE;
	hsram.Extended  = FMC_NORSRAM_EXTENDED_DEVICE;

	/*
		AD7606�����Ҫ��(3.3Vʱ)��RD���źŵ͵�ƽ���������21ns���ߵ�ƽ������̿��15ns��

		������������ ������������Ϊ�˺�ͬBANK��LCD������ͬ��ѡ��3-0-6-1-0-0
		3-0-5-1-0-0  : RD�߳���75ns�� �͵�ƽ����50ns.  1us���ڿɶ�ȡ8·�������ݵ��ڴ档
		1-0-1-1-0-0  : RD��75ns���͵�ƽִ��12ns���ң��½��ز��Ҳ12ns.  ���ݶ�ȡ��ȷ��
	*/
	/* FMCʹ�õ�HCLK����Ƶ168MHz��1��FMCʱ�����ھ���5.95ns */
	SRAM_Timing.AddressSetupTime       = 3;  /* 3*5.95ns=17.85ns����ַ����ʱ�䣬��Χ0 -15��FMCʱ�����ڸ��� */
	SRAM_Timing.AddressHoldTime        = 0;  /* ��ַ����ʱ�䣬����ΪģʽAʱ���ò����˲��� ��Χ1 -15��ʱ�����ڸ��� */
	SRAM_Timing.DataSetupTime          = 6;  /* 6*5.95ns=35.7ns�����ݱ���ʱ�䣬��Χ1 -255��ʱ�����ڸ��� */
	SRAM_Timing.BusTurnAroundDuration  = 1;  /* �������ò���������� */
	SRAM_Timing.CLKDivision            = 0;  /* �������ò���������� */
	SRAM_Timing.DataLatency            = 0;  /* �������ò���������� */
	SRAM_Timing.AccessMode             = FMC_ACCESS_MODE_A; /* ����ΪģʽA */

	/*
	 LCD configured as follow:
	    - Data/Address MUX = Disable
	    - Memory Type = SRAM
	    - Data Width = 16bit
	    - Write Operation = Enable
	    - Extended Mode = Enable
	    - Asynchronous Wait = Disable
	*/
	hsram.Init.NSBank             = FMC_NORSRAM_BANK2;              /* ʹ�õ�BANK2����ʹ�õ�ƬѡFMC_NE2 */
	hsram.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;   /* ��ֹ��ַ���ݸ��� */
	hsram.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;           /* �洢������SRAM */
	hsram.Init.MemoryDataWidth    = FMC_NORSRAM_MEM_BUS_WIDTH_32;	/* 32λ���߿�� */
	hsram.Init.BurstAccessMode    = FMC_BURST_ACCESS_MODE_DISABLE;  /* �ر�ͻ��ģʽ */
	hsram.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;   /* �������õȴ��źŵļ��ԣ��ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;      /* �ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;     /* ����ʹ�ܻ��߽�ֹд���� */
	hsram.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;        /* �ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.ExtendedMode       = FMC_EXTENDED_MODE_DISABLE;      /* ��ֹ��չģʽ */
	hsram.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;  /* �����첽�����ڼ䣬ʹ�ܻ��߽�ֹ�ȴ��źţ�����ѡ��ر� */
	hsram.Init.WriteBurst         = FMC_WRITE_BURST_DISABLE;        /* ��ֹдͻ�� */
	hsram.Init.ContinuousClock    = FMC_CONTINUOUS_CLOCK_SYNC_ONLY; /* ��ͬ��ģʽ����ʱ����� */
    hsram.Init.WriteFifo          = FMC_WRITE_FIFO_ENABLE;          /* ʹ��дFIFO */

	

	/* ��ʼ��SRAM������ */
	if (HAL_SRAM_Init(&hsram, &SRAM_Timing, &SRAM_Timing) != HAL_OK)
	{
		/* ��ʼ������ */
		Error_Handler(__FILE__, __LINE__);
	}
	
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_SetOS
*	����˵��: ����AD7606�����˲�����Ҳ�����ù��������ʡ�
*			 ͨ������ AD7606_OS0��OS1��OS2���ߵĵ�ƽ���״̬�������������ʡ�
*			 ����ADת��֮��AD7606�ڲ��Զ�ʵ��ʣ�������Ĳɼ���Ȼ����ƽ��ֵ�����
*
*			 ����������Խ�ߣ�ת��ʱ��Խ����
*			 �޹�����ʱ��ADת��ʱ�� 4us;
*				2��������ʱ = 8.7us;
*				4��������ʱ = 16us
*			 	64��������ʱ = 286us
*
*	��    ��: _ucOS : ����������, 0 - 6
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_SetOS(uint8_t _ucOS)
{
	g_tAD7606.ucOS = _ucOS;
	switch (_ucOS)
	{
		case AD_OS_X2:
			OS2_0();
			OS1_0();
			OS0_1();
			break;

		case AD_OS_X4:
			OS2_0();
			OS1_1();
			OS0_0();
			break;

		case AD_OS_X8:
			OS2_0();
			OS1_1();
			OS0_1();
			break;

		case AD_OS_X16:
			OS2_1();
			OS1_0();
			OS0_0();
			break;

		case AD_OS_X32:
			OS2_1();
			OS1_0();
			OS0_1();
			break;

		case AD_OS_X64:
			OS2_1();
			OS1_1();
			OS0_0();
			break;

		case AD_OS_NO:
		default:
			g_tAD7606.ucOS = AD_OS_NO;
			OS2_0();
			OS1_0();
			OS0_0();
			break;
	}
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_SetInputRange
*	����˵��: ����AD7606ģ���ź��������̡�
*	��    ��: _ucRange : 0 ��ʾ����5V   1��ʾ����10V
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_SetInputRange(uint8_t _ucRange)
{
	if (_ucRange == 0)
	{
		g_tAD7606.ucRange = 0;
		RANGE_0();	/* ����Ϊ����5V */
	}
	else
	{
		g_tAD7606.ucRange = 1;
		RANGE_1();	/* ����Ϊ����10V */
	}
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_Reset
*	����˵��: Ӳ����λAD7606����λ֮��ָ�����������״̬��
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_Reset(void)
{
	RESET_0();	/* �˳���λ״̬ */

	RESET_1();	/* ���븴λ״̬ */
	RESET_1();	/* �������ӳ١� RESET��λ�ߵ�ƽ��������С50ns�� */
	RESET_1();
	RESET_1();

	RESET_0();	/* �˳���λ״̬ */
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_StartConvst
*	����˵��: ����1��ADCת��
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_StartConvst(void)
{
	/* page 7��  CONVST �ߵ�ƽ�����Ⱥ͵͵�ƽ��������� 25ns */
	/* CONVSTƽʱΪ�� */
	CONVST_0();
	CONVST_0();
	CONVST_0();

	CONVST_1();
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_ReadNowAdc
*	����˵��: ��ȡ8·�������������洢��ȫ�ֱ��� g_tAD7606
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_ReadNowAdc(void)
{
	g_tAD7606.sNowAdc[0] = AD7606_RESULT();	/* ����1·���� */
	g_tAD7606.sNowAdc[1] = AD7606_RESULT();	/* ����2·���� */
	g_tAD7606.sNowAdc[2] = AD7606_RESULT();	/* ����3·���� */
	g_tAD7606.sNowAdc[3] = AD7606_RESULT();	/* ����4·���� */
	g_tAD7606.sNowAdc[4] = AD7606_RESULT();	/* ����5·���� */
	g_tAD7606.sNowAdc[5] = AD7606_RESULT();	/* ����6·���� */
	g_tAD7606.sNowAdc[6] = AD7606_RESULT();	/* ����7·���� */
	g_tAD7606.sNowAdc[7] = AD7606_RESULT();	/* ����8·���� */
}



/*
*********************************************************************************************************
*		����ĺ������ڶ�ʱ�ɼ�ģʽ�� TIM5Ӳ����ʱ�ж��ж�ȡADC���������ȫ��FIFO
*
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*	�� �� ��: AD7606_EnterAutoMode
*	����˵��: ����Ӳ���������Զ��ɼ�ģʽ������洢��FIFO��������
*	��    ��:  _ulFreq : ����Ƶ�ʣ���λHz��	1k��2k��5k��10k��20K��50k��100k��200k
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_EnterAutoMode(uint32_t _ulFreq)
{
#if 1
	/* ����PC6ΪTIM1_CH1���ܣ����ռ�ձ�50%�ķ��� */
	bsp_SetTIMOutPWM(GPIOC, GPIO_PIN_6, TIM8, 1, _ulFreq, 5000);
#else

	/* ����PC6Ϊ���ù��ܣ�TIM3_CH1 . ִ�к�bsp_InitAD7606()��PC6���ߵ����ý�ʧЧ */
	{
		GPIO_InitTypeDef gpio_init_structure;

		/* TIM3 clock enable */
		__HAL_RCC_TIM3_CLK_ENABLE();

		/* GPIOC clock enable */
		__HAL_RCC_GPIOC_CLK_ENABLE();

		/* GPIOH Configuration: PC6  -> TIM3 CH1 */
		gpio_init_structure.Pin = GPIO_PIN_6;
		gpio_init_structure.Mode = GPIO_MODE_AF_PP;
		gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio_init_structure.Alternate = GPIO_AF2_TIM3;
		gpio_init_structure.Pull = GPIO_PULLUP;
		
		HAL_GPIO_Init(GPIOC, &gpio_init_structure);
	}

	{
		TIM_HandleTypeDef TIM3_Handler;        //��ʱ��3PWM��� 
		TIM_OC_InitTypeDef TIM3_CH1Handler;	    //��ʱ��3ͨ��1���

		uint32_t uiTIMxCLK;
		uint16_t usPrescaler;
		uint16_t usPeriod;

		//TIM_DeInit(TIM3);	/* ��λTIM��ʱ�� */

	    /*-----------------------------------------------------------------------
			system_stm32f4xx.c �ļ��� void SetSysClock(void) ������ʱ�ӵ��������£�

			HCLK = SYSCLK / 1     (AHB1Periph)
			PCLK2 = HCLK / 2      (APB2Periph)
			PCLK1 = HCLK / 4      (APB1Periph)

			��ΪAPB1 prescaler != 1, ���� APB1�ϵ�TIMxCLK = PCLK1 x 2 = SystemCoreClock / 2;
			��ΪAPB2 prescaler != 1, ���� APB2�ϵ�TIMxCLK = PCLK2 x 2 = SystemCoreClock;

			APB1 ��ʱ���� TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM6, TIM12, TIM13,TIM14
			APB2 ��ʱ���� TIM1, TIM8 ,TIM9, TIM10, TIM11
		*/

		uiTIMxCLK = SystemCoreClock / 2;

		if (_ulFreq < 3000)
		{
			usPrescaler = 100 - 1;					/* ��Ƶ�� = 10 */
			usPeriod =  (uiTIMxCLK / 100) / _ulFreq  - 1;		/* �Զ���װ��ֵ */
		}
		else	/* ����4K��Ƶ�ʣ������Ƶ */
		{
			usPrescaler = 0;					/* ��Ƶ�� = 1 */
			usPeriod = uiTIMxCLK / _ulFreq - 1;	/* �Զ���װ��ֵ */
		}

		/* Time base configuration */
		TIM3_Handler.Instance = TIM3;
		TIM3_Handler.Init.Prescaler = usPrescaler;
		TIM3_Handler.Init.Period = usPeriod;
		TIM3_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
		TIM3_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		HAL_TIM_PWM_Init(&TIM3_Handler);
		
		/* PWM1 Mode configuration: Channel1 */
		TIM3_CH1Handler.OCMode = TIM_OCMODE_PWM1;    //ģʽѡ��PWM1
		TIM3_CH1Handler.Pulse = usPeriod/4;   //Դ�����и�����4
		TIM3_CH1Handler.OCPolarity = TIM_OCPOLARITY_LOW;   //����Ƚϼ���Ϊ��
		HAL_TIM_PWM_ConfigChannel(&TIM3_Handler, &TIM3_CH1Handler, TIM_CHANNEL_1);   //����TIM3ͨ��1
		
		HAL_TIM_PWM_Start(&TIM3_Handler, TIM_CHANNEL_1);
		
	}
#endif
		

	/* ����PE5, BUSY ��Ϊ�ж�����ڣ��½��ش��� */
	{
		GPIO_InitTypeDef   gpio_init_structure;
		
		/* Enable GPIOE clock */
		__HAL_RCC_GPIOE_CLK_ENABLE();

		/* Configure PE5 pin as input floating */   //����PE5Ϊ�½��ش������ⲿ�ж����
		gpio_init_structure.Mode = GPIO_MODE_IT_FALLING;
		gpio_init_structure.Pin = GPIO_PIN_5;
		gpio_init_structure.Pull = GPIO_PULLDOWN;
		HAL_GPIO_Init(GPIOE, &gpio_init_structure);

		/* Enable and set EXTI Line6 Interrupt to the lowest priority */
		HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);   //��ռ���ȼ�6�������ȼ�0
		HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);  //ʹ���ж���5
		
	}
		
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_HasNewData
*	����˵��: �ж�FIFO���Ƿ���������
*	��    ��:  _usReadAdc : ���ADC����ı���ָ��
*	�� �� ֵ: 1 ��ʾ�У�0��ʾ��������
*********************************************************************************************************
*/
uint8_t AD7606_HasNewData(void)
{
	if (g_tAdcFifo.usCount > 0)
	{
		return 1;
	}
	return 0;
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_FifoFull
*	����˵��: �ж�FIFO�Ƿ���
*	��    ��:  _usReadAdc : ���ADC����ı���ָ��
*	�� �� ֵ: 1 ��ʾ����0��ʾδ��
*********************************************************************************************************
*/
uint8_t AD7606_FifoFull(void)
{
	return g_tAdcFifo.ucFull;
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_ReadFifo
*	����˵��: ��FIFO�ж�ȡһ��ADCֵ
*	��    ��:  _usReadAdc : ���ADC����ı���ָ��
*	�� �� ֵ: 1 ��ʾOK��0��ʾ��������
*********************************************************************************************************
*/
uint8_t AD7606_ReadFifo(uint16_t *_usReadAdc)
{
	if (AD7606_HasNewData())
	{
		*_usReadAdc = g_tAdcFifo.sBuf[g_tAdcFifo.usRead];
		if (++g_tAdcFifo.usRead >= ADC_FIFO_SIZE)
		{
			g_tAdcFifo.usRead = 0;
		}

		DISABLE_INT();
		if (g_tAdcFifo.usCount > 0)
		{
			g_tAdcFifo.usCount--;
		}
		ENABLE_INT();
		return 1;
	}
	return 0;
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_StartRecord
*	����˵��: ��ʼ�ɼ�
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_StartRecord(uint32_t _ulFreq)
{
	AD7606_StopRecord();

	AD7606_Reset();					/* ��λӲ�� */
	AD7606_StartConvst();			/* ���������������1������ȫ0������ */

	g_tAdcFifo.usRead = 0;			/* �����ڿ���TIM2֮ǰ��0 */
	g_tAdcFifo.usWrite = 0;
	g_tAdcFifo.usCount = 0;
	g_tAdcFifo.ucFull = 0;

	AD7606_EnterAutoMode(_ulFreq);
}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_StopRecord
*	����˵��: ֹͣ�ɼ���ʱ��
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_StopRecord(void)
{
	__HAL_RCC_TIM5_CLK_DISABLE();

	/* ��PE5 ��������Ϊ��ͨ����� */
	{
		GPIO_InitTypeDef gpio_init_structure;

		/* ʹ�� GPIOʱ�� */
		__HAL_RCC_GPIOE_CLK_ENABLE();
		

		gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
		gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio_init_structure.Pull  = GPIO_NOPULL;
		gpio_init_structure.Pin = GPIO_PIN_5;
		HAL_GPIO_Init(GPIOE, &gpio_init_structure);
	}

//	/* ����PE5, ��ֹ BUSY ��Ϊ�ж������ */
//	{
//		EXTI_InitTypeDef   EXTI_InitStructure;

//		/* Configure EXTI Line5 */
//		EXTI_InitStructure.EXTI_Line = EXTI_Line5;
//		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
//		EXTI_InitStructure.EXTI_LineCmd = DISABLE;
//		EXTI_Init(&EXTI_InitStructure);
//	}
	CONVST_1();					/* ����ת����GPIOƽʱ����Ϊ�� */

}

/*
*********************************************************************************************************
*	�� �� ��: AD7606_ISR
*	����˵��: ��ʱ�ɼ��жϷ������
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_ISR(void)
{
	uint8_t i;

	AD7606_ReadNowAdc();
	

	for (i = 0; i < 8; i++)
	{
		g_tAdcFifo.sBuf[g_tAdcFifo.usWrite] = g_tAD7606.sNowAdc[i];
		if (++g_tAdcFifo.usWrite >= ADC_FIFO_SIZE)
		{
			g_tAdcFifo.usWrite = 0;
		}
		if (g_tAdcFifo.usCount < ADC_FIFO_SIZE)
		{
			g_tAdcFifo.usCount++;
		}
		else
		{
			g_tAdcFifo.ucFull = 1;		/* FIFO ������������������������ */
		}
	}
	
}

/*
*********************************************************************************************************
*	�� �� ��: EXTI9_5_IRQHandler
*	����˵��: �ⲿ�жϷ��������ڡ�PI6 / AD7606_BUSY �½����жϴ���
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/

void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);//�����жϴ����ú���
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	AD7606_ISR();
}

/***************************** (END OF FILE) *********************************/
