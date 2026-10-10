#include "stm32f407xx.h"
#include "stm324xg_lcd_sklin.h"
#include <stdio.h>	// for sprintf
#include <stdlib.h>
#include "gif.h"

#ifndef Bit
#define Bit(x) 	(0x01ul<<x)
#endif

//#define Read_File
//############################# NEW Additional
//gif資料登錄回呼函數
//s_gif:儲存待解碼的物件資訊的結構體
//buff:輸入資料緩衝區 (NULL:執行位址偏移)
//nbyte:需要從輸入資料流程讀出的資料量/位址偏移量
//返回值:讀取到的位元組數/位址偏移量
uint32_t gif_in_func(gif89a* s_gif, uint8_t* buff, uint32_t nbyte) 
{ 
#ifdef Read_File	
   uint32_t  rb; //讀取到的位元組數
		FILE*	dev = s_gif-> device;
    if(buff)     				//讀取資料有效，開始讀取資料
    { 
        rb = fread(buff, 1, nbyte, dev);//調用FATFS的f_read函數，用於把gif檔的資料讀取出來
        return rb;        		//返回讀取到的位元組數目
    }else{
//			return (f_lseek(dev, f_tell(dev)+nbyte)==FR_OK)?nbyte:0;//重新定位資料點，相當於刪除之前的n位元組資料 
			return fseek (dev, nbyte, SEEK_CUR) ? 0: nbyte;
		}
#else
		uint8_t* s = (uint8_t*) s_gif->device;
		uint8_t* d = buff;
		uint32_t cnt = nbyte;

		if (buff) { /* Raad data from input stream */
				while (cnt--){
					*d++ = *s++;
				}
				s_gif->device = s;
    } else {    /* Remove data from input stream */
				s = s + nbyte;
				s_gif->device = s;
    }
		return nbyte;
#endif		
} 

//#####################################################################################
void gif_draw_HLine(uint16_t Xpos, uint16_t Ypos, uint16_t Length, uint16_t color)
{
	LCD_SetTextColor(color);
	if (Length > 1)
//			pic_phy.draw_hline(Xpos, Ypos, Length, color);
			LCD_DrawHLine(Xpos, Ypos, Length);
	else	
//			pic_phy.draw_point(Xpos, Ypos, color);
			LCD_DrawHLine(Xpos, Ypos, Length);
}

//#####################################################################################
void gif_fill_Rect(uint16_t Xpos, uint16_t Ypos, uint16_t X1, uint16_t Y1, uint16_t color)
{
	uint16_t Width, Height;
	
	LCD_SetTextColor(color);
	Width = X1 - (Xpos -1);
	Height = Y1 - (Ypos -1);
	LCD_FillRect(Xpos, Ypos, Width, Height);
}

//#####################################################
/**
  * @brief  Draws a gif picture 
  * @param  Xpos: X position in the LCD
  * @param  Ypos: Y position in the LCD
  * @param  p_gifAddress: Pointer to gif file/buffer address.
	* @retval res: 0 = OK; others =  ERROR.
  */
