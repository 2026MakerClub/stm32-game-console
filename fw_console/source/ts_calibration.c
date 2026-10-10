
/* Includes ------------------------------------------------------------------*/
#include "touch_module.h"
#include <stdio.h>	// for sprintf

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TouchValue_16bits
#ifdef	TouchValue_16bits
	#define Q_format 16384 // Q14
//	#define Q_format (1ul<<20) // Q20
#else			// TouchValue_12bits
	#define Q_format 1024 // Q10
#endif
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern uint8_t Calibration_Done;
extern int32_t  X1, Y1, Xx, Yx, Xy, Yy ;


/* Private function prototypes -----------------------------------------------*/
static void TouchscreenCalibration_SetHint(void);
static uint8_t GetTouchValues(int32_t LCDx, int32_t LCDy, int32_t aTch[2]) ;
static void TouchLabel(char text[10], uint16_t BackColor); 

/* Private functions ---------------------------------------------------------*/
#include "user_defined.h"

#define ABS(X)  ((X) > 0 ? (X) : -(X))
void	Normal_eq_for_4x3_Matrix(int32_t aa[4][2], int32_t b[4], float newA[3][3], float new_b[3], uint8_t Index);
void Gaussian_for_3x3(float aa[3][3], float b[3], float xx[3]);
	uint32_t  Q_f = (1ul<<14);
	uint32_t  Q1, Q2, Q3;
