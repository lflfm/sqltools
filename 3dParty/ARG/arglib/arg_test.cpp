//  File. . . . : arg_test.cpp
//  Description : Utility functions for building a test harness
//  Rev . . . . :
//
//  Classes . . : 
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
//  18-May-99 : Port to M$VC++ v5                               Alan Griffiths
//  03-Apr-99 : Original version                                Alan Griffiths
//
	/*-------------------------------------------------------------------**
	** arglib: A collection of utilities (e.g. smart pointers).          **
	** Copyright (C) 1999 Alan Griffiths.                                **
	**                                                                   **
	**  This code is part of the 'arglib' library. The latest version of **
	**  this library is available from:                                  **
	**																	 **
	**      http://www.octopull.demon.co.uk/arglib/                      **
	**																	 **
	**  This code may be used for any purpose provided that:             **
	**                                                                   **
	**    1. This complete notice is retained unchanged in any version   **
	**       of this code.                                               **
	**                                                                   **
	**    2. If this code is incorporated into any work that displays    **
	**       copyright notices then the copyright for the 'arglib'       **
	**       library must be included.                                   **
	**                                                                   **
	**  This library is distributed in the hope that it will be useful,  **
    **  but WITHOUT ANY WARRANTY; without even the implied warranty of   **
    **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.             **
	**                                                                   **
	**  You may contact the author at: alan@octopull.demon.co.uk         **
	**-------------------------------------------------------------------*/

#include "arg_test.h"

#include "arg_compiler.h"

#include ARG_CPPSTDHDR(iostream)
#include ARG_CSTDHDR(stdlib)

static int count_tests_run  = 0;
static int count_tests_fail = 0;

namespace arg
{

int reset_test_count()
{
	int old_count(count_tests_run);
	count_tests_run = 0;
	count_tests_fail = 0;
	return old_count;
}


void report_test_failure(const char* file, int line, const char* test)
{
	++count_tests_run;
	++count_tests_fail;
	std::cerr << file << "(" << line << "): FAILURE: " << test << std::endl;
}


void report_test_success(const char* file, int line, const char* test)
{
	++count_tests_run;
	if (getenv("REPORT_TEST_SUCCESS"))
		std::cerr << file << "(" << line << "): SUCCESS: " << test << std::endl;
}


bool report_test(bool result, const char* file, int line, const char* test)
{
	if (!result)
		report_test_failure(file, line, test);
	else
		report_test_success(file, line, test);
		
	return result;		
}


int report_errors(const char* file, int line, const int expected_tests)
{
	int errors(count_tests_fail);
	
	std::cerr << file << "(" << line << "): END OF TEST RUN: ";
	
	if (expected_tests != count_tests_run)
	{
		++errors;
	}


	if (0 == errors)
	{
		std::cerr << "SUCCESS: " << count_tests_run << " tests run";
	}
	else
	{
		std::cerr << "ERRORS: ";
		
		if (0 != count_tests_fail)
		{
			std::cerr << count_tests_fail 
				<< (count_tests_fail == 1 ? " test failed " : " tests failed ");
		}
		
		if (expected_tests != count_tests_run)
		{
			std::cerr << count_tests_run << "test run BUT " 
					  << expected_tests << " tests expected";
		}
	}

	std::cerr << std::endl;
	
	return errors;
}

}
