//  File. . . . : test_grin_ptr1.cpp
//  Description : 
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

#include "arg_grin_ptr.h"

#include "test_grin_ptr.h"


namespace arg_test
{
	// Complete the definition of inner, and implement the non-default
	// methods of "outer". (The compiler is free to do the rest elsewhere

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
}