#define Offset_Pixel	15
/**
  * @brief  Performs the TS calibration
  * @param  None
  * @retval None
*/
void Touchscreen_Calibration (void)
{ 
  uint8_t i = 0;
  int32_t aTch[4][2], bLCDx[4], bLCDy[4];
		uint16_t  XL, YL;
		char text[64];
		float	xx[3], newA[3][3], new_b[3];

	  /* Clear the LCD */ 
  LCD_Clear(WHITE);
restart:	
  TouchscreenCalibration_SetHint();
  
      bLCDx[0] = LCD_Pixel_Width() - Offset_Pixel;
      bLCDy[0] = Offset_Pixel;
      bLCDx[1] = Offset_Pixel;
      bLCDy[1] = Offset_Pixel;
      bLCDx[2] = Offset_Pixel;
      bLCDy[2] = LCD_Pixel_Height() - Offset_Pixel;
      bLCDx[3] = LCD_Pixel_Width() - Offset_Pixel;
      bLCDy[3] = LCD_Pixel_Height() - Offset_Pixel;
      
      for (i = 0; i < 4; i++) 
      {
				uint8_t  abort;
        abort = GetTouchValues(bLCDx[i], bLCDy[i], aTch[i]);
				if (abort == 1){
					return;
				}
				
 				sprintf(text, "point %01d: X=%06d Y=%06d    ", i, aTch[i][0], aTch[i][1]);
				LCD_SetColors(RED, YELLOW);
				LCD_DisplayStringAtLine(2+i, (char *)text);
     }
		delay_ms(1000);

/*	---------- LCDx axis ---------------------------
		bLCDx[i] = xx[0] + xx[1] * aTch[i][0] + xx[2] * aTch[i][1]
	  ==> b[i] == bLCDx[i]
		==> ai[0] == aTch[i][0]
		==> ai[1] == aTch[i][1]
			==> A x = b:
		 [1		a0[0]		a0[1]] [xx[0]	]		[b[0]]
	   [1		a1[0]		a1[1]] [xx[1]	] =	[b[1]]
	   [1		a2[0]		a2[1]] [xx[2]	] 	[b[2]]
	   [1		a3[0]		a3[1]]          	[b[3]]
//	-------- normal equations  A^{T} A x = A^{T} b ==> (newA) x = new_b ---------------
// void	Normal_eq_for_4x3_Matrix(int32_t a[4][2], int32_t b[4], float newA[3][3], 
//																float new_b[3], uint8_t Index)
//	Index: 1 = calculate newA; 0 = use the input values of newA 		 
//--------------------------------------------------------------------		 
//	void Gaussian_for_3x3(float aa[3][3], float b[3], float xx[3])
//------------------------------------------------------------------
			 newA = A^{T} A =
		 [1		      1		    1       1  ]  [1		a0[0]		a0[1]]
	   [a0[0]		a1[0]		a2[0]   a3[0]]  [1		a1[0]		a1[1]]
	   [a0[1]		a1[1]		a2[1]   a3[1]]  [1		a2[0]		a2[1]]
	                                    [1		a3[0]		a3[1]]  
			 -----------------
			new_b = A^{T} b =
		 [1		      1		    1       1  ]  [b[0]]
	   [a0[0]		a1[0]		a2[0]   a3[0]]  [b[1]]
	   [a0[1]		a1[1]		a2[1]   a3[1]]  [b[2]]
																			[b[3]]  
*/	//-------------------------------------------
		Normal_eq_for_4x3_Matrix(aTch, bLCDx, newA, new_b, 1);
		Gaussian_for_3x3(newA, new_b, xx);
		X1 = (int32_t) (xx[0]*Q_format);	// X1 = xx[0]
		Xx = (int32_t) (xx[1]*Q_format);  // Xx = X2 = xx[1]
		Xy = (int32_t) (xx[2]*Q_format);  // Xy = X3 = xx[2]
			Q1 = (1ul<<30) / ABS(xx[0]);		// Q1 = 2**30 / (|xx[0]|)
			Q2 = (1ul<<10) / ABS(xx[1]);		// Q2 = 2**10 / (|xx[1]|)
			Q3 = (1ul<<10) / ABS(xx[2]);		// Q1 = 2**10 / (|Xx[2]|)
			//-- Qf = min( Q1, Q2, Q3) ---
			if (Q1 < Q2){
				if (Q1 < Q3){
					Q_f = Q1;
				} else {
					Q_f = Q3;
				}
			}else{
				if (Q2 < Q3) {
					Q_f = Q2;
				}else{
					Q_f = Q3;
				}
			}

/*	---------- LCDy axis ---------------------------
		bLCDy[i] = xx[0] + xx[1] * aTch[i][0] + xx[2] * aTch[i][1]
	  ==> b[i] == bLCDy[i]
		==> ai[0] == aTch[i][0]
		==> ai[1] == aTch[i][1]
			==> A x = b:
		 [1		a0[0]		a0[1]] [xx[0]	]		[b[0]]
	   [1		a1[0]		a1[1]] [xx[1]	] =	[b[1]]
	   [1		a2[0]		a2[1]] [xx[2]	] 	[b[2]]
	   [1		a3[0]		a3[1]]          	[b[3]]
*/	//-------------------------------------------
		Normal_eq_for_4x3_Matrix(aTch, bLCDy, newA, new_b, 0);
		Gaussian_for_3x3(newA, new_b, xx);
		Y1 = (int32_t) (xx[0]*Q_format);	// Y1 = xx[0]
		Yx = (int32_t) (xx[1]*Q_format);  // Yx = Y3 = xx[1]
		Yy = (int32_t) (xx[2]*Q_format);  // Yy = Y2 = xx[2]

      Calibration_Done = 1;

			LCD_Clear(WHITE);

			LCD_SetColors(BLUE, WHITE);
			LCD_DisplayStringLineCol(1, 2, "Coefficients:   ");					
				LCD_DisplayStringLineCol(2, 2, "x_LCD = (X1 + Xx * X_tch ");					
				LCD_DisplayStringLineCol(3, 2, "            + Xy * Y_tch)/1024 ");					
				LCD_DisplayStringLineCol(4, 2, "y_LCD = (Y1 + Yx * X_tch ");					
				LCD_DisplayStringLineCol(5, 2, "            + Yy * Y_tch)/1024 ");					

				sprintf(text, "X1 = %08d, Xx =  %05d, ", X1, Xx);
				LCD_DisplayStringLineCol(7, 2, text);					
				sprintf(text, "Xy =  %05d", Xy);
				LCD_DisplayStringLineCol(8, 17, text);					
				sprintf(text, "Y1 = %08d, Yx =  %05d, ", Y1, Yx);
				LCD_DisplayStringLineCol(9, 2, text);					
				sprintf(text, "Yy =  %05d", Yy);
				LCD_DisplayStringLineCol(10, 17, text);					
			
			XL = LCD_Pixel_Width();
			YL = LCD_Pixel_Height();
			LCD_SetFont(&Font20);
			LCD_SetColors(RED, BLUE);
			LCD_DisplayStringAt(XL/2-60, YL-60, (char*)"  Close  ", LEFT_MODE);
//			TouchLabel("  Close  ", BLUE);
			LCD_SetBackColor(DARKGREEN);
			LCD_DisplayStringAt(5, YL-30, (char*)"Press Key 1 to retry!", LEFT_MODE);
			
			//>>>>>-------------------------------
			while(1){
				uint16_t x, y;
				
				if (TSC_TouchDet(1)) {           // Show touch screen activity         
					Touch_GetVal(&x, &y);   // get x, y from touch module 
					if (LCD_Pixel_Width() == 319)
					{
								sprintf(text, "touch: X=%05d,Y=%05d", x, y);
								LCD_DisplayStringAtLine(7, (char *)text);
					} else
					{
								sprintf(text, "touch: X=%05d", x);
								LCD_DisplayStringAtLine(8, (char *)text);
								sprintf(text, "       Y=%05d ", y);
								LCD_DisplayStringAtLine(9, (char *)text);
					}
		Cal_GetXY(&x, &y);
				//###==== 	
					if (LCD_Pixel_Width() == 319)
					{
								sprintf(text, "LCD:  x=%04d  y=%04d", x, y);
								LCD_DisplayStringAtLine(8, (char *)text);
					} else
					{
								sprintf(text, "LCD:   x=%04d", x);
								LCD_DisplayStringAtLine(10, (char *)text);
								sprintf(text, "       y=%04d", y);
								LCD_DisplayStringAtLine(11, (char *)text);
					}
				//###====
					if (Touch_sensing_2(5, XL/2-60, YL-60, (Font20.Width)*9, (Font20.Height) ) ){
						break;			// break if (return value  != 0)
					}
				}
				
				// If Key 1 (PB8) is pressed, retry the calibation
				if((GPIOB->IDR & Bit(8)) == 0)						// PB8: normally high
				{
					while ((GPIOB->IDR & Bit(8)) == 0);
					delay_ms(20);
						LCD_SetBackColor(WHITE);
						LCD_Clear(LCD_COLOR_WHITE);
						LCD_DisplayStringAt(0, 50, (char *)"Retrying", CENTER_MODE);
						goto restart;
				}
				delay_ms(10);
			}
			//<<<<<---------------------------------

			LCD_SetTextColor(GRAY);
			LCD_FillRect(XL/2-60, YL-60, (Font20.Width)*9, (Font20.Height));
			WaitForTouchRelease(5);
			LCD_Clear(WHITE);
}

