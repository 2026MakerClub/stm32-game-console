/*----------------------------------------------------------------------------
 * Name:    STM32_nvic.c
 * Purpose: STM32 nested vector interrupt controller initialisation
  * Note(s):
 *----------------------------------------------------------------------------
 * This file was written by Shir-Kuan Lin (NCTU, Hsinchu, Taiwan)
 *----------------------------------------------------------------------------*/

#include "stm32f4xx.h"

#ifndef Bit
#define Bit(x) ( 1ul<<x )
#endif
//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//=========================================================================== Clock Configuration
// <h> ADC Common Registers
//       <o0.16..17> CCR.ADCPRE: ADC clock prescaler
//             <i> f_ADC<=36MHz for V_DDA=2.4~3.6V
//             <i> f_ADC<=18MHz for V_DDA=1.8~2.4V
//            <0=> PCLK2 / 2
//            <1=> PCLK2 / 4
//            <2=> PCLK2 / 6
//            <5=> PCLK2 / 8
//       <o0.23> CCR.TSVREFE (Ch.16): Temperature sensor and v_REFINT enable
//       <o0.22> CCR.VBATE (Ch.18): v_BAT enable
//       <o0.0..4> CCR.MULTI-MODE: multi mode: Dual: ADC1+ADC2; Triple ADC1+ADC2ADC3
//              <i> Only on ONE regular channel: Interleaved
//              <i> Only on injected group channel: Alternate trigger
//            <0=> Independant (no multi)
//            <1=> DUAL: Regular sim + Injected sim.
//            <2=> DUAL: Regular sim. + Alternate trigger
//            <5=> DUAL: Injected simultaneous
//            <6=> DUAL: Regular simultaneous
//            <7=> DUAL: Interleaved
//            <9=> DUAL: Alternate trigger
//            <17=> TRIPLE: Regular sim + Injected sim.
//            <18=> TRIPLE: Regular sim. + Alternate trigger
//            <21=> TRIPLE: Injected simultaneous
//            <22=> TRIPLE: Regular simultaneous
//            <23=> TRIPLE: Interleaved
//            <25=> TRIPLE: Alternate trigger
//           ------------------------
//       <o0.14..15> CCR.DMA: multi mode DMA mode
//            <0=> DMA disabled
//            <1=> 16-bits one by one 
//            <2=> 16-bits by pairs
//            <5=> 8-bits by pairs
//             <i> One by one:  ADC1, ADC2, then ADC3
//             <i> by pairs:  ADC2 & 1, ADC1 & 3, then ADC3 & 2
//       <o0.13> CCR.DDS: multi mode DMA is in circular mode
//       <o0.8..11> CCr.DELAY (5~20): "interleaved" delay between two sampling phases <5-20> <#-5>
//             <i> 5~20: 5*T_ADCCLK ~ 20*T_ADCCLK
//	</h>
#define ADC_CCR_VAL       0x00010000									// 0

