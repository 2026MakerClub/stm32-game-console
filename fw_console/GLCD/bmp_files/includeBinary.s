


	AREA	|.text|, CODE 
;    AREA    |.ARM.__AT_0x08000C00|, CODE, READONLY
              ALIGN
	DCD			0				; align with 4-byte boundary
startAddress
	INCBIN	chrome_w30.bmp		; 3 KB
;	INCBIN	chrome_w39.bmp		; 4.62 KB
;	INCBIN	chrome_w44.bmp		; 5.98 KB
;	INCBIN	chrome_w47.bmp		; 6.52 KB
;	INCBIN	chrome_16bits.bmp	; 4.36 KB
		
endAddress
	EXPORT 	startAddress

; The following is redundant.
;	AREA	|.text|, CODE 
;              ALIGN
;BINfile_length
;	DCD		endAddress - startAddress
;	EXPORT 	BINfile_length


; END of an ASM file
	END	
;;=================== C-language calling sample code ===========
;extern uint8_t startAddress;
;const uint8_t* p_bmp_data = (uint8_t*) &startAddress;
;or When use  AREA    |.ARM.__AT_0x08000C00|, CODE, READONLY
;  const uint8_t* p_bmp_data = (uint8_t*) 0x08000C04;
