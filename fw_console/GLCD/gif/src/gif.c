#include "gif.h"

//////////////////////////////////////////////////////////////////////////////////	 
 
//圖片解碼 驅動代碼-gif解碼部分
//STM32F4工程-庫函數版本
//淘寶店鋪：http://mcudev.taobao.com	
//********************************************************************************
//升級說明 
//無
//////////////////////////////////////////////////////////////////////////////////
					    

const uint8_t _aInterlaceOffset[]={8,8,4,2};
const uint8_t _aInterlaceYPos  []={0,4,2,1};
 


//定義是否使用malloc,這裡我們選擇使用malloc
#if GIF_USE_MALLOC==0 	
gif89a tgif89a;			//gif89a文件
FIL f_gfile;			//gif 文件
LZW_INFO tlzw;			//lzw
#endif




//將RGB888轉為RGB565
//ctb:RGB888顏色陣列首位址.
//返回值:RGB565顏色.
uint16_t gif_getrgb565(uint8_t *ctb) 
{
	uint16_t r,g,b;
	r=(ctb[0]>>3)&0X1F;
	g=(ctb[1]>>2)&0X3F;
	b=(ctb[2]>>3)&0X1F;
	return b+(g<<5)+(r<<11);
}
//讀取顏色表
//file:文件;
//gif:gif信息;
//num:tbl大小.
//返回值:0,OK;其他,失敗;
uint8_t gif_readcolortbl(gif89a* s_gif, uint16_t num, uint16_t* pCurTbl)
{
	uint8_t rgb[3];
	uint16_t t;
	uint32_t rb;
	for(t=0; t<num; t++)
	{
		rb = gif_in_func(s_gif, rgb, 3);
		if(rb!=3) return 1;		// reading ERROR!
		pCurTbl[t]=gif_getrgb565(rgb);
	}
	return 0;
} 

//初始化LZW相關參數	   
//gif:gif信息;
//codesize:lzw碼長度
void gif_initlzw(gif89a* s_gif, uint8_t min_codesize) 
{
	// For the zero initial values
	s_gif->lzw->CurBit = 0;
	s_gif->lzw->GetDone = 0;
	s_gif->lzw->LastByte = 0;
	// For the nonzero initial values
	s_gif->lzw->MinCodeSize  = min_codesize;						
	s_gif->lzw->CodeSizeBit     = min_codesize + 1;					// "bits per pixel"
	s_gif->lzw->ClearCodeNr    = (1 << min_codesize);				// 2^(codesize) + 0
	s_gif->lzw->EndCodeNr      = (1 << min_codesize) + 1;		// ClearCodeNr + 1
//	s_gif->lzw->MaxCodeNr      = (1 << min_codesize) + 2;		// i.e., the number of entries in code table: 0 ~ EndCodeNr
//	s_gif->lzw->MaxCodeSize  = (1 << min_codesize) << 1;	// 2^(codesize+1)
}

