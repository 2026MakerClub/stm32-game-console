/* Includes ------------------------------------------------------------------*/
#include "stm32f407xx.h"
#include "stm324xg_lcd_sklin.h"
#include "user_defined.h"
#include <stdio.h>	// for sprintf
#include "touch_module.h"

void stm32f4_Hardware_Init (void);
void Wait_PressPB8(uint16_t Cnum);
void Driver_GPIO(void);

void Default_Calibration(void);
void Touchscreen_demo (void);
void Touchscreen_playFig(void);

void Touchscreen_Calibration (void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
#define KEY_TIME 10  
 uint16_t keyTimer = KEY_TIME;  

	stm32f4_Hardware_Init();
	Driver_GPIO();

	/* Initialize the LCD */
	LCD_Init();
  delay_ms(1000); /* delay 1000 ms */

	/* Initialize the Touch module */
	Default_Calibration();
	
	Touchscreen_Calibration();
	LCD_Clear(GREEN);

startup:
	LCD_SetFont(&Font20);
	LCD_SetColors(RED, BLUE);
	LCD_DisplayStringAt(36, 140, (char*)" Press KEY1   ", LEFT_MODE);
	LCD_DisplayStringAt(36, 160, (char*)" then Release ", LEFT_MODE);

//=================
	Wait_PressPB8(KEY_TIME);	
//====================================
	
//>>>>===== Scinario 1 =======================		
	Touchscreen_demo();	       // see touch_demo.c
	
//>>>>===== Scinario 2 =======================		
  LCD_Clear(WHITE);
//	Touchscreen_playFig();		// see touch_playFig_comp.c

//>>>>===== Scinario 3 =======================		
  LCD_Clear(WHITE);
	LCD_SetTextColor(GREEN);
	LCD_FillRect(142, 144, 36, 36);
	LCD_SetFont(&Font20);
	LCD_SetColors(WHITE, BLUE2); // Text = white; back = blue2
  LCD_DisplayStringLineCol(0, 1,"Touch Screen DEMO"); // line 0, column 1

	LCD_SetColors(RED, WHITE);
	keyTimer = KEY_TIME;
	while(1)
	{
		uint16_t X_t, Y_t;		// touch coordinates in sensor resolution
		char text[64];
		uint16_t x, y;				// LCD coordinates
		
			//>>>-------------
		  if (TSC_TouchDet(1)) {           /* Show touch screen activity         */
					Touch_GetVal(&X_t, &Y_t);   // get x, y from touch module 
					x = X_t;
					y = Y_t;
					Cal_GetXY(&x, &y);
			}
			else {
				x = X_t = 0;
				y = Y_t = 0;
			}
					sprintf(text, "Phy: X=%06d Y=%06d ", X_t, Y_t);
					LCD_DisplayStringAtLine(5, (char *)text);
					sprintf(text, "LCD: x=%06d y=%06d ", x, y);
					LCD_DisplayStringAtLine(6, (char *)text);
			LCD_SetTextColor(GREEN);
			if((y > (143)) && (y < (180)) )
      {
        if( (x > (141)) &&(x < (178)) )
        {
					LCD_SetTextColor(BLUE);
				}
			} 
			LCD_FillRect(142, 144, 36, 36);
			LCD_SetTextColor(RED);

			//<<<-------------
			
			if((GPIOB->IDR & Bit(8)) == 0)				// PB8: normally high
    {
			if (--keyTimer==0){
				LCD_Clear(GREEN);
				goto startup;
			}
		} else keyTimer = KEY_TIME;
			

    delay_ms(10);
   
	}
}

//====================================================
static uint8_t Debounce(uint8_t status, uint8_t pin_no, GPIO_TypeDef *GPIOx)
{
			if ((Bit(pin_no) & GPIOx->IDR )>> pin_no == status){
				delay_ms(4);
				if ((Bit(pin_no) & GPIOx->IDR )>> pin_no == status) return 1;
			}
			return 0;
}

				#define HIGH 1
				#define LOW 0
//===============================
void Wait_PressPB8(uint16_t Cnum)
{
	uint16_t keyTime = Cnum;
	//---- Press -------
	while(keyTime)		
	{	
 			if ( Debounce(LOW, 8, GPIOB) == 1 ) keyTime--; // PB8 = LOW: pressed
			else	keyTime = Cnum;
			delay_ms(10);
	}
	//---- Release -------
	keyTime = 4;
	while(keyTime)		
	{	
 			if ( Debounce(HIGH, 8, GPIOB) == 1 ) keyTime--; // PB8 = HIGH: released
			else	keyTime = 4;
	}
}



