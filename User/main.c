/*
*********************************************************************************************************
*
*	ģ������ : ������ģ��
*	�ļ����� : main.c
*	��    �� : V1.0
*	˵    �� : ѧϰ������PCͨ�š�AD7606���ݲɼ�
*              ע�����
*                1. ��ʵ���Ƽ�ʹ�ô������SecureCRT�鿴��ӡ��Ϣ��������115200������λ8����żУ��λ�ޣ�ֹͣλ1��
*                2. ��ؽ��༭��������������TAB����Ϊ4���Ķ����ļ���Ҫ��������ʾ�����롣
*
*	�޸ļ�¼ :
*		�汾��   ����         ����        ˵��
*		V1.0    2019-10-20   Will     1. CMSIS����汾 V5.5.0
*                                         2. HAL��汾 V2.4.0
*
*********************************************************************************************************
*/	
#include "bsp.h"			/* �ײ�Ӳ������ */


static void AD7606_Mak(void);
static void AD7606_Disp(void);
static void SampleDataDealing(void);


static int16_t s_volt[8];
static int16_t s_dat[8];
int8_t g_DataBuf[4096];

/*
*********************************************************************************************************
*	�� �� ��: main
*	����˵��: c�������
*	��    ��: ��
*	�� �� ֵ: �������(���账��)
*********************************************************************************************************
*/
int main(void)
{
	uint8_t ucKeyCode;
	uint8_t ucRefresh = 0;
	uint8_t ucFifoMode;
	uint8_t data[3];
	const char buf1[] = "���յ���������1\r\n";
	const char buf2[] = "���յ���������2\r\n";
	const char buf3[] = "���յ���������3\r\n";
	const char buf4[] = "���յ���������4\r\n";
    char buf5[] = "Hello World!!!\r\n";
	
	bsp_Init();		/* Ӳ����ʼ�� */
	
	PrintfLogo();	/* ��ӡ�������ƺͰ汾����Ϣ */
	PrintfHelp();	/* ��ӡ������ʾ */
	
	ucFifoMode = 0;	 	/* AD7606������ͨ����ģʽ */
	AD7606_StartRecord(1000);		/* �����Զ��ɼ�ģʽ������Ƶ��1KHz�����ݴ����ȫ��FIFO */
	AD7606_SetOS(AD_OS_NO);		/* �޹����� */
	AD7606_SetInputRange(0);	/* 0��ʾ��������Ϊ����5V, 1��ʾ����10V */
	AD7606_StartConvst();		/* ����1��ת�� */

	bsp_StartAutoTimer(0, 100);	/* ����1��100ms���Զ���װ�Ķ�ʱ�� */
	ucRefresh = 0;
	
	/* �������ѭ�� */
	while (1)
	{

		bsp_Idle();		/* ����ʱִ�еĺ���,����ι��. ��bsp.c�� */

		if (ucRefresh == 1)
		{
			ucRefresh = 0;

			/* �������� */
			AD7606_Mak();
										 
			/* ��ӡADC������� */
//			AD7606_Disp();		
		}

		if (ucFifoMode == 0)	/* AD7606 ��ͨ����ģʽ */
		{
			if (bsp_CheckTimer(0))
			{
				/* ÿ��500ms ����һ��. ���������ת�� */
				AD7606_ReadNowAdc();		/* ��ȡ������� */
				AD7606_StartConvst();		/* �����´�ת�� */

				ucRefresh = 1;	/* ˢ����ʾ */
			}
		}
		else
		{
			/*
				��FIFO����ģʽ��bsp_AD7606�Զ����вɼ������ݴ洢��FIFO��������
				�������ͨ������ĺ�����ȡ:
				uint8_t AD7606_ReadFifo(uint16_t *_usReadAdc)

				����Խ����ݱ��浽SD�������߱��浽�ⲿSRAM��

				����δ��FIFO�е����ݽ��д������д�ӡ��ǰ���µ�����ֵ��

				����������ܼ�ʱ��ȡFIFO���ݣ���ô AD7606_FifoFull() �������档

				8ͨ��200K����ʱ�����ݴ����� = 200 000 * 2 * 8 = 3.2MB/S
			*/

			if (bsp_CheckTimer(0))
			{
				ucRefresh = 1;	/* ˢ����ʾ */
			}
		}

		/* ��������ɺ�̨systick�жϷ������ʵ�֣�����ֻ��Ҫ����bsp_GetKey��ȡ��ֵ���ɡ������������
		�ȴ��������£��������ǿ�����whileѭ���������������� */
		ucKeyCode = bsp_GetKey();	/* ��ȡ��ֵ, �޼�����ʱ���� KEY_NONE = 0 */
		if (ucKeyCode != KEY_NONE)
		{
			/*
				���ڰ��������¼���ȱʡ��bsp_button.c �������TAMPER��WAKEUP��USER����ҡ��OK���ĵ����¼�
				�������Ӧ�ó�����Ҫ�����������緽������ĵ����¼�������Ҫ���޸�һ��bsp_button.c�ļ�
			*/
			switch (ucKeyCode)
			{
				case KEY_DOWN_K1:			/* K1������ �л����� */
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

				case KEY_DOWN_K2:			/* K2������ */
					ucFifoMode = 1;	 				/* AD7606����FIFO����ģʽ */
					printf("\33[%dA", (int)1);  /* �������n�� */	
					printf("AD7606����FIFO����ģʽ (200KHz 8ͨ��ͬ���ɼ�)...\r\n");
					AD7606_StartRecord(200000);		/* ����200kHz�������� */
					break;

				case KEY_DOWN_K3:			/* K3������ */
					AD7606_StopRecord();	/* ֹͣ��¼ */
					ucFifoMode = 0;	 		/* AD7606������ͨ����ģʽ */
					printf("\33[%dA", (int)1);  /* �������n�� */
					printf("AD7606������ͨ����ģʽ(0.5s��ʱ8ͨ��ͬ���ɼ�)...\r\n");
					break;

				case JOY_DOWN_U:			/* ҡ��UP������ */
					if (g_tAD7606.ucOS < 6)
					{
						g_tAD7606.ucOS++;
					}
					AD7606_SetOS(g_tAD7606.ucOS);
					ucRefresh = 1;
					break;

				case JOY_DOWN_D:			/* ҡ��DOWN������ */
					if (g_tAD7606.ucOS > 0)
					{
						g_tAD7606.ucOS--;
					}
					AD7606_SetOS(g_tAD7606.ucOS);
					ucRefresh = 1;
					break;

				case JOY_DOWN_L:			/* ҡ��LEFT������ */
					break;

				case JOY_DOWN_R:			/* ҡ��RIGHT������ */
					break;

				case JOY_DOWN_OK:			/* ҡ��OK������ */
					break;

				default:
					/* �����ļ�ֵ������ */
					break;
			}
		}
		
	}
}



/* ��16�з�������ת��Ϊ8Ϊ�з������ν��д��� */
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
*	�� �� ��: AD7606_Mak
*	����˵��: ��������������
*	��    �Σ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void AD7606_Mak(void)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{		
	/* 
		32767 = 5V , ��������ֵ��ʵ�ʿ��Ը���5V��׼��ʵ��ֵ���й�ʽ���� 
		volt[i] = ((int16_t)dat[i] * 5000) / 32767;	����ʵ�ʵ�ѹֵ�����ƹ���ģ�������׼ȷ�������У׼            
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

	/* ��ӡ�ɼ����� */
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
	printf("\33[%dA", (int)9);  /* �������n�� */		
}

/***************************** (END OF FILE) *********************************/
