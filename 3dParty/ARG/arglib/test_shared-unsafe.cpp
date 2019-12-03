//  File. . . . : test_shared-unsafe.cpp
//  Description : 	A test harness for (un)counted_ptr classes.
//  				This also contains the code base for the article 
//					"uncounted pointers in C++".
//
//  Classes . . : 
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
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
#define ARG_UNSAFE_XVERT

#include "arg_shared.h"

#include "arg_test.h"

#include ARG_STLHDR(set)

//#define ARG_TEST_FOR_DOWNCAST_COMPILE_ERROR

struct instance_count
{
	instance_count()	{ ++instances; }
	~instance_count()	{ --instances; }
	static int instances;
};

struct derived_ic : instance_count {};

int instance_count::instances = 0;


// Corresponds to first example in article
namespace example1
{
	using arg::counted_ptr;

	class man;


	class woman
	{
	public:
		void set_husband(counted_ptr<man> value);
	private:
		counted_ptr<man> husband;
		instance_count   t;
	};


	class man
	{
	public:
		void set_wife(counted_ptr<woman> value);
	private:
		counted_ptr<woman> wife;
		instance_count     t;
	};


	void woman::set_husband(counted_ptr<man> value) { husband = value; }
	void man::set_wife(counted_ptr<woman> value)    { wife = value; }


	void f()
	{
		{
			counted_ptr<woman> pippa(new woman);
			ARG_TEST(1 == instance_count::instances);
			{
				counted_ptr<man> alan(new man); 
				ARG_TEST(2 == instance_count::instances);
				
				alan->set_wife(pippa);
				pippa->set_husband(alan);
			}
			ARG_TEST(2 == instance_count::instances);
		}
		
		// "Look ma - I've leaked!"
		ARG_TEST(2 == instance_count::instances);
	}
}	


// Corresponds to second example in article
namespace example2
{
	using arg::counted_ptr;

	class man;


	class woman
	{
	public:
		void set_husband(counted_ptr<man> value);
		~woman();
	private:
		counted_ptr<man> husband;
		instance_count   t;
	};


	class man
	{
	public:
		man();
		void set_wife(woman* value);
	private:
		woman*				 wife;
		instance_count       t;
	};


	void woman::set_husband(counted_ptr<man> value) { husband = value; }
	woman::~woman()	 { if (husband.get()) husband->set_wife(0); }
	man::man() : wife(), t() {}
	void man::set_wife(woman* value)  { wife = value; }


	void f()
	{
		{
			counted_ptr<woman> pippa(new woman);
			ARG_TEST(1 == instance_count::instances);
			{
				counted_ptr<man> alan(new man); 
				ARG_TEST(2 == instance_count::instances);
				
				alan->set_wife(pippa.get());
				pippa->set_husband(alan);
			}
			ARG_TEST(2 == instance_count::instances);
		}
		ARG_TEST(0 == instance_count::instances);
	}
}	


// Corresponds to third example in article
namespace example3
{
	using arg::counted_ptr;
	using arg::uncounted_ptr;

	class man;


	class woman
	{
	public:
		void set_husband(uncounted_ptr<man> value);
	private:
		uncounted_ptr<man> husband;
		instance_count   t;
	};


	class man
	{
	public:
		void set_wife(uncounted_ptr<woman> value);
	private:
		uncounted_ptr<woman> wife;
		instance_count       t;
	};


	void woman::set_husband(uncounted_ptr<man> value) { husband = value; }
	void man::set_wife(uncounted_ptr<woman> value)  { wife = value; }


	void f()
	{
		{
			counted_ptr<woman> pippa(new woman);
			ARG_TEST(1 == instance_count::instances);
			{
				counted_ptr<man> alan(new man); 
				ARG_TEST(2 == instance_count::instances);
				
				alan->set_wife(pippa);
				pippa->set_husband(alan);
			}
			// "alan" has gone, but no need to inform "pippa"
			ARG_TEST(1 == instance_count::instances);
		}
		ARG_TEST(0 == instance_count::instances);
	}
}	


