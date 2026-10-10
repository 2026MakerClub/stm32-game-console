#ifndef __myPower
#define __myPower

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

/* ------------- PWR registers bit address in the alias region ---------------*/
#define PWR_OFFSET      (PWR_BASE - PERIPH_BASE)

/* Alias word address of RPWEN bit in RCC->APB1ENR register */
#define APB1ENR_OFFSET       (0x40023840-0x40000000)        // RCC->APB1ENR @ 0x40023800 + 0x40
#define PWREN_BitNumber   28
#define RCC_APB1ENR_PWREN_BB       (PERIPH_BB_BASE + (APB1ENR_OFFSET * 32) + (PWREN_BitNumber * 4))

/* --- CR Register ---*/
/* Alias word address of DBP bit */
#define CR_OFFSET       (PWR_OFFSET + 0x00)         // 0x7000 (PWR) + 0x00 (CR)
#define DBP_BitNumber   0x08
#define CR_DBP_BB       (PERIPH_BB_BASE + (CR_OFFSET * 32) + (DBP_BitNumber * 4))

/* Alias word address of PVDE bit */
#define PVDE_BitNumber  0x04
#define CR_PVDE_BB      (PERIPH_BB_BASE + (CR_OFFSET * 32) + (PVDE_BitNumber * 4))

/* Alias word address of FPDS bit */
#define FPDS_BitNumber  0x09
#define CR_FPDS_BB      (PERIPH_BB_BASE + (CR_OFFSET * 32) + (FPDS_BitNumber * 4))

/* Alias word address of PMODE bit */
#define PMODE_BitNumber 0x0E
#define CR_PMODE_BB     (PERIPH_BB_BASE + (CR_OFFSET * 32) + (PMODE_BitNumber * 4))

/* --- CSR Register ---*/
/* Alias word address of EWUP bit */
#define CSR_OFFSET      ((PWR_BASE - PERIPH_BASE) + 0x04)		// 0x7000 (PWR) + 0x04 (CSR)
#define EWUP_BitNumber  0x08	// bit 8
#define CSR_EWUP_BB     (PERIPH_BB_BASE + (CSR_OFFSET * 32) + (EWUP_BitNumber * 4))

/* Alias word address of BRE bit */
#define BRE_BitNumber   0x09
#define CSR_BRE_BB      (PERIPH_BB_BASE + (CSR_OFFSET * 32) + (BRE_BitNumber * 4))

//--------------------
// short cut definition ----------
//#define my_PWR_ClockEnable     (RCC->APB1ENR |= Bit(28))  // bit 28 = (RCC_APB1ENR_PWREN))
#define my_PWR_ClockEnable     (*(uint32_t *) RCC_APB1ENR_PWREN_BB = 1)  // RCC->APB1ENR.PWREN=1
#define my_PWR_ClockDisable     (*(uint32_t *) RCC_APB1ENR_PWREN_BB = 0)  // RCC->APB1ENR.PWREN=0
#define FlashPowerDown_Set_Reset(x)	(*(uint32_t *) CR_FPDS_BB = x)			// x=0: disable; x=1: enable
#define Clear_Standby_n_Wakepin_flags (PWR->CR |= (Bit(2) | Bit(3)))  // PWR->CSR.CWUF and .CSBF not reset from standby mode
                                                                     // cleared by PWR->CR.[2:3]= 0b11

void PWR_EnterSTOPMode(void);
void PWR_EnterSTANDBYMode(void);

#endif
