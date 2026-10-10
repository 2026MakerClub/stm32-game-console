/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm324xg_lcd_sklin.h"
#include "user_defined.h"
#include <stdio.h>	// for sprintf
#include "touch_module.h"

void stm32f4_Hardware_Init (void);
void Wait_PressPB8(uint16_t Cnum);

void Driver_GPIO(void);

void Default_Calibration(void);
void stm32f4_adc(void);
void	ADC_channelGPIO(void);
void Display_ADC_register(uint32_t Line, uint32_t value);
void Display(uint16_t* pV);
void Draw3Dbox(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);

__WEAK void Enable_LSI(void)
{
     /* Enable the Internal Low Speed oscillator (LSI). */
     RCC->CSR |= 0x01; 										// bit 0 (LSION) = 1; 
     while((RCC->CSR & 0x02) == 0);				// wait until	LSIRDY (bit 2)=1
}
__WEAK void write_access_Backup_domain(void)
{
    /* Enable Power Clock */
		RCC->APB1ENR |= Bit(28);   // bit 28 = PWREN
    
    /* Enable write access to Backup domain */
		//my_Backup_domain_ACCESS;		
		PWR->CR = PWR->CR | Bit(8); // PWR->CR.DBP (bit 8) = 1
    while((PWR->CR & PWR_CR_DBP) == 0);	// wait until PWR->CR.DBP = 1
}
#define Clear_Standby_n_Wakepin_flags (PWR->CR |= (Bit(2) | Bit(3)))  
											// PWR->CSR.CWUF and .CSBF not reset from standby mode
#define RTC_WPR_lock	(RTC->WPR = 0xFF)	// Writing a wrong key reactivates the RTC write protection.
void	PWR_EnterSTANDBYMode(void);
void PWR_EnterSTOPMode(void);	
void	do_STOPMode(void){
			LCD_DisplayOff();		// turn off LCD screen
			PWR_EnterSTOPMode();
			//--- Wake Up already! --//
			LCD_DisplayOn();		// turn on LCD screen
			ADC1->CR2 |= Bit(0);	//bit 0 = ADON
}
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
#define KEY_TIME 10  
	uint16_t KeyTimer = KEY_TIME;  
	uint32_t cnt_adcTrigger;
	uint32_t temp;

	stm32f4_Hardware_Init();

	/* Initialize the LCD */
	LCD_Init();
	LCD_Clear(GREEN);
	LCD_DisplayOn();

	/* Initialize the Touch module */
	Default_Calibration();
	Driver_GPIO();
	//>>>>=========== Low Power Modes Test ==========
	Enable_LSI();
	temp = RCC->CSR;
	temp |=Bit(24);  // bit 24: RMVF= clear the reset flags bits 25~31.
	RCC->CSR = temp;
	
	LCD_SetColors(GREY, LIGHTGRAY);
	if(!(temp & Bit(27))) LCD_DisplayStringLineCol(1, 5, "POR");
	if(!(temp & Bit(26))) LCD_DisplayStringLineCol(1, 10, "PIN");
	if(!(temp & Bit(25))) LCD_DisplayStringLineCol(1, 15, "BOR");
	if(!(temp & Bit(28))) LCD_DisplayStringLineCol(2, 1, "SFT");
	if(!(temp & Bit(29))) LCD_DisplayStringLineCol(2, 6, "IWDG");
	if(!(temp & Bit(30))) LCD_DisplayStringLineCol(2, 12, "WWDG");
	if(!(temp & Bit(31))) LCD_DisplayStringLineCol(2, 18, "LPW");

	LCD_SetColors(YELLOW, BLUE);
	if(temp & Bit(27)) LCD_DisplayStringLineCol(1, 5, "POR");
	if(temp & Bit(26)) LCD_DisplayStringLineCol(1, 10, "PIN");
	if(temp & Bit(25)) LCD_DisplayStringLineCol(1, 15, "BOR");
	if(temp & Bit(28)) LCD_DisplayStringLineCol(2, 1, "SFT");
	if(temp & Bit(29)) LCD_DisplayStringLineCol(2, 6, "IWDG");
	if(temp & Bit(30)) LCD_DisplayStringLineCol(2, 12, "WWDG");
	if(temp & Bit(31)) LCD_DisplayStringLineCol(2, 18, "LPW");

	temp = PWR->CSR;
	Clear_Standby_n_Wakepin_flags;  // set CWUF and CSBF to clear WUF and SBF in PWR->CSR
	//  PWR->CSR.SBF=1 ---> wakeup from STANDBY mode instead of POR
	if (temp & Bit(1))	// PWR->CSR.SBF: standby flag
	{
		LCD_DisplayStringLineCol(4, 2, "Wakeup from STANDBY");		
		if (temp & Bit(0) )   // PWR->CSR.WUF: WKUP pin or RTC Alarm
		{
#define RTC_ISR_bits (Bit(8) | Bit(9) | Bit(10) | Bit(11))
			// RTC Alarm A, Alarm B, auto-wakeup timer, timestamp
			if (!(RTC->ISR & RTC_ISR_bits))	
			{
				LCD_DisplayStringLineCol(5, 2, "by WKUP pin");		
			}else {
				LCD_DisplayStringLineCol(5, 2, "by RTC events");		
			// clear RTC->ISR all flags
				write_access_Backup_domain();
				RTC->ISR = 0;				// rc_w0: clear all RTC flags including WUTF
				RTC_WPR_lock;
			}
		}
	}
  //<<<<============================================