//讀取一個資料塊
//gfile:gif文件;
//buf:數據緩存區
//maxnum:最大讀寫資料限制
uint8_t gif_getdatablock(gif89a* s_gif, uint8_t *buf, uint16_t maxSize) 
{
	uint8_t cnt;
	gif_in_func(s_gif, &cnt, 1); // get block size in one byte

	if(cnt) 
	{
		uint8_t *pbuf;
		if (buf){
			if (cnt > maxSize) // Not enough buffer for data
				pbuf = 0;
			else
				pbuf = buf;
		} else{				//直接跳過 不讀取 
			pbuf = 0;					// pbuf = 0: 不讀取
		}
		cnt = gif_in_func(s_gif, pbuf, cnt);	// if buf = 0: 不讀取
	}
	return cnt;
}
//ReadExtension		 
//Purpose:
//Reads an extension block. One extension block can consist of several data blocks.
//If an unknown extension block occures, the routine failes.
//返回值:0,成功;
// 		 其他,失敗
uint8_t gif_readextension(gif89a* s_gif, int *pTransIndex,uint8_t *pDisposal)
{
	uint8_t bLabel;
	uint8_t buf[5];  
	gif_in_func(s_gif, &bLabel, 1); // get Block Label
	switch(bLabel)
	{
		case GIF_PLAINTEXT:
		case GIF_APPLICATION:
		case GIF_COMMENT:
			while(gif_getdatablock(s_gif, 0, 256)>0);			//獲取資料塊
			return 0;
		case GIF_GRAPHICCTL://圖形控制擴展塊
			if (gif_in_func(s_gif, buf, 1) != 1)	// get Block Size
			if (buf[0] != 4) return 1;    // the block size of Graphic Control Extension must be 4
			if(gif_in_func(s_gif, buf, (4+1)) != 5)return 1;	//圖形控制擴展塊的長度必須為4 
			*pDisposal=(buf[0]>>2)&0x7; 	    			// Disposal at bit 2:4 of byte 0
			//============= Delay Time =========================
 		 	s_gif->delay = (buf[2]<<8)|buf[1];					// delay Time at byte 1 and 2
			if((buf[0]&0x1)!=0) *pTransIndex = buf[3];			// Transparent Color Index at byte 3
		  //====== Block Terminator is "0x00" ======
 			if(buf[4] != 0x00) return 1;							//讀取資料塊結束符錯誤.
			return 0;
	}
	return 1;//錯誤的資料
}

//從LZW緩存中得到下一個LZW碼,每個碼包含12位
//返回值:<0,錯誤.
//		 其他,正常.
int gif_nextLZWcode(gif89a* s_gif, uint8_t codeSizeBit) 
{
	uint32_t i, CodeEndBit, CodeStartBit, lastBit, lastByte;
	uint8_t	shiftBit, codeW;
	uint32_t Result;
	uint16_t*	pDataBuf;

	CodeStartBit = s_gif->lzw->CurBit;
	lastByte = s_gif->lzw->LastByte;	//the last byte in the read buffer aBuffer[]
	lastBit = lastByte * 8;
	
	CodeEndBit = CodeStartBit + codeSizeBit;

	if(CodeEndBit > lastBit)
	{
		uint8_t Count;
		uint8_t* p_aBuffer;
		
		if(s_gif->lzw->GetDone) return -1;  // Error 
		p_aBuffer = s_gif->lzw->aBuffer;
		if (lastByte > 0) {
			p_aBuffer[1] = p_aBuffer[lastByte-1];
			if (lastByte >=2)
				p_aBuffer[0] = p_aBuffer[lastByte-2];
			CodeStartBit = 16 - ((int) lastBit - CodeStartBit);		// plus 16 bits (i.e., 2 bytes)
		} else {
			CodeStartBit = 16;
		}
		//==============================================================
		Count = gif_getdatablock(s_gif, &p_aBuffer[2], 256); // maxSize = 256 Bytes
		if(Count==0) return -1; // No data Reading
		//<<<<<----------------------------------------------------------------------------------

		//s_gif->lzw->CurBit = CodeStartBit;
		CodeEndBit = CodeStartBit + codeSizeBit;

		lastByte = 2+Count;
		s_gif->lzw->LastByte = lastByte;
	}
	
	pDataBuf = (uint16_t*) s_gif->lzw->aBuffer;
	i = CodeStartBit>>4;	// i = CodeStartBit/16 to be in Word = double Bytes
	shiftBit =	(CodeStartBit&0x0F);			//  CodeStartBit % 16
	codeW =  ((shiftBit+codeSizeBit) >> 4); // /16  = 0, 1  extra words
	Result = pDataBuf[i];
	if (codeW == 1){
				Result |= (pDataBuf[i+1] << 16);
	}

	Result = (Result >> shiftBit);  
	Result = Result & (0x0FFF >>(12-codeSizeBit)); // MAX_NUM_LWZ_BITS = 12
	//-------------------------------
	// update CurBit again!
	s_gif->lzw->CurBit = CodeStartBit + codeSizeBit;	// next current bit
	
	return (int)Result;
}	

