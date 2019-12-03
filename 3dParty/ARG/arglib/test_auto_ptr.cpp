//  File. . . . : test_auto_ptr.cpp
//  Description : a test harness
//  Rev . . . . :
//
//  Classes . . : 
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
//  21-Apr-99 : Original version                                Alan Griffiths
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

// Hack to check library version of auto_ptr
//#define ARG_TEST_LIBRARY

#ifndef ARG_TEST_LIBRARY
#	include "arg_auto_ptr.h"
#else
#	include <memory>
#	define arg std
#endif

#include "arg_test.h"


struct test_auto_ptr
{
	test_auto_ptr()
	{
		++instances;
	}


	test_auto_ptr(const test_auto_ptr& )
	{
		++instances;
	}

	~test_auto_ptr()
	{
		--instances;
	}

	test_auto_ptr* clone() const
	{
		return new test_auto_ptr;
	}
	
	static int instances;
};



int test_auto_ptr::instances = 0;


	

int main()
{
	ARG_TEST(0 == test_auto_ptr::instances);
	
	// Default constructor
	arg::auto_ptr<test_auto_ptr> ptcp0;
	ARG_TEST(0 == test_auto_ptr::instances);
	ARG_TEST(0 == ptcp0.get());

	{
		// Explict constructor
		arg::auto_ptr<test_auto_ptr> ptcp1(new test_auto_ptr);
		ARG_TEST(1 == test_auto_ptr::instances);
		
		// Check initial ownership
		ARG_TEST(0 == ptcp0.get());
		ARG_TEST(0 != ptcp1.get());
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp1;
		ARG_TEST(1 == test_auto_ptr::instances);

		// Check transferred ownership
		ARG_TEST(0 != ptcp0.get());
		ARG_TEST(0 == ptcp1.get());
	}	

	// Destructor deletes OK (0)
	ARG_TEST(1 == test_auto_ptr::instances);
	
	// Release, then delete
	{
		test_auto_ptr* tmp = ptcp0.release();
		ARG_TEST(1 == test_auto_ptr::instances);
		delete tmp;
		ARG_TEST(0 == test_auto_ptr::instances);
	}
	
	// Reset to !0
	ptcp0.reset(new test_auto_ptr);
	ARG_TEST(1 == test_auto_ptr::instances);

	{
		// Copy constructor
		arg::auto_ptr<test_auto_ptr> ptcp2(ptcp0);
		ARG_TEST(1 == test_auto_ptr::instances);

		// Check transferred ownership
		ARG_TEST(0 == ptcp0.get());
		ARG_TEST(0 != ptcp2.get());
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp2;
		ARG_TEST(1 == test_auto_ptr::instances);

		// Check transferred ownership
		ARG_TEST(0 != ptcp0.get());
		ARG_TEST(0 == ptcp2.get());
		
		// assignment (lhs 0, rhs !0)
		ptcp2 = ptcp0;
		ARG_TEST(1 == test_auto_ptr::instances);

		// Check transferred ownership
		ARG_TEST(0 == ptcp0.get());
		ARG_TEST(0 != ptcp2.get());

		// self assignment (lhs !0, rhs !0)
		ptcp2 = ptcp2;
		ARG_TEST(1 == test_auto_ptr::instances);

		ptcp0.reset(0);
		ARG_TEST(1 == test_auto_ptr::instances);

		// assignment (lhs !0, rhs 0)
		ptcp2 = ptcp0;
		ARG_TEST(0 == test_auto_ptr::instances);

		// self assignment (lhs 0, rhs 0)
		ptcp0 = ptcp0;
		ARG_TEST(0 == test_auto_ptr::instances);
		
		ptcp2.reset(new test_auto_ptr);
		ptcp0.reset(new test_auto_ptr);
		ARG_TEST(2 == (*ptcp2).instances);	// Via operator*
	}	

	// Destructor deletes OK (!0)
	ARG_TEST(1 == ptcp0->instances);		// Via operator->

	ptcp0.reset(0);
	ARG_TEST(0 == test_auto_ptr::instances);
	
	return ARG_END_TESTS(29);
}	
