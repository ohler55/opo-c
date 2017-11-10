// Copyright 2009, 2015, 2016, 2017 by Peter Ohler, All Rights Reserved

#ifndef __OPO_UT_H__
#define __OPO_UT_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @file test.h
 *
 * A unit test framework is provided by the functions in this file. Tests are
 * composed of multiple test cases. Each test case has a name and a function
 * that should run the test. Results are collected using comparison
 * functions. Output from tests is sent to stdout and optionally to a file.
 */
 
/**
 * Structure for listing the test cases in when calling the ut_init()
 * function. Only the <i>name</i> and <i>func</i> members need to be set.
 */
typedef struct _utTest {
    const char	*name;			// name of test case
    void	(*func)(void *ctx);	// function to run to execute test case
    void	*ctx;			// context for test function
    int		pass;			// true, false, -1 for skip
    bool	run;			// ignored
    bool	abort;			// abort on fail
} *utTest;

extern utTest	ut_append(utTest tests, const char *name, void (*func)(void *ctx), void *ctx);

extern utTest	ut_appenda(utTest tests, const char *name, void (*func)(void *ctx), void *ctx);

/**
 * Initializes the test framework and runs the specified tests. The
 * <i>argc</i> and <i>argv</i> arguments should be the command line arguments
 * to the application. The usage for the command line is. <pre>
 *     [-m] [-o file] [-c file]
 *       -m       turns on memory usage test
 *       -o file  name of output file to append to
 *       -c file  name of output file to create and write to
 * </pre>
 * The all_tests array should be terminated with a utTest with a name value of 0
 * (NULL).
 * @param argc same as main argc
 * @param argv same as main argv
 * @param group_name name of test suite
 * @param all_tests tests to execute, 0 name terminated
 */
extern void	ut_init(int argc, char **argv, const char *group_name, utTest all_tests);

/**
 * Prints to the unit test output file and to stdout. It uses the standard
 * printf formating rules.
 * @param format printf format specification
 * @param ... additional arguments to printf
 */
extern void	ut_print(const char *format, ...);

/**
 * Cleans up and outputs the test results.
 */
extern void	ut_done(void);

/**
 * Compares two strings. If they are the same then the test passes. If not it
 * is marked as failing. The <i>expected</i> string can contain special wild
 * card characters that match specific patterns. The wild card characters are:
 * <ul>
 *  <li><b>?</b> - match exactly one character, any character
 *  <li><b>$</b> - match a decimal number
 *  <li><b>#</b> - match a hexidecimal number (0-9,a-f,A-F)
 *  <li><b>*</b> - match any number of characters, any characters
 * </ul>
 * @param expected expected pattern
 * @param actual actual test result string
 * @return Returns the result of the test, true for pass, false for failure.
 */
extern bool	ut_same(const char *expected, const char *actual, const char *format, ...);

/**
 * Compares two integers. If they are the same then the test passes. If not it
 * is marked as failing.
 * @param expected expected number
 * @param actual actual test result number
 * @return Returns the result of the test, true for pass, false for failure.
 */
extern bool	ut_same_int(int64_t expected, int64_t actual, const char *format, ...);

/**
 * Compares two objects with a provided comparison function. If they are the
 * same then the test passes. If not it is marked as failing.
 * @param expected expected value
 * @param actual actual value
 * @param cmp comparison function
 * @return Returns the result of the test, true for pass, false for failure.
 */
extern bool	ut_same_cmp(void *expected,
				  void *actual,
				  char* (*cmp)(void *ex, void *ac),
				  const char *format, ...);

extern bool	ut_null(void *value, const char *fmt, ...);
extern bool	ut_not_null(void *value, const char *fmt, ...);

/**
 * Check the result of a test by looking at the value of the <i>condition</i>
 * argument. If the <i>condition</i> is not zero the test passes.
 * @param condition condition to check
 * @return Returns the result of the test, true for pass, false for failure.
 */
extern bool	ut_true(bool condition, const char *fmt, ...);

/**
 * Check the result of a test by looking at the value of the <i>condition</i>
 * argument. If the <i>condition</i> is zero the test passes.
 * @param condition condition to check
 * @return Returns the result of the test, true for pass, false for failure.
 */
extern bool	ut_false(bool condition, const char *fmt, ...);

/**
 * Marks the test as failing. There can be multiple successes or passes in a
 * test and one failure will cause the whole test to fail.
 */
extern void	ut_fail(const char *fmt, ...);

/**
 * Creates a string that is the contents of a file. Useful for loading in
 * results from a stream output. The result must be freed by the caller.
 * @param filename name of file to load
 * @return Returns the contents of the specified file as a string.
 */
extern char*	ut_load_file(const char *filename);

/**
 * Force the generation of a test report. Generally called by ut_done().
 * @param testName test name to find the results
 */
extern void	ut_report_test(const char *testName);

/**
 * Resets the test to not-executed, erasing any pass or failed status of the test.
 * @param testName test name to rest
 */
extern void	ut_reset_test(const char *testName);

/**
 * Prints out a hex dump of an array of bytes in hex and ascii format.
 *
 * @param data bytes to print
 * @param len number of bytes to print.
 * @param out output buffer should be 4.5 times the size of the data
 */
extern void	ut_hex_dump_buf(const uint8_t *data, int len, char *out);

/**
 * Fills a buffer with a hex dump of an array of bytes in hex and ascii
 * format.
 *
 * @param data bytes to print
 * @param len number of bytes to print.
 */
extern void	ut_hex_dump(const uint8_t *data, int len);

/**
 * Returns a string suitable for placing in code.
 *
 * @param data bytes to translate
 * @param len number of bytes to translate.
 */
extern char*	ut_to_code_str(const uint8_t *data, int len);

/**
 * Output file for the test print function. It can be used to direct output to
 * the test results.
 */
extern FILE	*ut_out;

/**
 * Flag indicating the test should run in verbose mode. This can also be set
 * on the command line with the -v option.
 */
extern bool	ut_verbose;

#endif /* __OPO_UT_H__ */
