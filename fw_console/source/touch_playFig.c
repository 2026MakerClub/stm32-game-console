#include "touch_module.h"
#include <stdio.h>	// for sprintf

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

void delay_ms(uint32_t wait_ms);
void LCD_Play_Pause(uint8_t md, uint8_t press);
#define LCD_COLOR_LLBLUE     0x071F

#define point_Count  3
#define X_c	160
#define Y_c 162
#define R_c 18
#define x_P (R_c-1)
#define y_P (R_c/2)
Point Points[point_Count]={ 
{(X_c-R_c/2+1), (Y_c-R_c/2)}, 
{(X_c+R_c/2+1), (Y_c)}, 
{(X_c-R_c/2+1), (Y_c+R_c/2)}, 
};

#define  PlayFig_RADIUS        R_c
/* Private macro -------------------------------------------------------------*/
#define  PlayFig_XPOS       X_c
#define  PlayFig_YPOS       Y_c



/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void Touchscreen_Calibration (void);

/**
  * @brief  Touchscreen play & pause
  * @param  None
  * @retval None
  */
void Touchscreen_playFig (void)
{ 
  uint16_t x, y, bar_x;
	uint8_t TS_press;
  uint8_t state = 0;
	TS_StateTypeDef  TS_State;
#define KEY_LONGTIME 10  
	uint16_t KeyTimer = KEY_LONGTIME;  

 	//----------------------------------
re_start:	
  LCD_Clear(LCD_COLOR_WHITE);

	LCD_SetFont(&Font20);
	LCD_SetColors(LCD_COLOR_WHITE, LCD_COLOR_BLUE2); // Text = white; back = blue2
  LCD_DisplayStringLineCol(0, 1,"CAL"); // line 0, column 1

  /*Draw a rectangle with: Start X-Cood,  Start Y-Cood,  Width,  Height*/
#define X_Cood	30	
#define Y_Cood	121
#define bar_Width	240	
#define bar_Height	20	
  LCD_SetTextColor(LCD_COLOR_RED);
	LCD_FillRect(X_Cood, Y_Cood, bar_Width, bar_Height);
	LCD_SetTextColor(LCD_COLOR_BROWN); 
  LCD_DrawRect(X_Cood-1, Y_Cood-1, bar_Width+2, bar_Height+2);			// boundary
#define original_x_end	(X_Cood+bar_Width-1)
	bar_x = original_x_end;
	
	//----------------------------------
 	LCD_Play_Pause(0, 0);

	while(GPIOA->IDR & Bit(0))			// wait until release KEY1
    delay_ms(20);						// wait 20 msec for debouncing

	while (1)
  {
      TS_GetState(&TS_State);
      x = TS_State.x;
      y = TS_State.y;
		  TS_press = TS_State.TouchDetected;

		if(TS_press)
		{
		//###==== Request Calibration		
      if((y > 0)&& (y < (Font20.Height) ))
      {
        if( (x > (Font20.Width*1)) &&	
           (x < (Font20.Width*4) ) )
        {
					LCD_SetColors(LCD_COLOR_BLUE2, LCD_COLOR_LIGHTGRAY); // Text,  back 
					LCD_DisplayStringLineCol(0, 1,"CAL"); // line 0, column 1
			//>>>-------------			
					WaitForTouchRelease(5);
			//<<<-----------------			
					//Touchscreen Calibration
					Touchscreen_Calibration();
					goto re_start;
        }
      }   
		//###<<<===========================

		//###==== Running Bar		
      if((y >= Y_Cood)&& (y < (Y_Cood+bar_Height) ))
      {
        
        if( (x > (bar_x-3)) && (x < (bar_x+3) ) )
        {
					int dif;
					
					LCD_SetTextColor(LCD_COLOR_MAGENTA);
					LCD_FillRect(X_Cood, Y_Cood, bar_x-(X_Cood-1), bar_Height);

					while( (x > (bar_x-3)) &&	(x < (bar_x+3) ) )
					{
			//>>>-------------	
						delay_ms(100);
						TS_GetState(&TS_State);
						if (TS_press == 0) 
						{
							break;
						} 
						
						x = TS_State.x;
						y = TS_State.y;
						
						if((y < Y_Cood)|| (y >= (Y_Cood+bar_Height) )){			// outside of the bar
							break;
						}

					if(x > (original_x_end)){
						x = original_x_end;
					}
					if(x < X_Cood){
						x = X_Cood;
					}
						dif = x- bar_x;
						if (dif > 0){	// If x > bar_x, THEN extend the bar width
							LCD_SetTextColor(LCD_COLOR_MAGENTA);
							LCD_FillRect(bar_x+1, Y_Cood, dif, bar_Height);	
						}else {
							if (dif < 0){	// If x < bar_x, THEN shrink the bar width
								LCD_SetTextColor(LCD_COLOR_WHITE);
								LCD_FillRect(bar_x+1+dif, Y_Cood, -dif, bar_Height);
							}
						}
						bar_x = x;	// update bar_x
			//<<<-----------------			
					}
					LCD_SetTextColor(LCD_COLOR_WHITE);
					LCD_FillRect(bar_x+1, Y_Cood, (original_x_end)-bar_x, bar_Height);
					LCD_SetTextColor(LCD_COLOR_RED);
					LCD_FillRect(X_Cood, Y_Cood, bar_x-(X_Cood-1), bar_Height);
					WaitForTouchRelease(5);		// wait for release
				}
      }   
		//###<<<===========================

			//###==== Play & Pause		
      if((y > (PlayFig_YPOS - PlayFig_RADIUS))&&
           (y < (PlayFig_YPOS + PlayFig_RADIUS)) )
      {
        
        if( (x > (PlayFig_XPOS - PlayFig_RADIUS)) &&	
           (x < (PlayFig_XPOS + PlayFig_RADIUS)) )
        {
          if((state & 1) == 0)		// state=0: PAUSE fig. ==> PLAY fig.
          {
						LCD_Play_Pause(0,1);	// change gray level
			//>>>-------------			
					WaitForTouchRelease(5);	// wait for release
			//<<<-----------------			
						LCD_Play_Pause(1,0);	// change figure
            state = 1;						// update state
						}else										// state=1: PLAY fig. ==> PAUSE fig.
					{
						LCD_Play_Pause(1,1);	// change gray level
			//>>>-------------			
					WaitForTouchRelease(5);	// wait for release
			//<<<-----------------			

						LCD_Play_Pause(0,0);	// change figure
            state = 0;						// update state
					}
        }
      }   //END of  if((y > (PlayFig_YPOS - PlayFig_RADIUS))&&
		//###<<<===========================
		}	//	END of if(TS_press)

	
		if(GPIOB->IDR & (0x01<<8))						// PB8: normally high
    {
			KeyTimer = KEY_LONGTIME;
		} else {
			if (--KeyTimer==0) return;
		}
    
    delay_ms(10);
  }
}


