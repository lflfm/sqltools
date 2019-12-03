/** \file arg_deep_copy_utils.h
*   
*   A number of smart pointer types need mechanisms
*                 for copying the pointee objects.  This file supplies
*                 utility functions implementing the default algorithms 
*                 and permitting tailoring.
* 
*   Classes . . : cloneable, Cloneable  - tags to support deep copying.
*   Functions . : deep_copy<>()         - functions to support deep copying.
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   22-Sep-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_DEEP_COPY_UTILS_H
#define ARG_DEEP_COPY_UTILS_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
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
	*   Indicates that the class supports replication via a "clone()" member
    *   function.  This is only a "type tag" for the deep_copy template - no
    *   functionality is supported.  (This assumes standard C++ style naming.)
	*
	* @author Alan Griffiths
	**/
	struct cloneable {};
	

	/**
	*   Indicates that the class supports replication via a "makeClone()" member
    *   function.  This is only a "type tag" for the deep_copy template - no
    *   functionality is supported.  (This assumes Experian naming standard.)
	*
	* @author Alan Griffiths
	**/
	struct Cloneable {};

	/**
    *   This is not intended for direct use - it provides a default copy
    *   mechanism for template<class p_type> p_type* deep_copy(const type*).
    *
    *   This copies using the copy constructor.  Overloading this template 
    *   permits use of smart pointers with  "deep copy" semantics with other 
    *   copying interfaces. 
	*
	*	An overload based upon arg::cloneable is provided which
	*	uses a clone() member function.   Users should provide additional
    *   overloads of their own if they have different copying requirements.
	*
	* @author Alan Griffiths
	**/
	template<class p_type>
	inline p_type* deep_copy(const p_type* p, const void*) 
	{
		return p ? new p_type(*p) : 0;
	}
	
	
	/**
    *   This is not intended for direct use - it provides a "cloneable" copy
    *   mechanism for template<class p_type> p_type* deep_copy(const type*).
    *
	*	This is called for classes that implement the "cloneable" interfaces. 
	*
	* @author Alan Griffiths
	**/
	template<class p_type>
    inline p_type* deep_copy(const p_type *p, const cloneable *)
	{
		return p ? p->clone() : 0;
	}
	

	/**
    *   This is not intended for direct use - it provides a "Cloneable" copy
    *   mechanism for template<class p_type> p_type* deep_copy(const type*).
    *
	*	This is called for classes that implement the "Cloneable" interfaces. 
	*
	* @author Alan Griffiths
	**/
	template<class p_type>
    inline p_type* deep_copy(const p_type *p, const Cloneable *)
	{
		return p ? p->makeClone() : 0;
	}
		

	/**
    *   Makes a "deep copy" of the supplied pointer (it copies the
    *   referenced object - the object is responsible for copying its parts).
    *
    *   By default the object is copied using the default constructor, but 
    *   other copy methods are possible.  In particular if the referenced 
    *   object is of a type derived from arg::cloneable then the copy is 
    *   obtained by calling its clone() method.
    *   
	*	The copying mechanism is selected at compile time by forwarding to 
    *   the overloaded two parameter version of this function.  (This 
    *   mechanism was suggested by Kevlin Henney.)
	*
	* @author Alan Griffiths
	**/
	template<class p_type>
	inline p_type* deep_copy(const p_type* p) 
	{
		return deep_copy(p, p);
	}
}

#endif