//得到LZW的下一個碼
//返回值:<0,錯誤(-1,不成功;-2,讀到結束符了)
//		 >=0,OK.(LZW的第一個碼)
// FirstCode = First color index (i.e., <ClearCodeNr) of Current CodeNr if it has two or more color indexes
// OldCode = CodeNr of last read code
int gif_nextColorIndex(gif89a* s_gif, uint8_t *pTmp) 
{
	uint16_t i, newCode, oldCode, clearCodeNr, maxCodeNr_plus1, K_index;
	int Code;
	uint8_t codeSizeBit;
	
	clearCodeNr = s_gif->lzw->ClearCodeNr;		// it is  usually 256 = the number of colors 
	codeSizeBit = s_gif->lzw->CodeSizeBit;		// it is usually 9 bits, i.e., 8 bits plus 1 to represent ClearCode and EndCode
	
		Code = gif_nextLZWcode(s_gif, codeSizeBit);
	
		if (Code == -1) return Code;  // No data Reading!
		
		//==== Clear Code ==========
		if(Code == clearCodeNr)
		/*
		Whenever you come across the clear code in the image data, 
		it's your cue to REINITIALIZE the code table.
		*/
		{
			//Corrupt GIFs can make this happen  
			if(clearCodeNr >= (1<<MAX_NUM_LWZ_BITS))return -1;//Error 
			//Calculate the'special codes' changed during decompressing
			// (i.e, CodeSizeBit, MaxCodeSize, MaxCodeNr)
			//and initialize the stack pointer 
			codeSizeBit = s_gif->lzw->MinCodeSize+1;		
			s_gif->lzw->CodeSizeBit = codeSizeBit;		
			s_gif->lzw->MaxCodeSize = clearCodeNr<<1;		//  2^(codeSizeBit+1) = clearCode * 2
			s_gif->lzw->MaxCodeNr = clearCodeNr+2;
			// Read the first code from the stack after "Clear Code" 
			// The first code no. must be < ClearCodeNr.
			//-------------------------------------------------------
			// LZW codes: N_CL, N1, N2, ...
			// where N_CL = ClearCodeNr, N1<ClearCodeNr, 
			//-------------------------------------------------------
				Code = gif_nextLZWcode(s_gif, codeSizeBit);
			
				s_gif->lzw->OldCodeNr = Code;
				pTmp[0] = Code;
			
			return 0;  //i.e., s_gif->lzw->FirstCode
		}
		
		//==== EOI: end of information code  ==========
		if (Code==s_gif->lzw->EndCodeNr){
			return -2;	//End code: block Terminator of Image Data
		}
		/*
		The end of information code (EOI). When you come across EOI,
		this means you've reached the end of the image, 
		which is in the byte just before the block Terminator of Image Data. 
		*/
		
		//========== Decompression Algorithm =======================
		/*
		0. Initialization: (execute if Code == Clear Code)
												Prefix[i] = i, i= 0, ..., (Clear Code -1): first index in Code
												OldCode = first input code = the second code in the LZW compressed data 
												output: OldCode
		1. newCode  = next input code
		2.  IF newCode = (maximum code no. +1), i.e, if NewCode is not in the Code table,
		      maximum code no. = newCode
					K_index = first index of table[OldCode]
					table[newCode] = {table[OldCode], K_index}
					tailIndex[newCode] = K_index
					OUTPUT: table[newCode]
				ELSE
					i = maximum code no. = the end no. in the code table
					i = i+1
					maximum code no. = i
					IF newCode < Clear Code,
							K_index = newCode
							table[i] = {table[OldCode], K_index}
							tailIndex[i] = K_index
							OUTPUT: newCode  // (= table[newCode])
					ELSE (i.e., newCode > (Clear Code + 1) ) // i.e., newCode > End Code
							K_index = first index of table[newCode]
							table[i] = {table[OldCode], K_index}
							tailIndex[i] = K_index
							OUTPUT: table[newCode] = {K_index, ..., tailIndex[newCode]},
					ENDIF (newCode < Clear Code)
				ENDIF (newCode = (maximum code no. +1))
				OldCode = newCode
		*/
		newCode = (uint16_t) Code;
		maxCodeNr_plus1 = s_gif->lzw->MaxCodeNr;
		oldCode = s_gif->lzw->OldCodeNr;
		s_gif->lzw->OldCodeNr = newCode;						// last read CodeNr = just read CodeNr
		
		i= 0;
		if (newCode < clearCodeNr){
			// Output 	{Code} and
			// establish  MaxCode = {OldCodeNr, K_index}, where K_index = Code
				K_index = Code; 
				pTmp[0] = Code; 
				goto nextstep;
		}
		
		if (newCode == maxCodeNr_plus1)		// Code == 1 + the no. of last entry in the code table
		{
			// output {OldCodeNr, first index of the Old code} and
		  // establish MaxCode = {OldCodeNr, first color index of the Old code}			
//			pTmp[0] = first index in OldCodeNr
			i ++;
			Code = oldCode;		
		} 
		
		while(1)
		{
			if (Code < clearCodeNr) break;
			pTmp[i++] = s_gif->lzw->aTail_index[Code];					// tail index of the code
			if(i >= sizeof(s_gif->lzw->aCodeDumpBuffer)){
				return i; // if size of aCodeDumpBuffer is not enough, then error!
			}
			Code = s_gif->lzw->aCode[Code];
		}
		pTmp[i] = K_index = Code;
		if (newCode == maxCodeNr_plus1){
			pTmp[0] = Code;									//pTmp[0] = first index in OldCodeNr
		}
		
nextstep:
		// increase s_gif->lzw->MaxCodeNr by 1
		if(maxCodeNr_plus1 < (1<<MAX_NUM_LWZ_BITS))		
		{
			uint16_t maxCodeSize;
			
			// the new maxCode at no. [maxCodeNr_plus1] = (OldCodeNr, K_index)
			//  saved as (aCode[no.], aTail_index[no.])
			s_gif->lzw->aCode[maxCodeNr_plus1] = oldCode;
			s_gif->lzw->aTail_index[maxCodeNr_plus1] = K_index;

			maxCodeNr_plus1++;
			s_gif->lzw->MaxCodeNr = maxCodeNr_plus1;
			maxCodeSize = s_gif->lzw->MaxCodeSize;
			if((maxCodeNr_plus1 >= maxCodeSize) && (maxCodeSize < (1<<MAX_NUM_LWZ_BITS)))
			{
				// --- change CodeSizeBit and MaxCodeSize ------------
				s_gif->lzw->MaxCodeSize = (maxCodeSize << 1);
				codeSizeBit++;
				s_gif->lzw->CodeSizeBit = codeSizeBit;			// change CodeSizeBit
			}
		}

		return i; 
}


