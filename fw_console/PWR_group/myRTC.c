/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "myRTC.h"

uint8_t Alarm_A_triggered=0;
uint8_t Alarm_B_triggered=0;

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
//=========================================================================== External interrupt/event Configuration
//  <o0.8..9> RCC->BDCR.RTCSEL: RTC Clock source selction
//					<0=> no clock 
//					<1=> LSE
//					<2=> LSI
//					<3=> HSE 
//            <i> For HSE, see RCC->CFGR.[16:20]: RTCPRE
// <h> RTC->CR: RTC control initialization value
//    <o1.0..2> WUCKSEL[0:2]: Wakeup clock selection
//          <0=> RTC/16 clock
//    <o1.16..18> summer time change
//          <0=> not used
//		<o1.3> TSEDGE: Timestamp event active edge
//					<0=> rising 
//					<1=> falling 
//		<o1.6> FMT: Hour format (24 or AM/PM (not supported))
//					<0=> 24 format
//    <o1.21..22> OSEL[0:1]: PC13 pin Output selection
//          <0=> Output disabled
//          <1=> Alarm A
//          <2=> Alarm B
//          <3=> Auto Wakeup
//    <o1.20> POL: PC13 Output polarity when ALRAF/ALRBF/WUTF is asserted
//          <0=> High
//          <1=> Low
//    <o2.0> RTC->TAFCR.ALARMOUTTYPE: PC13 Output pin type
//          <0=> open-drain
//          <1=> push-pull
// </h>
//------------------------------------
#define _RTC_CLOCK_SELECTION 0x00000200
#define _RTC_CR_INIT_VALUE	 0x00000000
#define _TAFCR_bit18         0x00
//-------------<<< end of configuration section >>>

void write_access_Backup_domain(void)
{
    /* Enable Power Clock */
		RCC->APB1ENR |= Bit(28);   // bit 28 = PWREN
    
    /* Enable write access to Backup domain */
	  // bit-banding PWR->CR.DBP (bit 8) = 1
		my_Backup_domain_ACCESS;		
    while((PWR->CR & PWR_CR_DBP) == 0);	// wait until PWR->CR.DBP = 1
}

void Disable_write_protection_RTC(void)
{
	/* Disable the write protection for RTC registers */
		RTC_WPR_unlock_key1;   //RTC->WPR = 0xCA;
		RTC_WPR_unlock_key2;   //RTC->WPR = 0x53;
}

void Enable_LSI(void)
{
     /* Enable the Internal Low Speed oscillator (LSI). */
     RCC->CSR |= 0x01; 										// bit 0 (LSION) = 1; 
     while((RCC->CSR & 0x02) == 0);				// wait until	LSIRDY (bit 2)=1
}


void Enable_LSE(void)
{
    /* Enable write access to Backup domain */
	  // bit-banding PWR->CR.DBP (bit 8) = 1
		my_Backup_domain_ACCESS;		
    while((PWR->CR & PWR_CR_DBP) == 0);	// wait until PWR->CR.DBP = 1

	  // clear  LSEBYP bit (only when LSEON bit = 0) before configuring the LSE 
		*(uint8_t*) &RCC->BDCR = 0x00;					// LSE = LSEBYP = 0
    while(RCC->BDCR & Bit(1));	  // wait until RCC->BDCR.LSERDY = 0 
	  /* After the LSEON bit is cleared, 
	     LSERDY goes low after 6 external low-speed oscillator clock cycles.*/
			
	/* Enable the Internal Low Speed oscillator (LSI). */
		*(uint8_t*) &RCC->BDCR = 0x01;					// LSE= 1
    while( (RCC->BDCR & Bit(1))== 0);	  // wait until RCC->BDCR.LSERDY = 1 
}

