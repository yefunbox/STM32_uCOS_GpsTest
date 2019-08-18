#include <includes.h>

static  OS_STK         App_TaskStartStk[APP_TASK_START_STK_SIZE];

static  void  App_TaskCreate		(void);
static  void  App_TaskStart			(void		*p_arg);  
extern  void  App_GPS_TaskCreate  (void);
static  void  GPIO_Configuration    (void);

/*************************************************
??: void RCC_Configuration(void)
??: ??????? ??
??: ?
??: ?
**************************************************/
void RCC_Configuration(void)
{
  ErrorStatus HSEStartUpStatus;                    //????????????????
  RCC_DeInit();                                    //??RCC???????????
  RCC_HSEConfig(RCC_HSE_ON);                       //????????
  HSEStartUpStatus = RCC_WaitForHSEStartUp();      //???????????
  if(HSEStartUpStatus == SUCCESS)                  //???????????
  {
    //FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); //??FLASH??????,??FLASH??????????????.??:RCC????????,??????
    //FLASH_SetLatency(FLASH_Latency_2);                    //flash?????
      	
    RCC_HCLKConfig(RCC_SYSCLK_Div1);               //??AHB(HCLK)????==SYSCLK
    RCC_PCLK2Config(RCC_HCLK_Div1);                //??APB2(PCLK2)?==AHB??
    RCC_PCLK1Config(RCC_HCLK_Div2);                //??APB1(PCLK1)?==AHB1/2??
         
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);  //??PLL?? == ???????? * 9 = 72MHz
    RCC_PLLCmd(ENABLE);                                   //??PLL??
   
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)    //??PLL????
    {
    }
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);            //?????? = PLL??
    while(RCC_GetSYSCLKSource() != 0x08)                  //??PLL??????????
    {
    }
  }

}
/*******************************************************************************
* Function Name   : NVIC_Configuration
* Description        : Configures NVIC and Vector Table base location.
* Input                    : None
* Output                 : None
* Return                 : None
*******************************************************************************/
void NVIC_Configuration(void)
{
   NVIC_InitTypeDef NVIC_InitStructure;
  
   /* Set the Vector Table base location at 0x08000000 */
   NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  
   /* Configure the NVIC Preemption Priority Bits */  
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  
   /* Enable the USART1 Interrupt */
   NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;     //???????1??
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	   //???????0
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		   //????
   NVIC_Init(&NVIC_InitStructure); 						           //???
	
	 /* Enable the USART2 Interrupt */
   NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;     //???????2??
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	   ///???????0
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		   //????
   NVIC_Init(&NVIC_InitStructure); 						           //???
}

#define    QTBL_SIZE                    20   //????????

OS_EVENT*  g_MesgQ;                         //????
void*      g_mesgqtbl[QTBL_SIZE];           //??????

/*
===============================================================================
 Function    : BuildMQ()
 ??:??????????
===============================================================================
*/
void BuildMQ(void)
{

	g_MesgQ = OSQCreate(&g_mesgqtbl[0], QTBL_SIZE);  //???????
}

INT32S main (void)
{
    CPU_INT08U  os_err;
	os_err = os_err; /* prevent warning... */

 	/* Note:  ????UCOS, ?OS??????,?????????. */
	CPU_IntDis();                    /* Disable all ints until we are ready to accept them.  */

    OSInit();                        /* Initialize "uC/OS-II, The Real-Time Kernel".         */
	BuildMQ();
	os_err = OSTaskCreateExt((void (*)(void *)) App_TaskStart,  /* Create the start task.                               */
                             (void          * ) 0,
                             (OS_STK        * )&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],
                             (INT8U           ) APP_TASK_START_PRIO,
                             (INT16U          ) APP_TASK_START_PRIO,
                             (OS_STK        * )&App_TaskStartStk[0],
                             (INT32U          ) APP_TASK_START_STK_SIZE,
                             (void          * )0,
                             (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));

	OSStart();               /* Start multitasking (i.e. give control to uC/OS-II).  */

	return (0);
}


/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : The startup task.  The uC/OS-II ticker should only be initialize once multitasking starts.
*
* Argument(s) : p_arg       Argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none.
*
* Caller(s)   : This is a task.
*
* Note(s)     : none.
*********************************************************************************************************
*/	  
static  void  App_TaskStart (void *p_arg)
{   
    (void)p_arg;
	
    /***************  Init hardware ***************/
    OS_CPU_SysTickInit();                                    /* Initialize the SysTick.                              */
	RCC_Configuration();
	NVIC_Configuration();
	GPIO_Configuration();
    USART1_Configuration();
	//USART2_Configuration();
	//RTC_INIT();
#if (OS_TASK_STAT_EN > 0)
    OSStatInit();                                            /* Determine CPU capacity.                              */
#endif

    App_TaskCreate();                                        /* Create application tasks.                            */
	//printf("build %s_%s\n", __DATE__, __TIME__);
    printf("=========Main Task Start=====\r\n");
	while(1){
      	OSTimeDlyHMSM(0, 1, 0, 0);							 /* Delay One minute */
    }	
}


/*
*********************************************************************************************************
*                                            App_TaskCreate()
*
* Description : Create the application tasks.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : App_TaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  App_TaskCreate (void)
{
	
	App_GPS_TaskCreate();	

}

/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configure GPIO Pin
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB , ENABLE); 						 

  //LED -> PB12		 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
