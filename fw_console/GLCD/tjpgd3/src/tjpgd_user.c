/* Includes ------------------------------------------------------------------*/
/*
recommend: Heap Size >= 0x1000 = 4096
*/

#include "stm32f407xx.h"
#include "stm324xg_lcd_sklin.h"
#include <stdio.h>	// for sprintf
#include <stdlib.h>
#include "tjpgd.h"

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

/* Session identifier for input/output functions (name, members and usage are as user defined) */
typedef struct {
    FILE *fp;               /* Input stream */
    uint8_t *fbuf;          /* Output frame buffer */
    unsigned int wfbuf;     /* Width of the frame buffer [pix] */
} IODEV;

typedef struct
{		
	uint32_t	S_XOFF;	  	
	uint32_t  S_YOFF;
}_DisplayWindow;	// LCD display initial left top position

_DisplayWindow LCDwindow;

/*------------------------------*/
/* User defined input funciton  */
/*------------------------------*/
size_t in_func (    /* Returns number of bytes read or removed (zero on file read error) */
    JDEC* jd,       /* Decompression object */
    uint8_t* buff,  /* Pointer to the read buffer (null to remove data) */
    size_t nbyte    /* Number of bytes to read/remove */
){
		uint8_t* s = (uint8_t*) jd->device;
		uint8_t* d = buff;
		uint32_t cnt = nbyte;

		if (buff) { /* Raad data from input stream */
				while (cnt--){
					*d++ = *s++;
				}
				jd->device = s;
    } else {    /* Remove data from input stream */
				s = s + nbyte;
				jd->device = s;
    }
		return nbyte;
}

/*------------------------------*/
/* User defined output funciton */
/*------------------------------*/
int out_func (      /* Returns 1 to continue, 0 to abort */
    JDEC* jd,       /* Decompression object */
    void* bitmap,   /* Bitmap data to be output or DISPLAYed */
    JRECT* rect     /* Rectangular region of output image (DISPLAY WINDOW)*/
){
		uint16_t Xpos, Ypos, Xsize, Ysize;
		uint8_t *pdata;

    pdata = (uint8_t*)bitmap;                           /* Output bitmap */
		Xpos = rect->left + LCDwindow.S_XOFF;
		Ypos = rect->top + LCDwindow.S_YOFF;
		Xsize = rect->right - rect->left + 1;
		Ysize = rect->bottom - rect->top + 1;
		LCD_DrawRGBImage(Xpos, Ypos, Xsize, Ysize, pdata);
    return 1;    /* Continue to decompress */
}


uint8_t	get_scale(JDEC* jd, uint16_t S_Width, uint16_t S_Height)
{
		uint8_t	 scale; // image scaling factor: (0, 1, 2, 3) with respect to (1, 1/2, 1/4, or 1/8)
		uint16_t tmpW, tmpH;

				for(scale=0; scale<4; scale++)
				{ 
					tmpW = jd->width>>scale;
					tmpH = jd->height>>scale;
					if( (tmpW <= S_Width) && (tmpH<=S_Height) )
						// within the display window
					{	
						if(((tmpW)!=S_Width)&&((tmpH)!=S_Height && scale) )scale--;//不能貼邊,則不縮放
						break; 							
					} 
				} 
				if(scale==4) scale=3;
				return scale;
}

		char text[64];
char* get_JPG_error_code(void)
{
		return text;
}	

#ifdef Callback_Case	
#include "callback_example.h"

//################# Callback Exercise ###############
// Step 1. Type Declaration
//typedef size_t  (* CALLBACK_for_JPG_IN) (JDEC* jd, uint8_t* buff, size_t nbyte);
//// Step 2. Variable Declaration
//extern CALLBACK_for_JPG_IN  p_callback_InFunc;
//#########################################
#endif

/**
  * @brief  Draws a jpg picture 
  * @param  Xpos: X position in the LCD
  * @param  Ypos: Y position in the LCD
  * @param  p_jpgAddress: Pointer to jpg file address.
	* @retval JRESULT: 0 = OK; 3 = low Heap size; 5~8 = jd_prepare ERROR.
  */
#define SZ_workpool 3500	// 3500 = 0x0DAC  /* Size of work area (bytes) */
/*-----------------------------------------------------------------------*/
// S_Width=0 or S_Height=0, then no scaling, i.e., scale = 0
/*-----------------------------------------------------------------------*/
uint8_t LCD_DrawJPG(uint16_t Xpos, uint16_t Ypos, uint8_t *p_jpgAddress, uint16_t S_Width, uint16_t S_Height)	
{
    JRESULT res;      /* Result code of TJpgDec API */
    JDEC jdec;        /* Decompression object */
    void *pWorkpool;       /* Pointer to the work area */
		uint8_t scale;

		LCDwindow.S_XOFF = Xpos;
		LCDwindow.S_YOFF = Ypos;
    /* Prepare to decompress */
    pWorkpool = (void*) malloc(SZ_workpool);
		if (!pWorkpool){
      sprintf(text, "ERROR: NOT enough HEAP SIZE for SZ_workpool!");
			return JDR_MEM1;
		}
#ifdef Callback_Case		
//################# Callback Exercise ###############
		if (p_callback_InFunc == 0){
      sprintf(text, "ERROR: No input Function!");
			return JDR_INP;
		}
    res = jd_prepare(&jdec, p_callback_InFunc, pWorkpool, SZ_workpool, p_jpgAddress);
//#########################################		
#else		
    res = jd_prepare(&jdec, in_func, pWorkpool, SZ_workpool, p_jpgAddress);
#endif		
    if (res == JDR_OK) {
        /* It is ready to dcompress and image info is available here */
//        sprintf(text, "Image size is %u x %u.\n%u bytes of work ares is used.\n", jdec.width, jdec.height, sz_work - jdec.sz_pool);
				if( !S_Width || !S_Height) 
						// within the display window
				{
						scale = 0;
				}	else
				{
						scale = get_scale(&jdec, S_Width, S_Height);
				}
        /* Initialize output device */
        res = jd_decomp(&jdec, out_func, scale);   /* Start to decompress with 1/1 scaling */
        if (res == JDR_OK) {
            /* Decompression succeeded. You have the decompressed image in the frame buffer here. */
            sprintf(text, "Decompression succeeded.\n");

        } else {
					sprintf(text, "ERROR: jd_decomp() (rc=%d)\n", res);
        }

    } else {
			sprintf(text, "ERROR: jd_prepare() (rc=%d)\n", res);
    }
    free(pWorkpool);             /* Discard work area */
    return res;
}
