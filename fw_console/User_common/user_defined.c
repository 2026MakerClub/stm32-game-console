/* Includes ------------------------------------------------------------------*/
#include "stm32f407xx.h"
#include "user_defined.h"


#define USE_FLASH
#define USE_GPIO_functions
#define USE_EXTI_DEBOUNCE

__inline static void exti_key_debounce(uint32_t N, GPIO_TypeDef* GPIOx);
void HoldCheck_per_4ms(void);

//====================
//  Core Timer (SysTem Ticker) Interrupt Handler
//=========================================================
volatile uint32_t msTick;
#ifndef Call_Back
void SysTick_Handler(void)			// for Targets "FLASH", "OPT", "RAM"
#else
__WEAK void SysTick_Handler(void)		// for Target "callback"
#endif
{
	msTick++;
//-------------------
	#ifdef USE_EXTI_DEBOUNCE
	if ((msTick & 0x03) == 0){  // bit 0 = bit 1 = 0: 4ms
		exti_key_debounce(8, GPIOB);		// pin PB8
//		exti_key_debounce(0 GPIOA);		// pin PA0
//		exti_key_debounce(13, GPIOC);		// pin PC13
		
		HoldCheck_per_4ms();

    //<<<<<<<+++++++++++++++++++++++++++
	} // ENd OF if ((ms_timer & 0x03) == 0)
  #endif
//<------------------
}

void delay_ms(uint32_t wait_ms)
{
    uint32_t Time_last = msTick;
	while ((msTick - Time_last) < wait_ms);
}	

//====================
//  Embedded Flash Interface
#ifdef USE_FLASH
//-----------------------------
#define myHAL_FLASH_TIMEOUT_VALUE  ((uint32_t)50000)  // 50 s
//extern volatile uint32_t msTick;
uint32_t FLASH_check_BUSY_bit(void)  // 0=OK, 3=timeout
{
	if((FLASH->SR & (1ul<<16) ) != 0) // bit 16: BUSY
	{
    uint32_t Time_last = msTick;
		while((FLASH->SR & (1ul<<16) ) != 0)  // bit 16: BUSY
    { 
      if( (msTick - Time_last) >= myHAL_FLASH_TIMEOUT_VALUE)
      {
        return 3;		// time out
      }
	  }
	}
		return 0;		// OK!
}

//=========================================================
uint32_t  User_FLASH_SectorErase(uint32_t beginSector, uint32_t endSector, uint32_t pSize)
{
  uint32_t index = 0, busyStatus, reg_value;
  

  /* Wait for last operation to be completed */
	busyStatus = FLASH_check_BUSY_bit();      // busyState: 0=OK, 3=timeout

  if (busyStatus == 0)
  {
    pSize &= 0x00000003;
		pSize = (1ul<<1) | (pSize<<8)  | (1ul<<16);    //bit 1: FLASH_CR_SER; bits 8~9: FLASH_CR_PSIZE; bit 16: STRT
      /* Erase by sector by sector */
      for(index = beginSector; index <= endSector; index++)
      {
		   reg_value = FLASH->CR & 0xFFFFFC00;	// clear lowest 9 bits
		   reg_value |= (index << 3) | pSize; //bit 1=FLASH_CR_SER; bits 3~6: FLASH_CR_SNB; bits 8~9: CR_PSIZE
		   FLASH->CR = reg_value;         //FLASH_CR_STRT;
    
    /* Wait for last operation to be completed */
			busyStatus = FLASH_check_BUSY_bit();  // busyState: 0=OK,  3=timeout

        if((busyStatus != 0) || (FLASH->SR & (0x05<<4)) )  // SR.4 = WRPERR; SR.6=PGPERR
        {
          break;
        }
      }
		//------ FLASH_clear_REGISTER_CR_9bits;	
		FLASH->CR &= 0xFFFFFC00;	
  }
  return index;	
}

//=========================================================
uint32_t  User_FLASH_beforeWRITE(uint32_t pSize)
{
  uint32_t busyStatus;
  

  /* Wait for last operation to be completed */
	busyStatus = FLASH_check_BUSY_bit();    // busyState: 0=OK,  3=timeout

  if (busyStatus == 0)
  {
  /* If the previous operation is completed, proceed to program the new data */
    pSize &= 0x00000003;
		pSize = (1ul<<0) | (pSize<<8);          //bit 0: FLASH_CR_PG; bits 8~9: FLASH_CR_PSIZE
		pSize |= (FLASH->CR & 0xFFFFFC00);	// clear lowest 9 bits
		FLASH->CR = pSize;
	  // *(__IO uint32_t*)Address = (uint32_t) Data;
  }
		return busyStatus;	
}
#endif  // #ifdef USE_FLASH
//<<<<<<----------------------------

//=========================================================
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//====================
//  External Interrupt Lines
#ifdef USE_EXTI_DEBOUNCE

//=========================================================
#define DEBOUNCE_TIME 3;  //  3 units 
uint8_t	debState[16]={0}, waitCnt[16]={0};

/**
  * @brief  This function handles key debouncing.
  * @param  None
  * @retval : None
  */
