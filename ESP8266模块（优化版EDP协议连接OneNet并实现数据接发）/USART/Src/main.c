/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "onenet.h"
#include "string.h"
#include "edpkit.h"

uint8_t my_re_buf2;
uint8_t aRxBuffer;			//接收中断缓冲
char Rx_Buff[200];
int  Rx_count=0;   //用于ESP8266判断接受数据的多少
char Message_Buf[20];  //用于存贮操作指令
unsigned char Message_length;
//char RST_Start[]="AT+RST\r\n";
//char MODE_Start[]="AT+CWMODE=1\r\n";
//char WIFI_Start[]="AT+CWJAP=\"HUAWEI-3AHFVD\",\"13550695909\"\r\n";
//char Connected_Start[]="AT+CIPMUX=0\r\n";
//char TCP_Start[]="AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n";
//char TX_Start[]="AT+CIPMODE=1\r\n";
//char Data[]="AT+CIPSEND\r\n";
//char TX_Data[]="POST /devices/610177377/datapoints HTTP/1.1\r\napi-key:Nhw=N=8b0eElqPR0k2Ultdyh0tA=\r\nHost:api.heclouds.com\r\nContent-Length:63\r\n\r\n{\"datastreams\":[{\"id\":\"sys_time\",\"datapoints\":[{\"value\":50}]}]}";

#define AT          "AT\r\n"	
#define CWMODE      "AT+CWMODE=1\r\n"		//STA+AP模式
#define RST         "AT+RST\r\n"
#define CIFSR       "AT+CIFSR\r\n"
#define CWJAP       "AT+CWJAP=\"HUAWEI-3AHFVD\",\"13550695909\"\r\n"	//
#define CIPSTART    "AT+CIPSTART=\"TCP\",\"183.230.40.39\",876\r\n"	//EDP服务器 183.230.40.39/876
//#define CIPSTART    "AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n"		//HTTP服务器183.230.40.33/80
#define CIPMODE0    "AT+CIPMODE=0\r\n"		//非透传模式
#define CIPMODE1    "AT+CIPMODE=1\r\n"		//透传模式
#define CIPSEND     "AT+CIPSEND\r\n"
#define CIPSTATUS   "AT+CIPSTATUS\r\n"		//网络状态查询

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
#define MAX_RCV_LEN  500
//#define API_KEY     "0nvdlujSYYdK81esrgh0wRPSs80="		//需要定义为用户自己的参数
//#define DEV_ID      "611678262"							          //需要定义为用户自己的参数
unsigned char  usart2_rcv_buf[MAX_RCV_LEN];
char count,AT_mode;
unsigned int Heart_Pack=0;  //用于定时器TIM1自加计数，用于满足设定自加数值时发送EDP心跳包的标志位
/*
*  @brief USART2串口接收状态初始化
*/
void USART2_Clear(void)
{

		memset(usart2_rcv_buf, 0, sizeof(usart2_rcv_buf));
    count=0;
}

/*
*  @brief USART2串口发送api
*/
void USART2_Write(USART_TypeDef* USARTx, uint8_t *Data, uint8_t len)
{
	HAL_UART_Transmit(&huart2, Data, len,0xff);
  while(HAL_UART_GetState(&huart2) == HAL_UART_STATE_BUSY_TX);//检测UART发送结束   
}

/*
 *  @brief USART2串口发送AT命令用
 *  @para  cmd  AT命令
 *  @para  result 预期的正确返回信息
 *  @para  timeOut延时时间,ms
 */
