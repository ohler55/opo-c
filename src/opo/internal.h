// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_INTERNAL_H__
#define __OPOC_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

    typedef enum {
	VAL_NULL	= (uint8_t)'Z',
	VAL_TRUE	= (uint8_t)'t',
	VAL_FALSE	= (uint8_t)'f',
	VAL_INT1	= (uint8_t)'i',
	VAL_INT2	= (uint8_t)'2',
	VAL_INT4	= (uint8_t)'4',
	VAL_INT8	= (uint8_t)'8',
	VAL_STR1	= (uint8_t)'s',
	VAL_STR2	= (uint8_t)'S',
	VAL_STR4	= (uint8_t)'B',
	VAL_KEY1	= (uint8_t)'k',
	VAL_KEY2	= (uint8_t)'K',
	VAL_DEC		= (uint8_t)'d',
	VAL_UUID	= (uint8_t)'u',
	VAL_TIME	= (uint8_t)'T',
	VAL_OBJ1	= (uint8_t)'o',
	VAL_OBJ2	= (uint8_t)'O',
	VAL_OBJ4	= (uint8_t)'{',
	VAL_ARRAY1	= (uint8_t)'a',
	VAL_ARRAY2	= (uint8_t)'A',
	VAL_ARRAY4	= (uint8_t)'[',
	VAL_DICS	= (uint8_t)'q',
	VAL_DICK	= (uint8_t)'Q',
    } ValType;
    
#ifdef __cplusplus
}
#endif
#endif /* __OPOC_INTERNAL_H__ */
