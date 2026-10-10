
#ifndef __myRTC
#define __myRTC

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

/* --- BDCR Register ---*/
/* Alias word address of RTCEN bit */
#define RCC_BDCR_OFFSET           (0x40023870-0x40000000)
#define RTCEN_BitNumber           0x0F
#define BDCR_RTCEN_BB             (PERIPH_BB_BASE + (RCC_BDCR_OFFSET * 32) + (RTCEN_BitNumber * 4))
/* Alias word address of BDRST bit */
#define BDRST_BitNumber           0x10
#define BDCR_BDRST_BB             (PERIPH_BB_BASE + (RCC_BDCR_OFFSET * 32) + (BDRST_BitNumber * 4))

#define RTC_ASYNCH_PREDIV    0x7F					// (7 bits) 128 = 0x7F + 0x01
#define RTC_SYNCH_PREDIV     0x0130				// (15 bits) 305 = 0x130 + 1

#ifndef	CR_DBP_BB
#define CR_DBP_BB       (PERIPH_BB_BASE + (((PWR_BASE - PERIPH_BASE)) * 32) + (0x08 * 4))
#endif

#define my_Backup_domain_ACCESS   (*(uint32_t *) CR_DBP_BB = 1)    // PWR->CR.DBP = 1
#define my_Backup_domain_noACCESS  (*(uint32_t *) CR_DBP_BB = 0)					  // PWR->CR.DBP = 0

//---------------------
#define RTC_WPR_unlock_key1		(RTC->WPR = 0xCA)
#define RTC_WPR_unlock_key2		(RTC->WPR = 0x53)
#define RTC_WPR_lock		      (RTC->WPR = 0xFF)			// Writing a wrong key reactivates the RTC write protection.
#define WUTE_WUTIE	(Bit(10) | Bit(14))

//============================
typedef struct{
		unsigned Seconds: 8;		// bit 7 reserved
		unsigned Minutes: 8;		// bit 7 reserved
		unsigned Hours:   8;		// bit 7 reserved
		unsigned rev:	   8;
}RTC_timeTYPE;

typedef struct{
	unsigned Date: 	 8;		// bits [7:6] reserved
	unsigned W_M_byte: 8;  // unsigned Month:  5;  unsigned WeekDay: 3;
	unsigned Year:    8;
	unsigned Century: 8;
}RTC_dateTYPE;
#define u_WeekDay_Month(w,M)	((w<<5) | M)
#define Alarm_weekdayBit		0x40		// bit 6 = 1

typedef struct{
	unsigned Seconds: 8;		// bit 7 is MSK1 (not compare)
	unsigned Minutes: 8;		// bit 7 is MSK2 (not compare)
	unsigned Hours:   8;		// bit 7 is MSK3; bit 6 (PM) = 0 
	unsigned Date_o_Wkday: 	 8;		// bits [0:5]= Date When bit 6=0; bits [0:3]=Wkday When bit 6 = 1
                                // bit 7 is MSK4 (not compare)  
}RTC_AlarmTypeDef;


typedef struct{
	unsigned Sec: 8;		// bit 1 as MSK1 (not compare)
	unsigned Min: 8;		// bit 1 as MSK2 (not compare)
	unsigned Hr:   8;		// bit 1 as MSK3; bit 6 (PM) = 0 
	unsigned DaWk: 	 8;		// bit 1 as MSK4 (not compare)  
}RTC_AlarmMSK_TypeDef;

/*----  All Date and Time data in BCD -----------------
* An example:
	p_datestructure->Century = 	 0x20;		// Century (ignorable)
	p_datestructure->Year = 	 0x16;		// yesr - cnetury
	p_datestructure->W_M_byte = u_WeekDay_Month(0x4,0x2);		// w=4: Weekday, M=2: Month
	p_datestructure->Date = 	 0x18;		// day date

	p_timestructure->Hours = 	 0x23;		// Hours
	p_timestructure->Minutes = 	 0x59;		// Minutes
	p_timestructure->Seconds = 	 0x00;		// Seconds
//-------------------------------------------- */
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


extern uint8_t Alarm_A_triggered;
extern uint8_t Alarm_B_triggered;

//---------------------
void Enable_LSI(void);
void write_access_Backup_domain(void);
void Disable_write_protection_RTC(void);
void stm32f4_RTC_autoWakeUP(uint32_t counts);
void RCC_BDCR_for_RTC_Enable(void);
void Enable_BKPSRAM_Regulator(void);
void Configure_RTC_PRE_CR(void);
void Configure_RTC_TimeStamp(void);
void RTC_Time_Date_setting(RTC_dateTYPE* p_date, RTC_timeTYPE* p_time);
void RTC_Alarm_A_setting(RTC_AlarmTypeDef* p_alarm, RTC_AlarmMSK_TypeDef* p_MSK);

void Global_preemption_bits(void);
void Configure_NVICpriority(uint8_t IRQn, uint8_t pre, uint8_t subpri);


#endif