//========================================================================
//========================================================================
void	Normal_eq_for_4x3_Matrix(int32_t a[4][2], int32_t b[4], float newA[3][3], 
																float new_b[3], uint8_t Index)
//		a0[i] = a[0][i]; a1[i] = a[1][i]; a2[i] = a[2][i]; a3[i] = a[3][i];
/*	-------- normal equations  A^{T} A x = A^{T} b ---------------
			 newA = A^{T} A =
		 [1		      1		    1       1  ]  [1		a0[0]		a0[1]]
	   [a0[0]		a1[0]		a2[0]   a3[0]]  [1		a1[0]		a1[1]]
	   [a0[1]		a1[1]		a2[1]   a3[1]]  [1		a2[0]		a2[1]]
	                                    [1		a3[0]		a3[1]]  
			 -----------------
			new_b = A^{T} b =
		 [1		      1		    1       1  ]  [b[0]]
	   [a0[0]		a1[0]		a2[0]   a3[0]]  [b[1]]
	   [a0[1]		a1[1]		a2[1]   a3[1]]  [b[2]]
																			[b[3]]  
*/	//-------------------------------------------
/*	-------- Step 2 modify newA ---------------
    -------- so that the first column of newA is [1, 1, 1]^{T} 																
			newA = diag{1/newA[0][0], 1/newA[1][0], 1/newA[2][0]} * newA
			Note: the values of column 0 is saved for future use to get new_b. 
			 -----------------
			new_b = diag{1/newA[0][0], 1/newA[1][0], 1/newA[2][0]} * new_b
*/	//-------------------------------------------
{
		uint8_t i, j, k;
		float temp[3], tempX;

//-------- new  A (symmetrical matrix) ----------------
	if(Index)		// Index = 1: calculate newA
	{
			newA[0][0] = temp[0] = 4;	
			for (j = 0; j < 2; j++) 
			{
				temp[j+1] = 0;
				for (k = 0; k<4; k++)
					temp[j+1] += (float) a[k][j];
				// row 0: temp[1] = SUM(a[k][0]);  temp[2] = SUM(a[k][1]);
				
				newA[0][j+1] = temp[j+1]/temp[0];	
				// newA[0][1] = SUM(a[k][0])/ newA[0][0]; 
				// newA[0][2] = SUM(a[k][1])/ newA[0][0].
				newA[j+1][0] = temp[j+1];	// symmetrical matrix
				// newA[1][0] = SUM(a[k][0]); 
				// newA[2][0] = SUM(a[k][1]).
			}
			for (i = 0; i < 2; i++) 
			{
				for (j = i; j < 2; j++)
				{
					tempX = 0;
					for (k = 0; k<4; k++)
						tempX += ((float) a[k][i]) * ((float) a[k][j]);
				// A(1,1): tempX = SUM(a[k][0] * a[k[0]); A(1,2): tempX = SUM(a[k][0] * a[k[1]);
				// A(2,2): tempX = SUM(a[k][1] * a[k[1])
						
					newA[i+1][j+1] = tempX / temp[i+1];
				// newA[1][1] = A(1,1)/ newA[1][0]; 
				// newA[1][2] = A(1,2)/ newA[2][0].
				// newA[2][2] = A(2,2)/ newA[2][0].
					if (i != j) newA[j+1][i+1] = tempX / temp[j+1]; // no more symmetrical
				// newA[2][1] = A(1,2)/ newA[1][0].
				}
			}
		} else	// Index = 0: use the input values of newA
		// Note: the values of newA remain unchanged after "Gaussian_for_3x3"
		{
			temp[0] = 4;
			// temp[1] = newA[1][0]; temp[2] = newA[2][0]; 
			for (i = 1; i < 3; i++) 
			{
				temp[i] = newA[i][0];
			}
		}
//-------- new b -----------------
		tempX = 0;
    for (i = 0; i < 4; i++) 
    {
			tempX += (float) b[i];
		}
		new_b[0] = tempX / temp[0];
		for (i = 0; i<2; i++)
		{
			tempX = 0;
      for (k = 0; k < 4; k++) 
				tempX += ((float) a[k][i]) * ((float) b[k]);	// A^{T} * b

			new_b[i+1] = tempX/temp[i+1];			
		}
}

