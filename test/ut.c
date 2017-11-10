// Copyright 2009, 2015, 2016, 2017 by Peter Ohler, All Rights Reserved

#include <stdarg.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>

#include "ut.h"

static void	usage(const char *appName);
static utTest	find_test(const char *name);
static int	mark_tests(const char *name);

FILE		*ut_out = NULL;
bool		ut_verbose = false;

static const char	*group = NULL;
static utTest		tests = NULL;
static utTest		current_test = NULL;
static jmp_buf		jump_env;

void
ut_print(const char *format, ...) {
    va_list	ap;

    va_start(ap, format);
    vfprintf(ut_out, format, ap);
    va_end(ap);
}

utTest
ut_append(utTest tests, const char *name, void (*func)(void *ctx), void *ctx) {
    for (; NULL != tests->name; tests++) {
    }
    tests->name = name;
    tests->func = func;
    tests->ctx = ctx;
    tests->pass = -1;
    tests->run = true;
    tests->abort = false;
    tests++;
    tests->name = NULL;
    tests->func = NULL;

    return tests - 1;
}

utTest
ut_appenda(utTest tests, const char *name, void (*func)(void *ctx), void *ctx) {
    for (; NULL != tests->name; tests++) {
    }
    tests->name = name;
    tests->func = func;
    tests->ctx = ctx;
    tests->pass = -1;
    tests->run = true;
    tests->abort = true;
    tests++;
    tests->name = NULL;
    tests->func = NULL;

    return tests - 1;
}

void
ut_init(int argc, char **argv, const char *group_name, utTest all_tests) {
    utTest	t;
    char	*app_name = *argv;
    char	*a;
    bool	run_all = true;

    ut_out = stdout;
    tests = all_tests;
    group = group_name;
    for (t = tests; t->name != 0; t++) {
	t->pass = -1;
	t->run = false;
    }
    argc--;
    argv++;
    for (; 0 < argc; argc--, argv++) {
	a = *argv;
	if (0 == strcmp("-o", a)) {
	    argc--;
	    argv++;
	    if (0 == (ut_out = fopen(*argv, "a"))) {
		printf("Failed to open %s\n", *argv);
		usage(app_name);
	    }
	} else if (0 == strcmp("-c", a)) {
	    argc--;
	    argv++;
	    if (0 == (ut_out = fopen(*argv, "w"))) {
		printf("Failed to open %s\n", *argv);
		usage(app_name);
	    }
	} else if (0 == strcmp("-v", a)) {
	    ut_verbose = true;
	} else if (0 == strcmp("-h", a)) {
	    usage(app_name);
	} else {
	    if (0 == mark_tests(a)) {
		printf("\n%s does not contain test %s\n", group, a);
		usage(app_name);
	    }
	    run_all = false;
	}
    }
    if (run_all) {
	for (t = tests; t->name != 0; t++) {
	    t->run = true;
	}
    }
    ut_print("%s tests started\n", group);

    for (current_test = tests; NULL != current_test->name; current_test++) {
	if (current_test->run && NULL != current_test->func) {
	    if (ut_verbose) {
		ut_print("  %s\n", current_test->name);
	    }
	    if (current_test->abort) {
		if (0 == setjmp(jump_env)) {
		    current_test->func(current_test->ctx);
		} else {
		    current_test->pass = 0;
		}
	    } else {
		current_test->func(current_test->ctx);
	    }
	}
    }
}

