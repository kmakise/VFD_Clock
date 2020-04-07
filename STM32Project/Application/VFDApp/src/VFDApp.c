/**
  ******************************************************************************
  * @file    VFDApp.c
  * @author  张东
  * @version V1.0.0
  * @date    2019-7-5
  * @brief   真空荧光表示器应用程序
  ******************************************************************************
  */
#include "stm32f1xx_hal.h"
#include "VFDApp.h"
#include "tim.h"
//hard ware
#include "oled.h"
#include "DS3231.h"
#include "VFDDriver.h"
#include "keyBoard.h"
#include "bmp.h"

/*Global data space -----------------------------------------------*/

DisModTypedef VFDDisMod = NTIME;
CFGTypedef  SysState = NORMAL;

/*VFD display group -----------------------------------------------*/
void VFDNTIMEDisplay(void)//常规时间显示模式
{
	uint8_t str[7] = {0,0,0,0,0,0};
	
	str[0] = TimeValue.week;
	
	str[1] = TimeValue.hour / 10;
	str[2] = TimeValue.hour % 10;
	str[3] = TimeValue.minute / 10;
	str[4] = TimeValue.minute % 10;
	str[5] = TimeValue.second / 10;
	str[6] = TimeValue.second % 10;
	VFDNumSegment(str);
}

void VFDNDATEDisplay(void)//常规日期显示模式
{
	uint8_t str[7] = {0,0,0,0,0,0};
	
	str[0] = 23;
	
	str[1] = TimeValue.year / 10;
	str[2] = TimeValue.year % 10;
	str[3] = TimeValue.month / 10;
	str[4] = TimeValue.month % 10;
	str[5] = TimeValue.date / 10;
	str[6] = TimeValue.date % 10;
	VFDNumSegment(str);
}
void VFDTEMPDisplay(void)//温度显示模式
{
	uint8_t str[7] = {0,0,0,0,0,0};
	
	str[0] = 23;
	
	str[1] = TimeValue.str[0] - 0x30;
	str[2] = TimeValue.str[1] - 0x30;
	str[3] = TimeValue.str[3] - 0x30;
	str[4] = TimeValue.str[4] - 0x30;
	str[5] = 23;
	str[6] = 0x0C;
	VFDNumSegment(str);
	
}
//下方数字循环
void DownNumDisplay(void)
{
	static uint8_t lastnum = 0;
	static uint8_t lasttime = 0;
	
	if(lastnum != TimeValue.second)
	{
		VFDDownNumCtrl((lastnum - 1) % 20 + 1,0);
		lastnum = TimeValue.second;
	}
	VFDDownNumCtrl((TimeValue.second - 1) % 20 + 1,1);	
	
	if(lasttime == 59 && TimeValue.second == 0)
	{
		for(int i = 1;i < 21;i++)
		{
			VFDDownNumCtrl(i,1);
			HAL_Delay(20);
		}
		for(int i = 1;i < 21;i++)
		{
			VFDDownNumCtrl(i,0);
			HAL_Delay(20);
		}
	}
	lasttime = TimeValue.second;
}

//右侧数字循环
void RightNumdisplay(void)
{
	const uint8_t LISTNUM[6] = {1,2,3,6,5,4};
	static uint8_t  num[6] = {1,2,3,4,5,6};
	static uint32_t lasttick = 0;
	static uint8_t lastsec = 0;
	
	if((HAL_GetTick() - lasttick) > 142)
	{
		lasttick = HAL_GetTick();
		
		for(int i = 0;i < 6;i++)
		{
			num[i] = (num[i] == 5) ? 0 :num[i] + 1;
		}
		
		RightNumCtrl(LISTNUM[num[3]],0,1);
		RightNumCtrl(LISTNUM[num[2]],0,0);
		
		RightNumCtrl(LISTNUM[num[2]],1,1);
//		RightNumCtrl(LISTNUM[num[1]],1,0);
//		
	}
	
	if(lastsec != TimeValue.second)
	{
		
		RightNumCtrl(LISTNUM[TimeValue.second % 6],2,1);
		RightNumCtrl(LISTNUM[lastsec % 6],2,0);
		
		lastsec = TimeValue.second;
	}
}

