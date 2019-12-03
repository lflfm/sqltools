/** \file arg_iterator.h
*   
*   forward_iterator<> - Proxy for an arbitary forward iterator
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   30-Mar-00 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_GRIN_PTR_H
#define ARG_GRIN_PTR_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#ifndef ARG_DEEP_COPY_H
#include "arg_deep_copy.h"
#endif

#include ARG_STLHDR(iterator)



/**
*   arglib: A collection of utilities. (E.g. smart pointers).
*   Copyright (C) 1999, 2000 Alan Griffiths.
*
*   This code is part of the 'arglib' library. The latest version of
*   this library is available from:
*
*      <A href=http://www.octopull.demon.co.uk/arglib/>
*		http://www.octopull.demon.co.uk/arglib/
*      </a>
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
    *   Implementation classes supporting the arg::forward_iterator and 
    *   arg::bidirectional_iterator templates.
    **/
	namespace iterator_body
	{
        ///   Implementation class supporting the arg::forward_iterator
		template<typename value_type>
		struct forward : cloneable
		{
			virtual ~forward() {}
			virtual forward* clone() const = 0;
			virtual value_type& operator*() const = 0;
			virtual void increment() = 0;
			virtual bool equals(const forward& rhs) const = 0;
		};
		

        ///   Implementation class supporting the arg::bidirectional_iterator
		template<typename value_type>
		struct bidirectional
#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
			 : forward<value_type>
#else             
		     : cloneable
#endif
		{
			virtual ~bidirectional() {}
			virtual bidirectional* clone() const = 0;
			virtual value_type& operator*() const = 0;
			virtual void increment() = 0;
			virtual void decrement() = 0;
			virtual bool equals(const bidirectional& rhs) const = 0;
#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
			virtual bool equals(const forward<value_type>& rhs) const
				{ return equals(static_cast<const bidirectional&>(rhs)); }
#endif                
		};
	}

	template<typename value_type> class bidirectional_iterator;

	/**
    *	Proxy template class for wrapping forward iterators. These allow
    *	a class interface to be defined without either:
    *<UL> 
    *   <LI>exposing the choice of container used in the implementation,</LI>
    *   <LI>imposing a choice of container on the client code, or</LI>
    *   <LI>templating member functions on the type of iterators.</LI>
	*</UL>
    *
    *   <EM>Note that MSVC5 has some serious problems compiling and using this
    *   class (see number of _MSC_VER == 1100 frigs in the test harness).</EM>
	*
	*	All methods support the strong exception safety guarantee with the
    *   proviso that the wrapped iterator must have a non-throwing destructor.
	*
	*	@author alan@octopull.demon.co.uk
	**/
	template<typename value_type>
	class forward_iterator : public std::forward_iterator_tag
	{
	public:
		/**
		*	Construct a forward_iterator proxy onto a container of value_type
		*	from any iterator (onto a container of value_type) that meets the 
		*	forward iterator requirements.
		**/
		template<typename iterator>
		forward_iterator(const iterator& it)
			: body(new implementation<iterator>(it))
			{
			}

#ifdef _MSC_VER
        // MSVC won't use the above template constructor with pointer types
		forward_iterator(value_type* it)
			: body(new implementation<value_type*>(it))	{}
#endif

		/*
		*	Used to constructs a forward interator from a 
		*	bidirectional_iterator.
        *
        *   NB This conversion is not supportable on compilers (MSVC++) that
        *   disallow covarient return types.  This simply leads to less 
        *   efficient code - as the bidirectional_iterator gets wrapped up 
        *   inside the forward_iterator.
		*/
#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
        forward_iterator(const bidirectional_iterator<value_type>& rhs)
		: body(rhs.body->clone()) {}
#endif

		///
		value_type& operator*() const       { return **body; }

		///
		value_type* operator->() const      { return &**body; }

		///
		forward_iterator& operator++()      { body->increment(); return *this; }

		///
		forward_iterator operator++(int)
			{ forward_iterator r(*this); return ++r; }

		/// Used to implement equivalance operators
		bool equals(const forward_iterator& rhs) const
			{ return body->equals(*rhs.body); }

	private:

		typedef iterator_body::forward<value_type> body_type;

		body_part_ptr<body_type> body;

		template<typename iterator>
		struct implementation : body_type
		{
			implementation(iterator it) : i(it) 	{}
			virtual ~implementation() 				{}
            
#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
			virtual implementation* clone() const
#else
			virtual body_type* clone() const
#endif
				{ return new implementation(*this); }
			
			virtual value_type& operator*() const	{ return *i; }
			virtual void increment()				{ ++i;  }
			virtual bool equals(const body_type& rhs) const
				{ return i == static_cast<const implementation&>(rhs).i; }
			
			iterator i;
		};
	};

	///
	template<typename value_type>
	inline bool operator==(
		const forward_iterator<value_type>& lhs,
		const forward_iterator<value_type>& rhs)
	{
		return lhs.equals(rhs);
	}

	///
	template<typename value_type>
	inline bool operator!=(
		const forward_iterator<value_type>& lhs,
		const forward_iterator<value_type>& rhs)
	{
		return !lhs.equals(rhs);
	}


	/**
    *	Proxy template class for wrapping bidirectional iterators. These allow 
    *	a class interface to be defined without either:
    *<UL> 
    *   <LI>exposing the choice of container used in the implementation,</LI>
    *   <LI>imposing a choice of container on the client code, or</LI>
    *   <LI>templating member functions on the type of iterators.</LI>
	*</UL>
    *
    *   <EM>Note that MSVC5 has some serious problems compiling and using this
    *   class (see number of _MSC_VER == 1100 frigs in the test harness).</EM>
	*
	*	All methods support the strong exception safety guarantee with the
    *   proviso that the wrapped iterator must have a non-throwing destructor.
	*
	*	@author alan@octopull.demon.co.uk
	**/
	template<typename value_type>
	class bidirectional_iterator : public std::bidirectional_iterator_tag
	{
	public:
		/**
		*	Construct a bidirectional_iterator proxy onto a container of 
		*	value_type from any iterator (onto a container of value_type) 
		*	that meets the forward iterator requirements.
		**/
		template<typename iterator>
		bidirectional_iterator(const iterator& it)
			: body(new implementation<iterator>(it))	{}

#ifdef _MSC_VER
        // MSVC can't use the above with pointer types
		bidirectional_iterator(value_type* it)
			: body(new implementation<value_type*>(it))	{}
#endif

		///
		value_type& operator*() const       	{ return **body; }

		///
		value_type* operator->() const      	{ return &**body; }

		///
		bidirectional_iterator& operator++()    
			{ body->increment(); return *this; }

		///
		bidirectional_iterator operator++(int)
			{ bidirectional_iterator r(*this); return ++r; }

		///
		bidirectional_iterator& operator--()
			{ body->decrement(); return *this; }

		///
		bidirectional_iterator operator--(int)
			{ bidirectional_iterator r(*this); return --r; }

		/// Used to implement equivalance operators
		bool equals(const bidirectional_iterator& rhs) const
			{ return body->equals(*rhs.body); }


	private:
#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
        friend forward_iterator<value_type>;
#endif
        
		typedef iterator_body::bidirectional<value_type> body_type;

		body_part_ptr<body_type> body;

		template<typename iterator>
		struct implementation : body_type
		{
			implementation(iterator it) : i(it) 	{}
			virtual ~implementation() 				{}

#ifndef ARG_COMPILER_NO_COVARIENT_RETURNS            
			virtual implementation* clone() const
#else
			virtual body_type* clone() const
#endif
				{ return new implementation(*this); }
			
			virtual value_type& operator*() const	{ return *i; }
			virtual void increment()				{ ++i; }
			virtual void decrement()				{ --i; }
			virtual bool equals(const body_type& rhs) const
				{ return i == static_cast<const implementation&>(rhs).i; }
			
			iterator i;
		};
	};

	///
	template<typename value_type>
	inline bool operator==(
		const bidirectional_iterator<value_type>& lhs,
		const bidirectional_iterator<value_type>& rhs)
	{
		return lhs.equals(rhs);
	}

	///
	template<typename value_type>
	inline bool operator!=(
		const bidirectional_iterator<value_type>& lhs,
		const bidirectional_iterator<value_type>& rhs)
	{
		return !lhs.equals(rhs);
	}
}
#endif
