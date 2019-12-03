//  File. . . . : test_deep_copy.cpp
//  Description : a test harness
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

#include "arg_deep_copy.h"

#include "arg_test.h"


struct test_deep_copy
{
	test_deep_copy()
	{
		++instances;
	}


	test_deep_copy(const test_deep_copy& )
	{
		++instances;
	}

	~test_deep_copy()
	{
		--instances;
	}
	
	bool is_non_const() 
	{
		return true;
	}

	bool is_non_const() const
	{
		return false;
	}

	static int instances;
};



struct clonable_test_deep_copy : arg::cloneable
{
	clonable_test_deep_copy()
	{
		++instances;
	}

	~clonable_test_deep_copy()
	{
		--instances;
	}

	clonable_test_deep_copy* clone() const
	{
		return new clonable_test_deep_copy;
	}
	
	static int instances;

protected:

	clonable_test_deep_copy(const clonable_test_deep_copy& )
	{
		++instances;
	}
};


struct ClonableTestDeepCopy : arg::Cloneable
{
	ClonableTestDeepCopy()
	{
		++instances;
	}

	~ClonableTestDeepCopy()
	{
		--instances;
	}

	ClonableTestDeepCopy* makeClone() const
	{
		return new ClonableTestDeepCopy;
	}
	
	static int instances;

protected:

	ClonableTestDeepCopy(const ClonableTestDeepCopy& )
	{
		++instances;
	}
};



int test_deep_copy::instances = 0;
int clonable_test_deep_copy::instances = 0;
int ClonableTestDeepCopy::instances = 0;



void test_default()
{
	ARG_TEST(0 == test_deep_copy::instances);
	
	// Default constructor
	arg::deep_copy_ptr<test_deep_copy> ptcp0;
	ARG_TEST(0 == test_deep_copy::instances);

	{
		// Explict constructor
		arg::deep_copy_ptr<test_deep_copy> ptcp1(new test_deep_copy);
		ARG_TEST(1 == test_deep_copy::instances);
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp1;
		ARG_TEST(2 == test_deep_copy::instances);
	}	

	// Destructor deletes OK (!0)
	ARG_TEST(1 == test_deep_copy::instances);
	
	// Reset to 0
	delete ptcp0.release();
	ARG_TEST(0 == test_deep_copy::instances);
	ARG_TEST(0 == ptcp0.get());
	
	// Reset to !0
	ptcp0.reset(new test_deep_copy);
	ARG_TEST(1 == test_deep_copy::instances);

	{
        using std::swap;
        
		// Copy constructor
		arg::deep_copy_ptr<test_deep_copy> ptcp2(ptcp0);
		ARG_TEST(2 == test_deep_copy::instances);
		
		// assignment (lhs !0, rhs !0)
		ptcp0 = ptcp2;
		ARG_TEST(2 == test_deep_copy::instances);

		// assignment (lhs !0, rhs !0)
		ptcp2 = ptcp0;
		ARG_TEST(2 == test_deep_copy::instances);

		// self assignment (lhs !0, rhs !0)
		ptcp0 = ptcp0;
		ARG_TEST(2 == test_deep_copy::instances);

		ptcp0.reset(0);
		ARG_TEST(1 == test_deep_copy::instances);

		swap(ptcp2, ptcp0);
		
		ptcp0.reset(new test_deep_copy);
		ARG_TEST(1 == test_deep_copy::instances);

		// assignment (lhs !0, rhs 0)
		ptcp0 = ptcp2;
		ARG_TEST(0 == test_deep_copy::instances);

		// self assignment (lhs 0, rhs 0)
		ptcp0 = ptcp0;
		ARG_TEST(0 == test_deep_copy::instances);
	}	

	// Destructor deletes OK (0)
	ARG_TEST(0 == test_deep_copy::instances);
}	

	

