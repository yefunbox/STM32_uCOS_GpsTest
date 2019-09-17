#include <includes.h>  

#define SCL_H         GPIO_SetBits(GPIOB , GPIO_Pin_10)
#define SCL_L         GPIO_ResetBits(GPIOB , GPIO_Pin_10)
#define SDA_H         GPIO_SetBits(GPIOB , GPIO_Pin_11)
#define SDA_L         GPIO_ResetBits(GPIOB , GPIO_Pin_11)
#define SDA_Read      GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_11)

#define SDA_IN()      {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=(u32)8<<12;} //pb11输入模式
#define SDA_OUT()     {GPIOB->CRH&=0XFFFF0FFF;GPIOB->CRH|=(u32)3<<12;} //pb11输出模式

void I2C_Init(void)				//pb6 pb7--> pb8 pb9(remape)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;		
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	SDA_H;
	SCL_H;	
}


void I2C_Delay(void)		  //软件延时（非精确）
{	
   uint8_t i = 60;
	
   while(i) { 
     i--; 
   } 
}

u8 I2C_Start(void)	 //I2C开始位
{
	SDA_OUT();
	SDA_H;
	SCL_H;
	I2C_Delay();
	SDA_L;
	I2C_Delay();
	SCL_L;							//SCL为高电平时，SDA的下降沿表示停止位
	I2C_Delay();
	return 1;
}

void I2C_Stop(void)			   //I2C停止位
{
	SDA_OUT();
	SCL_L;
	I2C_Delay();
	SDA_L;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	SDA_H;					   //SCL为高电平时，SDA的上升沿表示停止位
	I2C_Delay();
}
static void I2C_Ack(void)		//I2C响应位
{	
	SDA_OUT();
	SCL_L;
	I2C_Delay();
	SDA_L;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	SCL_L;
	I2C_Delay();
}

static void I2C_NoAck(void)		//I2C非响应位
{	
	SDA_OUT();

	SCL_L;
	I2C_Delay();
	SDA_H;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	SCL_L;
	I2C_Delay();
}

u8 I2C_WaitAck(void) 	  //I2C等待应答位
{
	u8 count;

	SDA_IN();
	SDA_H;
	I2C_Delay();
	SCL_H;
	I2C_Delay();
	while(SDA_Read){
		if(++count>250){
			I2C_Stop();
			return 1;
		}
	}
	SCL_L;
	SDA_OUT();
	SDA_H;
	return 0;
}

void I2C_SendByte(u8 SendByte) 
 {
	u8 i;
	 
 	SDA_OUT();
	for(i = 0;i < 8; i++) 
	{
	   SCL_L;
	   I2C_Delay();
	   if(SendByte & 0x80)
		 SDA_H; 			   //在SCL为低电平时，允许SDA数据改变
	   else 
		 SDA_L;   
	   SendByte <<= 1;
	   I2C_Delay();
	   SCL_H;
	   I2C_Delay();
	 }
	 SCL_L;
 }
 
u8 I2C_SendString(u8 chipAddr,u8 const *buffer,u8 Number)
{
    u8 i;
    I2C_Start();
    I2C_SendByte(chipAddr);   //发送器件地址0XA0,写数据

    if(I2C_WaitAck()==1){
        printf("Wait_Ack err\n");
        return 0;
    }

    for(i = 0;i < Number;i++) {
        I2C_SendByte(*buffer);
        if(I2C_WaitAck() == 1) {
            printf("\r\nerr\r\n");
            return 0;
        }
        buffer++;
    }
    I2C_Stop();
    return 1;
}

u8 I2C_ReceiveByte(u8 ack)
{
    u8 i,ReceiveByte = 0;
    
	SDA_IN();
    SDA_H;
    for(i = 0;i < 8;i++)
    {
      ReceiveByte <<= 1;      
      SCL_L;
      I2C_Delay();
	  SCL_H;
      I2C_Delay();	
      if(SDA_Read)				   //在SCL为高电平时，SDA上的数据保持不变，可以读回来
      {
        ReceiveByte |= 0x01;
      }
    }
    SCL_L;
	if(ack)	
		I2C_Ack();
	else
		I2C_NoAck();
    return ReceiveByte;
}
u8 I2C_ReadString(u8 chipAddr,u8 *buffer,u8 Number)
{
    u8 i;
	u8 data[200];
	
    I2C_Start();
    I2C_SendByte(chipAddr);   //发送器件地址,写数据
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 00 err\n");
        return 0;
    }
	I2C_SendByte(0x00); //Register Addressing
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 11 err\n");
        return 0;
    }
	I2C_SendByte(chipAddr|1);   //发送器件地址,读数据
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 22 err\n");
        return 0;
    }
	for(i = 0;i < Number;i++) {
		data[i] = I2C_ReceiveByte(1);
	}
	data[Number] = I2C_ReceiveByte(0);
	I2C_Stop();
	printf("data = ");
	for(i = 0;i <Number;i++) {
		printf("%c,",data[i]);
	}
	printf("\r\n");
}

u8 I2C_ReadNumber(u8 chipAddr)
{
    u8 i;
	u8 Number = 100;
	u8 data[100];
	
    I2C_Start();
    I2C_SendByte(chipAddr);   //发送器件地址,写数据
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 00 err\n");
        return 0;
    }
	I2C_SendByte(0xFE); //Register Addressing
	if(I2C_WaitAck()==1){
        //printf("Wait_Ack 11 err\n");
        return 0;
    }
	I2C_SendByte(0xFD); //Register Addressing
	if(I2C_WaitAck()==1){
        //printf("Wait_Ack 11 err\n");
        return 0;
    }
	
    I2C_Start();
	I2C_SendByte(chipAddr|1);   //发送器件地址,读数据
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 22 err\n");
        return 0;
    }
	for(i = 0;i < Number;i++) {
		data[i] = I2C_ReceiveByte(1);
		if(data[i] == 0xff) {
			Number = i;
			break;
		}
	}
	data[Number] = I2C_ReceiveByte(0);
	I2C_Stop();
	//printf("number = 0x%x\r\n",Number);
	for(i = 0;i <Number;i++) {
		if((data[i] >='0' && data[i]<='9') || data[i] == '$'|| data[i] == '*'|| data[i] == ',' ||data[i] == '\n' ||(data[i] >='a' && data[i]<='z' || data[i] >='A' && data[i]<='Z')){
			printf("%c",data[i]);
		}
	}
}

u8 I2C_ReadCurrentAddress(u8 chipAddr,u8 *buffer,u8 Number)
{
    u8 i;
	u8 data[200];
	
    I2C_Start();
    I2C_SendByte(chipAddr|1);   //发送器件地址,写数据
	if(I2C_WaitAck()==1){
        printf("Wait_Ack 333 err\n");
        return 0;
    }
	for(i = 0;i < Number;i++) {
		data[i] = I2C_ReceiveByte(1);
		if(data[i] == 0xff) {
			Number = i;
			break;
		}
	}
	data[Number] = I2C_ReceiveByte(0);
	I2C_Stop();
	//printf("data = ");
	for(i = 0;i <Number;i++) {
		if((data[i] >='0' && data[i]<='9') || data[i] == '$'|| data[i] == '*'|| data[i] == ',' ||data[i] == '\n' ||(data[i] >='a' && data[i]<='z' || data[i] >='A' && data[i]<='Z')){
			printf("%c",data[i]);
		}
	}
	//printf("\r\n");
}
