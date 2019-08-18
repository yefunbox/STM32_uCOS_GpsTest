#include <includes.h> 

extern OS_EVENT* g_MesgQ;
extern struct_gpsDataMQ  gpsDataMQ[5];
#define MAX_BUFFER_SIZE  100
#define MAX_ARRAY        6

static int dataIndix = 0;
static int dataFlag = 1;

/*******************************************************************************
	��������USART2_Configuration
	��  ��:
	��  ��:
	����˵����
	��ʼ������Ӳ���豸�������ж�
	���ò��裺
	(1)��GPIO��USART��ʱ��
	(2)����USART�����ܽ�GPIOģʽ
	(3)����USART���ݸ�ʽ�������ʵȲ���
	(4)ʹ��USART�����жϹ���
	(5)���ʹ��USART����
*/
void USART2_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* ��1������GPIO��USART2������ʱ�� */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* ��2������USART2 Tx��GPIO����Ϊ���츴��ģʽ */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ��3������USART2 Rx��GPIO����Ϊ��������ģʽ
		����CPU��λ��GPIOȱʡ���Ǹ�������ģʽ���������������費�Ǳ����
		���ǣ��һ��ǽ�����ϱ����Ķ������ҷ�ֹ�����ط��޸���������ߵ����ò���
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/*  ��3���Ѿ����ˣ�����ⲽ���Բ���
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	*/
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/* ��4��������USART2����
	    - BaudRate = 115200 baud
	    - Word Length = 8 Bits
	    - One Stop Bit
	    - No parity
	    - Hardware flow control disabled (RTS and CTS signals)
	    - Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

  /* ���������ݼĴ�������������ж� */
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	/* ��5����ʹ�� USART2�� ������� */
	USART_Cmd(USART2, ENABLE);

  /* �����������1���ֽ��޷���ȷ���ͳ�ȥ������ */
	USART_ClearFlag(USART2, USART_FLAG_TC);     // ���־
}

/*******************************************************************/
/*                                                                 */
/* STM32�򴮿�2����1�ֽ�                                           */
/*                                                                 */
/*                                                                 */
/*******************************************************************/
void Uart2_PutChar(u8 ch)
{
  USART_SendData(USART2, (u8) ch);
  while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}
void Uart2_PutString(u8 *ptr)
{
    while(*ptr != '\0')
    {
        Uart2_PutChar(*(ptr++));
    }
}
void receivOneFrameData(u8 dat) {
	if(dat == '\r') {

	} else if(dat == '\n') {
		gpsDataMQ[dataFlag].size         = dataIndix;
		gpsDataMQ[dataFlag].ProtocalType = 1;
		gpsDataMQ[dataFlag].flag         = dataFlag;
		OSQPost(g_MesgQ, (void*)&gpsDataMQ[dataFlag]);
		dataIndix = 0;
		dataFlag = (dataFlag+1)%MAX_ARRAY;
	} else {
		//gpsData[gpsDataMQ.flag][dataIndix] = dat;
		gpsDataMQ[dataFlag].pData[dataIndix] = dat;
		dataIndix++;
	}
}

/*******************************************************************/
/*                                                                 */
/* STM32�ڴ���2����1�ֽ�                                           */
/* ˵��������2�����ж�                                             */
/*                                                                 */
/*���жϷ�������У�����������Ӧ�ж�ʱ����֪�����ĸ��ж�Դ������
  ��������˱������жϷ�������ж��ж�Դ�����б�Ȼ��ֱ����
  ������Ȼ�����ֻ�漰��һ���ж������ǲ����������б�ġ�����
  ����ʲô������������б��Ǹ���ϰ��*/
/*******************************************************************/
void USART2_IRQHandler(void)            
{
  u8 dat;
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)    //���������ݼĴ�����
  {
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    dat = (u8)USART_ReceiveData(USART2);
	receivOneFrameData(dat);
	//USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
  }
}