/*
*  External source (LSE bypass): an external clock source must be provided. 
*  It must have a frequency up to 1 MHz. 
*  The external clock signal (square, sinus or triangle) with ~50% duty cycle 
*    has to drive the OSC_IN pin while the OSC_OUT pin should be left HI-Z. 
*/
void Enable_LSEBYP(void)
{
    /* Enable write access to Backup domain */
	  // bit-banding PWR->CR.DBP (bit 8) = 1
		my_Backup_domain_ACCESS;		
    while((PWR->CR & PWR_CR_DBP) == 0);	// wait until PWR->CR.DBP = 1

	  // modify  LSEBYP bit only when LSEON bit = 0
		*(uint8_t*) &RCC->BDCR = 0x00;					// LSE =0
    while(RCC->BDCR & Bit(1));	  // wait until RCC->BDCR.LSERDY = 0 
	  /* After the LSEON bit is cleared, 
	     LSERDY goes low after 6 external low-speed oscillator clock cycles.*/
		*(uint8_t*) &RCC->BDCR = (0x01<<2);					// LSEBYP (bit 2) =1
			
	/* Enable the Internal Low Speed oscillator (LSI). */
		*(uint8_t*) &RCC->BDCR = 0x01;					// LSE= 1
    while( (RCC->BDCR & Bit(1))== 0);	  // wait until RCC->BDCR.LSERDY = 1 
}

/**
  * @brief  This function configures  RTC by LSE or LSI 
  *          and with Backup SRAM ON 
  *         ====================================================
  *           - RCC->BDCR:  RTC Clocked by LSE or LSI
  *           - Backup SRAM ON
  * @param  None
  * @retval None
  */
void RCC_BDCR_for_RTC_Enable(void)
{
    /* Enable write access to Backup domain OUTSIDE before call this function */
		//	write_access_Backup_domain();      // be called outside  
	
    /*##-1- Reset the entire Backup domain (except BKPSRAM) ONLY if the RTC Clock source selction is modified */ 
		// RCC_BDCR_RTCSEL = Bit(8) | Bit(9);
		// bits[8:9]=02: LSI, 01: LSE, 03: HSE (with RCC_CFGR.RTCPRE[4:0])
		if((RCC->BDCR & (RCC_BDCR_RTCSEL|Bit(15)) ) != ( ( _RTC_CLOCK_SELECTION & RCC_BDCR_RTCSEL)|Bit(15) ) )   // if RTC source is not LSI
    {		
			uint32_t tmpreg;
			
      tmpreg = (RCC->BDCR & ~(RCC_BDCR_RTCSEL));	// clear bits[8:9]
      /* RTC Clock selection can be changed only if the entire Backup Domain is reset */
			// The BKPSRAM is not affected by this reset,
      *(volatile uint32_t *) BDCR_BDRST_BB = 0x01; // RCC->BDCR.BDRST= 1: __HAL_RCC_BACKUPRESET_FORCE();
      *(volatile uint32_t *) BDCR_BDRST_BB = 0x00; // RCC->BDCR.BDRST= 0: __HAL_RCC_BACKUPRESET_RELEASE();
      /* select the RTC Clock source as LSI */
      RCC->BDCR = tmpreg | _RTC_CLOCK_SELECTION;	    // bits[8:9]=02: LSI, 01: LSE, 03: HSE (with RCC_CFGR.RTCPRE[4:0])

		/*##-2- Enable the RTC peripheral Clock ####################################*/ 
		/* Enable RTC Clock: bit-banding RCC->BDCR.RTCEN = 1 */ 
			*(uint32_t *) BDCR_RTCEN_BB =0x01;   
    }
}


/**
  * @brief  This function Backup SRAM ON 
  *         ====================================================
  *           - PWR->CSR:  BRE and BRR bits
  *           - Backup SRAM ON
  */
void Enable_BKPSRAM_Regulator(void)
{
/*##-- Enable BKPRAM Clock */
  RCC->AHB1ENR |= (RCC_AHB1ENR_BKPSRAMEN);  // __BKPSRAM_CLK_ENABLE();
		/* Enable the Backup SRAM low power Regulator: PWR->CSR.BRE (bit 9) */
		RCC->APB1ENR |= Bit(28);   // bit 28 = PWREN

		PWR->CSR |= Bit(9);  // bit 9= BRE
		while((PWR->CSR & (0x08) ) == RESET);  // wait until ready flag PWR->CSR.BRR (bit 3) = 1
}

