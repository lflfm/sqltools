/** \file arg_grin_ptr.h
*   
*   grin_ptr<> - Provides support for "Cheshire Cat" idiom
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   21_Sep-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_GRIN_PTR_H
#define ARG_GRIN_PTR_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#ifndef ARG_DEEP_COPY_UTILS_H
#include "arg_deep_copy_utils.h"
#endif

#include ARG_STLHDR(algorithm)



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
	*	Pointer type to support "Cheshire Cat" idiom - only the indicated 
    *   constructor requires access to the complete pointee type.
    *
	*   A const-qualified smart pointer that takes ownership of the 
    *   referenced object and implements copy construction/assignment by 
    *   copying the referenced object.
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
	*                                               @author Alan Griffiths
	**/
	template<typename p_type>
	class grin_ptr
	{
		///
        typedef void (*delete_ftn)(p_type* p);
		
		///
        typedef p_type* (*copy_ftn)(const p_type* p);

	public:
	//  Construction & assignment

		/// Construct owining p - p_type must be a complete type
		explicit grin_ptr(p_type* pointee) 
			: do_copy(&my_copy_ftn), p(pointee), do_delete(my_delete_ftn) 
            {
            // "sizeof(p_type)" will force a diagnostic for an incomplete type
        	    sizeof(p_type);
            }
		
		/// Makes a copy of the object referenced by rhs
		grin_ptr(const grin_ptr& rhs);
		
		/// Destroys the referenced object
		~grin_ptr() throw()              { do_delete(p); }

	
	//  Accessors - (overloaded on const)

		/// Return contents (const)
		const p_type* get() const        { return p; }
	
		/// Return contents (non-const)
		p_type* get()                    { return p; }
	
		/// Indirection operator (const)
		const p_type* operator->() const { return p; }
	
		/// Indirection operator (non-const)
		p_type* operator->()             { return p; }
	
		/// Dereference operator (const)
		const p_type& operator*() const	 { return *p; }

		/// Dereference operator (non-const)
		p_type& operator*()              { return *p; }

	
	//  Mutators

		/// Swaps contents with "with"
		void swap(grin_ptr& with) throw()
			{ p_type* pp = p; p = with.p; with.p = pp; }
		
		/// Makes a copy of the object referenced by rhs (destroys old)
		grin_ptr& operator=(const grin_ptr& rhs);

	
	private:
		copy_ftn	do_copy;
		p_type*		p;
		delete_ftn	do_delete;

        static void my_delete_ftn(p_type* p)
        {
        	delete p;
        }
		
        static p_type* my_copy_ftn(const p_type* p)
        {
        	return deep_copy(p);
        }
	};


	template<typename p_type>
    inline grin_ptr<p_type>::grin_ptr(const grin_ptr& rhs)
	:
		do_copy(rhs.do_copy),
		p(do_copy(rhs.p)),
		do_delete(rhs.do_delete)
	{
	}
        
	template<typename p_type>
	inline grin_ptr<p_type>& grin_ptr<p_type>::operator=(const grin_ptr& rhs)
	{
		p_type* pp = do_copy(rhs.p);
		do_delete(p);
		p = pp;
		return *this;
	}
}

namespace std
{
	/// Overload swap algorithm to provide an efficient, non-throwing version
    template<class p_type>
	inline void swap(
		::arg::grin_ptr<p_type>& lhs, 
		::arg::grin_ptr<p_type>& rhs) throw()
	{
		lhs.swap(rhs);
	}
}
#endif
