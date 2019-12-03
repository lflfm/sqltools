/** \file arg_test.h
*   
*   Utility functions for building a test harness
*   
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   03-Apr-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_TEST_REPORTING_H
#define ARG_TEST_REPORTING_H

/**
*   arglib: A collection of utilities. (E.g. smart pointers).
*   Copyright (C) 1999, 2000 Alan Griffiths.
*
*   This code is part of the 'arglib' library. The latest version of
*   this library is available from:
*
*      http:*www.octopull.demon.co.uk/arglib/
*
*   This code may be used for any purpose provided that:
*
*   1. This complete notice is retained unchanged in any version
*       of this code.
*
*   2. If this code is incorporated into any work that displays
*       copyright notices then the copyright for the 'arglib'
*       library must be included.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*   @author alan@octopull.demon.co.uk
**/
namespace arg
{
	/**
    *   Reset the counts of tests executed/failed to zero and return
    *   the old count of tests executed.
    *
    *   @author alan@octopull.demon.co.uk
    **/
    int reset_test_count();
    
	/**
    *   Normally invoked via the ARG_TEST macro.  Incorporates the result of
    *   the test into the statistics.  Calls either report_test_failure() or
    *   report_test_success() according to the "result" parameter.
    *
    *   @return result
    *
    *   @author alan@octopull.demon.co.uk
    **/
	bool report_test(bool result, const char* file, int line, const char* test);

	/**
    *   Normally invoked via report_test().  Writes an error message to 
    *   std::cerr and increments both the count of tests run and of tests 
    *   failed.
    *
    *   @author alan@octopull.demon.co.uk
    **/
	void report_test_failure(const char* file, int line, const char* test);

	/**
    *   Normally invoked via report_test().  Increments the count of tests run.
    *
    *   @author alan@octopull.demon.co.uk
    **/
	void report_test_success(const char* file, int line, const char* test);

	/**
    *   Normally invoked via the ARG_END_TESTS macro.  Writes summary 
    *   information of number of tests run (and failed).  Returns the 
    *   count of tests failed.
    *
    *   @author alan@octopull.demon.co.uk
    **/
	int report_errors(const char* file, int line, const int expected_tests);
}	


/**
*   Expands to a call of arg::report_test() with the file, line and
*   test expression parameters completed.
*
*   @author alan@octopull.demon.co.uk
**/
#define ARG_TEST(bool_expr)\
arg::report_test(bool_expr, __FILE__, __LINE__, #bool_expr)

/**
*   Expands to a call of arg::report_errors() with the file and line 
*   parameters completed.
*
*   @author alan@octopull.demon.co.uk
**/
#define ARG_END_TESTS(expected_tests)\
arg::report_errors(__FILE__, __LINE__, expected_tests)

#endif