//###############################################
//  Configure RTC->PRE and RTC->CR
//-----------------------------------------------
void Configure_RTC_PRE_CR(void)
{
		//###### --- After RTC (RCC->BDCR.RTCEN=1) enabled, it needs to re-unlock write to RTC registers
  // Disable_write_protection_RTC();  // be called outside  

		/*##-3- Set INIT mode to Configure  prescaler RTC->PRER ############################*/
		// >>>------ RTC INIT mode: FMT, OSEL, POL; and prescaler RTC->PRER
		/* Check if the Initialization mode is set */
	if((RTC->ISR & RTC_ISR_INITF) == RESET)  // if RTC->ISR.INITF (bit 6) = 0
  {
    /* Set the Initialization mode */
    RTC->ISR = 0xFFFFFFFF;					// only INIT (bit 7) is rw, the others are rc_w0 or r
    while((RTC->ISR & RTC_ISR_INITF) == RESET);	// wait until RTC->ISR.INITF = 1
	}	
    /* Configure the RTC prescaler register */
    RTC->PRER = (RTC_SYNCH_PREDIV)|(RTC_ASYNCH_PREDIV<< 16);
    
    /* Exit Initialization mode */
	// Counters are stopped until RTC->ISR.INIT = 0 and then starts counting from the new value.
		RTC->ISR &= ~RTC_ISR_INIT; 
  //<<<<<------------------- END of RTC INIT mode
  
	/* Clear RTC->CR.FMT (bit 6), .OSEL (bits[21:22] and .POL (bit 20) Bits */
		RTC->CR = _RTC_CR_INIT_VALUE;
//&		RTC->CR &= (~(RTC_CR_FMT | RTC_CR_OSEL | RTC_CR_POL));
	/* Set RTC_CR register: hour format = 0 (24hours), alarm output=0 (high), alarm output =0 (disabled) */
//&    RTC->CR |= (RTCHandle.Init.HourFormat | RTCHandle.Init.OutPut | RTCHandle.Init.OutPutPolarity);
    
#if (_RTC_CR_INIT_VALUE & 0x00600000) // if bits[21:22] are not zero
	uint32_t tmpreg;
	/* RTC_ALARM output type (PC13=RTC_AF1) = open-drain (0) or push-pull (1)*/
	tmpreg = RTC->TAFCR & (~(0x01<<18)); // bit 18 (RTC_TAFCR_ALARMOUTTYPE) = 0: open drain;
	tmpreg |= _TAFCR_bit18<<18;           // bit 18 (RTC_TAFCR_ALARMOUTTYPE) = 1: push-pull;
	RTC->TAFCR = tmpreg;
#endif  
	
	/* OUTSIDE: Enable the write protection for RTC registers */
	// 	RTC->WPR = 0xFF;			// Writing a wrong key reactivates the write protection.
	//<<<--------end of HAL_RTC_Init
}	



//###############################################
//  Configure RTC Time Stamp peripheral
//-----------------------------------------------
void Configure_RTC_TimeStamp(void)
{
	uint32_t tmpreg;

  /*##-2- Configure the Time Stamp peripheral ################################*/
  /*  RTC TimeStamp flag Generation: TimeStamp Rising Edge on PC13 Pin */
	Configure_NVICpriority(TAMP_STAMP_IRQn, 0x03, 0x00); // this_pre 0x03, this_sub 0x00

  NVIC->ICPR[(TAMP_STAMP_IRQn>>5)]  = (1 << (TAMP_STAMP_IRQn & 0x1F));    // clear interrupt pending of RTC Time Stamp
  NVIC->ISER[(TAMP_STAMP_IRQn>>5)]  = (1 << (TAMP_STAMP_IRQn & 0x1F));    // enable interrupt RTC Time Stamp
//  HAL_RTCEx_SetTimeStamp_IT(&RtcHandle, RTC_TIMESTAMPEDGE_RISING, RTC_TIMESTAMPPIN_PC13);
  tmpreg = RTC->CR;
  tmpreg &= ~(RTC_CR_TSEDGE | RTC_CR_TSE);	// bit 3=TSEDGE: 0 = rising; bit 11 = TSE =0 disabled 

  // Disable the write protection for RTC registers 
  // Disable_write_protection_RTC();  // be called OUTSIDE 
	
	RTC->CR = tmpreg;
  RTC->TAFCR &= (uint32_t)~RTC_TAFCR_TSINSEL;	// bit 17 (TSINSEL)= 0: PC13 as time stamp pin
	tmpreg |= (RTC_CR_TSE);		// enable time stamp
	RTC->CR = tmpreg | (RTC_CR_TSIE);		// enable TSIE
	
    // Clear the TIMESTAMP interrupt pending bit 
	RTC->ISR &= ~RTC_ISR_TSF; // rc_w0; bit 11 (bits[8:13] not write protection)

// RTC Time Stamp Interrupt Configuration: EXTI configuration 
  EXTI->RTSR |= (0x01ul<<21);  // bit 21: EXTI line 21 Time STAMP
	EXTI->PR |= (0x01ul<<21);  ///rc_w1; clear pending flag for line 21
  EXTI->IMR |= (0x01ul<<21);	// bit 21: EXTI line 21 Time STAMP

	/* OUTSIDE: Enable the write protection for RTC registers */
	// 	RTC->WPR = 0xFF;			// Writing a wrong key reactivates the write protection.
}


