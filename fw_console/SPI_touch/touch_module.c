
//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
//=================================================
//     <h> Touch Sensor Pin (Board A: PF10; Board B: PB1)
//     <o0>   port label of Touch Sensor Pin 
//						<i> 0: PORTA, 1: PORTB, ..., 10: PORTK
//                 <0=> PORT A 
//                 <1=> PORT B 
//                 <2=> PORT C 
//                 <3=> PORT D 
//                 <4=> PORT E 
//                 <5=> PORT F 
//                 <6=> PORT G 
//                 <7=> PORT H 
//                 <8=> PORT I 
//                 <9=> PORT J 
//                 <10=> PORT K 
//     <o1>  (0~15) Pin number of Touch Sensor Pin <0-15>
//     </h>
//     <q2> Show the coordinates of the touched point
//-------- <<< end of configuration section >>> ---------------
#define Touch_Port 1		// 0
#define Touch_pin	1     // 1
#define ShowTouchValues	0	//2

#if ((Touch_Port == 1) && (Touch_pin	== 1))
#define Board_B
#endif
#if (ShowTouchValues) 
//#include <string.h>
#include <stdio.h>
#endif


#include "stm32f407xx.h"
#include "stm324xg_lcd_sklin.h"

typedef unsigned char       bool;
#define  True  1
#define  False 0

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif
#define GPIO_Touch  ( (GPIO_TypeDef *)(AHB1PERIPH_BASE + 0x0400 * Touch_Port) )
#define TP_INT_IN   ( (GPIO_Touch->IDR & Bit(Touch_pin)) >> Touch_pin )		

#define TouchSenseCount	1
#if (TouchSenseCount == 1)
#define TouchJudge		(!TP_INT_IN)
#else
#define TouchJudge		TSC_TouchDet(TouchSenseCount)
#endif	



uint8_t Calibration_Done=0;
int32_t  X1, Y1, Xx, Yx, Xy, Yy ;

typedef struct
{
  uint16_t TouchDetected;
  uint16_t x;
  uint16_t y;
}TS_StateTypeDef; 


/*-------------------------------------------------------------------------------------------------------
*  SPI driver										 
-------------------------------------------------------------------------------------------------------*/

void Driver_SPICS(bool sta);
bool Driver_SPIMISO(void);
void Driver_SPIMOSI(bool sta);
void Driver_SPISCK(bool sta);

void Delay_SPI(uint16_t Num);
void Touch_SPIWrite(uint8_t Val);
uint16_t Touch_SPIRead(void);
//=================
void Cal_GetXY(uint16_t* pX, uint16_t* pY);


__WEAK void delay_ms(uint32_t wait_ms){
	Delay_SPI(wait_ms);
}

/********************************************************************************************************
*  Function: 			                                                           
*  Object: 
*  Input: pX and pY: pointers of the buffers for X and Y values
*  Output:  							                         	                                     
*  brief:	Get X and Y values of the touched point.
********************************************************************************************************/
#define CMD_RDX 0xD0
#define CMD_RDY 0X90
void Touch_GetVal(uint16_t *pX, uint16_t *pY)
{
    Driver_SPICS(0);				// enable chip by pulling down CS pin
		//--------------------
		Touch_SPIWrite(CMD_RDX);   // send 0xd0 to ask Åª¨úX¶b ÀË´ú­È
    *pX = Touch_SPIRead();	// read data X and save in buffer
		Touch_SPIWrite(CMD_RDY);   // send 0x90 to ask Åª¨úY¶b ÀË´ú­È
    *pY = Touch_SPIRead();	// read data Y and save in buffer
    //--------------------
		Driver_SPICS(1);			// disable chip by pulling up CS pin
}


/********************************************************************************************************
*  Function: TSC_TouchDet		(PF10)			                                                           
*  Object: touch module IRQ
*  Input: none
*  Output: 0/1 							                         	                                     
*  brief:	none                                 
********************************************************************************************************/
uint32_t TSC_TouchDet (uint16_t Cnum) 
{
	uint16_t  x1, y1, x2, y2, count;
	
	count = Cnum;
		//###==== 	
    if( !TP_INT_IN )
    {
			if(--count == 0) return 1;
			while (1)
			{
				Touch_GetVal(&x1, &y1);   // get x, y from touch module 
				delay_ms(10);
				if( TP_INT_IN ) break;
				Touch_GetVal(&x2, &y2);   // get x, y from touch module 
				if ((-3 < (x2-x1) && (x2-x1) < 3)  && (-3< (y2-y1) && (y2-y1) < 3 ) )
				{	
					if(--count == 0) return 1;
				} else {
					break;
				}
			}
 		}
		
		return 0;
}