void
ut_done(void) {
    char	name_format[32];
    utTest	t;
    char	*result = "Skipped";
    time_t	tt;
    int		cnt = 0;
    int		fail = 0;
    int		skip = 0;
    int		len, max_len = 1;
    
    for (t = tests; t->name != 0; t++) {
	len = strlen(t->name);
	if (max_len < len) {
	    max_len = len;
	}
    }
    sprintf(name_format, "  %%%ds: %%s\n", max_len);
    ut_print("Summary for %s:\n", group);
    for (t = tests; t->name != 0; t++) {
	if (0 == t->run) {
	    continue;
	}
	switch (t->pass) {
	case -1:
	    result = "Skipped";
	    skip++;
	    break;
	case 0:
	    result = "FAILED";
	    fail++;
	    break;
	case 1:
	default:
	    result = "Passed";
	    break;
	}
	cnt++;
	ut_print(name_format, t->name, result);
    }
    ut_print("\n");
    if (0 < fail) {
	ut_print("%d out of %d tests failed for %s\n", fail, cnt, group);
	if (0 < skip) {
	    ut_print("%d out of %d skipped\n", skip, cnt);
	}
	ut_print("%d out of %d passed\n", (cnt - fail - skip), cnt);
    } else if (0 < skip) {
	ut_print("%d out of %d skipped\n", skip, cnt);
	ut_print("%d out of %d passed\n", (cnt - skip), cnt);
    } else {
	ut_print("All %d tests passed!\n", cnt);
    }
    tt = time(0);
    ut_print("%s tests completed %s\n", group, ctime(&tt));
    if (stdout != ut_out) {
	fclose(ut_out);
    }
    // exit((cnt << 16) | fail);
    //exit(0);
}