void	Debounce(uint32_t N, GPIO_TypeDef* GPIOx)
{
  uint32_t tmp_c, Bit_N=(1ul<<N), debS;

  	debS = debState[N];
	//----------------------------
	  if (waitCnt[N] !=0)	 {
		waitCnt[N] --;
	  } else{	   // when (wait ==0)
		switch(debS)
		{
		 case 2:
		 	tmp_c = GPIOx->IDR & Bit_N; 					// pin PxN
		 	if ( (tmp_c && (EXTI->FTSR & Bit_N) ) ||  		// state return to 1 after debouncing  for falling trigger
		   	 	   ((tmp_c==0) && (EXTI->RTSR & Bit_N) ) )	// state return to 0 after debouncing  for rising trigger
			{
			 debS = 1;
		   	 waitCnt[N]=  DEBOUNCE_TIME;
		 	}
			break;

		 case 1:
		   	 debS = 0;
			 EXTI->PR = Bit_N;    // clear pending interrupt (rc_w1)
			 EXTI->IMR |= Bit_N;  // enable interrupt request for debouncing
			 break;
		}
		debState[N] = debS;
	  }    // END of if (waitCnt[N] !=0)
	//<<<<<<<<<<---------------------
}

/**
  * @brief  This function handles EXTI IRQHandler.
  * @param  None
  * @retval : None
  */
static void handle_EXTI_ISR(uint32_t N, uint32_t State)
{
   uint32_t Bit_N = (1ul<<N);

	EXTI->IMR &= ~Bit_N;  // disable interrupt request during debouncing
	waitCnt[N] = DEBOUNCE_TIME;
	debState[N] = State;
	EXTI->PR = Bit_N;    // clear pending interrupt (rc_w1)
}
					
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/**
  * @brief  This function is called by function SysTick_Handler.
  */
__inline static void exti_key_debounce(uint32_t N, GPIO_TypeDef* GPIOx)
{  
  //>>>>>>>> Keys Debounce
		if (debState[N] != 0 )
		   Debounce(N, GPIOx); 	// pin PxN

  //<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

typedef enum 
{
  IDLE = 0,
  TRIGGER = Bit(0),
  PRESS = Bit(1),
} Button_Hold;

volatile Button_Hold  trig_status;
volatile uint32_t 		MyCounts;
volatile uint8_t 			trig_PASS=0;
uint32_t HoldRequestTime;

//=========================================================
void hold_EXTI_ms(uint32_t wait_ms)
{
	MyCounts = 1+ wait_ms/4;     // in unit of 4ms
	trig_status = TRIGGER;
	trig_PASS = 0;
	HoldRequestTime = MyCounts;
}	

uint8_t get_ButtonHoldState(void)
{
	 return trig_PASS;
}
//===========================
void HoldCheck_per_4ms(void)
{
	if (trig_status & PRESS){  
		 if (--MyCounts == 0){
			trig_PASS = 1;
			trig_status = IDLE; 
//			GPIOx_OUTPUT_resetPINS(LEDport, LEDpin);  // Turn off LED pin
		 } 
	}			// end of 	if (trig_status & PRESS)
}	

/**
  * @brief  This function handles EXTI0_IRQHandler .
  * @param  None
  * @retval : None
  */
void EXTI0_IRQHandler(void)
{
	//---- For PA0 -----
  if( (EXTI->PR & Bit(0)) == 0) return;
  
 	  handle_EXTI_ISR(0, 1);		// Line No. = 0; debState = 1
   
	if (trig_status & TRIGGER)		// have been asking long-hold-button
	{
    //--  task for pressing PA0=1  --/
		if ((GPIOA->IDR & Bit(0)) == 1)  // normally low
		{
			trig_status |= PRESS;
//			GPIOx_OUTPUT_setPINS(LEDport, LEDpin);  // Turn on LED pin
		}else	 
    //--  task for releasing PA0=0  --/
		{
			if (trig_status & PRESS){  
				trig_status = TRIGGER;
				MyCounts = HoldRequestTime;
			}
//			GPIOx_OUTPUT_resetPINS(LEDport, LEDpin);  // Turn off LED pin
		}
	}		
}



/**
  * @brief  This function handles EXTI9_5_IRQHandler .
  * @param  None
  * @retval : None
  */
/*
void EXTI9_5_IRQHandler(void)
{
	//---- For PG8 -----
//  if( (EXTI->PR & Bit(8)) != 0)
//  {
//	  handle_EXTI_ISR(8, 1);		// Line No. = 8; debState = 1
//  
//    //--  task for pressing PG8  --/
//		if ((GPIOG->IDR & Bit(8)) == 0)  // normally high
//		{
//		 trig_instant = msTick;
//		 trigger = 1;
//		}else	 
//    //--  task for releasing PG8  --/
//		{
//		 trig_instant = msTick;
//		 trigger = 0;
//		}
//  }
	//---- For PB8 -----
  if( (EXTI->PR & Bit(8)) == 0) return;
  
	  handle_EXTI_ISR(8, 1);		// Line No. = 8; debState = 1
   
	if (trig_status & TRIGGER)		// have been asking long-hold-button
	{
    //--  task for pressing PB8 = 0  --/
		if ((GPIOB->IDR & Bit(8)) == 0)  // normally high
		{
			trig_status |= PRESS;
//			GPIOx_OUTPUT_setPINS(LEDport, LEDpin);  // Turn on LED pin
		}else	 
    //--  task for releasing PA0=0  --/
		{
			if (trig_status & PRESS){  
				trig_status = TRIGGER;
				MyCounts = HoldRequestTime;
			}
		}
	}		
}
*/

/**
  * @brief  This function handles EXTI15_10_IRQHandler .
  * @param  None
  * @retval : None
  */
/*
void EXTI15_10_IRQHandler(void)
{
  if( (EXTI->PR & Bit(13)) != 0)
  {
 	handle_EXTI_ISR(??, ?);		// Line No. = 13; debState = 2
  
    //--  toggle LED2  --/
//    ??????????????;
  }
}
*/


#endif	//#ifdef USE_EXTI_DEBOUNCE
//<<<--------------------