/*-----------------------------------------------------------------------*/
// S_Width=0 or S_Height=0, then no cutting, i.e., 
/*-----------------------------------------------------------------------*/
uint8_t LCD_DrawGIF(uint16_t Xpos, uint16_t Ypos, uint8_t *p_gifAddress, uint16_t S_Width, uint16_t S_Height)	
{
	uint8_t res;
	uint16_t X0, Y0;
	uint16_t dtime;//解碼延時
	gif89a *mygif89a;
#ifdef Read_File	
	FILE *g_file;
#endif
	
	if(S_Width==0 || S_Height==0) return GIF_WINDOW_ERR;	//視窗設定錯誤
	
#if GIF_USE_MALLOC==1 	//定義是否使用malloc,這裡我們選擇使用malloc
#ifdef Read_File	
	g_file=(FILE*)malloc(sizeof(FILE));
	
	res = GIF_MEM_ERR;//申請記憶體失敗 
	if(g_file==NULL) goto memERROR3;
#endif
	mygif89a=(gif89a*) malloc(sizeof(gif89a));
	if(mygif89a==NULL) goto memERROR2;
	mygif89a->lzw=(LZW_INFO*) malloc(sizeof(LZW_INFO));
	if(mygif89a->lzw==NULL) goto memERROR1;
#else
	g_file=&f_gfile;
	mygif89a=&tgif89a;
	mygif89a->lzw=&tlzw;
#endif

		mygif89a->device = 0;
#ifdef Read_File	
		g_file = fopen((char*)p_gifAddress, "rb");
		if(g_file){
			mygif89a->device = g_file;
		}
#else
			mygif89a->device = p_gifAddress;
#endif
		if(mygif89a->device)//if it is not 0
		{
			//================================
			res = gif89a_prepare(mygif89a);
			//<<<<----------------------------
			if (res == 0){
				X0 = mygif89a->gifLSD.width;
				Y0 = mygif89a->gifLSD.height;
				if(X0 >S_Width)
				{
					X0 = S_Width;
					mygif89a->gifLSD.width = S_Width;
				}
				if(Y0 > S_Height)
				{
					Y0 = S_Height;
					mygif89a->gifLSD.height = S_Height;
				}
				// note: now X0 <= S_Width and Y0 <= S_Height
// make picture in the center of the given retangular region: (Xpos, Ypos) to ((Xpos+S_Width-1), (Xpos+S_Height-1)) 
					X0=(S_Width - X0)/2 + Xpos;			
					Y0=(S_Height - Y0)/2 + Ypos;		
					mygif89a->gifIMD.width = 0;			// first pieces of image
					mygif89a->gifIMD.height = 0;		// first pieces of image
					mygif89a->delay = 0;
			}
//################################################################################	
			while(res==0)//解碼迴圈
			{	 
				res=gif_decomp(mygif89a, X0, Y0);//顯示一張圖片

				if(mygif89a->delay)dtime = mygif89a->delay;
				else dtime=10;//默認延時
				
				while(dtime-- ) delay_ms(10);
				if(res==2)
				{
					res=0;
					break;
				}
			}
#ifdef Read_File	
			fclose(g_file);
#endif
			}

#if GIF_USE_MALLOC==1 	//定義是否使用malloc,這裡我們選擇使用malloc
memERROR1:
	free(mygif89a->lzw);
memERROR2:
	free(mygif89a); 
#ifdef Read_File	
memERROR3:
	free(g_file);
#endif
#endif 
	return res;
}
	

//------------------------------------------------------------
#define imageSize 100
uint16_t pixelIndex[imageSize]; 
uint16_t codeStream[64];
//==============================================================
void  Example_getIndexcodes(void)
{
#define nRawdata 9
uint8_t rawdata[nRawdata] = {0x68, 0xBA, 0xDC, 0xFE, 0x30, 0xCA, 0x49, 0x6B, 0x4C};
//uint8_t rawdata[nRawdata] = {0x8C, 0x8F, 0xA9, 0xCB, 0xED, 0x0F, 0x63, 0x2B};
uint8_t codeSizeBit = 3;
	uint16_t i;
	uint8_t *pTmp, *p_aBuffer;
	int nBytes;


	gif89a *s_gif;
	
	s_gif=(gif89a*)malloc(sizeof(gif89a));
	if(s_gif==NULL){
		while(1);
	};//申請記憶體失敗    
	s_gif->lzw=(LZW_INFO*)malloc(sizeof(LZW_INFO));
	if(s_gif->lzw==NULL){
		while(1);
	};//申請記憶體失敗 

	gif_initlzw(s_gif, codeSizeBit);//Initialize the LZW stack with the LZW code size 
	p_aBuffer = s_gif->lzw->aBuffer;
	for (i=0; i<nRawdata; i++)
	{
		p_aBuffer[i] = rawdata[i];
	}
		p_aBuffer[nRawdata] = 0x00;	// terminator code = 0x00

	pTmp = s_gif->lzw->aCodeDumpBuffer;

	s_gif->lzw->LastByte = nRawdata+1;
	nBytes = 0;

		i=0;
	while(1){
			if(nBytes == 0) 
			{
					//########### Next Bytes of Color Index #####################
					nBytes = gif_nextColorIndex(s_gif, pTmp);
			//>>>>> ======== End or ERROR ==================
				if(nBytes == -2){
					goto end_example;	//End code     
				}
//				if (nBytes < 0 || nBytes >= sizeof(s_gif->lzw->aCodeDumpBuffer) ) 
				if (nBytes < 0 || nBytes >= 64 ) 
				{
				//If out of legal range stop decompressing
					goto end_example; //Error
				}
				if (i==0){
					pixelIndex[i++] = s_gif->lzw->ClearCodeNr;
				}
					pixelIndex[i++] = pTmp[nBytes];
			}
			//<<<---------------------------------------------------
		  else {		// i.e., nBytes > 0
					pixelIndex[i++] = pTmp[--nBytes];
			}
		}
end_example:
		nBytes = s_gif->lzw->EndCodeNr;
		codeStream[0] = nBytes-1;		// Clear Code
		for (i = (nBytes+1); i<s_gif->lzw->MaxCodeNr; i++)
		{
				codeStream[i-nBytes] = s_gif->lzw->aCode[i];
		}
		codeStream[i-nBytes] = s_gif->lzw->OldCodeNr;
		codeStream[i-nBytes+1] = nBytes;					// End Code
		while(1);
}
