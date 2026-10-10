/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "myPower.h"
#include "myRTC.h"
#include "user_defined.h"
//#define _RAM_Debug	

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
//=========================================================================== External interrupt/event Configuration
// <e0.0> STOP mode: Available
//     <o0.1> Wake up type (interrupt or event)
//        <i> __WFI = Request Wait For Interrupt
//        <i> __WFE = Request Wait For Event
//          <0=>         __WFI
//          <1=>         __WFE
//     <q0.8> ADCx->CR2.ADON: Disable ADCs' conversion before entering STOP mode
//     <q0.9> DAC->CR.[EX1,EX2]: Disable DAC two channels before entering STOP mode
//     <e0.12> Configure all GPIO as analog to reduce current consumption
//     			<q0.13> Retain DEGUG pins for JTAG or SW
//     			<q0.14> stm32f4_GPIO_Init(): After wake-up, reconfigure GPIO by calling stm32f4_GPIO_Init() which is in GPIO_Ini.c
//		 </e>
//     <q0.16> EXTI->IMR = EXTI->EMR = 0: Clear requests of all EXTI interrupts and events
//     <e0.19> Use a "new" EXTI to trigger WakeUp from STOP mode
//     		<o1.0..7> EXTI line No.
//          <0=>         pin 0
//          <1=>         pin 1
//          <2=>         pin 2
//          <3=>         pin 3
//          <4=>         pin 4
//          <5=>         pin 5
//          <6=>         pin 6
//          <7=>         pin 7
//          <8=>         pin 8
//          <9=>         pin 9
//          <10=>         pin 10
//          <11=>         pin 11
//          <12=>         pin 12
//          <13=>         pin 13
//          <14=>         pin 14
//          <15=>         pin 15
//          <16=>         PVD
//          <17=>         RTC ALARM
//     		<o1.8> Interrupt (IMR) or Event (EMR) mode
//          <0=>         IMR
//          <1=>         EMR
//        <o1.9> use Rising trigger for interrupt/event
//        <o1.10> use Falling trigger for interrupt/event
//        <o1.12..15> use Port for EXTI line (ignored for line no. > 15)
//          <0=>         pin at PA
//          <1=>         pin at PB
//          <2=>         pin at PC
//          <3=>         pin at PD
//          <4=>         pin at PE
//          <5=>         pin at PF
//          <6=>         pin at PG
//          <7=>         pin at PH
//          <8=>         pin at PI
//        <o1.16> pull-up or pull-down of the EXTI pin
//          <0=>         pull-down
//          <1=>         pull-up
//    </e>
//   <h> uint32_t "STOP_PWR_CR_Regulator": setting parameter before entering STOP mode
//     <q2.0> PWR->CR.LPDS: Voltage regulator in low-power mode during Stop mode
//     <q2.9> PWR->CR.FPDS: Flash ROM power-down during Stop mode (ignored in normal running)
//          <i>  can be set in the initialization stage using
//          <i> 	"FlashPowerDown_Set_Reset(1);" see myPower.h
//   </h>
// </e>
//--------------------------------------------------------------------------- EXTI line 17
// <e3.0> STANDDBY mode: Available
//     <q3.4> PWR->CSR.EWUP: WKUP (PA0) enabled 
//        <i> WKUP pin (PA0) can wake up the system from STANDBY mode.
//     <e3.8> Use RTC periodic auto Wakeup
//        <q3.9> Setup RTC initialization (in "myRTC.c")
//        <o4> Auto WakeUp period in "usec" (500~31981080 usec)  <500-31981080> <#/488>
//				 <i> WakeUp Count (16 bits) = 1,000,000 * (waiting sec) /488, 
//         <i> Ex. 1: count = 65535 = 0xFFFF ==>31.981s.,
//         <i> because 65535 * 488 = 31981080 usec
//         <i> Ex. 2: 10s.==> count = 1M * 10/488=20491.8,
//         <i> and 20491 * 488 = 9999608 usec (about 10 sec)
//     </e>
// </e>
//--------------------------------------------------------------------------- EXTI line 17
//--------------------------------------------------------------------------- EXTI line 17
#define __STOP_SETUP             		0x92303  		// 0 ==> bit 1: 0=__WFI; 1=__WFE;  
#define __STOP_EXTI             		0x11508  		// 1 ==> bit 8: 0=IMR; 1=EMR; 
const uint32_t STOP_PWR_CR_Regulator = 0x201;		// 2 ==> bit0 = LPDS; bit9 = FPDS;
#define __STANDDBY_SETUP            0x28050311  // 3                
#define __Auto_WakeUp_Period            20491   // 4 ==> #/488                 

