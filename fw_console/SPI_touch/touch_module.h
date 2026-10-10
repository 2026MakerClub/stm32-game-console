#include "stdint.h"
#include "stm324xg_lcd_sklin.h"

typedef struct
{
  uint16_t TouchDetected;
  uint16_t x;
  uint16_t y;
}TS_StateTypeDef; 

void Touch_GetVal(uint16_t *pX, uint16_t *pY);
uint32_t TSC_TouchDet (uint16_t Cnum);
void TS_GetState(TS_StateTypeDef* pTS_State);
void Touch_sensing(uint16_t Cnum, uint16_t xp, uint16_t yp, uint16_t width, uint16_t highth);
void WaitForTouchRelease(uint32_t Cnum);
void Cal_GetXY(uint16_t* pX, uint16_t* pY);
uint8_t Touch_sensing_2(uint16_t Cnum, uint16_t xp, uint16_t yp, uint16_t width, uint16_t highth);