//==================================================================
//========================================================================
void Gaussian_for_3x3(float aa[3][3], float b[3], float xx[3])
/*	---------- STEP 1 ---------------------------
	   [1		a01		a02] [xx[0]	]		[b0]
	   [1		a11		a12] [xx[1]	] =	[b1]
	   [1		a21		a22] [xx[0]	] 	[b2]
		----------- STEP 2 ----------------------
	   [1		a01				a02    ] [xx[0]	]	  [b0]
	   [0		a11-a01		a12-a02] [xx[1]	] =	[b1-b0]
	   [0		a21-a01		a22-a02] [xx[0]	]	  [b2-b0]
		----------- STEP 3 ---------------------
	   [1		a01				a02    				 ] [xx[0] ]		[b0]
	   [0		a11-a01		a12-a02				 ] [xx[1]	] =	[b1-b0]
	   [0		0		(a22-a02)-w*(a12-a02)] [xx[0]	] 	[(b2-b0)-w*(b1-b0)]
				w = (1/(a11-a01)) * (a21-a01)
*/	//-------------------------------------------
{ 
	float a11, a12, a22, w, t[3];		// t[3] uses stack memory instead of RAM memory to increase efficency
	//---- Forward ----------
	a11 = (aa[1][1]-aa[0][1]);	// a11-a01
	a12 = (aa[1][2]-aa[0][2]);	// a12-a02
	a22 = (aa[2][2]-aa[0][2]); 	// a22-a02
	w = ((aa[2][1]-aa[0][1]) ) / a11;	// (a21 - a01)/a11
	a22 = a22 - w * a12; 								// new a22 = a22 - (a21/a11)*a12
	t[0] = b[0];
	t[1] = (b[1] - b[0]);
	t[2] = (b[2] - b[0]) - w * t[1];
	//---- Backward ----------------
	t[2] = t[2] / a22;
	t[1] = t[1] - a12 * t[2];
	t[1] = t[1] / a11;
	t[0] = t[0] -  aa[0][2] * t[2] - aa[0][1] * t[1];
	xx[0] = t[0];
	xx[1] = t[1];
	xx[2] = t[2];
}

void Touch_GetVal(uint16_t *pX, uint16_t *pY);
void Cal_GetXY(uint16_t* pX, uint16_t* pY);

/**
  * @brief  Display calibration hint
  * @param  None
  * @retval None
  */
