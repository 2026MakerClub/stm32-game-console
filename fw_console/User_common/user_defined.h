// USER Defined ShortCUT 
#ifndef __STM32F407xx_H
#include "stm32f407xx.h"
#endif

#ifndef __stdint_h
#include "stdint.h"
#endif

#ifndef __user_defined_H
#define __user_defined_H


#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif
//====================
//  GPIO
//--------------------
#define GPIOx_OUTPUT_setPINS(GPIOx, _bitmap) (GPIOx->BSRR  = (uint32_t)_bitmap)
#define GPIOx_OUTPUT_resetPINS(GPIOx, _bitmap) (GPIOx->BSRR  = (uint32_t) _bitmap<<16)
#define GPIOx_OUTPUT_togglePINS(GPIOx, _bitmap) (GPIOx->ODR  ^= (uint32_t) _bitmap)

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
//====================
//  Embedded Flash Interface
//=========================================================================== Embedded Flash Configuration
//   <h> Embedded Flash for code programming (FLASH_CR.SNB)
//     <o0> Start user SECTOR No.
//                     <0=> 0 (16KB) at 0x08000000
//                     <1=> 1 (16KB) at 0x08004000
//                     <2=> 2 (16KB) at 0x08008000
//                     <3=> 3 (16KB) at 0x0800C000
//                     <4=> 4 (64KB) at 0x08010000
//                     <5=> 5 (128KB) at 0x08020000
//                     <6=> 6 (128KB) at 0x08040000
//                     <7=> 7 (128KB) at 0x08060000
//                     <8=> 8 (128KB) at 0x08080000
//                     <9=> 9 (128KB) at 0x080A0000
//                    <10=> 10 (128KB) at 0x080C0000
//                    <11=> 11 (128KB) at 0x080E0000
//     <o1> Final user SECTOR No.
//                     <0=> 0 (16KB) at 0x08000000
//                     <1=> 1 (16KB) at 0x08004000
//                     <2=> 2 (16KB) at 0x08008000
//                     <3=> 3 (16KB) at 0x0800C000
//                     <4=> 4 (64KB) at 0x08010000
//                     <5=> 5 (128KB) at 0x08020000
//                     <6=> 6 (128KB) at 0x08040000
//                     <7=> 7 (128KB) at 0x08060000
//                     <8=> 8 (128KB) at 0x08080000
//                     <9=> 9 (128KB) at 0x080A0000
//                    <10=> 10 (128KB) at 0x080C0000
//                    <11=> 11 (128KB) at 0x080E0000
//     <o2> CR.PSIZE: Fixed number of bytes to be programmed each time
//                     <0=> 1B (for 1.8 - 2.1 V)
//                     <1=> 2B (for 2.1 - 2.7 V) 
//                     <2=> 4B (for 2.7 - 3.6 V)
//                     <3=> 8B (with Ext. Vpp) 
//   </h>
#define START_FLASH_SECTOR             2
#define FINAL_FLASH_SECTOR             5
#define FLASH_PSIZE	                   0x02
//====================
//  Embedded Flash Interface
//#define RDP_KEY                  ((uint16_t)0x00A5)
//#define FLASH_KEY1               ((uint32_t)0x45670123)
//#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)
//#define FLASH_OPT_KEY1           ((uint32_t)0x08192A3B)
//#define FLASH_OPT_KEY2           ((uint32_t)0x4C5D6E7F)
//--------------------
#define FLASH_clear_allErrorStatus_SR (FLASH->SR = 0x000001F2)         //clear all error status flags (bits 1, 4~8) by writing 1;
#define FLASH_lock_REGISTER_CR (FLASH->CR |= Bit(31))   //FLASH_CR_LOCK
#define FLASH_unlock_REGISTER_CR                                               \
                                  do{                                          \
                                     if((FLASH->CR & Bit(31)) != 0)            \
                                     {                                         \
                                      FLASH->KEYR = ((uint32_t)0x45670123);    \
                                      FLASH->KEYR = ((uint32_t)0xCDEF89AB);    \
                                     }                                         \
                                    }while (0)     // KEY1="0x45670123" and KEY2="0xCDEF89AB"
#define FLASH_clear_REGISTER_CR_9bits (FLASH->CR &= 0xFFFFFC00);  // bits 0~6, 8, 9

#define FLASH_unlock_REGISTER_OPTCR                                               \
                                  do{                                          \
                                      FLASH->OPTKEYR = ((uint32_t)0x08192A3B); \
                                      FLASH->OPTKEYR = ((uint32_t)0x4C5D6E7F); \
                                    }while (0)     // KEY1="0x08192A3B" then KEY2="0x4C5D6E7F"
          
uint32_t FLASH_check_BUSY_bit(void);
uint32_t  User_FLASH_SectorErase(uint32_t beginSector, uint32_t endSector, uint32_t pSize);
uint32_t  User_FLASH_beforeWRITE(uint32_t pSize);
void delay_ms(uint32_t i);
void hold_key_ms(uint32_t wait_ms);
void hold_EXTI_ms(uint32_t wait_ms);
uint8_t get_ButtonHoldState(void);

extern volatile uint32_t msTick;
																		
//#####################################
#endif  //#ifndef __user_defined_H