//==================================================
void TS_GetState(TS_StateTypeDef* pTS_State)
{
		uint16_t x, y, X1, Y1;
		uint8_t TS_press=0;
#if (ShowTouchValues) 
		char text[64];
#endif	

		if (TouchJudge) {           // Show touch screen activity         
				Touch_GetVal(&x, &y);   // get x, y from touch module 
				TS_press = 1;
				X1 = x;
				Y1 = y;
				Cal_GetXY(&x, &y);
		} else {
			x = X1 = 0;
			y = Y1 = 0;
		}
//==========>>>>>>>>>>>>>>			
#if (ShowTouchValues) 
			sprintf(text, "X:%05d Y:%05d P=%01d", X1, Y1, TS_press);
			LCD_SaveFont();
			LCD_SaveColors();
			LCD_SetFont(&Font16);
			LCD_SetColors(RED, WHITE); // Text = red; back = white
			LCD_DisplayStringLineCol(12, 2, text);					
			LCD_RestoreColors();
			LCD_RestoreFont();
#endif
//<<<<==================================
			
			pTS_State->TouchDetected = TS_press;
			pTS_State->x = x;
			pTS_State->y = y;
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
void Touch_sensing(uint16_t Cnum, uint16_t xp, uint16_t yp, uint16_t width, uint16_t highth)
{
	uint16_t x, y, count;
	
	count = Cnum;
	while (1)
  {
		if (TouchJudge) {           // Show touch screen activity         
				Touch_GetVal(&x, &y);   // get x, y from touch module 
				Cal_GetXY(&x, &y);
      if((y >= yp)&& (y < yp+highth) )
      {
        if( (x >= xp) &&	(x < xp+width) )
        {
					if(--count == 0) return;
        }
      }   
 		} else
		{
			count = Cnum;
		}
		delay_ms(10);
	}
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
uint8_t Touch_sensing_2(uint16_t Cnum, uint16_t xp, uint16_t yp, uint16_t width, uint16_t highth)
{
	uint16_t x, y, count;
	uint16_t cnt2;
	
	count = Cnum;
	while (1)
  {
		if (TouchJudge) {           // Show touch screen activity         
				Touch_GetVal(&x, &y);   // get x, y from touch module 
				Cal_GetXY(&x, &y);
      if((y >= yp)&& (y < yp+highth) )
      {
        if( (x >= xp) &&	(x < xp+width) )
        {
					if(--count == 0) return 1;
        }
      } else break;  
 		} else break;
		
		delay_ms(10);
	}
	return 0;
}

/**
  * @brief  
  * @param  None
  * @retval None
  */
void WaitForTouchRelease(uint32_t Cnum) 
{
	uint32_t count;
	
	count = Cnum;
  do 
  {
    delay_ms(1);
		if (TouchJudge == 0)
    {
      if(--count == 0) return;
		} else
		{
			count = Cnum;
		}
  } while (1);
}

#define TouchValue_16bits
#ifdef	TouchValue_16bits
	#define Q_format 16384 // Q14
//	#define Q_format (1ul<<20) // Q20
	#define timesV	16
#else			// TouchValue_12bits
	#define Q_format 1024 // Q10
	#define timesV	1
#endif
/**
  * @brief  Calibrate X and Y position
  * @param  x, y : X, Y touch position
  * @retval calibrated y = (Y1 + Yy*y) / Q_format; 
  */
void Cal_GetXY(uint16_t* pX, uint16_t* pY)
{
  int32_t x, y, temp;
//----- 3 parameters calibration
	x = *pX;
	y = *pY;
	temp = (X1 + (Xx * x) + (Xy * y))/(Q_format);
	y = (Y1 + (Yy * y) + (Yx * x))/(Q_format);
	x = temp;
//<<<=============================
	if (x < 0) x = 0;
	else {
		temp = LCD_Pixel_Width();
		if (x > temp) x = temp; 
	}
	if (y < 0) y = 0;
	else {
		temp = LCD_Pixel_Height();
		if (y > temp) y = temp; 
	}

	*pX = x;
	*pY = y;
}

//=====================================
//=====================================
void Default_Calibration(void)
{
  if (LCD_Pixel_Width() == 319)
	{
				// X axis
				X1 = 5500741;
				Xx = 0;
				Xy = -90;
				// Y axis
				Y1 = 4123068;
		    Yx = -64;
				Yy = 0;
	} else 
	{
				// X axis
				X1 = -12525*timesV;
				Xx = 65;
				Xy = 0;
				// Y axis
				Y1 = -33962*timesV;
		    Yx = 0;
				Yy = 91;
	}
}