//===================================================================================
//DispGIFImage		 
//Purpose:
//   This routine draws a GIF image from the current pointer which should point to a
//   valid GIF data block. The size of the desired image is given in the image descriptor.
//Return value:
//  0 if succeed
//  1 if not succeed
//Parameters:
//  pDescriptor  - Points to a IMAGE_DESCRIPTOR structure, which contains infos about size, colors and interlacing.
//  x0, y0       - Obvious.
//  Transparency - Color index which should be treated as transparent.
//  Disposal     - Contains the disposal method of the previous image. If Disposal == 2, the transparent pixels
//                 of the image are rendered with the background color.
uint8_t gif_dispimage(gif89a* s_gif, uint16_t X0, uint16_t Y0, int TransIndex, uint8_t Disposal) 
{
  uint8_t lzwMCsize;		 // LZW minimum code size
	int Index, OldIndex, XPos, YPos, YCnt, Pass, Interlace;
	int Cnt, nBytes;
	uint16_t x0, y0, or_x1, im_Height, x_end;
	uint16_t S_y1, S_x1;
	uint16_t color_16bits, BKcolor;// BKcolor: background color
	uint16_t *pColorTbl;
	uint8_t *pTmp;

	S_y1=s_gif->gifLSD.height + Y0 - 1;
	x0 = X0+s_gif->gifIMD.xoff;
	y0 = Y0+s_gif->gifIMD.yoff;
	im_Height=s_gif->gifIMD.height;
//	or_y1 = y0 + im_Height -1; 
	or_x1 = s_gif->gifIMD.width+x0-1;
	S_x1 = s_gif->gifLSD.width+X0-1;		
//	if (S_y1 < or_y1) im_Height = S_y1 - y0 +1;	 // i.e., let y1 = S_y1
	if (S_x1 < or_x1){	 // change x1 and save in x_end
		x_end = S_x1;
	}	else {
		x_end = or_x1;
	}
	
	BKcolor=s_gif->colortbl[s_gif->gifLSD.bkcindex];
	pColorTbl = s_gif->pCurColorTbl;
	//######################################
	gif_in_func(s_gif, &lzwMCsize, 1); // get LZW minimum code size
	
	gif_initlzw(s_gif, lzwMCsize);//Initialize the LZW stack with the LZW code size 
	Interlace=s_gif->gifIMD.flag&0x40;//是否交織編碼
	pTmp = s_gif->lzw->aCodeDumpBuffer;
	
		//==== Check the First Code of LZW data ==========
	nBytes = gif_nextColorIndex(s_gif, pTmp); // get Clear Code and the first color index!
	if (nBytes != 0) return 1; //Error
	nBytes++;
	//<<<<<---------------------------------------------------

	for(YCnt=0,YPos=y0,Pass=0; YCnt<im_Height; YCnt++)
	{
		Cnt=0;
		for(XPos=x0; XPos<=or_x1; )
		{
			if(nBytes == 0) 
			{
				//########### Next Bytes of Color Index #####################
				nBytes = gif_nextColorIndex(s_gif, pTmp);
		//>>>>> ======== End or ERROR ==================
				if(nBytes == -2) return 0;	//End code     
				if (nBytes < 0 || nBytes >= sizeof(s_gif->lzw->aCodeDumpBuffer)) 
				{
			//If out of legal range stop decompressing
					return 1; //Error
				}
					Index = pTmp[nBytes];
			}
			//<<<---------------------------------------------------
			else {		// i.e., nBytes > 0
				Index = pTmp[--nBytes];
			}
		
			if (YPos <= S_y1){
				if(XPos == x0){
					OldIndex = Index;
				} else {
				// ignore if XPos > S_x1
					if (XPos <= S_x1){
			//If current index equals old index, increment counter
						if(Index==OldIndex) Cnt++;
						else
						{
							// ignore it if((OldIndex == TransIndex) && Disposal != 2) 
							if((OldIndex!=TransIndex) || (Disposal==2))
							{
								if (OldIndex != TransIndex)
									color_16bits = pColorTbl[OldIndex];
								else	// if (OldIndex == TransIndex)
									color_16bits = BKcolor;		// use background color as transparent

								gif_draw_HLine(XPos-Cnt-1,YPos, Cnt+1, color_16bits);
							}
							Cnt = 0;			//  It may be the case of Cnt> 0
							OldIndex = Index;
						}
					}		//end of if (XPos <= S_x1)
				}	//end of if(XPos == x0)
				XPos++;
			} else {	// ignore index data if (YPos > S_y1)
					XPos += nBytes+1;
					nBytes = 0;
			} // end of if (YPos <= S_y1)
		}
		if (YPos <= S_y1){
			if((OldIndex!=TransIndex)||(Disposal==2))
			{
				if(OldIndex!=TransIndex){
					color_16bits = pColorTbl[OldIndex]; 
				}	else {
					color_16bits = BKcolor;
				}
				gif_draw_HLine(x_end-Cnt, YPos, Cnt+1, color_16bits);
			}
		}
		
		//Adjust YPos if image is interlaced 
		if(Interlace)//交織編碼
		{
			YPos+=_aInterlaceOffset[Pass];
			if((YPos-y0)>=im_Height)
			{
				++Pass;
				YPos=_aInterlaceYPos[Pass]+y0;
			}
		}else YPos++;	    
	}

		while(1){
			nBytes = gif_nextColorIndex(s_gif, pTmp);
			if (nBytes ==-2) return 0;		// When get End code, nBytes = -2 is set.
			if (nBytes == -1) return 1;
		}
}  			   
//恢復成背景色
//x,y:座標
//s_gif:gif信息.
//pimge:圖像描述塊資訊
/*================================================
		  -------------------------------------------------
     | Previous Image         (NEW_y0>pv_y0) area      |
     |       [NEW_y0] ------------------               | <--- [NEW_y0]
     |               |                  |              |
     |               |   NEW  Image     |(NEW_x1<pv_x1)|
     |(NEW_x0>pv_x0) |                  |     area     |
     |   area         ------------------               |
     |               [NEW_x0]                          |
     |                        (NEW_y1<pv_y1) area      |
		  -------------------------------------------------
                     /|\
                      | [NEW_x0]
*/ //================================================
void gif_clear2bkcolor(uint16_t X0, uint16_t Y0, gif89a* s_gif, ImageDescriptor previmg)
/*  
	prevmge.xoff;		  //x offset: image left
	prevmge.yoff;		  //y offset: image top
	prevmge.width;		//image width
	prevmge.height;		//image height
*/
{
	uint16_t NEW_x0,NEW_y0, NEW_x1, NEW_y1;
	uint16_t pv_x0, pv_y0, pv_x1, pv_y1, BKcolor;
	uint16_t s_x1, s_y1;

	s_x1 = X0 + s_gif->gifLSD.width -1;
	s_y1 = Y0 + s_gif->gifLSD.height -1;
	
	if(previmg.width==0||previmg.height==0)return;//直接不用清除了,原來沒有圖像!!

		pv_x0=X0+previmg.xoff;
		pv_y0=Y0+previmg.yoff;
		pv_x1=pv_x0+previmg.width-1;;
		pv_y1=pv_y0+previmg.height-1;;
		if(pv_x0>pv_x1 || pv_y0>pv_y1 || pv_x1>320 || pv_y1>320) return; //設定xy,的範圍不能太大.
	if ((pv_x0 > s_x1) || (pv_y0 > s_y1) ) return;
	if (pv_x1 > s_x1) pv_x1 = s_x1;
	if (pv_y1 > s_y1) pv_y1 = s_y1;

	BKcolor=s_gif->colortbl[s_gif->gifLSD.bkcindex];
	
	NEW_x0 = X0+ s_gif->gifIMD.xoff;
	NEW_y0 = Y0+ s_gif->gifIMD.yoff;
	NEW_x1 = NEW_x0 + s_gif->gifIMD.width -1;
	NEW_y1 = NEW_y0 + s_gif->gifIMD.height -1;

	if (NEW_x0 > s_x1) NEW_x0 = s_x1;
	if (NEW_y0 > s_y1) NEW_y0 = s_y1;
	if (NEW_x1 > s_x1) NEW_x1 = s_x1;
	if (NEW_y1 > s_y1) NEW_y1 = s_y1;

	if(NEW_y0 > pv_y0)		
	{
		gif_fill_Rect(pv_x0, pv_y0, pv_x1, (NEW_y0-1), BKcolor); 
		pv_y0 = NEW_y0;
	}
	
	if(NEW_x0 > pv_x0)		// new_x0 > pv_x0
	{
		gif_fill_Rect(pv_x0, pv_y0, (NEW_x0-1), pv_y1, BKcolor);
		pv_x0 = NEW_x0;
	}
	
	if(NEW_y1 < pv_y1)	
	{
		gif_fill_Rect(pv_x0, (NEW_y1+1), pv_x1, pv_y1, BKcolor);
		pv_y1 = NEW_y1;
	}
	
 	if(NEW_x1 < pv_x1)	
	{
		gif_fill_Rect((NEW_x1+1), pv_y0, pv_x1, pv_y1, BKcolor);
	}   
}