//======= EWUP = 0: PA0 is a GPIO only.	
//======= EWUP = 1: Rising edge on WKUP pin (PA0) wakes up the system from STANDBY mode.	
//----- PWR->CSR.EWUP (bit 8) = 1 --------	
#define PWR_EnableWakeUpPin 	(*(uint32_t *) CSR_EWUP_BB = 1)	
//----- PWR->CSR.EWUP (bit 8) = 0 --------	
#define PWR_DisableWakeUpPin 	(*(uint32_t *) CSR_EWUP_BB = 0)

//########################################################
//    All functions needed for STOP Mode
//########################################################
#if (__STOP_SETUP & Bit(0))
//==================================
  // @brief Configure all GPIO as analog to reduce current consumption
//==============================
#if (__STOP_SETUP & Bit(12)) 
void stm32f4_GPIO_Init (void);

__inline static void Configure_all_GPIO_as_analog(void)
//void Configure_all_GPIO_as_analog(void)
{	
	int i; 
	uint32_t 	p_address;

//  GPIOx  = (GPIO_TypeDef *)(GPIOA_BASE);  
  p_address  = (uint32_t)(AHB1PERIPH_BASE);  

 /* Enable GPIOs clock */
	RCC->AHB1ENR |= 0x01FF;	// bits 0~8: GPIOAEN~GPIOIEN

	/* Configure all GPIO as analog */
	for ( i=0; i<9; i++)
	{
	GPIO_TypeDef *GPIOx;

		GPIOx = (GPIO_TypeDef *) p_address;				
#if (__STOP_SETUP & Bit(13)) 
		if (i==0){   // GPIOA
			GPIOA->MODER   |= 0x03FFFFFF; // PA13~PA15 as JTAG pins
			GPIOA->PUPDR   &=  0xFC000000; // PA13~PA15 (up, down, up) as JTAG pins
		} 
			else { 
			if (i == 1){ // GPIOB	
				GPIOB->MODER   |= 0xFFFFFC3F; // PB3~PB4 as JTAG pins
				GPIOB->PUPDR   &= 0x000003C0; // PB3~PB4 (no, up) as JTAG pins
			}
			else {
				GPIOx->MODER = 0xFFFFFFFF;		// 0x3: analog input
				GPIOx->PUPDR = 0x00000000;		// 0x0: no pull-up or pull-down
			}
		}
#else
				GPIOx->MODER = 0xFFFFFFFF;		// 0x3: analog input
				GPIOx->PUPDR = 0x00000000;		// 0x0: no pull-up or pull-down
#endif
		p_address += 0x0400;				// address increment 0x0400;
	}
	
 /* Disable GPIOs clock */
#if (__STOP_SETUP & Bit(13)) 
	RCC->AHB1ENR &= ~0x01FC;	// bits 2~8: GPIOCEN~GPIOIEN
#else
	RCC->AHB1ENR &= ~0x01FF;	// bits 0~8: GPIOAEN~GPIOIEN
#endif
}
#endif	//(__STOP_SETUP & Bit(16)) 

//==================================
  // @brief Use a new  EXTI to trigger WakeUp from STOP mode
//==============================
#if (__STOP_SETUP & Bit(19)) 
/*----------------------------------------------------------------------------
 initializes the EXTI register for stop wake-up pin
 *----------------------------------------------------------------------------*/