//=========================================================================== Clock Configuration
// <e0.0> ADC1 Register Configuration 
//       <o5.24..25> CR1.RES: resolution of the conversion
//            <0=> 12-bit
//            <1=> 10-bit 
//            <2=> 8-bit 
//            <3=> 6-bit 
//       <o6.11> CR2.ALIGN: Right/Left alignment of 12 bits to 16 bits
//            <i> If 6-bit resolution and Left alignment, then the lower byte of DR is the result.
//            <0=> Right
//            <1=> Left
//       <o6.1> CR2.CONT: Continuous/Single round conversion for REGULAR
//            <0=> Single
//            <1=> Continuous
//       <o5.8> CR1.SCAN: group Scan Mode in SQRx (if DISCEN=0) and JSQRx (if JDISCEN=0)
//            <i>  NO action when DISCEN=1 for regular or when JDISCEN = 1 for injected
//            <0=> Only First
//            <1=> Scan All 
//       <e5.10> ~CR1.JAUTO (JAUTO=0): "DISABLE" auto-injected mode      (checked [v] means JAUTO=0)
//		   <i> NOT auto-injected mode = injected group NOT automatically after regular group conversion
//         <i> ( ): not checked ==> JAUTO=1, then clear JDISCEN=DISCEN=CR2.JEXTTEN=0 automatically
//         <i> (v): checked ==> JAUTO=0 to use JDISCEN, DISCEN, and CR2.JEXTTRIG.
//         -------------------------------------
//         <h> Regular Conversion Setting (for JAUTO=0 case only)
//          <o5.11> CR1.DISCEN: Discontinuous regular mode (SCAN bit has no effect)
//           <i> E.g., DISCNUM =2, and SQRx = 0, 1, 3, 7, 11
//           <i> 1st trigger: ch. 0, 1 ;        2nd trigger: ch. 3, 7 
//           <i> 3rd trigger: ch. 11, EOC; 4th trigger: ch. 0, 1 
//          <o5.13..15> (CR1.DISCNUM+1): Number of Channels per trigger in discontinuous REGULAR mode <1-8:1> <#-1>
//           <i> only (DISCNUM+1) channels in SQRx per trigger   
//         </h>
//         ------------------------------------------------
//         <h> Injected Conversion Setting (for JAUTO=0 case only)
//          <o5.12> CR1.JDISCEN: Discontinuous injected mode (ONE channel per trigger and DISCEN=0) 
//           <i> E.g., JSQRx = 0, 1, 11 for 
//           <i> 1st trigger: ch. 0;                          2nd trigger: ch. 1
//           <i> 3rd trigger: ch. 11, EOC and JEOC; 4th trigger: ch. 0
//          <o6.16..19> CR2.JEXTSEL: Trigger event for the injected group
// 						<0=> 0: Timer 1 CC4 event
// 						<1=> Timer 1 TRGO event
// 						<2=> Timer 2 CC1 event
// 						<3=> Timer 2 TRGO event
// 						<4=> Timer 3 CC2 event
// 						<5=> Timer 3 CC4 event
// 						<6=> Timer 4 CC1 event
// 						<7=> Timer 4 CC2 event
// 						<8=> Timer 4 CC3 event
// 						<9=> Timer 4 TRGO event
// 						<10=> Timer 5 CC4 event
// 						<11=> Timer 5 TRGO event
// 						<12=> Timer 8 CC2 event
// 						<13=> Timer 8 CC3 event
// 						<14=> Timer 8 CC4 event
// 						<15=> EXTI line15
//          <o6.20..21> CR2.JEXTEN: Enable JEXTSEL's trigger
// 						<0=> disable JEXTSEL
// 						<1=> rising edge
// 						<2=> falling edge
// 						<3=> both edges
//         </h>
//       </e>
//--------------------------------------------------------------------------- Timer 1 channel 1
//   <e0.2> Regular Conversion Setting
//     <h> Regular Sequence
//      <o3.20..23> SQR1.(L+1): Seqence length = total number of channels to be converted <1-17:1> <#-1>
//         <i> start from SQ_1 to SQ_[L+1]
//      <o1.0..4> SQ1: Channel number <0-18:1>
//      <o1.5..9> SQ2: Channel number <0-18:1>
//      <o1.10..14> SQ3: Channel number <0-18:1>
//      <o1.15..19> SQ4: Channel number <0-18:1>
//      <o1.20..24> SQ5: Channel number <0-18:1>
//      <o1.25..29> SQ6: Channel number <0-18:1>
//      <o2.0..4> SQ7: Channel number <0-18:1>
//      <o2.5..9> SQ8: Channel number <0-18:1>
//      <o2.10..14> SQ9: Channel number <0-18:1>
//      <o2.15..19> SQ10: Channel number <0-18:1>
//      <o2.24..20> SQ11: Channel number <0-18:1>
//      <o2.29..25> SQ12: Channel number <0-18:1>
//      <o3.0..4> SQ13: Channel number <0-18:1>
//      <o3.5..9> SQ14: Channel number <0-18:1>
//      <o3.10..14> SQ15: Channel number <0-18:1>
//      <o3.15..19> SQ16: Channel number <0-18:1>
//     </h>
//--------------------------------------------------------------------------- 
//       <o6.8> CR2.DMA: Direct memory access mode for independent REGULAR group
//       <o6.9> CR2.DDS: DMA is in circular mode
//       <o6.10> CR2.EOCS: End of conversion selection
//            <0=> at end of group conversions
//            <1=> at end of EACH channnel
//       <o6.24..27> CR2.EXTSEL: Trigger event for the regular group
// 						<0=> 0: Timer 1 CC1 event
// 						<1=> Timer 1 CC2 event
// 						<2=> Timer 1 CC3 event
// 						<3=> Timer 2 CC2 event
// 						<4=> Timer 2 CC3 event
// 						<5=> Timer 2 CC4 event
// 						<6=> Timer 2 TRGO event
// 						<7=> Timer 3 CC1 event
// 						<8=> Timer 3 TRGO event
// 						<9=> Timer 4 CC4 event
// 						<10=> Timer 5 CC1 event
// 						<11=> Timer 5 CC2 event
// 						<12=> Timer 5 CC3 event
// 						<13=> Timer 8 CC1 event
// 						<14=> Timer 8 TRGO event
// 						<15=> EXTI line 11
//       <o6.28..29> CR2.EXTEN: Enable EXTSEL's trigger
// 						<0=> disable EXTSEL
// 						<1=> rising edge
// 						<2=> falling edge
// 						<3=> both edges
//   </e>	END of Regular Conversion Setting
//============================================================================
//   <e0.3> Injected Conversion Setting
//     <h> Injected Sequence
//      <o4.20..21> JSQR.(JL+1): Seqence length = total number of channels to be converted <1-4:1> <#-1>
//         <i> start from JSQ_[4-JL] to JSQ_4, 
//      <o4.15..19> JSQ4: the LAST Channel number <0-18:1>
//         <i> First one for (JL+1) = 1
//      <o4.10..14> JSQ3: the second LAST Channel number <0-18:1>
//         <i> First one for (JL+1) = 2
//         <i> not active for (JL+1) < 2
//      <o4.5..9> JSQ2: the third LAST Channel number <0-18:1>
//         <i> First one for (JL+1) = 3
//         <i> not active for (JL+1) < 3
//      <o4.0..4> JSQ1: the forth LAST Channel number <0-18:1>
//         <i> First one for (JL+1) = 4
//         <i> not active for (JL+1) < 4
//     </h>
//--------------------------------------------------------------------------- 
//       <e0.12> Injected Offset Setting
//         <o9> JOFR1: Data offset for First injected channel (JDR1) <0-4095>
//         <o10> JOFR2: Data offset for First injected channel (JDR1) <0-4095>
//         <o11> JOFR3: Data offset for First injected channel (JDR1) <0-4095>
//         <o12> JOFR4: Data offset for First injected channel (JDR1) <0-4095>
//       </e>
//--------------------------------------------------------------------------- 
//   </e>  END of Injected Conversion Setting
//====================================================================================
//   <e0.4> Analog Watchdog Setting
//       <o5.9> CR1.AWDSGL: Enable watchdog on all/one channels in scan mode
//            <0=> all scanned channels
//            <1=> one selected in CR1.AWDCH
//       <o5.0..4> CR1.AWDCH: watchdog channel number for AWDSGL=on one <0-18:1>
//       <o5.22> CR1.JAWDEN: 
//       <o5.23> CR1.AWDEN: 
//       <o7> HTR: Watchdog high threshold <0-4095> 
//       <o8> LTR: Watchdog low threshold <0-4095>
//   </e>
//====================================================================================
//     <e0.8> Interrupts (ADC_IRQHandler)
//       <o5.5> CR1.EOCIE: End interrupt of a (regular or injected) GROUP channel conversions	 
//       <o5.6> CR1.AWDIE: Analog Watchdog (thresholds crossing) interrupt
//       <o5.7> CR1.JEOCIE: End interrupt of all injected group channels conversions
//       <o5.28> CR1.OVRIE: Overrun interrupt
//     </e>
//====================================================================================
//
#define __ADC1_SETUP        0x0009					// 0; bit0 = ADC1;  bit2 = regular; bit3 = injected; bit8 = interrupt
#define ADC1_SQR3_VAL       0x00004851			// 1
#define ADC1_SQR2_VAL       0x00000000			// 2
#define ADC1_SQR1_VAL       0x00000000			// 3
#define ADC1_JSQR_VAL       0x002830E0 			// 4
#define ADC1_CR1_VALtmp     0x00000580	  	// 5;	bit10 = ~JAUTO; bit11 = DISCEN; bit12 = JDISCEN 	 
#define ADC1_CR2_VAL        0x2F0F0300			// 6;	bit(20:21) JEXTEN: 0= disable; 1 rising; 2= falling; 3= both
#define ADC1_HTR_VAL        2816				 		// 7
#define ADC1_LTR_VAL        768				 			// 8
//------------------------
#define ADC1_JOFR1_VAL        0				 					// 9
#define ADC1_JOFR2_VAL        0				 					// 10
#define ADC1_JOFR3_VAL        0				 					// 11
#define ADC1_JOFR4_VAL        0				 					// 12

