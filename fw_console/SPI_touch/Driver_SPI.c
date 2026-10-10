#include "stm32f407xx.h"
#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

#define pin_0 0
#define pin_1 1
#define pin_2 2
#define pin_5 5
#define pin_8 8
#define pin_9 9
#define pin_10 10
#define pin_11 11
#define pin_13 13
#define bit_GPIOA 0
#define bit_GPIOB 1
#define bit_GPIOC 2
#define bit_GPIOF 5
#define GPIO_SCK		GPIOB
#define GPIO_CS			GPIOC
#define GPIO_MI 		GPIOB
#define GPIO_MO 		GPIOF


//#######################################################################
void Driver_GPIO(void)
{
/*  --------------------------------------------------------------
    GPIOB: INPUT, pull-up
    --------------------------------------------------------------
    | Pin : PB |    8
    | Key 1     |   KEY1
   --------------------------------------------------------------
*/
   RCC->AHB1ENR |=  (1UL << bit_GPIOB);     // bit_GPIOB: enable clock for GPIOB
 // 0:PORTA, 1: PORTB, …, 10: PORTK
   GPIOB->MODER   = (GPIOB->MODER & ~(0x03ul<<(pin_8 *2)) );                  //Input mode (00b)
   GPIOB->PUPDR   = (GPIOB->PUPDR & ~(0x03ul<<(pin_8 *2)) ) | (0x01ul<<(pin_8 *2)); //pull-up (01b)
   GPIOB->AFR[1]  &= ~(0x0Ful<<(pin_8 -8)*4);      // AF0 (0000b)            

/*  --------------------------------------------------------------
    GPIOB: INPUT, floating (no Pull-Up or Pull-Down) 
	  --------------------------------------------------------------
	  | Pin : PB      |    1          2  
    | touch Module  |   TP_INT     MISO
	  --------------------------------------------------------------
*/
	#define  rr1_2   ((0x03ul<<(pin_1*2) ) | (0x03ul<<(pin_2*2) ))    	// 11b
	#define  AFrr1_2   ((0x0Ful<<(pin_1*4) ) | (0x0Ful<<(pin_2*4) ))    	// 11b
		RCC->AHB1ENR |=  (1UL << bit_GPIOB);     // enable clock for GPIOB
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
    GPIOB->MODER   = (GPIOB->MODER & ~rr1_2 ); // Input mode (00b)
    GPIOB->PUPDR   = (GPIOB->PUPDR & ~rr1_2);  // no pull (00b)
		GPIOB->AFR[0]  &= ~AFrr1_2;  				  // AF=0: System or Alternate Function
//		GPIOB->AFR[1]  &= ~(0x0Ful<<(pin_1*4));  				  // AF=0: System or Alternate Function

/*  --------------------------------------------------------------
    GPIOC: OUTPUT, Speed_25MHz medium, push pull, no pull 
		--------------------------------------------------------------
	  | Pin: PC |    13       
    | SPI     |    CS   
    | initial |   1(high)
	  --------------------------------------------------------------
*/
	#define  cc13   (0x03ul<<(pin_13*2) )    	// 11b
	#define  bb13   (0x01ul<<(pin_13*2) )    	// 01b
		RCC->AHB1ENR |=  (1UL << bit_GPIOC);     // enable clock for GPIOC
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
		GPIO_CS->BSRR = Bit(pin_13);			// Initial output value for CS = 1
		
    GPIO_CS->MODER   = (GPIO_CS->MODER & ~cc13) | bb13; 			//Output mode (01b)
    GPIO_CS->OSPEEDR = (GPIO_CS->OSPEEDR & ~cc13) | bb13; 					//medium speed (01b)
    GPIO_CS->OTYPER  &= ~Bit(pin_13);         					// push-pull (0b)
    GPIO_CS->PUPDR   = (GPIO_CS->PUPDR & ~cc13 ); // (00b)
		GPIO_CS->AFR[(pin_13/8)]  &= ~(0x0Ful<<((pin_13-8)*4) );  				  // AF=0: System or Alternate Function

/*  --------------------------------------------------------------
    GPIOB: OUTPUT, Speed_25MHz medium, push pull, no pull 
	  --------------------------------------------------------------
	  | Pin: PB |     0      
    | SPI     |    SCK   
    | initial |   0 (low) 
	  --------------------------------------------------------------
*/
	#define  cc0   (0x03ul<<(pin_0*2) )    	// 11b
	#define  bb0   (0x01ul<<(pin_0*2) )    	// 01b
//		RCC->AHB1ENR |=  (1UL << bit_GPIOB);     // enable clock for GPIOB
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
		GPIO_SCK->BSRR = Bit(pin_0)<<16;	// Initial output value for SCK = 0
		
    GPIO_SCK->MODER   = (GPIO_SCK->MODER & ~cc0 )  | bb0; // MODE
		GPIO_SCK->OSPEEDR = (GPIO_SCK->OSPEEDR & ~cc0) | bb0; // Output Speed
    GPIO_SCK->OTYPER  &= ~Bit(pin_0); 		// Output Type
    GPIO_SCK->PUPDR   = (GPIO_SCK->PUPDR & ~cc0 );  			// Pull-up Pull-down
		GPIO_SCK->AFR[(pin_0/8)]  &= ~(0x0Ful<<pin_0*4);	  // AF=0: System or Alternate Function

/*  --------------------------------------------------------------
    GPIOF: OUTPUT, Speed_25MHz medium, push pull, no pull 
	  --------------------------------------------------------------
	  | Pin: PF |    11   
    | SPI     |  MOSI
    | initial |   x (not care)
	  --------------------------------------------------------------
*/
	#define  cc11   (0x03ul<<(pin_11*2) )    	// 11b
	#define  bb11  	(0x01ul<<(pin_11*2) )    	// 01b
	RCC->AHB1ENR |=  (1UL << bit_GPIOF);     // enable clock for GPIOF
	// 0:PORTA, 1: PORTB, ..., 10: PORTK
		
    GPIO_MO->MODER   = (GPIO_MO->MODER & ~cc11 )  | bb11; // MODE
		GPIO_MO->OSPEEDR = (GPIO_MO->OSPEEDR & ~cc11) | bb11; // Output Speed
    GPIO_MO->OTYPER  &= ~Bit(pin_11); 		// Output Type
    GPIO_MO->PUPDR   = (GPIO_MO->PUPDR & ~cc11 );  			// Pull-up Pull-down
		GPIO_MO->AFR[(pin_11/8)]  &= ~(0x0Ful<< ((pin_11-8)*4));	  // AF=0: System or Alternate Function
}			// end of Driver_GPIO
//#######################################################################

