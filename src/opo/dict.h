// Copyright 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPOC_DICT_H__
#define __OPOC_DICT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "err.h"
    
    typedef struct _opoDictEntry {
	uint32_t	rank;
	uint8_t		len;
	uint8_t		index;
	char		*str;
    } *opoDictEntry;
    
    typedef struct _opoDict {
	struct _opoDictEntry	entries[256];
	int			size;
    } *opoDict;

    extern opoDict	opo_dict_create(opoErr err, const char **words);
    extern void		opo_dict_destroy(opoDict dict);
    extern int		opo_dict_index(opoDict dict, const char *word, int len);
    extern const char*	opo_dict_word(opoDict dict, uint8_t index);
    
#ifdef __cplusplus
}
#endif
#endif /* __OPOC_DICT_H__ */
