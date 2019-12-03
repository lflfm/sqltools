/** \file arg_deep_copy.h
*   
*   Various utility classes supporting deep copying idioms.
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   22_Sep-99 : Separated out deep copying algorithms           Alan Griffiths
*   18-May-99 : Port to M$VC++ v5                               Alan Griffiths
*   28-Apr-99 : Correct make_copy as suggested by Kevlin Henney Alan Griffiths
*   22-Apr-99 : Added body_part_ptr                             Alan Griffiths
*   03-Apr-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_DEEP_COPY_H
#define ARG_DEEP_COPY_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#ifndef ARG_DEEP_COPY_UTILS_H
#include "arg_deep_copy_utils.h"
#endif

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
	*   A "Smart pointer" that takes ownership of the referenced object and 
    *   implements copy construction/assignment by copying the referenced 
    *   object.
    *
    *   The correct algorithm for copying is resolved by using the 
    *   arg::deep_copy() template.  This means that the referenced object
    *   will be  copied using the copy construction unless it is tagged
    *   by inheriting from arg::cloneable or an overload of arg::deep_copy()
    *   is provided.
	*
	*	All methods support the strong exception safety guarantee with the
    *   proviso that the referenced type must have a non-throwing destructor.
	*
	* @author Alan Griffiths
	**/
	template<class pointee_type>
	class deep_copy_ptr
	{
	public:
	
	// Construction

		/// "0 initialised" pointer
		deep_copy_ptr();
		
		/// Takes ownership of p
		explicit deep_copy_ptr(pointee_type* p) throw();
		
		/// Creates a "deep copy" of rhs
		deep_copy_ptr(const deep_copy_ptr& rhs);

		~deep_copy_ptr() throw();

	
	// Accessors

		/// Returns contents
		pointee_type* get() const;
	
		/// Indirection operator
		pointee_type* operator->() const;
	
		///	Dereference operator
		pointee_type& operator*() const;

	
	// Mutators

		/// Becomes a "deep copy" of rhs
		deep_copy_ptr& operator=(const deep_copy_ptr& rhs);

		/// Release ownership (and becomes 0)
		pointee_type* release();

		/// Delete existing contents (and take ownership of p)
		void reset(pointee_type* p) throw();
		
		/// Swaps contents with "with"
		void swap(deep_copy_ptr& with) throw();

        
	private:
		pointee_type*	pointee;
	};
	


	/**
	*   A const-qualified smart pointer that takes ownership of the 
    *   referenced object and implements copy construction/assignment by 
    *   copying the referenced object.  (A non-const
    *   body_part_ptr behaves as a deep_copy_ptr while a const one
    *   provides const access to the referenced object.)
    *
    *   The correct algorithm for copying is resolved by using the 
    *   arg::deep_copy() template.  This means that the referenced object
    *   will be  copied using the copy construction unless it is tagged
    *   by inheriting from arg::cloneable or an overload of arg::deep_copy()
    *   is provided.
	*
	*	All methods support the strong exception safety guarantee with the
    *   proviso that the referenced type must have a non-throwing destructor.
	*
	* @author Alan Griffiths
	**/
	template<class pointee_type>
	class body_part_ptr
	{
	public:
	
	//  Construction & assignment

		/// Default 0 initialisation
		body_part_ptr() : pointee() {}
		
		/// Construct owining p
		explicit body_part_ptr(pointee_type* p) throw() : pointee(p) {}
		
		// Default copy is OK

	
	//  Accessors - (overloaded on const)

		/// Return contents (const)
		const pointee_type* get() const			{ return pointee.get(); }
	
		/// Return contents (non-const)
		pointee_type* get()						{ return pointee.get(); }
	
		/// Indirection operator (const)
		const pointee_type* operator->() const	{ return pointee.get(); }
	
		/// Indirection operator (non-const)
		pointee_type* operator->()				{ return pointee.get(); }
	
		/// Dereference operator (const)
		const pointee_type& operator*() const	{ return *pointee; }

		/// Dereference operator (non-const)
		pointee_type& operator*()				{ return *pointee; }

	
	//  Mutators

		/// Release ownership (and becomes 0)
		pointee_type* release()					{ return pointee.release(); }

		/// Delete existing contents (and take ownership of p)
		void reset(pointee_type* p)	throw()		{ pointee.reset(p); }
		
		/// Swaps contents with "with"
		void swap(deep_copy_ptr<pointee_type>& with) throw()
			{ pointee.swap(with); }
		
		/// Conversion to a non-const deep_copy_ptr
		operator deep_copy_ptr<pointee_type>&()	{ return pointee; }

		// Default assignment is OK

        
	private:
		deep_copy_ptr<pointee_type>	pointee;
	};


	template<class pointee_type>
	inline deep_copy_ptr<pointee_type>::deep_copy_ptr() : pointee(0) {}
		
	template<class pointee_type>
	inline deep_copy_ptr<pointee_type>::deep_copy_ptr(pointee_type* p) throw() 
	    : pointee(p) {}
		
	template<class pointee_type>
	inline deep_copy_ptr<pointee_type>::deep_copy_ptr(const deep_copy_ptr& rhs)
		: pointee(deep_copy(rhs.pointee)) {}
	
	template<class pointee_type>
	inline deep_copy_ptr<pointee_type>& deep_copy_ptr<pointee_type>::
    operator=(const deep_copy_ptr& rhs)
	{
		pointee_type* p = deep_copy(rhs.pointee);
			
		delete pointee,	pointee = p;
		
		return *this;
	}			

	template<class pointee_type>
	inline deep_copy_ptr<pointee_type>::~deep_copy_ptr() throw()
		{
			delete pointee;
		}

	template<class pointee_type>
	pointee_type* deep_copy_ptr<pointee_type>::get() const
	{
		return pointee;
	}
	
	template<class pointee_type>
	inline pointee_type* deep_copy_ptr<pointee_type>::operator->() const
	{
		return pointee;
	}
	
	template<class pointee_type>
	inline pointee_type& deep_copy_ptr<pointee_type>::operator*() const
	{
		return *pointee;
	}

	template<class pointee_type>
	inline pointee_type* deep_copy_ptr<pointee_type>::release() 
	{
		pointee_type* temp = pointee;
		pointee = 0;
		return temp;
	}

	template<class pointee_type>
	inline void deep_copy_ptr<pointee_type>::reset(pointee_type* p) throw()
	{
		pointee_type* temp = pointee;
		pointee = p;
		delete temp;
	}
		
	template<class pointee_type>
	inline void deep_copy_ptr<pointee_type>::swap(deep_copy_ptr& with) throw()
	{
		pointee_type* temp = pointee;
		pointee = with.pointee;
		with.pointee = temp;
	}
}


namespace std
{
	/// Overload swap algorithm to provide an efficient, non-throwing version
	template<class pointee_type>
	inline void swap(
		::arg::deep_copy_ptr<pointee_type>& lhs, 
		::arg::deep_copy_ptr<pointee_type>& rhs) throw()
	{
		lhs.swap(rhs);
	}

	/// Overload swap algorithm to provide an efficient, non-throwing version
	template<class pointee_type>
	inline void swap(
		::arg::body_part_ptr<pointee_type>& lhs, 
		::arg::body_part_ptr<pointee_type>& rhs) throw()
	{
		lhs.swap(rhs);
	}
}

#endif
