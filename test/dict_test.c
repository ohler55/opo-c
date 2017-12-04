// Copyright 2017 by Peter Ohler, All Rights Reserved


#include "opo/dict.h"
#include "ut.h"

static void
dict_test() {
    struct _opoErr	err = OPO_ERR_INIT;
    const char		*words[] = {
	"one",
	"two",
	"three",
	NULL,
    };
    opoDict		dict = opo_dict_create(&err, words);

    ut_same_int(OPO_ERR_OK, err.code, "error creating dictionary. %s", err.msg);

    int		index;
    const char	*word;
    
    for (const char **wp = words; NULL != *wp; wp++) {
	if (0 > (index = opo_dict_index(dict, *wp, -1))) {
	    ut_fail("index for %s was -1", *wp);
	}
	word = opo_dict_word(dict, index);
	ut_not_null(word, "NULL word return for index %d", index);
	ut_same(*wp, word, "word for %d mismatch", index);
    }

    ut_same_int(-1, opo_dict_index(dict, "nothing", -1), "word not in dict should return -1");

    opo_dict_destroy(dict);
}

static void
dict_null_test() {
    ut_same_int(-1, opo_dict_index(NULL, "nothing", -1), "null dict should return -1");
    ut_null(opo_dict_word(NULL, 1), "NULL dict return for word should return null");
}

void
append_dict_tests(utTest tests) {
    ut_appenda(tests, "opo.dict.normal", dict_test, NULL);
    ut_appenda(tests, "opo.dict.null", dict_null_test, NULL);
}
