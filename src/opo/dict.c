// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "dict.h"

opoDict
opo_dict_create(opoErr err, const char **words) {
    opoDict	dict = (opoDict)malloc(sizeof(struct _opoDict));

    if (NULL == dict) {
	opo_err_set(err, OPO_ERR_MEMORY, "failed to allocate memory for a opoDict.");
    } else {
	memset(dict, 0, sizeof(struct _opoDict));
	opoDictEntry	de = dict->entries;
	opoDictEntry	end = de + 256;
	int		len;
	
	for (; de < end && NULL != *words; de++, words++) {
	    de->set = true;
	    if (255 < (len = strlen(*words))) {
		opo_err_set(err, OPO_ERR_OVERFLOW, "dictionary entries must be no longer than 255 characters.");
		opo_dict_destroy(dict);
		return NULL;
	    }
	    de->len = (uint8_t)len;
	    de->str = strdup(*words);
	    de->index = de - dict->entries;
	}
    }
    return dict;
}

void
opo_dict_destroy(opoDict dict) {
    if (NULL != dict) {
	opoDictEntry	de = dict->entries;
	opoDictEntry	end = de + 256;

	for (; de < end; de++) {
	    if (!de->set) {
		break;
	    }
	    free(de->str);
	}
	free(dict);
    }
}

int
opo_dict_index(opoDict dict, const char *word, int len) {
    
    // TBD use a rank based on len, first, and last characters, sort then use bin search, go up and down in same rank for word

    int	index = -1;
    
    if (NULL != dict) {
	opoDictEntry	de = dict->entries;
	opoDictEntry	end = de + 256;

	if (0 > len) {
	    len = strlen(word);
	}
	for (; de < end; de++) {
	    if (!de->set) {
		break;
	    }
	    if (len == de->len && 0 == strcmp(word, de->str)) {
		index = de - dict->entries;
		break;
	    }
	}
    }
    return index;
}

const char*
opo_dict_word(opoDict dict, uint8_t index) {
    const char	*word = NULL;
    
    if (NULL != dict) {
	opoDictEntry	de = &dict->entries[index];

	if (de->set) {
	    word = de->str;
	}
    }
    return word;
}