//获得时间信息
void GetTimeValue(void)
{
	static uint32_t lasttick = 0;
	if((HAL_GetTick() - lasttick) > 100)
	{
		lasttick = HAL_GetTick();
		Time_Handle(); //每秒向DS3231 获取10次时间信息
	}
}
//按键状态显示
void KeyStateDisplay(void)
{
	WordDisCtrl(CCRS,getKey_Ctrl() == 0);
	WordDisCtrl(PLAY,getKey_Ctrl() == 0);
	
	WordDisCtrl(PAUSE,getKey_Ctrl() != 0);
	
	WordDisCtrl(A,getKey_Ctrl() == 1);
	WordDisCtrl(B,getKey_Ctrl() == 2);
	WordDisCtrl(L,getKey_Ctrl() == 3);
	WordDisCtrl(R,getKey_Ctrl() == 4);
	WordDisCtrl(VIDEOCD,getKey_Ctrl() == 5);
}

//分隔符每秒闪烁一次 高低各持续500ms
void SymbolFlash(void)
{
	static uint32_t histTick = 0;
	if((HAL_GetTick() - histTick) <= 500)
	{
		NumSymbolCtrl(0,1);
		NumSymbolCtrl(1,1);
	}
	else if((HAL_GetTick() - histTick) < 1000)
	{
		NumSymbolCtrl(0,0);
		NumSymbolCtrl(1,0);
	}
	else if((HAL_GetTick() - histTick) > 1000)
	{
		histTick = HAL_GetTick();
	}
}
//LED闪烁
void LEDFlash(void)
{
	static uint32_t lasttick = 0;
	
	if((HAL_GetTick() - lasttick) > 5000 && (HAL_GetTick() - lasttick) < 5010)
	{
		GPIOC->BRR = GPIO_PIN_13;
	}
	else if((HAL_GetTick() - lasttick) > 5010)
	{
		lasttick = HAL_GetTick();
	}
	else
	{
		GPIOC->BSRR = GPIO_PIN_13;
	}
}
//VFD 系统状态显示
void VFDStateDisplay(void)
{
	if(VFDDisMod != CLOSE)
	{
		//System state
		WordDisCtrl(PBC,(SysState == NORMAL));
		WordDisCtrl(PROGRAM,(SysState != NORMAL));
		WordDisCtrl(EDIT,(3 < SysState) && (SysState < 6));
		
		//display mode 
		WordDisCtrl(DISCNO,(VFDDisMod  == NTIME));
		WordDisCtrl(TOTAL,(VFDDisMod  == NTIME));
		WordDisCtrl(SINGLE,(VFDDisMod  != NTIME));
		WordDisCtrl(RANDOM,(VFDDisMod  == NTIME));
		
		WordDisCtrl(ENTRY,(VFDDisMod  == NTIME));
		WordDisCtrl(TRACK,(VFDDisMod  == NDATE));
		WordDisCtrl(SPACE,(VFDDisMod  == NDATE));
		WordDisCtrl(ALL,(VFDDisMod  == NDATE));
		
		WordDisCtrl(NO,(VFDDisMod  == NDATE) || (VFDDisMod  == NTIME));
		
		WordDisCtrl(SCENE,(VFDDisMod  == TEMP));
		WordDisCtrl(ONE,(VFDDisMod  == TEMP));
		WordDisCtrl(SEARCH,(VFDDisMod  == TEMP));
		
		//常亮部分
		WordDisCtrl(REPEAT,(VFDDisMod  != CLOSE));
		WordDisCtrl(PC,(VFDDisMod  != CLOSE));
		
	}
}

//LED 小时进位显示
void LEDHourAddFlash(void)
{
	static uint8_t lasthour = 0;
	//LED点亮顺序 34 25 16
	//LED熄灭顺序 34 25 16
	if((lasthour == 9) && ((TimeValue.minute % 10) == 0))
	{
		//点亮
		for(int i = 0; i < 6;i++)
		{
			LEDGroupCtrl(i / 2,1);
			LEDGroupCtrl(5 - (i / 2),1);
			HAL_Delay(50);
		}
		//熄灭
		for(int i = 0;i < 6;i++)
		{
			LEDGroupCtrl(i / 2,0);
			LEDGroupCtrl(5 - (i / 2),0);
			HAL_Delay(50);
		}
	}
	lasthour = TimeValue.minute % 10;
	
}