/* type change */
typedef unsigned char       bool;


//touch
/* --- include following 7 lines in *.h ----
void Driver_SPICS(bool sta);
void Driver_SPISCK(bool sta);
void Driver_SPIMOSI(bool sta);
bool Driver_SPIMISO(void);

void Delay_SPI(uint16_t Num);
void Touch_SPIWrite(uint8_t Val);
uint16_t Touch_SPIRead(void);
*/
//key
//-------------------------------------------------------------------------------------------------------
//-------------------------------------- TOUCH SPI DRIVER -----------------------------------------------
//-------------------------------------------------------------------------------------------------------

#define CS_bit 		Bit(13)
#define SCK_bit 	Bit(0)
#define MISO_bit 	Bit(2)
#define MOSI_bit 	Bit(11)

/********************************************************************************************************
*  Output Function: Driver_SPICS		(CS pin)
*  Object: touch ic spi enable/unable
*  brief:	set CS = 0 if t_f=0; CS = 1 if t_f = 1.
********************************************************************************************************/
void Driver_SPICS(bool t_f)
{
		if(!t_f) // t_f = 0
			GPIO_CS->BSRR = CS_bit<<16;	// reset ==> 0
		else		// t_f = 1
			GPIO_CS->BSRR = CS_bit;		// set ==> 1
}