/*###################################
   *----  All Date and Time data in BCD -----------------
* An example:
	p_datestructure->Century = 	 0x20;		// Century (ignorable)
	p_datestructure->Year = 	 0x16;		// yesr - cnetury
	p_datestructure->W_M_byte = u_WeekDay_Month(0x4,0x2);		// w=4: Weekday, M=2: Month
	p_datestructure->Date = 	 0x18;		// day date

	p_timestructure->Hours = 	 0x23;		// Hours
	p_timestructure->Minutes = 	 0x59;		// Minutes
	p_timestructure->Seconds = 	 0x00;		// Seconds
//-------------------------------------------- */
void RTC_Time_Date_setting(RTC_dateTYPE* p_datestructure, RTC_timeTYPE* p_timestructure)
{
	
  // Disable the write protection for RTC registers 
  // Disable_write_protection_RTC();  // be called outside  

  /* Check whether the Initialization MODE is set */
	if((RTC->ISR & RTC_ISR_INITF) == 0)  // if RTC->ISR.INITF (bit 6) = 0
  {
    /* Set the Initialization mode */
    RTC->ISR = 0xFFFFFFFF;					// only INIT (bit 7) is rw, the others are rc_w0 or r
    while((RTC->ISR & RTC_ISR_INITF) == 0);	// wait until RTC->ISR.INITF = 1
	}	

    /* Set the RTC_DR (RTC date) register */
    RTC->DR = *(uint32_t*) p_datestructure; //(datetmpreg & RTC_DR_RESERVED_MASK);
    
    /* Set the RTC_TR (RTC time) register */
    RTC->TR = *(uint32_t*) p_timestructure; //(tmpreg & RTC_TR_RESERVED_MASK);
     
    /* Exit Initialization mode */
    RTC->ISR &= (uint32_t)~RTC_ISR_INIT;  
		//<<<-----------
    
    /* If  CR_BYPSHAD bit = 0, wait for synchro else this check is not needed */
		// from the shadow registers, which are updated once every two RTCCLK cycles.
		if((RTC->CR & RTC_CR_BYPSHAD) == 0)					// RTC->CR.BYPSHAD (bit 5)
    {
			/* Calendar shadow registers synchronized */	
			RTC->ISR = 0xFFFFFF5F;  //clear RSF (bit 5):rc_w0; INIT (bit 7): rw
    /* Wait the registers to be synchronised */
			while((RTC->ISR & RTC_ISR_RSF) == 0);  // wait until CR->ISR.RSF = 1 
		}
	/* OUTSIDE: Enable the write protection for RTC registers */
	// 	RTC->WPR = 0xFF;			// Writing a wrong key reactivates the write protection.
}