/*OLED display group------------------------------------------------------*/
void InfoDisplay(void)
{
	static uint32_t lasttick = 0;
	if((HAL_GetTick() - lasttick) > 300)
	{
		lasttick = HAL_GetTick();
		uint8_t str[30];
		uint8_t temp[30];
		//显示时间
		sprintf((char *)str,"Time>:%02d:%02d:%02d",TimeValue.hour,TimeValue.minute,TimeValue.second);
		OLED_ShowString(0,1,(uint8_t *)str,13);
		//显示日期
		sprintf((char *)str,"Date>:20%02d/%02d/%02d",TimeValue.year,TimeValue.month,TimeValue.date);
		OLED_ShowString(0,3,(uint8_t *)str,13);
		//显示温度
		sprintf((char *)str,"Temp>:%c%c.%c%cC",TimeValue.str[0],TimeValue.str[1],TimeValue.str[3],TimeValue.str[4]);
		OLED_ShowString(0,5,(uint8_t *)str,13);
		//显示选项和光标
		sprintf((char *)str,"->Menu");
		OLED_ShowString(80,7,(uint8_t *)str,13);
	}
}



/*Menu and state node Function -------------------------------------------*/
void NormalDisModeChange(void)
{
	static uint8_t lastkey = 0;
	if((lastkey == 0)&&(getKey_Ctrl() != 0))
	{
		switch(getKey_Ctrl())
		{
			case 3: //显示模式正向切换
			{
				VFDDisMod = (VFDDisMod == CLOSE) ? NTIME : VFDDisMod + 1;
				break;
			}
			case 4://显示模式反向切换
			{
				VFDDisMod = (VFDDisMod == NTIME) ? CLOSE : VFDDisMod - 1;
				break;
			}
			case 5://进入显示菜单
			{
				SysState = MAIN;
				while(getKey_Ctrl() == 5);
				OLED_Clear();
				break;
			}
			default:break;
		}
		VFDAllSegment(0);
	}
	lastkey = getKey_Ctrl();
}

