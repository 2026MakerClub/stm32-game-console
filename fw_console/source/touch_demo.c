#include "touch_module.h"
#include <stdio.h>	// for sprintf

void delay_ms(uint32_t wait_ms);

#define  CIRCLE_RADIUS        30
/* Private macro -------------------------------------------------------------*/
#define  CIRCLE_XPOS(i)       ((i * LCD_Pixel_Width()) / 5)
#define  CIRCLE_YPOS(i)       (LCD_Pixel_Height() - CIRCLE_RADIUS - 60)



/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void Touchscreen_SetHint(void);
static void Touchscreen_DrawBackground (uint8_t state);


/**
  * @brief  Touchscreen Demo
  * @param  None
  * @retval None
  */
void Touchscreen_demo (void)
{ 
	TS_StateTypeDef  TS_State;
  uint16_t x, y;
  uint8_t TS_press;
  uint8_t state = 0;
#define KEY_LONGTIME 10  
static	uint16_t KeyTimer = KEY_LONGTIME;  

 
  Touchscreen_SetHint();

  Touchscreen_DrawBackground(state);
	while(GPIOA->IDR & 0x01)			// wait until release KEY1
    delay_ms(20);						// wait 20 msec for debouncing

  
  while (1)
  {
      TS_GetState(&TS_State);

      x = TS_State.x;
      y = TS_State.y;
		  TS_press = TS_State.TouchDetected;
		
			if (TS_press)
			{
				char text[64];
				LCD_SaveFont();
				LCD_SaveColors();
				LCD_SetFont(&Font16);
				LCD_SetColors(LCD_COLOR_RED, LCD_COLOR_WHITE); // Text = red; back = white
				sprintf(text, "x:%04d y:%04d (calb. )", x, y);
				LCD_DisplayStringLineCol(13, 2, text);					
				LCD_RestoreColors();
				LCD_RestoreFont();
			}
      
      if((TS_press) && 
         (y > (CIRCLE_YPOS(1) - CIRCLE_RADIUS))&&
           (y < (CIRCLE_YPOS(1) + CIRCLE_RADIUS)))
      {
        
        if( (x > (CIRCLE_XPOS(1) - CIRCLE_RADIUS)) &&	
           (x < (CIRCLE_XPOS(1) + CIRCLE_RADIUS)) )
        {
          if((state & 1) == 0)
          {
            Touchscreen_DrawBackground(state);
            LCD_SetTextColor(LCD_COLOR_BLUE); 
            LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS);
            state = 1;
          }
        }
        if((x > (CIRCLE_XPOS(2) - CIRCLE_RADIUS))&&
           (x < (CIRCLE_XPOS(2) + CIRCLE_RADIUS)))
        {
          if((state & 2) == 0)
          {          
            Touchscreen_DrawBackground(state);
            LCD_SetTextColor(LCD_COLOR_RED); 
            LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS);
            state = 2;
          }          
        }
        
        if((x > (CIRCLE_XPOS(3) - CIRCLE_RADIUS))&&
           (x < (CIRCLE_XPOS(3) + CIRCLE_RADIUS)))
        {
          if((state & 4) == 0)
          {           
            Touchscreen_DrawBackground(state);
            LCD_SetTextColor(LCD_COLOR_YELLOW); 
            LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS);
            state = 4;
          }            
        }
        
        if((x > (CIRCLE_XPOS(4) - CIRCLE_RADIUS))&&
           (x < (CIRCLE_XPOS(4) + CIRCLE_RADIUS)))
        {
          if((state & 8) == 0)
          {           
            Touchscreen_DrawBackground(state);
            LCD_SetTextColor(LCD_COLOR_GREEN); 
            LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS);
            state = 8;
          }           
        }        
      }   
    
		if(GPIOB->IDR & (0x01<<8))						// PB8: normally high
    {
			KeyTimer = KEY_LONGTIME;
		} else {
			if (--KeyTimer==0) return;
		}
    
    delay_ms(10);
  }
}

/**
  * @brief  Display TS Demo Hint
  * @param  None
  * @retval None
  */
static void Touchscreen_SetHint(void)
{
  /* Clear the LCD */ 
  LCD_Clear(LCD_COLOR_WHITE);
  
  /* Set Touchscreen Demo description */
  LCD_SetTextColor(LCD_COLOR_BLUE);
  LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 80);
  LCD_SetTextColor(LCD_COLOR_WHITE);
  LCD_SetBackColor(LCD_COLOR_BLUE); 
  LCD_SetFont(&Font24);
  LCD_DisplayStringAt(0, 0, (char *)"Touchscreen", CENTER_MODE);
  LCD_SetFont(&Font12);
  LCD_DisplayStringAt(0, 30, (char *)"Please use the Touchscreen to", CENTER_MODE);
  LCD_DisplayStringAt(0, 45, (char *)"activate the colored circle", CENTER_MODE);
  LCD_DisplayStringAt(0, 60, (char *)"inside the rectangle", CENTER_MODE);
  
  /* Set the LCD Text Color */
  LCD_SetTextColor(LCD_COLOR_BLUE);  
  LCD_DrawRect(10, 90, BSP_LCD_GetXSize() - 20, BSP_LCD_GetYSize()- 100);
  LCD_DrawRect(11, 91, BSP_LCD_GetXSize() - 22, BSP_LCD_GetYSize()- 102);
   
}

/**
  * @brief  Draw Touchscreen Background
  * @param  state : touch zone state
  * @retval None
  */
static void Touchscreen_DrawBackground (uint8_t state)
{
  
  switch(state)
  {
    
  case 0:
    LCD_SetTextColor(LCD_COLOR_BLUE); 
    LCD_FillCircle( CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS);
    
    
    LCD_SetTextColor(LCD_COLOR_RED); 
    LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS);
    
    
    LCD_SetTextColor(LCD_COLOR_YELLOW); 
    LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS);
    
    
    LCD_SetTextColor(LCD_COLOR_GREEN); 
    LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS);
    
    LCD_SetTextColor(LCD_COLOR_WHITE); 
    LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2);        
    LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2);
    LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2);
    LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2); 
    break;
    
  case 1:
    LCD_SetTextColor(LCD_COLOR_BLUE); 
    LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS);
    LCD_SetTextColor(LCD_COLOR_WHITE); 
    LCD_FillCircle(CIRCLE_XPOS(1), CIRCLE_YPOS(1), CIRCLE_RADIUS - 2);        
    break;
    
  case 2:
    LCD_SetTextColor(LCD_COLOR_RED); 
    LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS);
    LCD_SetTextColor(LCD_COLOR_WHITE); 
    LCD_FillCircle(CIRCLE_XPOS(2), CIRCLE_YPOS(2), CIRCLE_RADIUS - 2); 
    break;
    
  case 4:
    LCD_SetTextColor(LCD_COLOR_YELLOW); 
    LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS);
    LCD_SetTextColor(LCD_COLOR_WHITE); 
    LCD_FillCircle(CIRCLE_XPOS(3), CIRCLE_YPOS(3), CIRCLE_RADIUS - 2); 
    break;
    
  case 8:
    LCD_SetTextColor(LCD_COLOR_GREEN); 
    LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS);
    LCD_SetTextColor(LCD_COLOR_WHITE); 
    LCD_FillCircle(CIRCLE_XPOS(4), CIRCLE_YPOS(4), CIRCLE_RADIUS - 2); 
    break;
    
  }
}