//畫GIF圖像的一幀
//s_gif:gif信息.
//X0, Y0:開始顯示的座標
uint8_t gif_decomp(gif89a* s_gif, uint16_t X0, uint16_t Y0)
{		  
  uint32_t  rb; //讀取到的位元組數
	uint8_t res, temp, flag;    
	uint16_t numcolors;
	ImageDescriptor previmg;

	uint8_t Disposal;
	int TransIndex;
	uint8_t Introducer;
	
	TransIndex=-1;				  
	do
	{
		rb = gif_in_func(s_gif, &Introducer, 1); // read 1 byte and save it in "Introducer".
		if(rb != 1) return 1;   
		switch(Introducer)
		{		 
			case GIF_INTRO_EXTENSION:				//"!" = 0x21 Graphic Control Extension
				//Read image extension*/
				res=gif_readextension(s_gif, &TransIndex, &Disposal);//讀取圖像擴展塊消息
				if(res)return 1;
	 			break;
			case GIF_INTRO_IMAGE://圖像描述 "," = 0x2C Image Descriptor
				if(Disposal==2){
					previmg.xoff=s_gif->gifIMD.xoff;
					previmg.yoff=s_gif->gifIMD.yoff;
					previmg.width=s_gif->gifIMD.width;
					previmg.height=s_gif->gifIMD.height;
				}

				rb = gif_in_func(s_gif, (uint8_t*)&s_gif->gifIMD, 9); // read 9 bytes and save them in "s_gif->gifIMD".
				if(rb != 9) return 1;			 
			//>>>== If there is a Local  Color Table,
				flag = s_gif->gifIMD.flag;
				if(flag & 0x80) // There is a Local Color Table
				{	
					uint16_t *pTbl;
					
					numcolors=2<<(s_gif->gifIMD.flag & 0X07); //size of the color table entries
					pTbl = s_gif->bkpcolortbl;				// current color table pointer					
					if(gif_readcolortbl(s_gif, numcolors, pTbl)) return 1; // Reading ERROR!
					s_gif->pCurColorTbl = pTbl;	// pointer	to bkpcolortbl				
				} 
			//<<<===
				if(Disposal==2){
					if(previmg.width!=0 && previmg.height!=0)	// i.e., NOT the case of the first gif image
								gif_clear2bkcolor(X0, Y0, s_gif, previmg); 
				}
				//############ image decode and display
				gif_dispimage(s_gif, X0, Y0, TransIndex, Disposal);
				if(flag & 0x80) // There was a Local Color Table
				{	
					s_gif->pCurColorTbl = s_gif->colortbl;	// pointer to Global Color Table
				}
 				//<<<----------------------------------------
				while(1)
				{
					rb = gif_in_func(s_gif, &temp, 1); // read 1 byte and save it in "temp".
					if(temp==0) break;				// block terminator = 0x00
				}
				if(temp!=0)return 1;//Error 
				return 0;
			case GIF_INTRO_TERMINATOR://得到結束符了 ";" = 0x3B  GIF Terminator
				return 2;			//代表圖像解碼完成了.
			default:
				return 1;
		}
	}while(Introducer!=GIF_INTRO_TERMINATOR);//讀到結束符了
	return 0;
}