void MainFunctionChose(void)
{
	static uint8_t cursor = 0;//光标序号 setdate：0 settime:1 back:2
		
	//显示界面选项和光标
	if(cursor == 0)OLED_ShowString(0,3,(uint8_t *)"->TIME&DATE",13);
	else OLED_ShowString(0,3,(uint8_t *)"  TIME&DATE",13);
	if(cursor == 1)OLED_ShowString(0,5,(uint8_t *)"->WLAN",13);
	else OLED_ShowString(0,5,(uint8_t *)"  WLAN",13);
	//响应按键切换 上+ 下- 左返回 右确认
	switch(getKey_Ctrl())
	{
		case 1://上 移动光标
		{
			cursor = (cursor == 0)?1:0;
			while(getKey_Ctrl()==1)
				KeyStateDisplay();
			break;
		}
		case 3://左 返回
		{
			SysState = NORMAL;
			while(getKey_Ctrl()==3)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
		}
		case 2://下 移动光标
		{
			cursor = (cursor == 0)?1:0;
			while(getKey_Ctrl()==2)
				KeyStateDisplay();
			break;
		}
		case 4://右
		case 5://确认
		{
			switch(cursor)
			{
				case 0:SysState = TIMEDATE;break;
				case 1:SysState = WLAN;break;
			}
			while(getKey_Ctrl()==5 ||getKey_Ctrl()== 4)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
			break;
		}
	}
}
void TimeDateConfig(void)
{
	static uint8_t cursor = 0;//光标序号 setdate：0 settime:1 back:2
		
	//显示界面选项和光标
	if(cursor == 0)OLED_ShowString(0,3,(uint8_t *)"->Set Date",13);
	else OLED_ShowString(0,3,(uint8_t *)"  Set Date",13);
	if(cursor == 1)OLED_ShowString(0,5,(uint8_t *)"->Set Time",13);
	else OLED_ShowString(0,5,(uint8_t *)"  Set Time",13);
	if(cursor == 2)OLED_ShowString(0,7,(uint8_t *)"->Set Day",13);
	else OLED_ShowString(0,7,(uint8_t *)"  Set Day",13);
	//响应按键切换 上+ 下- 左返回 右确认
	switch(getKey_Ctrl())
	{
		case 1://上 移动光标
		{
			cursor = (cursor > 0)?cursor-1:2;
			while(getKey_Ctrl()==1 )
				KeyStateDisplay();
			break;
		}
		case 3://左 返回
		{
			SysState = MAIN;
			while(getKey_Ctrl()==3)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
		}
		case 2://下 移动光标
		{
			cursor = (cursor < 2)?cursor+1:0;
			while(getKey_Ctrl()==2)
				KeyStateDisplay();
			break;
		}
		case 4://右
		case 5://确认
		{
			switch(cursor)
			{
				case 0:SysState = DATE;break;
				case 1:SysState = TIME;break;
				case 2:SysState = DAY;break;
			}
			while(getKey_Ctrl()==5 ||getKey_Ctrl()== 4)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
			break;
		}
	}
}
//设定时间模式
void TimeStateFunc(void)
{
	static uint8_t cursor = 0;
	static uint8_t flag_now = 0;
	static Time_Typedef timeVal;
	uint8_t str[30];
	//当前未载入数据
	if(flag_now == 0)
	{
		flag_now = 1;//使能已载入数据标志位
		memcpy(&timeVal,&TimeValue,sizeof(Time_Typedef));
	}
	//显示日期
	sprintf(str,"    %02d:%02d:%02d",timeVal.hour,timeVal.minute,timeVal.second);
	OLED_ShowString(0,3,(uint8_t *)str,12);
	//显示光标
	switch(cursor)
	{
		case 0:OLED_ShowString(0,4,(uint8_t *)"    ^        ",12);break;
		case 1:OLED_ShowString(0,4,(uint8_t *)"     ^       ",12);break;
		case 2:OLED_ShowString(0,4,(uint8_t *)"       ^     ",12);break;
		case 3:OLED_ShowString(0,4,(uint8_t *)"        ^    ",12);break;
		case 4:OLED_ShowString(0,4,(uint8_t *)"          ^  ",12);break;
		case 5:OLED_ShowString(0,4,(uint8_t *)"           ^ ",12);break;
		default:OLED_ShowString(0,4,(uint8_t *)"             ",12);break;
	}
	if(cursor>5)
	{
		if(cursor == 6)OLED_ShowString(0,7,(uint8_t *)"->Save     Back",12);
		else OLED_ShowString(0,7,(uint8_t *)"  Save   ->Back",12);
	}
	else
	{
		OLED_ShowString(0,7,(uint8_t *)"  Save     Back",12);
	}
	//响应按键切换 上左+ 下右- 确认跳转
	switch(getKey_Ctrl())
	{ 
		case 1://上
		{
			switch(cursor)
			{
				case 0:timeVal.hour = ((timeVal.hour/10) == 2)?timeVal.hour%10:timeVal.hour+10;break;
				case 1:timeVal.hour = ((timeVal.hour%10) == 9)?(timeVal.hour/10)*10:timeVal.hour+1;break;
				case 2:timeVal.minute = ((timeVal.minute/10) == 5)?timeVal.minute%10:timeVal.minute+10;break;
				case 3:timeVal.minute = ((timeVal.minute%10) == 9)?(timeVal.minute/10)*10:timeVal.minute+1;break;break;
				case 4:timeVal.second = ((timeVal.second/10) == 5)?timeVal.second%10:timeVal.second+10;break;break;
				case 5:timeVal.second = ((timeVal.second%10) == 9)?(timeVal.second/10)*10:timeVal.second+1;break;
			}
			HAL_Delay(200);
			break;
		}
		case 2://下
		{
			switch(cursor)
			{
				case 0:timeVal.hour = ((timeVal.hour/10) == 0)?timeVal.hour + 20:timeVal.hour-10;break;
				case 1:timeVal.hour = ((timeVal.hour%10) == 0)?(timeVal.hour/10)*10+9:timeVal.hour-1;break;
				case 2:timeVal.minute = ((timeVal.minute/10) == 0)?timeVal.minute + 50:timeVal.minute-10;break;
				case 3:timeVal.minute = ((timeVal.minute%10) == 0)?(timeVal.minute/10)*10+9:timeVal.minute-1;break;
				case 4:timeVal.second = ((timeVal.second/10) == 0)?timeVal.second + 50:timeVal.second-10;break;
				case 5:timeVal.second = ((timeVal.second%10) == 0)?(timeVal.second/10)*10+9:timeVal.second-1;break;
			}
			HAL_Delay(200);
			break;
		}
		case 3://左
		{
			cursor = (cursor > 0)?cursor-1:7;
			while(getKey_Ctrl()==1 || getKey_Ctrl()== 3)
				KeyStateDisplay();
			break;
		}
		case 4://右
		{
			cursor = (cursor < 7)?cursor+1:0;
			while(getKey_Ctrl()==2 || getKey_Ctrl()== 4)
				KeyStateDisplay();
			break;
		}
		case 5://确认
		{
			switch(cursor)
			{
				case 6://save
				{
					Time_Handle();
					TimeValue.hour = timeVal.hour;
					TimeValue.minute = timeVal.minute;
					TimeValue.second = timeVal.second;
					DS3231_ReadWrite_Time(0);
					SysState = NORMAL;
					flag_now = 0;
					break;
				}
				case 7:SysState = TIMEDATE;flag_now = 0;break;//back
			}
			while(getKey_Ctrl()==5)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
			break;
		}
	}
	//时间检查
	if(timeVal.hour > 23)
	{
		timeVal.hour = 23;
	}
	if(timeVal.minute > 59)
	{
		timeVal.minute = 59;
	}
	if(timeVal.second > 59)
	{
		timeVal.second  = 59;
	}
}
//设定日期模式
void DateStateFunc(void)
{
	static uint8_t cursor = 0;
	static uint8_t flag_now = 0;
	static uint8_t lastkey = 0;
	static Time_Typedef timeVal;
	uint8_t str[30];
	//当前未载入数据
	if(flag_now == 0)
	{
		flag_now = 1;//使能已载入数据标志位
		memcpy(&timeVal,&TimeValue,sizeof(Time_Typedef));
	}
	//显示日期
	sprintf((char *)str,"  20%02d/%02d/%02d",timeVal.year,timeVal.month,timeVal.date);
	OLED_ShowString(0,3,(uint8_t *)str,12);
	//显示光标
	switch(cursor)
	{
		case 0:OLED_ShowString(0,4,(uint8_t *)"    ^        ",12);break;
		case 1:OLED_ShowString(0,4,(uint8_t *)"     ^       ",12);break;
		case 2:OLED_ShowString(0,4,(uint8_t *)"        ^    ",12);break;
		case 3:OLED_ShowString(0,4,(uint8_t *)"          ^  ",12);break;
		case 4:OLED_ShowString(0,4,(uint8_t *)"           ^ ",12);break;
		default:OLED_ShowString(0,4,(uint8_t *)"             ",12);break;
	}
	if(cursor>4)
	{
		if(cursor == 5)OLED_ShowString(0,7,(uint8_t *)"->Save     Back",12);
		else OLED_ShowString(0,7,(uint8_t *)"  Save   ->Back",12);
	}
	else
	{
		OLED_ShowString(0,7,(uint8_t *)"  Save     Back",12);
	}
	if((lastkey != 0) && (getKey_Ctrl() == 0))
	{
	//响应按键切换 上左+ 下右- 确认跳转
		switch(lastkey)
		{ 
			case 1://上
			{
				switch(cursor)
				{
					case 0:timeVal.year = ((timeVal.year/10) == 9)?timeVal.year%10:timeVal.year+10;break;
					case 1:timeVal.year = ((timeVal.year%10) == 9)?(timeVal.year/10)*10:timeVal.year+1;break;
					case 2:timeVal.month = (timeVal.month < 12)? timeVal.month + 1:1;break;
					case 3:timeVal.date = ((timeVal.date/10) == 3)?timeVal.date%10:timeVal.date+10;break;
					case 4:timeVal.date = ((timeVal.date%10) == 9)?(timeVal.date/10)*10:timeVal.date+1;break;
				}
				break;
			}
			case 2://下
			{
				switch(cursor)
				{
					case 0:timeVal.year = ((timeVal.year/10) == 0)?timeVal.year + 90:timeVal.year-10;break;
					case 1:timeVal.year = ((timeVal.year%10) == 0)?(timeVal.year/10)*10+9:timeVal.year-1;break;
					case 2:timeVal.month = (timeVal.month > 1)? timeVal.month - 1:12;break;
					case 3:timeVal.date = ((timeVal.date/10) == 0)?timeVal.date + 30:timeVal.date-10;break;
					case 4:timeVal.date = ((timeVal.date%10) == 0)?(timeVal.date/10)*10+9:timeVal.date-1;break;
				}
				break;
			}
			case 3://左
			{
				cursor = (cursor > 0)?cursor-1:6;
				while(getKey_Ctrl()==1 || getKey_Ctrl()== 3)
					KeyStateDisplay();
				break;
			}
			case 4://右
			{
				cursor = (cursor < 6)?cursor+1:0;
				while(getKey_Ctrl()==2 || getKey_Ctrl()== 4)
					KeyStateDisplay();
				break;
			}
			case 5://确认
			{
				switch(cursor)
				{
					case 5://save
					{
						Time_Handle();
						TimeValue.year = timeVal.year;
						TimeValue.month = timeVal.month;
						TimeValue.date = timeVal.date;
						DS3231_ReadWrite_Time(0);
						SysState = NORMAL;
						flag_now = 0;
						break;
					}
					case 6:SysState = TIMEDATE;flag_now = 0;break;//back
				}
				while(getKey_Ctrl()==5)
					KeyStateDisplay();
				OLED_Clear();
				cursor = 0;
				break;
			}
		}
	}
	lastkey = getKey_Ctrl();
	
	
	//日期检查
	if(timeVal.date > 31)
	{
		timeVal.date = 31;
	}
	if(timeVal.month == 2 && timeVal.date > 29)
	{
		timeVal.date = 29;
	}
	if(timeVal.month == 4||timeVal.month == 6||timeVal.month == 9||timeVal.month == 11)
	{
		if(timeVal.date > 30)
		{
			timeVal.date = 30;
		}
	}
}