startup:
	while(GPIOA->IDR & 0x01)			// wait until release KEY1
    delay_ms(20);						// wait 20 msec for debouncing

	LCD_SetFont(&Font20);
	LCD_SetColors(RED, BLUE);
	LCD_DisplayStringAt(36, 140, (char*)" Press KEY1   ", LEFT_MODE);
	LCD_DisplayStringAt(36, 160, (char*)" then Release ", LEFT_MODE);

//=================
	Wait_PressPB8(KEY_TIME);	
//====================================
	
//>>>>===== Scinario STOP/STANDBY =======================	
  LCD_Clear(ORANGE);
	LCD_SetColors(BLUE, ORANGE);
	LCD_SaveColors();
	LCD_SetColors(WHITE, BLUE);
	LCD_DisplayStringLineCol(0, 1, "ADC->DR:   ");		
	LCD_DisplayStringLineCol(1, 1, "ADC->JDR1: ");		
	LCD_DisplayStringLineCol(2, 1, "ADC->JDR2: ");		
	LCD_DisplayStringLineCol(3, 1, "ADC->JDR3: ");		
	LCD_DisplayStringLineCol(4, 1, "ADC->JDR4: ");		

	LCD_RestoreColors();
	LCD_DisplayStringLineCol(6, 4, "trig. cnt:");		
	LCD_DisplayStringLineCol(7, 4, "PA7  ADC:");		
	LCD_DisplayStringLineCol(8, 4, "PC2  ADC:");		
	LCD_DisplayStringLineCol(9, 4, "Temp ADC:");		

	//>>>>=========== Low Power Modes Test ==========
	LCD_SetColors(WHITE, MAGENTA);
	LCD_DisplayStringAt(14*18+6, 0, "    ", LEFT_MODE);
	LCD_DisplayStringAt(14*18+6, 20*1, "STOP", LEFT_MODE);
	LCD_DisplayStringAt(14*18+6, 20*2, "    ", LEFT_MODE);
	Draw3Dbox(14*18+6, 0, Font20.Width*4, 20*3);
	
	LCD_SetColors(WHITE, RED);
	LCD_DisplayStringAt(14*18+6, 20*3+6, "    ", LEFT_MODE);
	LCD_DisplayStringAt(14*18+6, 20*4, "SDBY", LEFT_MODE);
	LCD_DisplayStringAt(14*18+6, 20*5, "    ", LEFT_MODE);
	Draw3Dbox(14*18+6, 20*3+6, Font20.Width*4, 20*3-6);
	LCD_RestoreColors();
  //<<<<============================================

	ADC_channelGPIO();
 	stm32f4_adc();

	LCD_SetColors(LCD_COLOR_RED, LIGHTCYAN); // Text,  back 
	LCD_DisplayStringLineCol(10, 8," trigger "); // line 0, column 1
	Draw3Dbox(Font20.Width*8, Font20.Height*10, Font20.Width*9, Font20.Height);
	
	temp = msTick;				// save the last tick
	while(1){
		TS_StateTypeDef  TS_State;
		uint16_t x, y;
		char p_text[6] = "";
		uint16_t val[3];

		TS_GetState(&TS_State);

		if(TS_State.TouchDetected)
		{
			temp = msTick;	// update the last tick
			x = TS_State.x;
			y = TS_State.y;
		//###==== ADC triggered		
      if((y > (Font20.Height)*10)&& (y < (Font20.Height)*11 ))
      {
        if( (x > (Font20.Width*8)) &&	
           (x < (Font20.Width*17) ) )
        {
				//===========  ADC ==============
					ADC1->CR2 |= ADC_CR2_JSWSTART;			// trigger injected ADC
//					ADC1->CR2 |= ADC_CR2_SWSTART;			// trigger regular ADC
				//================================
					LCD_SetColors(LCD_COLOR_BLUE2, LCD_COLOR_LIGHTGRAY); // Text,  back 
					LCD_DisplayStringLineCol(10, 8," trigger "); // line 0, column 1
			//>>>-------------			
					WaitForTouchRelease(5);
					LCD_SetColors(LCD_COLOR_RED, LIGHTCYAN); // Text,  back 
					LCD_DisplayStringLineCol(10, 8," trigger "); // line 0, column 1
					Draw3Dbox(Font20.Width*8, Font20.Height*10, Font20.Width*9, Font20.Height);
			//<<<-----------------			
//				Clear!! 	LCD_DisplayStringLineCol(5, 0,"Time Out! 10s"); 
					LCD_DisplayStringLineCol(5, 0,"             "); 
				
				//===========  ADC ==============
					while (!(ADC1->SR & ADC_SR_JEOC) ); // ADC_SR_JEOC = JEOC (bit 2) = 0
					ADC1->SR &= ~(Bit(2) | Bit(3) );	// clear JSTRT (bit 3) and JEOC (bit 2): rc_w0

					LCD_SetColors(GREEN, BLUE);
					Display_ADC_register(0, ADC1->DR);
					Display_ADC_register(1, ADC1->JDR1);
					Display_ADC_register(2, ADC1->JDR2);
					Display_ADC_register(3, ADC1->JDR3);
					Display_ADC_register(4, ADC1->JDR4);
					LCD_RestoreColors();

					cnt_adcTrigger++;
					sprintf((char*)p_text,"%i ", cnt_adcTrigger);
					LCD_DisplayStringLineCol(6, 14, p_text);		
					val[0] = ADC1->JDR1;
					val[1] = ADC1->JDR2;
					val[2] = ADC1->JDR3;
				/* Display ADC converted value on LCD */
					Display(val);
				//================================			
        }
      } // END of if((y > (Font20.Height)*10)&& (y < (Font20.Height)*11 ))
	//>>>>=========== Low Power Modes Test ==========
			else
			{
				//------- STANDBY MODE
				if( (x > (Font20.Width*18+5)) &&	
           (x < (Font20.Width*22+6) ) )	
				{		
					if((y > (Font20.Height)*3+5)&& (y < (Font20.Height)*6 ))					
					{
						LCD_DisplayOff();		// turn off LCD screen
						PWR_EnterSTANDBYMode();
					}else{
				//------- STOP MODE
						if((y > (Font20.Height)*0)&& (y < (Font20.Height)*3 ))					
						{
							LCD_DisplayStringLineCol(5, 0,"Pressed STOP!"); 
							do_STOPMode();		// enter STOP mode
						}									
					}
				}
			}
  //<<<<============================================
		}			// END of if(TS_State.TouchDetected)
		
		if ((msTick - temp) >10000){				// no action over 10 sec. (temp = tick of last action)
			LCD_SetTextColor(MAGENTA);
			LCD_DisplayStringLineCol(5, 0,"Time Out! 10s"); 
			do_STOPMode();			// enter STOP mode

			temp = msTick;	// update the last tick
					Display_ADC_register(3, (uint16_t) (temp>>16));
					Display_ADC_register(4, (uint16_t) (temp & 0xFFFF));	
		}
		
	}	// END of while(1);

}


	//---- For PB8 as wakeup trigger form STOP -----