//s_gif: gif信息.
uint8_t gif89a_prepare(gif89a* s_gif)
{
	uint8_t res=0, flag;
	uint8_t gifversion[6];
	uint16_t *pTbl;

				res = gif_in_func(s_gif, gifversion, 6); // get GIF head 
			
				if(res != 6)return GIF_FORMAT_ERR;	   
				if((gifversion[0]!='G')||(gifversion[1]!='I')||(gifversion[2]!='F')||
						(gifversion[3]!='8')||((gifversion[4]!='7')&&(gifversion[4]!='9'))||
						(gifversion[5]!='a'))return GIF_FORMAT_ERR;
			
				res = gif_in_func(s_gif, (uint8_t*) &s_gif->gifLSD, 7); // get GIF Logical Screen Descriptor (LSD)
				if(res !=7 )return GIF_FORMAT_ERR;
				flag = s_gif->gifLSD.flag;
				if(flag & 0x80)  // there exists a global color table
				{
					s_gif->numcolors=2<<(flag & 0x07);			//= 1<<((flag & 0x07) + 1): size of the color table entries
					pTbl = s_gif->colortbl;	// current color table pointer
					if(gif_readcolortbl(s_gif, s_gif->numcolors, pTbl)) return GIF_FORMAT_ERR; // reading ERROR!
					s_gif->pCurColorTbl = pTbl;	// current color table pointer
				}	   
				return 0;
}