void DayStateFunc(void)
{
	static uint8_t flag_now = 0;
	static uint8_t cursor = 0;
	static Time_Typedef timeVal;
	uint8_t str[30];
	//当前未载入数据
	if(flag_now == 0)
	{
		flag_now = 1;//使能已载入数据标志位
		memcpy(&timeVal,&TimeValue,sizeof(Time_Typedef));
	}
	//显示星期
	sprintf(str,"    %02d:%02d:%02d",timeVal.hour,timeVal.minute,timeVal.second);
	sprintf(str,"    Week:%d",timeVal.week);
	OLED_ShowString(0,3,(uint8_t *)str,12);
	//显示光标
	
	if(cursor == 0)
	{
		OLED_ShowString(0,4,(uint8_t *)"         ^   ",12);
	}
	else
	{
		OLED_ShowString(0,4,(uint8_t *)"             ",12);
	}
	if(cursor>0)
	{
		if(cursor == 1)OLED_ShowString(0,7,(uint8_t *)"->Save     Back",12);
		else OLED_ShowString(0,7,(uint8_t *)"  Save   ->Back",12);
	}
	else
	{
		OLED_ShowString(0,7,(uint8_t *)"  Save     Back",12);
	}
	//响应按键切换 上左+ 下右- 确认跳转
	switch(getKey_Ctrl())
	{ 
		case 1://上
		{
			if(cursor == 0)
			{
				timeVal.week = (timeVal.week == 7) ? 1 : timeVal.week + 1;
			}
			HAL_Delay(200);
			break;
		}
		case 2://下
		{
			if(cursor == 0)
			{
				timeVal.week = (timeVal.week == 1) ? 7 : timeVal.week - 1;
			}
			HAL_Delay(200);
			break;
		}
		case 3://左
		{
			cursor = (cursor > 0) ? cursor - 1 :2;
			while(getKey_Ctrl()==1)
				KeyStateDisplay();
			break;
		}
		case 4://右
		{
			cursor = (cursor < 2) ? cursor + 1:0;
			while(getKey_Ctrl()== 4)
				KeyStateDisplay();
			break;
		}
		case 5://确认
		{
			switch(cursor)
			{
				case 1://save
				{
					Time_Handle();
					TimeValue.week = timeVal.week;
					DS3231_ReadWrite_Time(0);
					SysState = NORMAL;
					flag_now = 0;
					break;
				}
				case 3:SysState = TIMEDATE;flag_now = 0;break;//back
			}
			while(getKey_Ctrl()==5)
				KeyStateDisplay();
			OLED_Clear();
			cursor = 0;
			break;
		}
	}
}