void EXTI9_5_IRQHandler(void)
{
		EXTI->IMR &= ~Bit(8);	// disable EXTI 8 line interrupt (PB8 interrupt)
		LCD_SetTextColor(DARKGREEN);
		LCD_DisplayStringLineCol(5, 0,"EXTI9_5 IRQ"); 
		EXTI->PR = Bit(8);   // rc_w1; clear EXTI 8	line interrupt pending
}

	//---- For PA0 as wakeup trigger form STOP -----
/*void EXTI0_IRQHandler(void)
{
		EXTI->IMR &= ~Bit(0);	// disable EXTI 0 line interrupt 
		LCD_SetTextColor(DARKGREEN);
		LCD_DisplayStringLineCol(5, 0,"EXTI0 IRQ"); 
		EXTI->PR = Bit(0);   // rc_w1
}
*/

	//---- For PE4 as wakeup trigger form STOP -----
/* void EXTI4_IRQHandler(void)
{
		EXTI->IMR ?;	// disable EXTI 4 line interrupt (PE4 interrupt)
		LCD_SetTextColor(DARKGREEN);
		LCD_DisplayStringLineCol(5, 0,"EXTI4 IRQ    "); 
		EXTI->PR ?;   // rc_w1; clear EXTI 4 line interrupt pending
}
*/

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


