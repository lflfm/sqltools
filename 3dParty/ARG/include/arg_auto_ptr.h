/** \file arg_auto_ptr.h
*   
*   Auto pointer template implementation
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   18-May-99 : Port to M$VC++ v5                               Alan Griffiths
*   21-Apr-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_AUTO_PTR_H
#define ARG_AUTO_PTR_H

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
	*	auto_ptr<> - I think this is a conforming implementation of the standard.
    *
    *   Written both as an exercise and because the one in VC++ is
    *   badly broken (i.e. a buggy implementation of the wrong spec [CD2].)
    *   Also useful as some distributions of gcc have auto_ptr commented
    *   out.
    *
	*	All methods support the strong exception safety guarantee with the
    *   proviso that the referenced type must have a non-throwing destructor.
	*
    *   <STRONG>Note</STRONG>:
    *
    *   By historical accident the standard library provides only this 
    *   single smart pointer template. auto_ptr<> has what I will 
    *   politely describe as "interesting" copy semantics. Specifically, 
    *   if one auto_ptr<> is assigned (or copy constructed) from another 
    *   then they are both changed - the auto_ptr<> that originally owned 
    *   the object loses ownership and becomes 0. This is a trap for the 
    *   unwary! 
    *
    *   There are situations that call for this behaviour, but on 
    *   most occasions that require a smart pointer the copy semantics cause 
    *   a problem.  (For examples of situation that do not suit auto_ptr see:
    *   <A href=http://www.octopull.demon.co.uk/arglib/uncounted_ptr.html>Uncounted Pointers in C++</A>
    *   and
    *   <A href=http://www.octopull.demon.co.uk/arglib/TheGrin.html>Ending with the grin</A>)
    *
	*	                    @author alan@octopull.demon.co.uk
	**/
    template <class my_element_type> 
    class auto_ptr 
	{ 
      	/**
        *   Implementation "magic".  
        *
        *   A reference replacement class to support returning auto_ptr<>
		*   (This is passed by value to avoid a temporary being bound to 
		*   a non-const reference - which is not allowed.)
        **/
        template <class an_element_type> 
        struct auto_ptr_ref 
		{ 
            ///
            auto_ptr<an_element_type>& ref; 
            auto_ptr_ref(auto_ptr<an_element_type>& rhs) 
            : ref(rhs) 
			{ 
            } 
        }; 
		
      public: 
        ///
        typedef my_element_type element_type; 

        /// constructor 
        explicit auto_ptr(my_element_type* ptr = 0) throw() 
        : p(ptr) 
		{ 
        } 

        /**
        *   conversion constructor - reseats ownership from rhs (which changes).
        *   For this to work there must be an accessible conversion between 
        *   <CODE>rhs_element_type*</CODE> and <CODE>my_element_type*</CODE>.
        **/
		template <class rhs_element_type> 
        auto_ptr(auto_ptr<rhs_element_type>& rhs) throw() 
        : p(rhs.release()) 
		{ 
        } 
  
        /// copy constructor - reseats ownership from rhs (which changes)
        auto_ptr(auto_ptr& rhs) throw() 
        : p(rhs.release()) 
		{ 
        } 
        
		/**
        *    convertion assignment - reseats ownership from rhs (which changes).
        *   For this to work there must be an accessible conversion between 
        *   <CODE>rhs_element_type*</CODE> and <CODE>my_element_type*</CODE>.
        **/
        template <class rhs_element_type> 
        auto_ptr& operator=(auto_ptr<rhs_element_type>& rhs) throw() 
		{ 
            reset(rhs.release()); 
            return *this; 
        } 
  
        /// assignment - reseats ownership from rhs (which changes)
        auto_ptr& operator=(auto_ptr& rhs) throw() 
		{ 
            reset(rhs.release()); 
            return *this; 
        } 
		
        /// destructor - deletes the owned object
        ~auto_ptr() throw() 
		{ 
            delete p; 
        } 

        /// access raw ptr
        my_element_type* get() const throw() 
		{ 
            return p; 
        } 
        
		/// dereference
		my_element_type& operator*() const throw() 
		{ 
            return *p; 
        } 
        
		
		/// gets the pointer
		my_element_type* operator->() const throw() 
		{ 
            return p; 
        } 

        /// release ownership to caller - returns pointer to referenced object
        my_element_type* release() throw() 
		{ 
            my_element_type* temp = p;
            p = 0; 
            return temp; 
        } 

        /// reset pointer - deletes the owned object
        void reset(my_element_type* ptr=0) throw() 
		{ 
            my_element_type* temp = p;
            p = ptr; 
            delete temp; 
        } 

		/**
        *   Implementation "magic".
        *
        *   Create an auto_ptr from a wrapper around a reference to an auto_ptr.
        *   This is part of the mechanism for allowing auto_ptr<>s to
        *   be returned as temporaries.
        **/
		auto_ptr(auto_ptr_ref<my_element_type> rhs) throw() // NB rhs passed by _value_
        : p(rhs.ref.release()) 
		{ 
        }
		
		/**
        *   Implementation "magic".
        *
        *   Create a wrapper around a reference to an auto_ptr.
        *   This is part of the mechanism for allowing auto_ptr<>s to
        *   be returned as temporaries.
        *
        *   (I don't think this mechanism has the intended effect because
        *   auto_ptr_ref<foo> is private within an auto_ptr<bar>, but
        *   I'm just following the spec. - alan)
        **/
        template <class an_element_type> 
        operator auto_ptr_ref<an_element_type>() throw() 
		{ 
            return auto_ptr_ref<an_element_type>(*this); 
        } 
		
		/**
        *   Implementation "magic".
        *
        *   Conversion to an auto_ptr of supertype.
        *   Ownership is passed to the returned temporary.
        **/
        template <class an_element_type> 
        operator auto_ptr<an_element_type>() throw() 
		{ 
            return auto_ptr<an_element_type>(release()); 
        } 

      private: 
	  
        my_element_type* p;        // refers to the actual owned object(if any) 
    }; 
} 

#endif
