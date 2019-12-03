//  File. . . . : test_observer.cpp
//  Description : Templated collection of listeners
//  Rev . . . . :
//
//  Classes . . : Support for Observer/Subject pattern
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
//  01-Oct-99 : Original version                                Alan Griffiths
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

#include "arg_observer.h"

#include "arg_test.h"

#include ARG_CPPSTDHDR(iostream)

#ifdef _MSC_VER
#pragma warning(disable : 4127)
#define for if (false) ; else for
#endif

namespace
{
	class test_observer_interface
	{
	public:
		virtual void notification() = 0;
	};
	
	class test_notify_strategy : 
    public arg::subject<test_observer_interface>::notify_strategy
	{
		void operator()(test_observer_interface* observer) const
		{

			observer->notification();
		}
	};


	class test_observer : public test_observer_interface
	{
	public:
	
		test_observer() : notified(false)
		{
			
		}
		
		virtual void notification()
		{
			notified = true;
		}
		
		bool is_notified() const
		{
			return notified;
		}
		
		void set_notified(bool value) 
		{
			notified = value;
		}
		
	private:
		bool notified;
	};
	
	
	const int no_of_active_observers   = 5;
	const int no_of_inactive_observers = 5;
	const int no_of_observers = no_of_active_observers+no_of_inactive_observers;
}

    
int main()
{
	test_observer test_observers[no_of_observers];
	
	arg::subject<test_observer_interface> test_list;
	

	// Add the active observers to the list
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_active_observers; ++i)
	{
		test_list.add_observer(i);
	}
	
	
	// Confirm all observer notification flags are reset (no_of_observers tests)
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_observers; ++i)
	{
		ARG_TEST(false == i->is_notified());
	}
	
	
	// Nofify the active observers	
	test_list.notify(test_notify_strategy());

	// Confirm active observer notification flags are set and the
	// inactive observer nofification flags reset (no_of_observers tests)
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_observers; ++i)
	{
		ARG_TEST((i < test_observers+no_of_active_observers) == i->is_notified());
		
		i->set_notified(false);	// reset flags while we're here
	}

	// Repeat notification of the active observers	
	test_list.notify(test_notify_strategy());

	// Confirm active observer notification flags are set and the
	// inactive observer nofification flags reset (no_of_observers tests)
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_observers; ++i)
	{
		ARG_TEST((i < test_observers+no_of_active_observers) == i->is_notified());
		
		i->set_notified(false);	// reset flags while we're here
	}


	// Add the "inactive" observers to the list
	for (test_observer* i = test_observers+no_of_active_observers; 
		i != test_observers+no_of_observers; ++i)
	{
		test_list.add_observer(i);
	}
	
	
	// Notification of the all observers	
	test_list.notify(test_notify_strategy());

	// Confirm all observer notification flags are set (no_of_observers tests)
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_observers; ++i)
	{
		ARG_TEST(true == i->is_notified());
		
		i->set_notified(false);	// reset flags while we're here
	}


	// Remove the active observers to the list
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_active_observers; ++i)
	{
		test_list.remove_observer(i);
	}
	
	
	
	// Repeat notification of the active observers	
	test_list.notify(test_notify_strategy());

	// Confirm active observer notification flags are reset and the
	// "inactive" observer nofification flags set (no_of_observers tests)
	for (test_observer* i = test_observers; 
		i != test_observers+no_of_observers; ++i)
	{
        if (!ARG_TEST((i >= test_observers+no_of_active_observers) == i->is_notified()))
        {
            std::cout << "i=" << i-test_observers << '\n';
        }
		
		i->set_notified(false);	// reset flags while we're here
	}


	return ARG_END_TESTS(5*no_of_observers);
}	
