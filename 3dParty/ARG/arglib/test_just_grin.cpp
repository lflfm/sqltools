//  File. . . . : test_just_grin.cpp
//  Description : just_grin_ptr<> - Provides support for "Cheshire Cat" idiom
//  Rev . . . . :
//
//  Classes . . : 
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
//  22-Sep-99 : Original version                                Alan Griffiths
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

#include "arg_just_grin.h"

#include "arg_test.h"

// Force template instantiation, to ensure it all compiles!
template arg::just_grin_ptr<int>;

namespace
{
	class outer
	{
	public:
		outer();
		
		int get_current_instances() const;
		int get_copy_count() const;
		int get_delete_count() const;
		
	private:
		class inner;
		arg::just_grin_ptr<inner> body;
	};
}

int main()
{
	outer o1;
	
	ARG_TEST(1 == o1.get_current_instances());
	ARG_TEST(0 == o1.get_copy_count());
	ARG_TEST(0 == o1.get_delete_count());
	
	outer o2(o1);	// copy construction - should call inner copy ctor.
	
	ARG_TEST(2 == o1.get_current_instances());
	ARG_TEST(1 == o1.get_copy_count());
	ARG_TEST(0 == o1.get_delete_count());

	{
		outer o3;

		ARG_TEST(3 == o1.get_current_instances());
		ARG_TEST(1 == o1.get_copy_count());
		ARG_TEST(0 == o1.get_delete_count());
		
		o3 = o1;	// Assignment - should call inner copy ctor & dtor.

		ARG_TEST(3 == o1.get_current_instances());
		ARG_TEST(2 == o1.get_copy_count());
		ARG_TEST(1 == o1.get_delete_count());
	}	
	
	ARG_TEST(2 == o1.get_current_instances());
	ARG_TEST(2 == o1.get_copy_count());
	ARG_TEST(2 == o1.get_delete_count());

	return ARG_END_TESTS(15);
}


// All externally visible bits of the template have been instantiated,
// now complete the definition of inner, and implement the non-default
// methods of "outer".

class outer::inner
{
public:

	inner() 
	{
		++instances;
	}
	
	inner(const inner&)
	{
		++instances;
		++copies;
	}
	
	~inner() 
	{
		--instances;
		++deletes;
	}
	
	int get_current_instances() const
	{
		return instances;
	}
	
	int get_copy_count() const
	{
		return copies;
	}
	
	int get_delete_count() const
	{
		return deletes;
	}
	
private:	
	static int instances;
	static int copies;
	static int deletes;
};


int outer::inner::instances = 0;
int outer::inner::copies = 0;
int outer::inner::deletes = 0;


outer::outer()
:
	body(new inner())
{
}

int outer::get_current_instances() const
{
	return body->get_current_instances();
}

int outer::get_copy_count() const
{
	return body->get_copy_count();
}

int outer::get_delete_count() const
{
	return body->get_delete_count();
}