/********************************************************************************************************
*  Output Function: Driver_SPISCK		(SCK pin)
*  Object: touch spi clock output
*  brief:	set SCK = 0 if t_f=0; SCK = 1 if t_f = 1.
********************************************************************************************************/
void Driver_SPISCK(bool t_f)
{
		if(!t_f) // t_f = 0
			GPIO_SCK->BSRR = SCK_bit<<16;	// reset ==> 0
		else		// t_f = 1
			GPIO_SCK->BSRR = SCK_bit;		// set ==> 1
}

/********************************************************************************************************
*  Output Function: Driver_SPIMOSI		(MOSI pin)
*  Object: master out
*  brief:	set MOSI = 0 if t_f=0; MOSI = 1 if t_f = 1.
********************************************************************************************************/
void Driver_SPIMOSI(bool t_f)
{
		if(!t_f) // t_f = 0
			GPIO_MO->BSRR = MOSI_bit<<16;	// reset ==> 0
		else		// t_f = 1
			GPIO_MO->BSRR = MOSI_bit;		// set ==> 1
}

/********************************************************************************************************
*  Input Function: Driver_SPIMISO		(MISO pin)
*  Object: master in
*  Return: 1 if MISO=1; 0 if MISO = 0 							                         	                                     
********************************************************************************************************/
bool Driver_SPIMISO(void)
{
		  if (GPIO_MI->IDR & MISO_bit)	// if MISO pin = 1
				return 1;
			else
				return 0;
}

/*-------------------------------------------------------------------------------------------------------
*  Touch IC transmission interface:
*  SPI mode: (CPOL:CPHA)= (0:0), MSBit (Commands: 8 bits; Data: 16 bits (or 12 bits)
-------------------------------------------------------------------------------------------------------*/
void Delay_SPI(uint16_t Num){
		volatile uint16_t Timer;
		while(Num--){
		 	Timer = 20;
			while(Timer--); }
}

//--------- Send data to SPI
void Touch_SPIWrite(uint8_t Val)
{
    uint8_t i;
    Driver_SPISCK(0);			// make sure that the idle clock is low
    for(i=0; i<8; i++)		// 8 bit mode
		{
				if(Val & Bit(7)) // most significant bit first
					Driver_SPIMOSI(1);
				else
					Driver_SPIMOSI(0);
				
				Val <<= 1;
        Driver_SPISCK(0);	// load at the FALLing edge of SCK clock
				Delay_SPI(1);					// hold half period
        Driver_SPISCK(1);	// capture at the RISing edge of SCK Clock
				Delay_SPI(1);					// hold half period
    }
		Driver_SPISCK(0);			// idle mode
		Delay_SPI(1);
}

//--------- Receive Data from SPI

//====================================
//#define Test_CaseNormal
//====================================
uint16_t Touch_SPIRead(void)
{
    uint8_t i;
    uint16_t Val = 0;

//	for(i=0; i<12; i++)			// 12-bit data: max value = 4095
													  // the least 4 significant bits are ignorable
	for(i=0; i<16; i++)			// 16-bit data: max value = 65536
		{
        Val <<= 1;
#ifndef Test_CaseNormal			
        Driver_SPISCK(1);	// as soon as the master captures data
			  Delay_SPI(1);
#endif
        Driver_SPISCK(0);	// slave loads data
				Delay_SPI(1);
        if(Driver_SPIMISO()){ // if MISO is high
					Val++;
				}
#ifdef Test_CaseNormal			
        Driver_SPISCK(1);	// master captures data
			  Delay_SPI(1);
#endif
    }
#ifdef Test_CaseNormal			
		Driver_SPISCK(0);			// idle mode
		Delay_SPI(1);
#endif
    return Val;
}