/*###################################
   *----  All Date and Time data in BCD -----------------
* An example:
	p_alarmstructure->Date_o_Wkday = 	 0x19;		// day date 
	or p_alarmstructure->Date_o_Wkday = 	 Alarm_weekdayBit | 0x5 ;		// Weekday

	p_alarmstructure->Hours = 	 0x00;		// Hours
	p_alarmstructure->Minutes = 	 0x01;		// Minutes
	p_alarmstructure->Seconds = 	 0x05;		// Seconds

  p_MSK-> DaWk = 0;				// Date/Weekday  to be compared
  p_MSK-> Hr = 0;				  // Hour  to be compared
  p_MSK-> Min = 0;				// Minute to be compared
  p_MSK-> Sec = 0;				// Second to be compared
//-------------------------------------------- */
void RTC_Alarm_A_setting(RTC_AlarmTypeDef* p_alarmstructure, RTC_AlarmMSK_TypeDef* p_MSK)
{
		uint32_t tmpreg;
	//>>------ALARM A -------
		tmpreg = RTC->CR;
	    /* Disable the Alarm A  */
		tmpreg &= ~(RTC_CR_ALRAE);		// clear bit 8
		RTC->CR = tmpreg;

		// (redundant!!) RTC->ALRMAR can be written only when ALRAWF is set to 1 in RTC_ISR
    /* Wait till RTC ALRAWF flag is set, which is always 1 according to test  */
//    while( ((RTC->ISR) & Bit(0)) == 0); // wait until RTC->ISR.ALRAWF (bit 0) = 1
		RTC->ALRMAR = ( (*(uint32_t *)p_alarmstructure) & 0x7F7F7F7F ) | \
	                 ( ((*(uint32_t *) p_MSK) & 0x01010101)<<7);

	// RTC->ISR: only INIT (bit 7) is rw, the others are rc_w0 or r
		RTC->ISR = 0xFFFFFE7F;  //clear ALRAF (bit 8):rc_w0; INIT (bit 7): rw

	/* Configure the Alarm state: Enable Alarm A and its interrupt*/
		tmpreg |= RTC_CR_ALRAE | RTC_CR_ALRAIE;	// set bits 8 and 12
		RTC->CR = tmpreg;
		
   //<<----------
  /* RTC Alarm Interrupt Configuration: EXTI configuration */
  EXTI->RTSR |= (0x01ul<<17);  ///External interrupt line 17 Connected to the RTC Alarm event
	EXTI->PR |= (0x01ul<<17);  ///rc_w1; clear pending flag for line 17
	EXTI->IMR |= (0x01ul<<17);  ///External interrupt line 17 Connected to the RTC Alarm event

  /*## Configure the NVIC for RTC Alarm ###################################*/
	Configure_NVICpriority(RTC_Alarm_IRQn, 0x0F, 0x00); // this_pre 0x0F, this_sub 0x00

  NVIC->ICPR[(RTC_Alarm_IRQn>>5)]  = (1 << (RTC_Alarm_IRQn & 0x1F));    // clear interrupt pending of RTC_Alarm
  NVIC->ISER[(RTC_Alarm_IRQn>>5)]  = (1 << (RTC_Alarm_IRQn & 0x1F));    // enable interrupt RTC_Alarm
}

/**
  * @brief  This function configures  
  *           - Automatic Wakeup using RTC clocked by LSE/LSI (after ~20s)

  *  RTC Wakeup Interrupt Generation:
      Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSI))
      Wakeup Time = Wakeup Time Base * WakeUpCounter 
                  = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSI)) * WakeUpCounter
      ==> WakeUpCounter = Wakeup Time / Wakeup Time Base

  *   To configure the wake up timer to 20s the WakeUpCounter is set to 0xA017:
        RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16 
        Wakeup Time Base = 16 /(~32.768KHz) = ~0.488 ms
        Wakeup Time = ~20s = 0.488ms  * WakeUpCounter
        ==> WakeUpCounter = ~20s/0.488ms = 40983 = 0xA017 
*/
void stm32f4_RTC_autoWakeUP(uint32_t WUTcounts)
{
	uint32_t tmpreg;

  /* Enable write access to Backup domain OUTSIDE before call this function */
   Disable_write_protection_RTC();  // be called outside  
	
	// disable "Wakeup timer" and "Wakeup timer interrupt" enable
		tmpreg = RTC->CR;
		tmpreg &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);				// clear (bits 10, 14)
		RTC->CR = tmpreg;										// WUTE =0 and WUTIE=0

//REDUNDANT==>	while((RTC->ISR & RTC_ISR_WUTWF) == 0);// wait until "Wakeup timer write flag" RTC->ISR.WUTWF (bit 2) = 1
																					// after WUTE = 0			
		/* Configure the Wakeup Timer counter ONLY when RTC->ISR.WUTWF=1 which is set by hardware after RTC->CR.WUTE = 0 */
		RTC->WUTR = WUTcounts; //0xA017= 40983+1		//  the wake up timer to 20s 
		RTC->ISR = 0;				// rc_w0: clear all RTC flags including WUTF
  /* Configure the Wakeup Timer clock source bits in CR register */
		tmpreg &= ~RTC_CR_WUCKSEL;				// bits[0:2]: clear WUCKSEL[2:0]
		tmpreg |= (_RTC_CR_INIT_VALUE & 0x07);		
		                  // 000: RTC/16; 001: RTC/8; 010: RTC/4; 011: RTC/2
											// 10x: ck_spre (usually 1 Hz) clock
											/* 11x: ck_spre (usually 1 Hz) clock and
											   WUT = Wakeup unit counter value. WUT = (0x0000 to 0xFFFF) + 0x10000 added 
											   when WUCKSEL[2:1 = 11].  */

		tmpreg |= RTC_CR_WUTE | RTC_CR_WUTIE;				// bit 10: WUTE; WUTIE (bit 14)
		RTC->CR = tmpreg;
		
  /* ONLY if STOP mode, RTC WakeUpTimer Interrupt Configuration: EXTI_22 configuration */
