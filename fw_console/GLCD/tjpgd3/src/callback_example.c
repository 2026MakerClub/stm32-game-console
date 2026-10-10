/* Includes ------------------------------------------------------------------*/
#include "tjpgd.h"
#include <stdio.h>				// for data type of FILE

//>>=====================
// Step 1. Type Declaration
typedef size_t  (* CALLBACK_for_JPG_IN) (JDEC* jd, uint8_t* buff, size_t nbyte);
// Step 2. Variable Declaration
	CALLBACK_for_JPG_IN  p_callback_InFunc = 0;
// Step 3. register function
void register_JPG_callback (CALLBACK_for_JPG_IN p_func);
void register_JPG_callback (CALLBACK_for_JPG_IN p_func) {
	p_callback_InFunc =  p_func;
}
// Step 4. register user's function  (a calling sample, e.g., in_func_ROM)
// size_t in_func_ROM(JDEC* jd, uint8_t* buff, size_t nbyte);
// register_JPG_callback(in_func_ROM);		// to be in main.c
//<<==================


//========================================
size_t in_func_ROM (    /* Returns number of bytes read or removed (zero on file read error) */
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

//========================================
size_t in_func_FILE (    /* Returns number of bytes read (zero on error) */
    JDEC* jd,       /* Decompression object */
    uint8_t* buff,  /* Pointer to the read buffer (null to remove data) */
    size_t nbyte    /* Number of bytes to read/remove */
)
{
    FILE *dev_fp = (FILE*)jd->device;   /* Session identifier (5th argument of jd_prepare function) */


    if (buff) { /* Raad data from imput stream */
        return fread(buff, 1, nbyte, dev_fp);
    } else {    /* Remove data from input stream */
        return fseek(dev_fp, nbyte, SEEK_CUR) ? 0 : nbyte;
    }
}