void test_clone()
{
	ARG_TEST(0 == clonable_test_deep_copy::instances);
	
	// Default constructor
	arg::deep_copy_ptr<clonable_test_deep_copy> ptcp0;
	ARG_TEST(0 == clonable_test_deep_copy::instances);

	{
		// Explict constructor
		arg::deep_copy_ptr<clonable_test_deep_copy> ptcp1(new clonable_test_deep_copy);
		ARG_TEST(1 == clonable_test_deep_copy::instances);
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp1;
		ARG_TEST(2 == clonable_test_deep_copy::instances);
	}	

	// Destructor deletes OK (!0)
	ARG_TEST(1 == clonable_test_deep_copy::instances);
	
	// Reset to 0
	delete ptcp0.release();
	ARG_TEST(0 == clonable_test_deep_copy::instances);
	ARG_TEST(0 == ptcp0.get());
	
	// Reset to !0
	ptcp0.reset(new clonable_test_deep_copy);
	ARG_TEST(1 == clonable_test_deep_copy::instances);

	{
        using std::swap;
        
		// Copy constructor
		arg::deep_copy_ptr<clonable_test_deep_copy> ptcp2(ptcp0);
		ARG_TEST(2 == clonable_test_deep_copy::instances);
		
		// assignment (lhs !0, rhs !0)
		ptcp0 = ptcp2;
		ARG_TEST(2 == clonable_test_deep_copy::instances);

		// assignment (lhs !0, rhs !0)
		ptcp2 = ptcp0;
		ARG_TEST(2 == clonable_test_deep_copy::instances);

		// self assignment (lhs !0, rhs !0)
		ptcp0 = ptcp0;
		ARG_TEST(2 == clonable_test_deep_copy::instances);

		ptcp0.reset(0);
		ARG_TEST(1 == clonable_test_deep_copy::instances);

		swap(ptcp2, ptcp0);
		
		ptcp0.reset(new clonable_test_deep_copy);
		ARG_TEST(1 == clonable_test_deep_copy::instances);

		// assignment (lhs !0, rhs 0)
		ptcp0 = ptcp2;
		ARG_TEST(0 == clonable_test_deep_copy::instances);

		// self assignment (lhs 0, rhs 0)
		ptcp0 = ptcp0;
		ARG_TEST(0 == clonable_test_deep_copy::instances);
	}	

	// Destructor deletes OK (0)
	ARG_TEST(0 == clonable_test_deep_copy::instances);
}


void testClone()
{
	ARG_TEST(0 == ClonableTestDeepCopy::instances);
	
	// Default constructor
	arg::deep_copy_ptr<ClonableTestDeepCopy> ptcp0;
	ARG_TEST(0 == ClonableTestDeepCopy::instances);

	{
		// Explict constructor
		arg::deep_copy_ptr<ClonableTestDeepCopy> ptcp1(new ClonableTestDeepCopy);
		ARG_TEST(1 == ClonableTestDeepCopy::instances);
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp1;
		ARG_TEST(2 == ClonableTestDeepCopy::instances);
	}	

	// Destructor deletes OK (!0)
	ARG_TEST(1 == ClonableTestDeepCopy::instances);
	
	// Reset to 0
	delete ptcp0.release();
	ARG_TEST(0 == ClonableTestDeepCopy::instances);
	ARG_TEST(0 == ptcp0.get());
	
	// Reset to !0
	ptcp0.reset(new ClonableTestDeepCopy);
	ARG_TEST(1 == ClonableTestDeepCopy::instances);

	{
        using std::swap;
        
		// Copy constructor
		arg::deep_copy_ptr<ClonableTestDeepCopy> ptcp2(ptcp0);
		ARG_TEST(2 == ClonableTestDeepCopy::instances);
		
		// assignment (lhs !0, rhs !0)
		ptcp0 = ptcp2;
		ARG_TEST(2 == ClonableTestDeepCopy::instances);

		// assignment (lhs !0, rhs !0)
		ptcp2 = ptcp0;
		ARG_TEST(2 == ClonableTestDeepCopy::instances);

		// self assignment (lhs !0, rhs !0)
		ptcp0 = ptcp0;
		ARG_TEST(2 == ClonableTestDeepCopy::instances);

		ptcp0.reset(0);
		ARG_TEST(1 == ClonableTestDeepCopy::instances);

		swap(ptcp2, ptcp0);
		
		ptcp0.reset(new ClonableTestDeepCopy);
		ARG_TEST(1 == ClonableTestDeepCopy::instances);

		// assignment (lhs !0, rhs 0)
		ptcp0 = ptcp2;
		ARG_TEST(0 == ClonableTestDeepCopy::instances);

		// self assignment (lhs 0, rhs 0)
		ptcp0 = ptcp0;
		ARG_TEST(0 == ClonableTestDeepCopy::instances);
	}	

	// Destructor deletes OK (0)
	ARG_TEST(0 == ClonableTestDeepCopy::instances);
}


