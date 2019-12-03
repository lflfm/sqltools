//  File. . . . : arg_just_grin.h
//  Description : just_grin_ptr<> - Provides support for "Cheshire Cat" idiom
//  Rev . . . . :
//
//  Classes . . : just_grin_ptr<>
//
//  Modification History
//
//	04-Nov-99 : Revised copyright notice						Alan Griffiths
//  21_Sep-99 : Original version                                Alan Griffiths
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

#ifndef ARG_JUST_GRIN_H
#define ARG_JUST_GRIN_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#ifndef ARG_DEEP_COPY_UTILS_H
#include "arg_deep_copy_utils.h"
#endif

#include ARG_STLHDR(algorithm)



// just_grin_ptr<> - Provides support for "Cheshire Cat" idiom
namespace arg
{
	/**
	*	Factors out the grin pointer logic.  Can only be constructed via
	*	a derived class which provides deletion and copy functions.
	*<P>
	*	generic_grin<> assumes single ownership of the supplied pointer
	*	and utilises the methods supplied to its constructor to copy
	*	and delete the referenced object.
	*<P>
	*	N.B. The derived class is wholly responsible for type safety.
	*
	* @author Alan Griffiths
	**/
    class generic_grin
    {
    protected:
		///
        typedef void (*delete_ftn)(void* p);
		
		///
        typedef void* (*copy_ftn)(const void* p);

        ~generic_grin() throw() { do_delete(p); }
		
        ///
		generic_grin(void* p, copy_ftn do_copy, delete_ftn do_delete);
        
        ///
		generic_grin(const generic_grin& rhs);
        
        ///
		generic_grin& operator=(const generic_grin& rhs);
        
        ///
		void* release() { void* pp = p; do_delete(pp); p = 0; return pp; }
	
        ///
		void reset(void* pp) throw() { do_delete(p); p = pp; }
		
        ///
		void swap(generic_grin& with) throw()	{ std::swap(p, with.p); }

        ///
		void* get() const { return p; }

    private:
		copy_ftn	do_copy;
		void*		p;
		delete_ftn	do_delete;
    };
	

	inline generic_grin::generic_grin(void* pp, copy_ftn dc, delete_ftn dd)
	:
		do_copy(dc),
		p(pp),
		do_delete(dd)
	{
	}
        
    inline generic_grin::generic_grin(const generic_grin& rhs)
	:
		do_copy(rhs.do_copy),
		p(do_copy(rhs.p)),
		do_delete(rhs.do_delete)
	{
	}
        
	inline generic_grin& generic_grin::operator=(const generic_grin& rhs)
	{
		void* pp = do_copy(rhs.p);
		do_delete(p);
		p = pp;
		return *this;
	}
	
	/**
	*	Pointer type to support "Cheshire Cat" idiom.
	*<UL>
	*<LI>Each pointer owns a single pointee object.
	*</LI>
	*<LI>Copy and assignment make use of the arg::deep_copy algorithm
	*	to replicate the pointee.
	*</LI>
	*<LI>Only the indicated constructors requires access to the 
	*	complete pointee type.
	*</LI>
	*</UL>
	*<P>
	*	Implements the strong exception safety guarantee.
	*
	* @author Alan Griffiths
	**/
	template<typename p_type>
	class just_grin_ptr	: private generic_grin
	{
	public:
	/** @name 1 Construction & assignment */
	//@{	
		/// Default 0 initialisation
		just_grin_ptr() 
			: generic_grin(0, &copy_ftn, &delete_ftn) {}
		
		/// Construct owining p
		explicit just_grin_ptr(p_type* p) 
			: generic_grin(p, &copy_ftn, &delete_ftn) {}
		
		// Default copy is OK
	//@}	
	
	/** @name 2 Accessors - (overloaded on const) */
	//@{	
		/// Return contents (const)
		const p_type* get() const			{ return (p_type*)generic_grin::get(); }
	
		/// Return contents (non-const)
		p_type* get()						{ return (p_type*)generic_grin::get(); }
	
		/// Dereference op (const)
		const p_type* operator->() const	{ return (p_type*)generic_grin::get(); }
	
		/// Dereference op (non-const)
		p_type* operator->()				{ return (p_type*)generic_grin::get(); }
	
		/// Dereference op (const)
		const p_type& operator*() const	{ return *(p_type*)generic_grin::get(); }

		/// Dereference op (non-const)
		p_type& operator*()				{ return *(p_type*)generic_grin::get(); }
	//@}	
	
	/** @name 3 Mutators */
	//@{	
		/// Release ownership (and becomes 0)
		p_type* release()				{ return (p_type*)generic_grin::release(); }

		/// Delete existing contents (and take ownership of p)
		void reset(just_grin_ptr* p = 0) throw()	{ generic_grin::reset(p); }
		
		/// Swaps contents with "with"
		void swap(just_grin_ptr& with) throw()	{ generic_grin::swap(with); }
		
		// Default assignment is OK
	//@}	
	
	private:
        static void delete_ftn(void* p)
        {
        	delete static_cast<p_type*>(p);
        }
		
        static void* copy_ftn(const void* p)
        {
        	return deep_copy(static_cast<const p_type*>(p));
        }
	
	};
}

namespace std
{
	template<class p_type>
	inline void swap(
		::arg::just_grin_ptr<p_type>& lhs, 
		::arg::just_grin_ptr<p_type>& rhs) throw()
	{
		lhs.swap(rhs);
	}
}
#endif
