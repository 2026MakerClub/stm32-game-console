#include "stm32f4xx.h"


uint8_t NUM_preBits, NUM_subpriBits;

void Global_preemption_bits(void)
{
	// Get number of preemption bits: or use "uint32_t stm32_Get_PreempPrioBits (void) "
	NUM_preBits = 7- ((SCB->AIRCR &0x0700)>>8);  // 7- bits[8:10]
	NUM_subpriBits = 4-NUM_preBits;
}

void Configure_NVICpriority(uint8_t IRQn, uint8_t pre, uint8_t subpri)
{
	if (NUM_preBits > 4) NUM_preBits = 4;
	if (pre >= (0x01<<(NUM_preBits)) )
	{
		pre = (0x01<<NUM_preBits) -1;	// lowest priority
	}
	
	
		pre = pre<< NUM_subpriBits;
	
	if (subpri >= (0x01<<(NUM_subpriBits))) 
	{
		pre |= (0x01<<NUM_subpriBits)-1;	// lowest priority
	}else
	{
		pre |= subpri;	
	}

  NVIC->IP[IRQn] = pre<<4;  // stm32f4 uses 4-bit priority
}