__inline static void STOP_newExtiSetup (void) {                                                   
	GPIO_TypeDef *GPIOx;
	uint32_t temp;
	
#define	i_pin  (__STOP_EXTI & 0x00FF)
#if (i_pin < 15)	
#define	i_portNo  ( (__STOP_EXTI & 0xF000)>>12 )
		RCC->AHB1ENR |=  (1UL << i_portNo );     // bits 0~10: enable clock for GPIOx
    GPIOx  = (GPIO_TypeDef *)(AHB1PERIPH_BASE + 0x0400 * i_portNo);
	  GPIOx->MODER &= ~(0x03<<(i_pin*2));		// 0x0:  input
		temp = GPIOx->PUPDR & (~(0x03<<(i_pin*2)) );		// 0x0: no pull-up or pull-down
		if ( __STOP_EXTI & Bit(16) ){		// bit 16: 0=pull-down; 1=pullup
			temp |= (0x01<<(i_pin*2));		// //pull-up (01b)
		} else {
			temp |= (0x02<<(i_pin*2));		// //pull-down (10b)
		}
		GPIOx->PUPDR = temp;
	
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;    // enable clock for System Configuration
  temp = SYSCFG->EXTICR[(i_pin>>2)];
	temp &= ~(0x0F<<((i_pin & 0x03)*4));
	temp |= i_portNo<<((i_pin & 0x03)*4);    // select port for pin to use
	SYSCFG->EXTICR[(i_pin>>2)] =  temp;    // select port for pin to use
#endif

#if (!(__STOP_EXTI & Bit(8)))	
    EXTI->IMR       |= Bit(i_pin);             // unmask interrupt
#else
		EXTI->EMR       |= Bit(i_pin);             // unmask event
#endif	
/* Alias word address of EXTI->RTSR, FTSR bit in CC->APB1ENR register */
#define EXTI_OFFSET       ((APB2PERIPH_BASE + 0x3C00)-0x40000000)        // EXTI @ 0x40013C00
#define RTSR_OFFSET       (EXTI_OFFSET +0x08)         // EXTI->RTSR
#define FTSR_OFFSET       (EXTI_OFFSET + 0x0C)        // EXTI->FTSR
#define EXTI_RTSR_line_BB       (PERIPH_BB_BASE + (RTSR_OFFSET * 32) + (i_pin * 4))
#define EXTI_FTSR_line_BB       (PERIPH_BB_BASE + (FTSR_OFFSET * 32) + (i_pin * 4))
		*(uint32_t *) EXTI_RTSR_line_BB =(__STOP_EXTI & Bit(9))>>9;   // set/reset rising edge
    *(uint32_t *) EXTI_FTSR_line_BB =(__STOP_EXTI & Bit(10))>>10;   // set/reset falling edge
#if (i_pin == 0)
NVIC->ICPR[20>>5]=Bit(20);
      NVIC->ICPR[0]  = (1 << (EXTI0_IRQn & 0x1F));    // clear pending flag EXTI 0
      NVIC->ISER[0]  = (1 << (EXTI0_IRQn & 0x1F));    // enable interrupt EXTI 0
#endif
#if (i_pin == 1)
      NVIC->ICPR[0]  = (1 << (EXTI1_IRQn & 0x1F));    // clear pending flag EXTI 1
      NVIC->ISER[0]  = (1 << (EXTI1_IRQn & 0x1F));    // enable interrupt EXTI 1
#endif
#if (i_pin == 2)
      NVIC->ICPR[0]  = (1 << (EXTI2_IRQn & 0x1F));    // clear pending flag EXTI 2
      NVIC->ISER[0]  = (1 << (EXTI2_IRQn & 0x1F));    // enable interrupt EXTI 2
#endif
#if (i_pin == 3)
      NVIC->ICPR[0]  = (1 << (EXTI3_IRQn & 0x1F));    // clear pending flag EXTI 3
      NVIC->ISER[0]  = (1 << (EXTI3_IRQn & 0x1F));    // enable interrupt EXTI 3
#endif
#if (i_pin == 4)
      NVIC->ICPR[0]  = (1 << (EXTI4_IRQn & 0x1F));    // clear pending flag EXTI 4
      NVIC->ISER[0]  = (1 << (EXTI4_IRQn & 0x1F));    // enable interrupt EXTI 4
#endif
#if ((i_pin >= 5) && ((i_pin <= 9)))
      NVIC->ICPR[0]  = (1 << (EXTI9_5_IRQn & 0x1F));  // clear pending flag EXTI 9..5
      NVIC->ISER[0]  = (1 << (EXTI9_5_IRQn & 0x1F));  // enable interrupt EXTI 9..5
#endif
#if ((i_pin >= 10) && ((i_pin <= 15)))
      NVIC->ICPR[1]  = (1 << (EXTI15_10_IRQn & 0x1F));// clear pending flag EXTI 10..15
      NVIC->ISER[1]  = (1 << (EXTI15_10_IRQn & 0x1F));// enable interrupt EXTI 10..15
#endif
#if (i_pin == 16)
      NVIC->ICPR[1]  = (1 << (PVD_IRQn & 0x1F));    // clear pending flag PVD
      NVIC->ISER[1]  = (1 << (PVD_IRQn & 0x1F));    // enable interrupt PVD
#endif
#if (i_pin == 17)
      NVIC->ICPR[1]  = (1 << (RTC_Alarm_IRQn & 0x1F));    // clear pending flag RTC Alarm Event
      NVIC->ISER[1]  = (1 << (RTC_Alarm_IRQn & 0x1F));		// enable interrupt RTC Alarm Event
#endif
} // end of STOP_newExtiSetup
#endif	// (__STOP_SETUP & Bit(19)) 