void SendCmd(char* cmd, char* result, int timeOut)
{
    while(1)
    {
        USART2_Clear();
        USART2_Write(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
        HAL_Delay(timeOut);
        printf("%s %d cmd:%s,rsp:%s\n", __func__, __LINE__, cmd, usart2_rcv_buf);
        if((NULL != strstr((const char *)usart2_rcv_buf, result)))	//判断是否有预期的结果
        {
						USART2_Clear();
            break;
        }
        else
        {		
						USART2_Clear();
            HAL_Delay(100);
        }
    }
}



/*
 *  @brief ESP8266模块初始化
 */
void ESP8266_Init(void)
{
    SendCmd(AT, "OK", 1000);		//模块有效性检查
    SendCmd(CWMODE, "OK", 2000);	//模块工作模式
    SendCmd(RST, "", 2000);		//模块重置
    SendCmd(CIFSR, "OK", 1000);		//查询网络信息
    SendCmd(CWJAP, "OK", 2000);		//配置需要连接的WIFI热点SSID和密码
    SendCmd(CIPSTART, "OK", 2000);	//TCP连接
    SendCmd(CIPMODE1, "OK", 1000);	//配置透传模式
		SendCmd(CIPSEND, ">", 1000);	//进入透传模式
		USART2_Clear();

}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
///**
//  * @brief   组HTTP POST报文
//  * @param   pkt   报文缓存指针
//  * @param   key   API_KEY定义在Main.c文件中，需要根据自己的设备修改
//  *	@param 	 devid 设备ID，定义在main.c文件中，需要根据自己的设备修改
//  *	@param 	 dsid  数据流ID
//  *	@param 	 val   字符串形式的数据点的值
//  * @retval  整个包的长度
//  */
//uint32_t HTTP_PostPkt(char *pkt, char *key, char *devid, char *dsid, char *val)
//{
//    char dataBuf[100] = {0};
//    char lenBuf[10] = {0};
//    *pkt = 0;

//    sprintf(dataBuf, ",;%s,%s", dsid, val);     //采用分割字符串格式:type = 5
//    sprintf(lenBuf, "%d", strlen(dataBuf));

//    strcat(pkt, "POST /devices/");
//    strcat(pkt, devid);
//    strcat(pkt, "/datapoints?type=5 HTTP/1.1\r\n");

//    strcat(pkt, "api-key:");
//    strcat(pkt, key);
//    strcat(pkt, "\r\n");

//    strcat(pkt, "Host:api.heclouds.com\r\n");

//    strcat(pkt, "Content-Length:");
//    strcat(pkt, lenBuf);
//    strcat(pkt, "\r\n\r\n");

//    strcat(pkt, dataBuf);

//    return strlen(pkt);
//}

void Connect_OneNet(void)
{
	while(OneNet_DevLink())			//接入OneNET
	HAL_Delay(1000);
	AT_mode=1;
}

void Receive_Normal_Data()
{
	  static int i;        //静态变量i
	
		if(Rx_count==150)                   //最多接收150个数据，数据溢出 清空数组
		{
			Rx_count=0;                 	
			memset(Rx_Buff, 0, sizeof(Rx_Buff));  //清空当前数组所有数据
		}

		if(Message_length>0)                   //表示可以开始存贮操作指令
		{
			Message_Buf[i]=Rx_Buff[Rx_count];   //存贮操作指令数据
			i++;
			Message_length--;                  //存一个指令，剩余数量减一,判断操作指令是否存贮完成
		}
		 
		if(Rx_count>3&&Rx_Buff[Rx_count-2]==0&&Rx_Buff[Rx_count-1]==0&&Rx_Buff[Rx_count]>0)   
		//如果当前接收到的数据大于0，并且这个数据的前两个数据为00 代表当前数据就是操作指令的长度。
		{
			 memset(Message_Buf, 0, sizeof(Message_Buf)); //清空存贮操作指令的数组，准备存贮新的操作指令
			
			 Message_length=Rx_Buff[Rx_count];      //将接收到的数据存为操作指令长度。
			 i=0;                                   //清空i
		}
		Rx_count++;                               //准备存储下一个数据 
						
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
//	char HTTP_Buf[400];     //HTTP报文缓存区
//	char tempStr[]="30";       //字符串格式温度
//	int len;
	EDP_PACKET_STRUCTURE edpPacket = {NULL, 0, 0, 0};   //定义一个结构体变量   供心跳包装载数据用
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&aRxBuffer, 1);
	USART2_Clear();
	ESP8266_Init();
	HAL_Delay(1000);
	Connect_OneNet();

  /* USER CODE END 2 */
	
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		/*  定时发送心跳包  */			
		if(Heart_Pack>=50000)              //每隔25秒发一次心跳包   OneNet平台默认四分钟无数据交换就会断开与设备的连接 发心跳包可以保证连接。
		{
			EDP_PacketPing(&edpPacket);     //装载并发送心跳包
			Heart_Pack=0;
		}   
		
		if(strstr((const char*)Message_Buf,"OPEN"))    //判断到操作指令为OPEN 
		{
			OneNet_SendData("test",1);  //向平台发数据流test的数值为1
			HAL_Delay(20);     //延迟
			memset(Message_Buf, 0, sizeof(Message_Buf));    //执行完指令 清空指令存贮空间 防止继续执行该指令
		}
		if(strstr((const char*)Message_Buf,"CLOSE"))   //判断到操作指令为CLOSE 
		{
			OneNet_SendData("test",0);  //向平台发数据流test的数值为0
			HAL_Delay(20);   //延迟
			memset(Message_Buf, 0, sizeof(Message_Buf));    //执行完指令 清空指令存贮空间 防止继续执行该指令
		}
//		OneNet_SendData("test",20);
//		len = HTTP_PostPkt(HTTP_Buf, API_KEY, DEV_ID, "temp", tempStr); //HTTP组包;
//		USART2_Write(USART2, (unsigned char *)(HTTP_Buf), len);			//报文发送
//		printf("send HTTP msg:\r\n%s\r\n", HTTP_Buf);
//		USART2_Clear();
    /* USER CODE BEGIN 3 */
		Heart_Pack++;
		HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	
	UNUSED(huart);
	if(huart==&huart2)
	{
//		my_re_buf2 = aRxBuffer;
		usart2_rcv_buf[count++]=aRxBuffer;
				
		if(AT_mode==1)
		{
			Rx_Buff[Rx_count]=aRxBuffer;
			Receive_Normal_Data();
		}
//		HAL_UART_Transmit(&huart1, (uint8_t *)&my_re_buf2, 1,0xff);
//		while(HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY_TX);//检测UART发送结束
		
		HAL_UART_Receive_IT(&huart2, (uint8_t *)&aRxBuffer, 1);
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
