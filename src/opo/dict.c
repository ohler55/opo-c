// Copyright 2017 by Peter Ohler, All Rights Reserved

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "dict.h"

static const char	*reserved[] = {
    "\001$",
    "\002eq",
    "\002gt",
    "\002in",
    "\002lt",
    "\002or",
    "\003and",
    "\003gte",
    "\003has",
    "\003lte",
    "\003not",
    "\003ref",
    "\003rid",
    "\004$ref",
    "\004$rid",
    "\004code",
    "\004page",
    "\004sort",
    "\005count",
    "\005limit",
    "\005regex",
    "\005where",
    "\006delete",
    "\006filter",
    "\006insert",
    "\006unique",
    "\006update",
    "\007between",
    "\007results",
    "\007select",
    NULL,
};

static int
entry_cmp(const void *e1, const void *e2) {
    return ((opoDictEntry)e1)->rank - ((opoDictEntry)e2)->rank;
}

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

	dict->size = 0;
	for (; de < end && NULL != *words; de++, words++) {
	    if (255 < (len = strlen(*words))) {
		opo_err_set(err, OPO_ERR_OVERFLOW, "dictionary entries must be no longer than 255 characters.");
		opo_dict_destroy(dict);
		return NULL;
	    }
	    de->len = (uint8_t)len;
	    de->str = strdup(*words);
	    de->index = de - dict->entries;
	    de->rank = (uint32_t)de->len << 24 | (uint32_t)*de->str << 16;
	    if (0 < de->len) {
		de->rank |= (uint32_t)de->str[1] << 8 | (uint32_t)de->str[de->len - 1];
	    }
	    dict->size++;
	}
	words = reserved;
	for (; de < end && NULL != *words; de++, words++) {
	    de->len = (uint8_t)**words;
	    de->str = strdup(*words + 1);
	    de->index = de - dict->entries;
	    de->rank = (uint32_t)de->len << 24 | (uint32_t)*de->str << 16;
	    if (0 < de->len) {
		de->rank |= (uint32_t)de->str[1] << 8 | (uint32_t)de->str[de->len - 1];
	    }
	    dict->size++;
	}
	qsort(dict->entries, dict->size, sizeof(struct _opoDictEntry), entry_cmp);
    }
    return dict;
}

void
opo_dict_destroy(opoDict dict) {
    if (NULL != dict) {
	opoDictEntry	de = dict->entries;
	opoDictEntry	end = de + dict->size;

	for (; de < end; de++) {
	    if (NULL == de->str) {
		break;
	    }
	    free(de->str);
	}
	free(dict);
    }
}

int
opo_dict_index(opoDict dict, const char *word, int len) {
    
    // TBD use a rank based binary search, go up or down in same rank for word

    int	index = -1;
    
    if (NULL != dict) {
	opoDictEntry	de = dict->entries;
	opoDictEntry	end = de + dict->size;

	if (0 > len) {
	    len = strlen(word);
	}
	for (; de < end; de++) {
	    if (NULL == de->str) {
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
	word = dict->entries[index].str;
    }
    return word;
}