//=========================================================
//------------------------------
// md: 0= Pause figure; 1= Play figure
// p:  0= Released; 1: Pressed
//------------------------------
void LCD_Play_Pause(uint8_t md, uint8_t press)
{
	uint16_t xp, yp, l, i;
	
	LCD_SaveColors();
	if (press == 0){
		LCD_SetTextColor(LCD_COLOR_BLUE2-2);
		LCD_FillCircle(X_c, Y_c, PlayFig_RADIUS);
		LCD_SetTextColor(LCD_COLOR_DARKBLUE);

		xp=X_c-x_P;
		yp=Y_c-2;	
		l= 2*R_c -2;
//		for (i=0; i<(R_c/4+2); i++)
		for (i=0; i<(R_c/4+4); i++)
		{
			uint16_t  cl;
			LCD_DrawHLine(xp, yp++, l);
			LCD_DrawHLine(xp, yp++, l);
			cl= (DARKBLUE)+i*0x0100-i*0x01;
			if(i>2){
				l = l-2;
				xp++;
			cl= (DARKBLUE-0x02)+i*0x0080-i*0x01/2;
			}else{
				cl= (DARKBLUE-0x02)+i*0x0100-i*0x01;
			}
			  LCD_SetTextColor(cl);
		}

			LCD_SetTextColor(LCD_COLOR_WHITE);
		if (md == 0){		// Pause figure
			LCD_FillRect(X_c-(R_c/2),Y_c-(R_c+2)/2,8,R_c+2);
			LCD_FillRect(X_c+(R_c/2)-8,Y_c-(R_c+2)/2,8,R_c+2);
			LCD_SetTextColor(LCD_COLOR_BLUE-4);
			LCD_DrawRect(X_c-(R_c/2),Y_c-(R_c+2)/2,8,R_c+2);
			LCD_DrawRect(X_c+(R_c/2)-8,Y_c-(R_c+2)/2,8,R_c+2);
		}else {		// if md == 1: Play figure
			LCD_FillPolygon(Points,  3);
		}
	}else  // if (press == 1)
	{
		if (md == 0){		// Pause figure
			LCD_SetTextColor(LCD_COLOR_DARKBLUE);
			LCD_FillCircle(X_c, Y_c, 18);
			LCD_SetTextColor(LCD_COLOR_WHITE);
			LCD_FillRect(X_c-(R_c/2),Y_c-(R_c+2)/2,8,R_c+2);
			LCD_FillRect(X_c+(R_c/2)-8,Y_c-(R_c+2)/2,8,R_c+2);
		}else{					// if md == 1: Play figure
			LCD_SetTextColor(LCD_COLOR_DARKBLUE);
			LCD_FillCircle(X_c, Y_c, 18);
			LCD_SetTextColor(LCD_COLOR_WHITE);
			LCD_FillPolygon(Points,  3);
			LCD_FillTriangle(Points);
		}
	}
	LCD_RestoreColors();
}


