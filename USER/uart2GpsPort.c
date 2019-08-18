#include <includes.h> 

extern OS_EVENT* g_MesgQ;
extern struct_gpsDataMQ  gpsDataMQ[5];
#define MAX_BUFFER_SIZE  100
#define MAX_ARRAY        6

static int dataIndix = 0;
static int dataFlag = 1;

/*******************************************************************************
	函数名：USART2_Configuration
	输  入:
	输  出:
	功能说明：
	初始化串口硬件设备，启用中断
	配置步骤：
	(1)打开GPIO和USART的时钟
	(2)设置USART两个管脚GPIO模式
	(3)配置USART数据格式、波特率等参数
	(4)使能USART接收中断功能
	(5)最后使能USART功能
*/
void USART2_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* 第1步：打开GPIO和USART2部件的时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* 第2步：将USART2 Tx的GPIO配置为推挽复用模式 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* 第3步：将USART2 Rx的GPIO配置为浮空输入模式
		由于CPU复位后，GPIO缺省都是浮空输入模式，因此下面这个步骤不是必须的
		但是，我还是建议加上便于阅读，并且防止其它地方修改了这个口线的设置参数
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/*  第3步已经做了，因此这步可以不做
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	*/
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/* 第4步：配置USART2参数
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

  /* 若接收数据寄存器满，则产生中断 */
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	/* 第5步：使能 USART2， 配置完毕 */
	USART_Cmd(USART2, ENABLE);

  /* 如下语句解决第1个字节无法正确发送出去的问题 */
	USART_ClearFlag(USART2, USART_FLAG_TC);     // 清标志
}

/*******************************************************************/
/*                                                                 */
/* STM32向串口2发送1字节                                           */
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
/* STM32在串口2接收1字节                                           */
/* 说明：串口2接收中断                                             */
/*                                                                 */
/*在中断服务程序中，由于主机响应中断时并不知道是哪个中断源发出中
  断请求，因此必须在中断服务程序中对中断源进行判别，然后分别进行
  处理。当然，如果只涉及到一个中断请求，是不用做上述判别的。但是
  无论什么情况，做上述判别是个好习惯*/
/*******************************************************************/
void USART2_IRQHandler(void)            
{
  u8 dat;
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)    //若接收数据寄存器满
  {
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    dat = (u8)USART_ReceiveData(USART2);
	receivOneFrameData(dat);
	//USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
  }
}