static void TouchscreenCalibration_SetHint(void)
{
  /* Set Touchscreen Demo description */
//  LCD_SetTextColor(LCD_COLOR_BLACK);
//  LCD_SetBackColor(LCD_COLOR_WHITE);
		LCD_SetColors(BLACK, WHITE);

  LCD_SetFont(&Font12);
  LCD_DisplayStringAt(0, LCD_Pixel_Height()/2 - 27, (char *)"Before using the Toucscreen", CENTER_MODE);
  LCD_DisplayStringAt(0, LCD_Pixel_Height()/2 - 12, (char *)"you need to calibrate it.", CENTER_MODE);
  LCD_SetFont(&Font16);
		LCD_SetColors(BLUE, WHITE);
  LCD_DisplayStringAt(0, LCD_Pixel_Height()/2 + 3, (char *)"Press on the circles", CENTER_MODE);
		LCD_SetFont(&Font20);
		LCD_SetColors(RED, GREEN);
		LCD_DisplayStringAt(LCD_Pixel_Width()/2-100, LCD_Pixel_Height()-60, (char*)" Abort by Key1 ", LEFT_MODE);
  LCD_SetFont(&Font12);
}



void Draw_donut(int16_t x, int16_t y, int16_t Color_out, int16_t radius)
{
  LCD_SetTextColor(Color_out);
  LCD_FillCircle(x, y, radius);
  LCD_SetTextColor(LCD_COLOR_WHITE);
  LCD_FillCircle(x, y, 2);
}

static void TouchLabel(char text[10], uint16_t BackColor) 
{
	uint16_t XL, YL;	
	LCD_SaveColors();
	LCD_SaveFont();
			LCD_SetFont(&Font20);
			LCD_SetColors(RED, BackColor);
			XL = LCD_Pixel_Width();
			YL = LCD_Pixel_Height();
//			LCD_DisplayStringAt(XL/2-60, YL-60, (char*)"  Abort  ", LEFT_MODE);
			LCD_DisplayStringAt(XL/2-60, YL-60, text, LEFT_MODE);
	LCD_RestoreColors();
	LCD_RestoreFont();
}
/**
  * @brief  Get Touch position
  * @param  LCDx : logical X position
  * @param  LCDy : logical Y position
  * @param  aT[0] : Physical X position
  * @param  aT[1] : Physical Y position
  * @retval None
  */
static uint8_t GetTouchValues(int32_t LCDx, int32_t LCDy, int32_t aT[2]) 
{
	uint16_t X_t, Y_t, count;
	uint16_t XL, YL;
	
	LCD_SaveColors();
start_again:	
#define CountNUM 50	
	count = CountNUM;
  LCD_SetTextColor(DARKMAGENTA);
	LCD_DrawSimpleCross(LCDx, LCDy, 31);
  /* Draw the ring */
	Draw_donut(LCDx, LCDy, DARKGREEN, 8);
  
  /* Wait until touch is pressed */
  //WaitForPressedState(1);
	while(1)
  {
		if (TSC_TouchDet(1) == 1)
    {
			if (count == CountNUM){
				Draw_donut(LCDx, LCDy, RED, 8);			
				Touch_GetVal(&X_t, &Y_t);   // dummy action to eliminate some hardware bug 
			}
      if(--count == 0){
				Touch_GetVal(&X_t, &Y_t);   // get x, y from touch module 
					break;
			}
		} else
		{
			if (count < CountNUM)	goto start_again;	// This condition is to eliminate FLASHING!
		}
    delay_ms(10);
		if((GPIOB->IDR & Bit(8)) == 0)					// PB8: normally high
 		{
				while ((GPIOB->IDR & Bit(8)) == 0);
				return 1;
		}
 }
  LCD_SetTextColor(WHITE);
	LCD_DrawSimpleCross(LCDx, LCDy, 31);		// erase the drawn cross
  LCD_FillCircle(LCDx, LCDy, 8);					// erase the drawn donut
  
  aT[0] = X_t;
  aT[1] = Y_t; 
  
	LCD_RestoreColors();
  /* Wait until touch is released */
	WaitForTouchRelease(CountNUM);
	return 0;
}



/**check if the TS is calibrated
  * @param  None
* @retval calibration state (1 : calibrated / 0: no)
  */
uint8_t IsCalibrationDone(void)
{
  return (Calibration_Done);
}

void SkipCalibration(void)
{
  Calibration_Done=1;
}

void RedoCalibration(void)
{
  Calibration_Done=0;
}

/**
  * @}
  */
  
/**
  * @}
  */ 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
