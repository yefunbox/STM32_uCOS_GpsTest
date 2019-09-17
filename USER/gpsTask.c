#include <includes.h>            
#include "nmeaParse.h"
#include "gmath.h"


#define  TASK_GPS_STK_SIZE   	    512u
static __align(8) OS_STK         	TASK_GPS_STACK[TASK_GPS_STK_SIZE];

#define  LED1                       1
#define  LED2                       2

#define  OFF                        0
#define  ON                         1

#define  VALID_SNR                  35
#define  VALID_SAT_COUNT            2
struct_SatInfo ValidSatInfo[VALID_SAT_COUNT];
u8 valid_sta_count = 0;
u8 GSAMode[2];

static void GpsTask(void);
char g_Mode = 0;
nmeaPOS lastPos;
nmeaPOS nowPos;
double dst;
u8 GpsReportRate = 0;
u8 GpsGGACnt     = 0;
u8 GpsGSACnt     = 0;
u8 GpsGSVCnt     = 0;

void handerGpsData(const char* gpsBuffer,int size);

#define Log(...)   \
	printf(__VA_ARGS__), \
    printf("\r\n")

void  App_GPS_TaskCreate (void) {
    CPU_INT08U  os_err;
	os_err = os_err; /* prevent warning... */
	os_err = OSTaskCreate((void (*)(void *)) GpsTask,				
                          (void          * ) 0,							
                          (OS_STK        * ) &TASK_GPS_STACK[TASK_GPS_STK_SIZE - 1],		
                          (INT8U           ) APP_TASK_BLINK_PRIO  );
}

extern OS_EVENT* gMesgQ;
struct_gpsDataMQ  gpsDataMQ[MAX_ARRAY];
void parseGpsData(const u8* data,u8 size) {
    int i = 0;

	for(i = 0;i < size;i++) {
		GPS_Parser(data[i]);
	}
}
void GPRMC_CallBack(struct_GPSRMC GPS_RMC_Data) {
	GpsReportRate++;
	//Log("GPRMC GpsReportRate=%d",GpsReportRate);
}
void GPGGA_CallBack(struct_GPSGGA GPS_GGA_Data) {
	//Log("Altitude:%.1fmeter",atof(GPS_GGA_Data.Altitude));
	GpsGGACnt++;
}
void GPGSA_CallBack(struct_GPSGSA GPS_GSA_Data) {
   	//Log("GPGSA_CB %c%c",GPS_GSA_Data.Mode,GPS_GSA_Data.Mode2);
	GSAMode[0] = GPS_GSA_Data.Mode;
	GSAMode[1] = GPS_GSA_Data.Mode2;
	GpsGSACnt++;
}
u8 isExist(u8 SatID) {
	u8 i = 0;
	
	for(;i<VALID_SAT_COUNT;i++) {
		if(ValidSatInfo[valid_sta_count].SatID == SatID) {
			return 1;
		}
	}
	return 0;
}
void GPGSV_CallBack(struct_GPSGSV GPS_GSV_Data) {
	int i;
	
	GpsGSVCnt++;
    //Log("GPGSV_CB SatInView = %d",GPS_GSV_Data.SatInView);
    for(i=0;i < 4;i++) {
       //Log("SatID=%02d,SNR=%02d",GPS_GSV_Data.SatInfo[i].SatID,GPS_GSV_Data.SatInfo[i].SNR);
	   if(GPS_GSV_Data.SatInfo[i].SNR >= VALID_SNR && !isExist(GPS_GSV_Data.SatInfo[i].SatID)) {
			ValidSatInfo[valid_sta_count].SatID  = GPS_GSV_Data.SatInfo[i].SatID;
		  	ValidSatInfo[valid_sta_count].SNR    = GPS_GSV_Data.SatInfo[i].SNR;
			valid_sta_count++;
	   }
    }
}