//########################################################
//__INLINE static void Leave_STOPmode(uint32_t s_PLL_HSEON);
/*
  * @brief Leave Stop mode after Wake Up. 
  * @brief  Configures system clock after wake-up from STOP: enable HSE, PLL
  *         and select PLL as system clock source.
	* @param s_PLL_HSEON: 
  *            @arg bit 0: enable HSE (external high frequency oscillator )
  *            @arg bit 1: use PLL
  * @retval None
  */
//------------------------------
//  Alias word address of HSEON bit in RCC->CR register  // RCC_BASE = 0x4002 3800 =(AHB1PERIPH_BASE + 0x3800ul)
#define RCC_CR_OFFSET       (RCC_BASE-0x40000000)        // RCC->CR @ 0x40023800 + 0x00
#define HSEON_BitNr   16
#define PLLON_BitNr   24
#define RCC_CR_HSEON_BB       (PERIPH_BB_BASE + (RCC_CR_OFFSET * 32) + (HSEON_BitNr * 4))
#define RCC_CR_PLLON_BB       (PERIPH_BB_BASE + (RCC_CR_OFFSET * 32) + (PLLON_BitNr * 4))
//==========================================
__INLINE static void	Leave_STOPmode(uint32_t s_PLL_HSEON)
{
	/* Reset SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR &= ~Bit(2);  // bit 2: SCB.SLEEPDEEP = 0

		//--- Set HSE on
      if(s_PLL_HSEON & Bit(0)) 
      {
				*(uint32_t *) RCC_CR_HSEON_BB = 1;	// RCC->CR.HSEON = 1
        /* Wait till HSE is ready */  
        while((RCC->CR & Bit(17)) == 0);		// bit 16: HSERDY
      }
	
		//--- Set PLL on
      if(s_PLL_HSEON & Bit(1)) 
      {
				*(uint32_t *) RCC_CR_PLLON_BB = 1;	// RCC->CR.PLLON = 1
        /* Wait till PLL is ready */  
        while((RCC->CR & Bit(25)) == 0);		// bit 25: PLLRDY
				s_PLL_HSEON = 0x02;									// SW = 0x02; 
      }

	  // Set system clock source (SW) and Wait till SYSCLK is stabilized (depending on selected clock) 
			RCC->CFGR |= (s_PLL_HSEON & 0x03);				// set RCC->CFGR.SW
			while ((RCC->CFGR & RCC_CFGR_SWS) != (s_PLL_HSEON<<2) );
}
/*
  * @brief Enter Stop mode. 
  * @note In Stop mode, all I/O pins keep the same state as in Run mode.
  * @note When exiting Stop mode by issuing an interrupt or a wakeup event, 
  *         the HSI RC oscillator is selected as system clock.
  * @note When the voltage regulator operates in low power mode, an additional 
  *         startup delay is incurred when waking up from Stop mode. 
  *         By keeping the internal regulator ON during Stop mode, the consumption 
  *         is higher although the startup time is reduced.    
	* @param Regulator: 
  *            @arg PWR_MAINREGULATOR_ON: Stop mode with regulator ON
  *            @arg PWR_LOWPOWERREGULATOR_ON: Stop mode with low power regulator ON
  * @param (eliminated) STOPEntry: 
  *            @arg PWR_STOPENTRY_WFI: Enter Stop mode with WFI instruction
  *            @arg PWR_STOPENTRY_WFE: Enter Stop mode with WFE instruction
  * @retval None
  */
