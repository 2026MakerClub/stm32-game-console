
#include "tjpgd.h"

typedef size_t  (* CALLBACK_for_JPG_IN) (JDEC* jd, uint8_t* buff, size_t nbyte);

void register_JPG_callback (CALLBACK_for_JPG_IN p_func);

size_t  in_func_ROM(JDEC* jd, uint8_t* buff, size_t nbyte);
size_t  in_func_FILE(JDEC* jd, uint8_t* buff, size_t nbyte);