void test_body_part()
{
	ARG_TEST(0 == test_deep_copy::instances);
	
	// Default constructor
	arg::body_part_ptr<test_deep_copy> ptcp0;
	ARG_TEST(0 == test_deep_copy::instances);

	{
		// Explict constructor
		arg::body_part_ptr<test_deep_copy> ptcp1(new test_deep_copy);
		ARG_TEST(1 == test_deep_copy::instances);
		
		// assignment (lhs 0, rhs !0)
		ptcp0 = ptcp1;
		ARG_TEST(2 == test_deep_copy::instances);
	}	

	// Destructor deletes OK (!0)
	ARG_TEST(1 == test_deep_copy::instances);

	{
		ARG_TEST(ptcp0->is_non_const());
		ARG_TEST((*ptcp0).is_non_const());
		ARG_TEST(ptcp0.get()->is_non_const());
		
		const arg::body_part_ptr<test_deep_copy> ptcp1(new test_deep_copy);

		ARG_TEST(!ptcp1->is_non_const());
		ARG_TEST(!(*ptcp1).is_non_const());
		ARG_TEST(!ptcp1.get()->is_non_const());
		
	}
	
	// Reset to 0
	delete ptcp0.release();
	ARG_TEST(0 == test_deep_copy::instances);
	ARG_TEST(0 == ptcp0.get());
	
	// Reset to !0
	ptcp0.reset(new test_deep_copy);
	ARG_TEST(1 == test_deep_copy::instances);

	{
        using std::swap;
        
		// Copy constructor
		arg::body_part_ptr<test_deep_copy> ptcp2(ptcp0);
		ARG_TEST(2 == test_deep_copy::instances);
		
		// assignment (lhs !0, rhs !0)
		ptcp0 = ptcp2;
		ARG_TEST(2 == test_deep_copy::instances);

		// assignment (lhs !0, rhs !0)
		ptcp2 = ptcp0;
		ARG_TEST(2 == test_deep_copy::instances);

		// self assignment (lhs !0, rhs !0)
		ptcp0 = ptcp0;
		ARG_TEST(2 == test_deep_copy::instances);

		ptcp0.reset(0);
		ARG_TEST(1 == test_deep_copy::instances);

		swap(ptcp2, ptcp0);
		
		ptcp0.reset(new test_deep_copy);
		ARG_TEST(1 == test_deep_copy::instances);

		// assignment (lhs !0, rhs 0)
		ptcp0 = ptcp2;
		ARG_TEST(0 == test_deep_copy::instances);

		// self assignment (lhs 0, rhs 0)
		ptcp0 = ptcp0;
		ARG_TEST(0 == test_deep_copy::instances);
	}	

	// Destructor deletes OK (0)
	ARG_TEST(0 == test_deep_copy::instances);
}	

	

int main()
{
	test_default();
	test_clone();
	test_body_part();
	testClone();

	return ARG_END_TESTS(74);
}	