void PWR_EnterSTOPMode(void)
{
  uint32_t tmpreg;
#if (__STOP_SETUP & Bit(16))	
    /* save IMR and EMR if clear all IMR and EMR before STOP*/
	uint32_t	s_IMR, s_EMR;
#endif
  uint32_t s_PLL_HSEON;
	//---------------------------
	// SWS: bits 2~3
	// 00: HSI oscillator selected as system clock
	// 01: HSE oscillator selected as system clock
	// 10: PLL selected as system clock
	//------------
	// PLLSRC: bit 22
	// 0: HSI clock selected as PLL and PLLI2S clock entry
	// 1: HSE oscillator clock selected as PLL and PLLI2S clock entry
	s_PLL_HSEON = (RCC->CFGR  &  RCC_CFGR_SWS)>>2; // (bits 2~3)>>2
	s_PLL_HSEON |= (RCC->PLLCFGR  &  RCC_PLLCFGR_PLLSRC)>>22; // bit 22
	
	//>>>>>>>>>>>>>>>>>-----Additional Consideration -------------
#if (__STOP_SETUP & Bit(8)) 
    /* Disable ADCs' conversion by reseting ADON = 0 */
		ADC1->CR2 &= ~(Bit(0));	//bit 0 = ADON
		ADC2->CR2 &= ~(Bit(0));	//bit 0 = ADON
		ADC3->CR2 &= ~(Bit(0));	//bit 0 = ADON
#endif
#if (__STOP_SETUP & Bit(9)) 
    /* Disable DAC channels by reseting EX1= EX2 = 0 */
		DAC->CR &= ~(Bit(0)|Bit(16));	//bit 0 = EX1; bit 16 = EX2
#endif

#if (__STOP_SETUP & Bit(12)) 
    /* configure all GPIO as anlog inputs */
	Configure_all_GPIO_as_analog();
#endif

#if (__STOP_SETUP & Bit(16))	
    /* Clear all IMR and EMR before STOP*/
    s_IMR = EXTI->IMR;             // save EXTI->IMR
		s_EMR = EXTI->EMR;             // save EXTI->EMR
    EXTI->IMR =0;             // mask all interrupts
		EXTI->EMR =0;             // mask all events
#endif	

#if (__STOP_SETUP & Bit(19)) 
    /* Use a new  EXTI to trigger WakeUp from STOP mode */
	STOP_newExtiSetup();
#endif
	//<<<<<<<<<<<<<<<<<--------------------------
	
  /* To enter the Stop mode, all EXTI Line pending bits must be reset */
	EXTI->PR = 0x00FFFFFF; 		// rc_w1
	
  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR |= Bit(2);			// bit 2 = SCB_SCR_SLEEPDEEP_Msk;
  
  /* Select the regulator state in Stop mode ---------------------------------*/
  tmpreg = PWR->CR;
  /* Clear PDDS and LPDS bits */
  tmpreg &= ~(Bit(1) | Bit(0) |Bit(9));    // (PWR_CR_PDDS | PWR_CR_LPDS | PWR_CR_FPDS);
  /* Set LPDS, VOS and FPVDS bits according to Regulator value */
  tmpreg |= (STOP_PWR_CR_Regulator & (Bit(0)|Bit(9)));		// [PDDS:LPDS] = 01 or 00
	// STOP_PWR_CR_Regulator: bit0 = LPDS, bit9 = FPDS, bit1=PDDS always 0.
/* Store the new value */
  PWR->CR = tmpreg;
  
#ifndef _RAM_Debug	
  /* Stop mode entry --------------------------------------------------*/
#if (__STOP_SETUP & Bit(1)) 
    /* Request Wait For Event */
		__ASM volatile ("wfe");	// __WFE();
#else
    /* Request Wait For Interrupt */
		__ASM volatile ("wfi"); // __WFI();
#endif
	// z z z Z Z Z 
#endif	// #ifndef _RAM_Debug	
	
	// Wake Up from STOP mode
	/* clear all EXTI pending flags to stop EXTI interrupts */
	EXTI->PR = 0x00FFFFFF; 		// rc_w1
	Leave_STOPmode(s_PLL_HSEON);


	//>>>>>>>>>>>>>>>>>-----Additional Consideration -------------
#if (__STOP_SETUP & Bit(16))	
	/* Case: Clear all IMR and EMR before STOP*/
    EXTI->IMR =s_IMR;             // restore EXTI->IMR
		EXTI->EMR =s_EMR;             // restore EXTI->EMR
#else
  #if (__STOP_SETUP & Bit(19)) 
	/* Case:  Use a new  EXTI to trigger WakeUp from STOP mode */
   #if ((__STOP_EXTI & Bit(8)) == 0)	
    EXTI->IMR       &= ~(1UL << (__STOP_EXTI & 0x00FF) );             // mask interrupt
   #else
		EXTI->EMR       &= ~(1UL << (__STOP_EXTI & 0x00FF) );             // mask event
   #endif	
  #endif
#endif //(__STOP_SETUP & Bit(16))	

#if (__STOP_SETUP & Bit(12)) // if Configure_all_GPIO_as_analog before STOP
 #if (__STOP_SETUP & Bit(14)) // if call "stm32f4_GPIO_Init ()" after wake-up from STOP
	stm32f4_GPIO_Init ();
 #endif
#endif
	//<<<<<<<<<<<<<<<<<--------------------------
}