bool
ut_same(const char *expected, const char *actual, const char *format, ...) {
    const char	*e = expected;
    const char	*a = actual;
    int		line = 1;
    int		col = 1;
    bool	pass = true;
    va_list	ap;

    va_start(ap, format);

    if (expected == actual) {
	if (current_test->pass != false) {	// don't replace failed status
	    current_test->pass = true;
	}
	va_end(ap);
	return true;
    }
    if (0 == actual || 0 == expected) {
	current_test->pass = 0;
	if (NULL == format) {
	    ut_print("%s.%s Failed: NULL value\n", group, current_test->name);
	} else {
	    ut_print("%s.%s Failed: ", group, current_test->name);
	    vfprintf(ut_out, format, ap);
	    ut_print(" (NULL value)\n");
	}
	if (ut_verbose) {
	    ut_print("expected: '%s'\n", NULL == expected ? "<null>" : expected);
	    ut_print("  actual: '%s'\n\n", NULL == actual ? "<null>" : actual);
	}
	va_end(ap);

	if (current_test->abort) {
	    longjmp(jump_env, 1);
	}
	return false;
    }
    for (; '\0' != *e && '\0' != *a; e++, a++) {
	if (*e == *a || '?' == *e) {
	    if ('\n' == *a) {
		line++;
		col = 0;
	    }
	    col++;
	} else if ('$' == *e) {	// a digit
	    if ('0' <= *a && *a <= '9') {
		while ('0' <= *a && *a <= '9') {
		    a++;
		    col++;
		}
		a--;
	    } else {
		pass = 0;
		break;
	    }
	} else if ('#' == *e) {	// hexidecimal
	    if (('0' <= *a && *a <= '9') ||
		('a' <= *a && *a <= 'f') ||
		('A' <= *a && *a <= 'F')) {
		while (('0' <= *a && *a <= '9') ||
		       ('a' <= *a && *a <= 'f') ||
		       ('A' <= *a && *a <= 'F')) {
		    a++;
		    col++;
		}
		a--;
	    } else {
		pass = 0;
		break;
	    }
	} else if ('*' == *e) {
	    const char	ne = *(e + 1);

	    while (ne != *a && '\0' != *a) {
		a++;
		col++;
	    }
	    a--;
	} else {
	    pass = 0;
	    break;
	}
    }
    if ('\0' != *a) {
	if ('\0' == *e) {
	    if (NULL == format) {
		ut_print("%s.%s Failed: Actual result longer than expected\n", group, current_test->name);
	    } else {
		ut_print("%s.%s Failed: ", group, current_test->name);
		vfprintf(ut_out, format, ap);
		ut_print(" (Actual result longer than expected)\n");
	    }
	    pass = 0;
	    if (ut_verbose) {
		ut_print("expected: '%s'\n", expected);
		ut_print("  actual: '%s'\n\n", actual);
	    }
	} else {
	    if (NULL == format) {
		ut_print("%s.%s Failed: Mismatch at line %d, column %d\n", group, current_test->name, line, col);
	    } else {
		ut_print("%s.%s Failed: ", group, current_test->name);
		vfprintf(ut_out, format, ap);
		ut_print(" (Mismatch at line %d, column %d)\n", line, col);
	    }
	    if (ut_verbose) {
		ut_print("expected: '%s'\n", expected);
		ut_print("  actual: '%s'\n\n", actual);
	    }
	    pass = 0;
	}
    } else if ('\0' != *e) {
	if (NULL == format) {
	    ut_print("%s.%s Failed: Actual result shorter than expected\n", group, current_test->name);
	} else {
	    ut_print("%s.%s Failed: ", group, current_test->name);
	    vfprintf(ut_out, format, ap);
	    ut_print(" (Actual result shorter than expected)\n");
	}
	pass = 0;
	if (ut_verbose) {
	    ut_print("expected: '%s'\n", expected);
	    ut_print("  actual: '%s'\n\n", actual);
	}
    }
    if (current_test->pass != 0) {	// don't replace failed status
	current_test->pass = pass;
    }
    va_end(ap);

    if (!pass && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return pass;
}

bool
ut_same_int(int64_t expected, int64_t actual, const char *format, ...) {
    va_list	ap;

    va_start(ap, format);
    if (expected == actual) {
	if (current_test->pass != 0) {	// don't replace failed status
	    current_test->pass = 1;
	}
	va_end(ap);

	return true;
    }
    if (NULL == format) {
	ut_print("%s.%s Failed: %lld != %lld\n", group, current_test->name, (long long)expected, (long long)actual);
    } else {
	ut_print("%s.%s Failed: ", group, current_test->name);
	vfprintf(ut_out, format, ap);
	ut_print(" (%lld != %lld)\n", (long long)expected, (long long)actual);
    }
    current_test->pass = 0;
    va_end(ap);

    if (current_test->abort) {
	longjmp(jump_env, 1);
    }
    return false;
}

bool
ut_same_cmp(void *expected,
		  void *actual,
		  char* (*cmp)(void *ex, void *ac),
		  const char *format, ...) {
    char	*msg = cmp(expected, actual);

    if (NULL != msg) {
	va_list	ap;

	va_start(ap, format);
	if (NULL == format) {
	    ut_print("%s.%s Failed: %s\n", group, current_test->name, msg);
	} else {
	    ut_print("%s.%s Failed: ", group, current_test->name);
	    vfprintf(ut_out, format, ap);
	    ut_print(" (%s)\n", msg);
	}
	current_test->pass = 0;
	free(msg);
	va_end(ap);
    } else {
	if (current_test->pass == -1) {	// don't replace failed status
	    current_test->pass = 1;
	}
    }
    if (0 == current_test->pass && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return current_test->pass;
}

bool
ut_null(void *value, const char *fmt, ...) {
    if (NULL != value) {
	if (NULL != fmt) {
	    va_list	ap;

	    va_start(ap, fmt);
	    fprintf(ut_out, "%s.%s Failed:  ", group, current_test->name);
	    vfprintf(ut_out, fmt, ap);
	    fprintf(ut_out, " (expected NULL)\n");
	    va_end(ap);
	} else {
	    fprintf(ut_out, "%s.%s Failed: expected NULL\n", group, current_test->name);
	}
	current_test->pass = 0;
    } else {
	if (current_test->pass == -1) {	// don't replace failed status
	    current_test->pass = 1;
	}
    }
    if (0 == current_test->pass && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return current_test->pass;
}

bool
ut_not_null(void *value, const char *fmt, ...) {
    if (NULL == value) {
	if (NULL != fmt) {
	    va_list	ap;

	    va_start(ap, fmt);
	    fprintf(ut_out, "%s.%s Failed:  ", group, current_test->name);
	    vfprintf(ut_out, fmt, ap);
	    fprintf(ut_out, " (expected non-NULL)\n");
	    va_end(ap);
	} else {
	    fprintf(ut_out, "%s.%s Failed: expected non-NULL\n", group, current_test->name);
	}
	current_test->pass = 0;
    } else {
	if (current_test->pass == -1) {	// don't replace failed status
	    current_test->pass = 1;
	}
    }
    if (0 == current_test->pass && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return current_test->pass;
}

void
ut_fail(const char *fmt, ...) {
    if (NULL != fmt) {
	va_list	ap;

	va_start(ap, fmt);
	fprintf(ut_out, "%s.%s Failed - ", group, current_test->name);
	vfprintf(ut_out, fmt, ap);
	fprintf(ut_out, "\n");
	va_end(ap);
    } else {
	fprintf(ut_out, "%s.%s Failed\n", group, current_test->name);
    }
    current_test->pass = 0;
    if (current_test->abort) {
	longjmp(jump_env, 1);
    }
}

bool
ut_true(bool condition, const char *fmt, ...) {
    if (!condition) {
	if (NULL != fmt) {
	    va_list	ap;

	    va_start(ap, fmt);
	    fprintf(ut_out, "%s.%s Failed - ", group, current_test->name);
	    vfprintf(ut_out, fmt, ap);
	    fprintf(ut_out, "\n");
	    va_end(ap);
	}
	ut_print("%s.%s Failed: Condition was false\n", group, current_test->name);
	current_test->pass = 0;
    } else {
	if (current_test->pass == -1) {	// don't replace failed status
	    current_test->pass = 1;
	}
    }
    if (!condition && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return condition;
}

bool
ut_false(bool condition, const char *fmt, ...) {
    if (condition) {
	if (NULL != fmt) {
	    va_list	ap;

	    va_start(ap, fmt);
	    fprintf(ut_out, "%s.%s Failed - ", group, current_test->name);
	    vfprintf(ut_out, fmt, ap);
	    fprintf(ut_out, "\n");
	    va_end(ap);
	}
	ut_print("%s.%s Failed: Condition was true\n", group, current_test->name);
	current_test->pass = 0;
    } else {
	if (current_test->pass == -1) {	// don't replace failed status
	    current_test->pass = 1;
	}
    }
    if (condition && current_test->abort) {
	longjmp(jump_env, 1);
    }
    return !condition;
}

char*
ut_load_file(const char *filename) {
    FILE	*f;
    char	*buf;
    long	len;

    if (0 == (f = fopen(filename, "r"))) {
	ut_print("Error: failed to open %s.\n", filename);
	return 0;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    if (0 == (buf = malloc(len + 1))) {
	ut_print("Error: failed to allocate %ld bytes for file %s.\n", len, filename);
	return 0;
    }
    fseek(f, 0, SEEK_SET);
    if (len != fread(buf, 1, len, f)) {
	ut_print("Error: failed to read %ld bytes from %s.\n", len, filename);
	fclose(f);
	return 0;
    }
    fclose(f);
    buf[len] = '\0';

    return buf;
}

void
ut_report_test(const char *testName) {
    utTest	t = find_test(testName);
    const char	*result;

    if (0 == t) {
	if (0 != testName) {
	    return;
	}
	t = current_test;
	if (0 == t) {
	    return;
	}
    }
    switch (t->pass) {
    case -1:
	result = "Skipped";
	break;
    case 0:
	result = "FAILED";
	break;
    case 1:
    default:
	result = "Passed";
	break;
    }
    ut_print("%s: %s", t->name, result);
}

void
ut_reset_test(const char *testName) {
    utTest	t = find_test(testName);

    if (0 == t) {
	if (0 != testName) {
	    return;
	}
	t = current_test;
	if (0 == t) {
	    return;
	}
    }
    t->pass = -1;
}

static void
usage(const char *app_name) {
    printf("\n%s [-m] [-o file] [-c file] [<test>]\n", app_name);
    printf("  -h       display this help page\n");
    printf("  -v       verbose output\n");
    printf("  -o file  name of output file to append to\n");
    printf("  -c file  name of output file to create and write to\n");
    printf("  <test>   single test to run instead of all (optional)\n\n");
    exit(0);
}

static utTest
find_test(const char *name) {
    if (0 != name) {
	utTest	t;

	for (t = tests; t->name != 0; t++) {
	    if (0 == strcmp(name, t->name)) {
		return t;
	    }
	}
    }
    return 0;
}

static int
mark_tests(const char *name) {
    int	cnt = 0;
    
    if (0 != name) {
	utTest	t;
	int	len = strlen(name);

	for (t = tests; t->name != 0; t++) {
	    if (0 == strncmp(name, t->name, len)) {
		t->run = true;
		cnt++;
	    }
	}
    }
    return cnt;
}

void
ut_hex_dump(const uint8_t *data, int len) {
    const uint8_t	*b = data;
    const uint8_t	*end = data + len;
    char		buf[20];
    char		*s = buf;
    int			i;

    for (; b < end; b++) {
        printf("%02X ", *b);
	if (' ' <= *b && *b < 127) {
	    *s++ = (char)*b;
	} else {
	    *s++ = '.';
	}
	i = (b - data) % 16;
        if (7 == i) {
            printf(" ");
	    *s++ = ' ';
        } else if (15 == i) {
	    *s = '\0';
            printf("  %s\n", buf);
	    s = buf;
        }
    }
    i = (b - data) % 16;
    if (0 != i) {
	if (i < 8) {
	    printf(" ");
	}
	for (; i < 16; i++) {
	    printf("   ");
	}
	*s = '\0';
	printf("  %s\n", buf);
    }
}

void
ut_hex_dump_buf(const uint8_t *data, int len, char *out) {
    const unsigned char	*b = data;
    const unsigned char	*end = data + len;
    char		buf[20];
    char		*s = buf;
    int			i;

    for (; b < end; b++) {
        out += sprintf(out, "%02X ", *b);
	if (' ' <= *b && *b < 127) {
	    *s++ = (char)*b;
	} else {
	    *s++ = '.';
	}
	i = (b - data) % 16;
        if (7 == i) {
	    *out++ = ' ';
	    *s++ = ' ';
        } else if (15 == i) {
	    *s = '\0';
            out += sprintf(out, "  %s\n", buf);
	    s = buf;
        }
    }
    i = (b - data) % 16;
    if (0 != i) {
	if (i < 8) {
	    *out++ = ' ';
	}
	for (; i < 16; i++) {
	    *out++ = ' ';
	    *out++ = ' ';
	    *out++ = ' ';
	}
	*s = '\0';
	out += sprintf(out, "  %s\n", buf);
    }
    *out = '\0';
}

char*
ut_to_code_str(const uint8_t *data, int len) {
    const uint8_t	*d;
    const uint8_t	*end = data + len;
    int			clen = 0;
    char		*str;
    char		*s;
    
    for (d = data; d < end; d++) {
	if (*d < ' ' || 127 <= *d) {
	    clen += 4;
	} else if ('"' == *d || '\\' == *d) {
	    clen += 3;
	} else {
	    clen++;
	}
    }
    if (0 == (str = (char*)malloc(clen + 1))) {
	return 0;
    }
    for (s = str, d = data; d < end; d++) {
	if (*d < ' ' || 127 <= *d) {
	    *s++ = '\\';
	    *s++ = '0' + (*d >> 6);
	    *s++ = '0' + ((*d >> 3) & 0x07);
	    *s++ = '0' + (*d & 0x07);
	} else if ('"' == *d || '\\' == *d) {
	    *s++ = '\\';
	    *s++ = '\\';
	    *s++ = *d;
	} else {
	    *s++ = *d;
	}
    }
    *s = '\0';
    
    return str;
}