// Corresponds to fourth example in article
namespace example4
{
	using arg::counted_ptr;
	using arg::uncounted_ptr;

	class person; class man; class woman;
	
	
	class person
	{
	public:
		virtual uncounted_ptr<person> get_spouse() const = 0;
		virtual ~person() {}
	private:
		instance_count   t;
	};


	class woman : public person
	{
	public:
		void set_husband(uncounted_ptr<man> value);
		virtual uncounted_ptr<person> get_spouse() const;
	private:
		uncounted_ptr<man> husband;
	};


	class man : public person
	{
	public:
		void set_wife(uncounted_ptr<woman> value);
		virtual uncounted_ptr<person> get_spouse() const;
	private:
		uncounted_ptr<woman> wife;
	};


	void woman::set_husband(uncounted_ptr<man> value) { husband = value; }
	void man::set_wife(uncounted_ptr<woman> value)    { wife = value; }
	uncounted_ptr<person> woman::get_spouse() const   { return husband; }
	uncounted_ptr<person> man::get_spouse() const     { return wife; }

	void marry(counted_ptr<man> a, counted_ptr<woman> p)
	{
		if (a.get() && p.get())
		{
			a->set_wife(p); p->set_husband(a);
		}
	}

	void f()
	{
		ARG_TEST(0 == instance_count::instances);
		
		// We'll set these up in a moment
		uncounted_ptr<woman> pippa;
		uncounted_ptr<man>   alan; 
		
		// A convenient constant
		const uncounted_ptr<person> no_one;
		
		// A container for keeping track of the living - this
		// would normally reside in one (or more) owning objects.
		std::set<counted_ptr<person> > the_living;

		{
			counted_ptr<woman> p(new woman);
			counted_ptr<man>   a(new man);
			
			the_living.insert(a);
			the_living.insert(p);
			
			// Keep track of the individuals
			alan  = a;
			pippa = p;
		}
		
		ARG_TEST(2 == instance_count::instances);
		ARG_TEST(2 == the_living.size());

		marry(alan, pippa);
		
		ARG_TEST(alan == pippa->get_spouse());
		ARG_TEST(pippa == alan->get_spouse());
		
		// Remove pippa from the living
		the_living.erase(the_living.find(pippa));
		ARG_TEST(1 == instance_count::instances);
		
		// pippa has gone, alan lacks a spouse...
		ARG_TEST(no_one == pippa);
		ARG_TEST(no_one == alan->get_spouse());
		
		// Remove alan from the living
		the_living.erase(the_living.find(alan));
		ARG_TEST(0 == instance_count::instances);
	}
}	

int main()
{
	using arg::counted_ptr;
	using arg::uncounted_ptr;

	{
		counted_ptr<instance_count> cp(new instance_count);
		ARG_TEST(1 == instance_count::instances);
		ARG_TEST(0 != cp.get());
		ARG_TEST(cp == cp);
	}
	ARG_TEST(0 == instance_count::instances);
	
	uncounted_ptr<instance_count> ucp;
	ARG_TEST(ucp == ucp);
	
	{
		counted_ptr<instance_count> cp(new instance_count);
		ARG_TEST(1 == instance_count::instances);

		ARG_TEST(cp != ucp);
		ucp = cp;
		ARG_TEST(cp == ucp);

		ARG_TEST(1 == instance_count::instances);
		ARG_TEST(0 != ucp.get());
	}
	ARG_TEST(0 == instance_count::instances);
	ARG_TEST(0 == ucp.get());
	
	example2::f();
	example3::f();
	example4::f();
	example1::f();	// Put it last, otherwise the leak confuses other tests
	
	{
		counted_ptr<derived_ic> pdic;
		counted_ptr<instance_count> pic(pdic);
	}
#ifdef ARG_TEST_FOR_DOWNCAST_COMPILE_ERROR
	{
		counted_ptr<instance_count> pic;
		counted_ptr<derived_ic> pdic(pic);
	}
#endif	
	return ARG_END_TESTS(33);
}