//===============================
void	ADC_channelGPIO(void)
{
/*  --------------------------------------------------------------
    GPIOA: INPUT, analog (no Pull-Up or Pull-Down) 
	  --------------------------------------------------------------
	  | Pin : PA        |    7     
    | ADC channel 7   |   IN7 
	  --------------------------------------------------------------
*/
		RCC->AHB1ENR |=  (1UL << 0);     // enable clock for GPIOB
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
    GPIOA->MODER   = (GPIOA->MODER |  (0x03ul<<(7*2)) ); 		//Input mode (11b)
    GPIOA->PUPDR   = (GPIOA->PUPDR & ~(0x03ul<<(7*2)) );    //no pull-up or pull-down (00b)
    GPIOA->AFR[0]  &= ~(0x0Ful<<7*4);      // AF0 (0000b)
	
/*  --------------------------------------------------------------
    GPIOC: INPUT, analog (no Pull-Up or Pull-Down) 
	  --------------------------------------------------------------
	  | Pin : PC        |    2     
    | ADC channel 12  |   IN12 
	  --------------------------------------------------------------
*/
		RCC->AHB1ENR |=  (1UL << 2);     // enable clock for GPIOC
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
    GPIOC->MODER   = (GPIOC->MODER |  (0x03ul<<(2*2)) ); 		//Input mode (11b)
    GPIOC->PUPDR   = (GPIOC->PUPDR & ~(0x03ul<<(2*2)) );    //no pull-up or pull-down (00b)
    GPIOC->AFR[0]  &= ~(0x0Ful<<2*4);      // AF0 (0000b)
}

//===============================
void Display_ADC_register(uint32_t Line, uint32_t value)
{
	char p_text[7] = "";

		sprintf((char*)p_text,"0x%04X", (value & 0xFFFF));
		LCD_DisplayStringLineCol((uint8_t) Line, 12, p_text);		
}

//===============================
void Display(uint16_t *p_volt)
{
  uint32_t temp, v=0, mv=0, mmv, i;
  char text[50];

	for (i=0; i<3; i++){
    temp = ((uint32_t) p_volt[i]) *3300/0xFFF;
		v =(temp)/1000;
		temp = (temp%1000);
		mv = temp/100;
		mmv = (temp%100)/10;
		sprintf((char*)text,"%d.%d%d V ", v, mv, mmv);
		LCD_DisplayStringLineCol(7+i, 14, text);
	}
}

//===============================
void Draw3Dbox(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
	uint16_t x, y, L, H;
	
	x = Xpos + 1;
	y = Ypos + 1;
	L = Width - 2;
	H = Height - 2;
	LCD_SetTextColor(LIGHTGRAY);
	LCD_DrawHLine(x-1, y, L);
	LCD_DrawVLine(x+L-1, y, H);

	LCD_SetTextColor(GRAY);
	LCD_DrawHLine(x-1, y+H-1, L);
	LCD_DrawVLine(x-1, y+1, H);
	LCD_DrawVLine(x-2, y, H);
}