/*Setup Loop and Interrupt Function --------------------------------------*/
void VFDApp_Setup(void)
{
	//开始VFD刷新
	HAL_TIM_Base_Start_IT(&htim1);
	
	//启动第一阶段
	OLED_Clear();
	OLED_DrawBMP(0,0,128,8,(uint8_t *)BMP_Start);
	for(int i = 0;i < 10;i++)
	{
		WordDisCtrl(PROGRAM,i%2);
		HAL_Delay(100);
	}
	
	//启动第二阶段
	OLED_Clear();
	OLED_DrawBMP(0,0,128,8,(uint8_t *)BMP_For);
	for(int i = 0;i < 26;i++)
	{
		WordDisCtrl(i,1);
		VFDDownNumCtrl(i - 5,1);
		HAL_Delay(20);
	}
	VFDAllSegment(1);
	for(int i = 0;i < 6;i++)
	{
		LEDGroupCtrl(i,0);
	}
	for(int i = 0;i < 6;i++)
	{
		LEDGroupCtrl(i,1);
		HAL_Delay(100);
	}
	HAL_Delay(500);
	
	
	//启动结束
	OLED_Clear();
	VFDAllSegment(0);
}

void VFDApp_Loop(void) 
{
	//显示模式状态机---------------------------------------------------------/
	switch(VFDDisMod)
	{
		case NTIME:
		{
			VFDNTIMEDisplay();
			DownNumDisplay();
			LEDHourAddFlash();
			RightNumdisplay();
			KeyStateDisplay();
			SymbolFlash();
			break;
		}
		case NDATE:
		{
			VFDNDATEDisplay();
			DownNumDisplay();
			LEDHourAddFlash();
			KeyStateDisplay();
			break;
		}
		case TEMP:
		{
			VFDTEMPDisplay();
			DownNumDisplay();
			LEDHourAddFlash();
			KeyStateDisplay();
			break;
		}
		case CLOSE:
		{
			VFDAllSegment(0); 
			break;
		}
		default:break;
	}
	
	//系统操作状态机--------------------------------------------------------/
	switch(SysState)
	{
		case NORMAL:
		{
			InfoDisplay();//OLED显示
			NormalDisModeChange();		//显示模式切换与主菜单的进入
			break;
		}
		case MAIN:
		{
			MainFunctionChose();	//主菜单选项界面的选项选择
			break;
		}
		case TIMEDATE:
		{
			TimeDateConfig();		//选择需要修改的value
			break;
		}
					case TIME:
					{
						TimeStateFunc();//时间修改
						break;
					}
					case DATE:
					{
						DateStateFunc();//日期修改
						break;
					}
					case DAY:
					{
						DayStateFunc();//星期修改
						break;
					}
		case WLAN:
		{
			SysState = NORMAL;
			break;
		}
					case SSID:
					{
						break;
					}
					case PSWD:
					{
						break;
					}
		default:break;
	}
	
	/*常任务列 -------------------------------------------------------------*/
	VFDStateDisplay();//系统状态显示
	LEDFlash();	//led运行指示灯
	GetTimeValue();//获得RTC世界时间
}

void VFDApp_Interrupt(void)
{
	VFDDsaplayUpdate();
}