void initNmeaParserCallBack() {
    initParserCallBack(GPRMC_CallBack,GPGGA_CallBack,
	                   GPGSA_CallBack,GPGSV_CallBack);
}
void _TestGPSDemo() {
     int i = 0,j = 0,lengh;
	 
     const char *buff[] = {
		"$GPRMC,023048.00,A,2233.75606,N,11355.73507,E,22.867,182.55,090918,,,A*5F\r\n",
		"$GPVTG,182.55,T,,M,22.867,N,42.350,K,A*3F\r\n",
		"$GPGGA,023048.00,2233.75606,N,11355.73507,E,1,06,1.32,59.0,M,-2.8,M,,*7B\r\n",
		"$GPGSA,A,3,06,17,19,03,09,28,,,,,,,2.25,1.32,1.83*05\r\n",
		"$GPGSV,3,1,09,03,28,045,19,06,52,296,26,09,14,127,13,17,52,014,17*72\r\n",
		"$GPGSV,3,2,09,19,40,335,18,23,12,098,,24,01,297,,28,62,162,27*77\r\n",
		"$GPGSV,3,3,09,41,46,237,*41\r\n",
		"$GPGLL,2233.75606,N,11355.73507,E,023048.00,A,A*63\r\n",
     };
     lengh = sizeof(buff)/sizeof(buff[0]);
     Log("testGPSDemo lengh = %d",lengh);

     for(i = 0;i < lengh;i++) {
		 parseGpsData(buff[i],(int)strlen(buff[i]));
		 //handerGpsData(buff[i],(int)strlen(buff[i]));
	 }

}
void LedCtrl(u8 led,u8 onoff){
	switch (led)
	{
		case LED1:
			if(onoff) {
				GPIO_ResetBits(GPIOB , GPIO_Pin_12);  //LED-ON
			} else {
				GPIO_SetBits(GPIOB , GPIO_Pin_12);	  //LED-OFF
			}
			break;
		case LED2:
			if(onoff) {
				GPIO_ResetBits(GPIOB , GPIO_Pin_13);  //LED-ON
			} else {
				GPIO_SetBits(GPIOB , GPIO_Pin_13);	  //LED-OFF
			}
			break;
	}
}
void Timer1S_CallBack(OS_TMR *ptmr, void *parg) {
	u8 i = 0;
	static u8 LedState = 0;
	
	//Log("1S GpsReportRate=%d,C(%d,%d,%d),%c%c",GpsReportRate,GpsGGACnt,GpsGSACnt,GpsGSVCnt,GSAMode[0],GSAMode[1]);
	//有效定位A2或A3,LED1和LED2一直闪
	if(GSAMode[0] == 'A' &&(GSAMode[1] == '2' || GSAMode[1] == '3')) {
		LedState = (LedState==1?0:1);
		LedCtrl(LED1,LedState);
		LedCtrl(LED2,LedState);
	} else { //无效定位
		//检测有一次3HZ数据,LED1一直亮
		if(GpsReportRate >=3) {
			LedCtrl(LED1,ON);
		} else if((GpsReportRate + GpsGGACnt + GpsGSACnt + GpsGSVCnt) == 0) {
			//无GPS数据说明已经拔掉了GPS模块
			LedCtrl(LED1,OFF);
		} else if(GpsReportRate == 1) { //上报率通常都是1

		}

		//检测到2颗35DB,LED2常亮
		if(valid_sta_count >= VALID_SAT_COUNT) {
			LedCtrl(LED2,ON);
		} else {
			LedCtrl(LED2,OFF);
		}
	}
	GpsReportRate   = 0;
	GpsGGACnt       = 0;
	GpsGSACnt       = 0;
	GpsGSVCnt       = 0;
	valid_sta_count = 0;
	GSAMode[1] = 0;
	for(;i < VALID_SAT_COUNT;i++) {
		//Log("SatID=%02d,SNR=%02d",ValidSatInfo[i].SatID,ValidSatInfo[i].SNR);
		ValidSatInfo[i].SatID = 0;
		ValidSatInfo[i].SNR = 0;
	}

	
	//I2C_ReadString(0x84,NULL,20);
	//I2C_ReadNumber(0x84);
	//I2C_ReadCurrentAddress(0x84,NULL,100);
}
OS_TMR     *pTimer1S = NULL;   
void Timer1S_Init()     //timer1 callback
{
	u8 err;

	//1000ms
	pTimer1S = OSTmrCreate(1,10, OS_TMR_OPT_PERIODIC, (OS_TMR_CALLBACK)Timer1S_CallBack, (void *)0, "10STimer", &err);
	if(err != OS_ERR_NONE)
        Log("OSTmrCreate is err!!!");
    if(OSTmrStart((OS_TMR *)pTimer1S, &err) == OS_FALSE || err != OS_ERR_NONE)
        Log("OSTmrStart is error!!!");
}
u8 const par1[] = { 
	//上电第一组
	//0xB5,0x62,0x06,0x41,0x0C,0x00,0x00,0x00,0x03,0x1F,0x78,0xB7,0x58,0xB7,0xFF,0x76,0xCF,0xFF,0xF6,0x6B,
	0xB5,0x62,0x06,0x08,0x06,0x00,0xC8,0x00,0x01,0x00,0x01,0x00,0xDE,0x6A //rate=5hz
};
u8 const par2[] = { 
	//GPS
	0xB5,0x62,0x06,0x3E,0x3C,0x00,0x00,0x00,0x20,0x07,0x00,0x08,0x10,0x00,
	0x01,0x00,0x01,0x01,0x01,0x01,0x03,0x00,0x00,0x00,0x01,0x01,0x02,0x04,
	0x08,0x00,0x00,0x00,0x01,0x01,0x03,0x08,0x10,0x00,0x00,0x00,0x01,0x01,
	0x04,0x00,0x08,0x00,0x00,0x00,0x01,0x01,0x05,0x00,0x03,0x00,0x01,0x00,
	0x01,0x01,0x06,0x08,0x0E,0x00,0x01,0x00,0x01,0x01,0x2E,0x5D,
};
u8 const par3[] = {
	//NAV-5-2G
	0xB5,0x62,0x06,0x24,0x24,0x00,0xFF,0xFF,0x04,0x03,0x00,0x00,0x00,0x00,0x10,
	0x27,0x00,0x00,0x00,0x00,0x5E,0x01,0x5E,0x01,0xC8,0x00,0x5E,0x01,0xC8,0x3C,
	0x00,0x00,0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x91,0xC4,
};
void uboxInit()
{
    I2C_SendString(0x84,par1,sizeof(par1));
	OSTimeDlyHMSM(0,0,0,100);
    I2C_SendString(0x84,par2,sizeof(par2));
	OSTimeDlyHMSM(0,0,0,100);
    I2C_SendString(0x84,par3,sizeof(par3));
	OSTimeDlyHMSM(0,0,0,100);
}
/*
*1.检查上报率为5Hz,LED1常亮
*3.有效定位A2/A3，LED1和LED2闪烁(1Hz)
*/
static void GpsTask(void) {
	struct_gpsDataMQ*    mesg;
	u8      err;
	nmeaPOS from_pos,to_pos;
	
	//char data[] = {"$GPRMC,085432.00,A,2234.72646,N,11354.44461,E,0.472,,050918,,,A*7B"};
   	Log("=========GPS Task Start=====");
	
	LedCtrl(LED1,ON);
	LedCtrl(LED1,OFF);
	LedCtrl(LED2,OFF);
	initNmeaParserCallBack();
	//_TestGPSDemo();
	//nmea_parse_GPRMC(&data,sizeof(data),&pack);
	Timer1S_Init();
	USART2_Configuration();
	I2C_Init();
	uboxInit();
	I2C_ReadString(0x84,NULL,4);
   	while(1){
		//OSTimeDlyHMSM(0,0,0,10);		
		I2C_ReadCurrentAddress(0x84,NULL,100);
		continue;
		//mesg = (struct_gpsDataMQ* )OSQPend(gMesgQ, 0, &err);
		switch(mesg->ProtocalType) {
			case 1:
				mesg->pData[mesg->size] = '\0';
				//Log("%s,size = %d,f=%d",mesg->pData,mesg->size,mesg->flag);
				parseGpsData(mesg->pData,mesg->size);
				break;
			case PROTOCAL_TYPE_UBLOX:
				//handerUbxData(g_gpsData_ptr,mesg->size);
				break;
			case PROTOCAL_TYPE_DEBUG:
				g_Mode = (char)*(mesg->pData);
				printf("MQ_type=%d,debugMode=%d,err=%d\n",mesg->ProtocalType,g_Mode,err);
				break;
		}
   }
}