#define ADC1_CR1_VAL (ADC1_CR1_VALtmp ^ 0x0400)  // bit 10: 1==> 0; 0==> 1
//--------------------------------------------------------------------------- Timer 1 channel 1
//   <h> Channel sample time required to get its analog signal in ADC_CLK cycle
//       <i> Total conversion time = sample time + 12 conversion cycles (12-bit resolution)
//       <i> MULTI mode: use same sample time for the same sequence order in Simultaneous mode
//      <o0.0..2> SMPR2: Channel 0 (PA0) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.3..5> SMPR2: Channel 1 (PA1) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.6..8> SMPR2: Channel 2  (PA2)
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.9..11> SMPR2: Channel 3  (PA3)
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.12..14> SMPR2: Channel 4 (PA4, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.15..17> SMPR2: Channel 5 (PA5, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.18..20> SMPR2: Channel 6 (PA6, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.21..23> SMPR2: Channel 7 (PA7, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.24..26> SMPR2: Channel 8 (PB0, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o0.27..29> SMPR2: Channel 9 (PB1, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//-------------------------------------------
//      <o1.0..2> SMPR1: Channel 10 (PC0) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.3..5> SMPR1: Channel 11 (PC1)  
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.6..8> SMPR1: Channel 12 (PC2)   
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.9..11> SMPR1: Channel 13 (PC3)  
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.12..14> SMPR1: Channel 14 (PC4, no ADC3)  
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.15..17> SMPR1: Channel 15 (PC5, no ADC3) 
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.18..20> SMPR1: Channel 16 (Temperature Sensor: 10 us)
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.21..23> SMPR1: Channel 17  (Internal Reference Voltage: typical 1.21 V, 10 us)
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//      <o1.24..26> SMPR1: Channel 18  (VBAT battery charge monitoring, 5 us)
//            <0=> 3 cycles
//            <1=> 15 cycles
//            <2=> 28 cycles
//            <3=> 56 cycles
//            <4=> 84 cycles
//            <5=> 112 cycles
//            <6=> 144 cycles
//            <7=> 480 cycles
//   </h>
//--------------------------------------------------------------------------- Timer 1 channel 1
//--------------------------------------------------------------------------- Timer 1 channel 1
// </e> End of ADC1 Configuration
#define __ADC1_SMPR2_VAL       0x00000000				  // 1
#define __ADC1_SMPR1_VAL       0x06D80000				  // 2






//-------- <<< end of configuration section >>> ---------------


#if ( (__ADC1_SETUP & 0x01) | (__ADC2_SETUP & 0x01))
void stm32f4_adc(void) { 
  uint32_t tmpCR1;
	
  /* ADC registers reset ----------------------------------------------------*/
    RCC->APB2RSTR |= RCC_APB2RSTR_ADCRST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_ADCRST;
	
#if ( (__ADC1_SETUP & 0x01))
   /* ADC1 configuration ------------------------------------------------------*/
  RCC->APB2ENR |= (1<<8);                      /* enable periperal clock for ADC1      */
//  RCC->APB2ENR |= (1<<10);                      /* enable periperal clock for ADC3      */
	


  ADC1->SQR1  = ADC1_SQR1_VAL;           /* regular sequence conversions                   */
  ADC1->SQR2  = ADC1_SQR2_VAL;           /* regular sequence conversions                   */
  ADC1->SQR3  = ADC1_SQR3_VAL;           /* regular sequence conversions                   */
  ADC1->JSQR  = ADC1_JSQR_VAL;           /* injected sequence conversions                   */
  ADC1->SMPR2 =  __ADC1_SMPR2_VAL;     /* set sample time       */ 
  ADC1->SMPR1 =  __ADC1_SMPR1_VAL;     /* set sample time        */ 

 #if (__ADC1_SETUP & 0x1000)             /* bit 12 = Injected Offset Setup */ 
  ADC1->JOFR1  = ADC1_JOFR1_VAL;           /* injected data 1 offset    */
  ADC1->JOFR2  = ADC1_JOFR2_VAL;           /* injected data 2 offset    */
  ADC1->JOFR3  = ADC1_JOFR3_VAL;           /* injected data 3 offset    */
  ADC1->JOFR4  = ADC1_JOFR4_VAL;           /* injected data 4 offset    */
 #endif


 #if (__ADC1_SETUP & 0x10)             /* bit 4 = Analog Watchdog Setup */ 
  ADC1->HTR = ADC1_HTR_VAL;			   /* watchdog high threshold */
  ADC1->LTR = ADC1_LTR_VAL;			   /* watchdog high threshold */
  #define __ADC1_CR1_J_AWDEN  0xFFFFFFFF  // bit[22:23] = 11 (AWDEN: JAWDEN)
 #else
  #define __ADC1_CR1_J_AWDEN  0xFF3FFFFF  // bit[22:23] = 00
 #endif


 #if (ADC1_CR1_VAL & 0x0400 )    // if bit(10) = JAUTO = 1
  ADC1->CR2   =  ADC1_CR2_VAL & ~( Bit(15) | Bit(0));   // clear JEXTTRIG and ADON
  tmpCR1 = ADC1_CR1_VAL & ~( Bit(12) | Bit(11)) & __ADC1_CR1_J_AWDEN;         // clear JDISCEN
  #else
  ADC1->CR2   =  ADC1_CR2_VAL& ~(Bit(0));            // clear ADON (ADC_CR2_ADON)
  tmpCR1 = ADC1_CR1_VAL & __ADC1_CR1_J_AWDEN;
 #endif

  ADC1->CR1   =  tmpCR1 & (~(Bit(5) | Bit(6) |Bit(7)) );
 #if (__ADC1_SETUP & 0x0100)		// interrupt enabled
  ADC1->SR    =  0;				// clear interrupt flags
  ADC1->CR1   =  tmpCR1;        // set interrupt enable bits
  #define	  j_adc1  ADC_IRQn
      NVIC->ICPR[j_adc1>>5] = Bit( (j_adc1&0x1F) );	 // clear pending interrupt IRQ[j]
      NVIC->ISER[j_adc1>>5] = Bit( (j_adc1&0x1F) );	 // enable interrupt IRQ[j]
 #endif

  // Enable ADC1
	ADC1->CR2 |=  Bit(0);            // set ADON (ADC_CR2_ADON)
#endif	 // END of if ( (__ADC1_SETUP & 0x01))
    

	//------------- ADC Common Control Register--------------------------
	ADC->CCR = ADC_CCR_VAL;
} 
#endif	 // END of if ( (__ADC1_SETUP & 0x01) | (__ADC2_SETUP & 0x01))