//		EXTI->RTSR |= (0x01<<22);
//		EXTI->PR 	 = (0x01<<22);			// rc_w1: clear pending bit 22 (RTC WakeUpTimer Interrupt)
//		EXTI->IMR  |= (0x01<<22);			// line 22: RTC Wakeup event
//
//	Configure_NVICpriority(RTC_WKUP_IRQn, 0x0F, 0);	   //  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x0F, 0);
//  NVIC->ICPR[(RTC_WKUP_IRQn>>5)]  = (1 << (RTC_WKUP_IRQn & 0x1F));    // clear interrupt pending 
//  NVIC->ISER[(RTC_WKUP_IRQn>>5)]  = (1 << (RTC_WKUP_IRQn & 0x1F));    // enable NVIC interrupt

  
		
	/* OUTSIDE: Enable the write protection for RTC registers */
	// 	RTC->WPR = 0xFF;			// Writing a wrong key reactivates the write protection.

}


/**
  * @brief  This function handles Alarm interrupt request.
  * @param  hrtc: RTC handle
  * @retval None
  */
void RTC_Alarm_IRQHandler(void)
{
  if((RTC->ISR & RTC_ISR_ALRAF ) && (RTC->CR & RTC_CR_ALRAIE))	// bit 8 in ISR: ALRAF
  {
      /* AlarmA function */ 
				Alarm_A_triggered=1;
		
      /* Clear the Alarm interrupt pending bit by rc_w0*/
	// RTC->ISR: only INIT (bit 7) is rw, the others are rc_w0 or r
		RTC->ISR = 0xFFFFFE7F;  //clear ALRAF (bit 8):rc_w0; INIT (bit 7): rw
  }
  /* Clear the EXTI's line Flag for RTC Alarm */
	EXTI->IMR &= ~Bit(17);  ///disable External interrupt line 17 Connected to the RTC Alarm event
	EXTI->PR = Bit(17); // rc_w1: EXTI line 17
}

#include "stdio.h"
char aShowTstamp[50]={0};

/* @brief  This function handles Tamper interrupt request.
* @param  None
* @retval None
*/
void TAMP_STAMP_IRQHandler(void)
{
	uint32_t tmpreg;
	uint8_t	hour, minute, sec;

	if((RTC->ISR & RTC_ISR_TSF) && (RTC->CR & RTC_CR_TSIE))  	// bit 11 in ISR (TSF); bit 15 in RC (TSIE)
  {
      /* TIMESTAMP callback */ 
//  HAL_RTCEx_GetTimeStamp(&RtcHandle, &sTimeStampget, &sTimeStampDateget, FORMAT_BIN);
		tmpreg = RTC->TSTR;
		hour	 = ((uint8_t) (tmpreg>>16)) & 0x3F;
		minute	 = ((uint8_t) (tmpreg>>8));
		sec	 = ((uint8_t) tmpreg);
		/* Display time Format : hh:mm:ss */
  sprintf((char*)&aShowTstamp[0],"Time Stamp: %02x:%02x:%02x ",hour, minute, sec);
  tmpreg = (uint32_t)RTC->TSDR; 
  hour	 = ((uint8_t) (tmpreg>>13)) & 0x07; // weekday
  minute	 = ((uint8_t) (tmpreg>>8)) & 0x1F; // month
  sec	 = ((uint8_t) tmpreg);                 // date	
		sprintf((char*)&aShowTstamp[24],"M.D: %02x.%02x  Weekday:%1x", minute, sec, hour);
 
      /* Clear the TIMESTAMP and TAMP1F interrupt pending bit */
		RTC->ISR &= ~(0x05ul<<11);		// rc_w0: clear bit 11, 13 (TSF, TAMP1F) (NOT write protected)
  }
  
  /* Clear the EXTI's line Flag for Time Stamp */
	EXTI->PR = (0x01ul<<21); // rc_w1: EXTI line 21

}