//<<<<######################################################
//   END of  "All functions needed for STOP Mode"
//<<<<####################################################
#endif // (__STOP_SETUP & Bit(0))		// Line 102 to Line 417


//>>>>######################################################
//   "All functions needed for STANDBY Mode"
//>>>>####################################################
/**
  * @brief Enter Standby mode.
  * @note In Standby mode, all I/O pins are high impedance except for:
  *          - Reset pad (still available) 
  *          - RTC_AF1 pin (PC13) if configured for tamper, time-stamp, RTC 
  *            Alarm out, or RTC clock calibration out.
  *          - RTC_AF2 pin (PI8) if configured for tamper or time-stamp.  
  *          - WKUP pin 1 (PA0) if enabled (PWR->CRS.EWUP = 1).       
  * @param None
  * @returnValue None
  */
#if (__STANDDBY_SETUP & Bit(0))
void PWR_EnterSTANDBYMode(void)
{
#if (__STANDDBY_SETUP & Bit(4))	//Bit 4: WKUP (PA0) enabled
		PWR_EnableWakeUpPin;	// PWR->CRS.EWUP = 1
#endif	
	
  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR |= Bit(2);	// bit 2 = SCB_SCR_SLEEPDEEP_Msk;
  
  /* Select Standby mode */
  PWR->CR |= Bit(1);	// bit 1 = PWR_CR_PDDS;
  
  /* Clear Wakeup flag */
  PWR->CR |= Bit(2);   // bit 2 = PWR_CR_CWUF;
  
//>>>>>>>>>>>---------------------	
	#if (__STANDDBY_SETUP & Bit(8))	// bit 8: Use RTC periodic auto Wakeup
    /* Enable write access to Backup domain before modify RTC and BACKUP sram registers  */
		write_access_Backup_domain();
		if ((PWR->CSR & (0x08)) == 0){  // if flag PWR->CSR.BRR (bit 3) = 0
			Enable_BKPSRAM_Regulator();
		}
	
		#if (__STANDDBY_SETUP & Bit(9))// bit 9: Setup RTC initialization (in "myRTC.c")
	// bits[8:9]=02: LSI, 01: LSE, 03: HSE (with RCC_CFGR.RTCPRE[4:0])
	if( (RCC->BDCR & (RCC_BDCR_RTCSEL|Bit(15)) ) != ((0x02ul<<8) | Bit(15) ) ) // if RTC source is not LSI or RTCEN (bit 15) = 0
	{
		RCC_BDCR_for_RTC_Enable();
		Disable_write_protection_RTC();
		Configure_RTC_PRE_CR();
	}
		#endif
		//  reload RTC->WUTR to count from the beginning; otherwise the auto wakeup time will not be correct
//	  stm32f4_RTC_autoWakeUP(__STANDDBY_SETUP>>16);	
	  stm32f4_RTC_autoWakeUP(__Auto_WakeUp_Period);	
	/* Enable the write protection for RTC registers */
	RTC_WPR_lock;
	#endif
//<<<<----------------------
	
#ifndef _RAM_Debug	
  /* enter STANDBY mode */
  __WFI();
#endif	
}
#endif //(__STANDDBY_SETUP & Bit(0))
//<<<<######################################################
//   "All functions needed for STANDBY Mode"
//<<<<####################################################


