//  File. . . . : arg_property.h
//  Description : Property template implementation
//  Rev . . . . :
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
	
#ifndef ARG_PROPERTY_H
#define ARG_PROPERTY_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#if 1
    #ifndef ARG_LISTENER_LIST_H
    #include "arg_listener_list.h"
    #endif	
#else
    #ifndef ARG_NEW_LISTENER_H
    #include "arg_new_listener.h"
    #endif	
#endif

#ifndef ARG_AUTO_PTR_H
#include "arg_auto_ptr.h"
#endif	


namespace arg
{
	/**
	*	Interface for validating property change requests.
	**/
	template<typename value_type>
	class property_validator
	{
	public:
	
		virtual ~property_validator() = 0;
		
		///
		virtual bool is_valid(value_type value) const = 0;
	};
		

	/**
	*	Interface for receiving property change notifications.
	**/
	template<typename value_type>
	class property_listener
	{
	public:
	
		virtual ~property_listener() = 0;
		
		///
		virtual void set_enabled(bool enabled)   = 0;
		
		///
		virtual void set_value(value_type value) = 0;
	};
		

	/**
	*	Default implementation of interface for validating property change 
	*	requests.
	**/
	template<typename value_type>
	class default_property_validator :
		public property_validator<value_type>
	{
	public:
	
		virtual ~default_property_validator();
		
		///
		bool is_valid(value_type value) const;
	};
		

	/**
	*	Default implementaion of interface for receiving property 
	*	change notifications.
	**/
	template<typename value_type>
	class default_property_listener : public property_listener<value_type>
	{
	public:
	
		virtual ~default_property_listener();
		

		/// does nothing
		void set_enabled(bool enabled);

		/// does nothing
		void set_value(value_type value);
	};


	/**
	*	Adapter implementaion of interface for receiving property 
	*	change notifications.
	**/
	template<typename value_type, typename adaptee_type>
	class property_listener_adapter : public property_listener<value_type>
	{
	public:
	
		///
		property_listener_adapter(
			adaptee_type adapt, 
			void (adaptee_type::* set_enabled_function)(bool),
			void (adaptee_type::* set_value_function)(value_type))
	    :
		    adaptee(adapt),
		    set_enabled_fn(set_enabled_function),
		    set_value_fn(set_value_function)
	    {
	    }
	
		virtual ~property_listener_adapter();

		/// Calls set_enabled_function supplied to constructor
		void set_enabled(bool enabled);

		/// Calls set_value_function supplied to constructor
		void set_value(value_type value);

	private:	
	
		adaptee_type* 	adaptee;
		void (adaptee_type::* set_enabled_fn)(bool);
		void (adaptee_type::* set_value_fn)(value_type);
	};


	template<typename value_type>
	class property;
	
	
	/**
	*	A class that takes ownership of a property_listener and 
	*	automates registration & deregistration with the property.
	**/
	template<typename value_type>
	class auto_property_listener_ptr
	{
	public:

		typedef property<value_type>			property_type;
		typedef property_listener<value_type>	listener_type;

		/// @param listener - the listener to own and delegate to
		explicit auto_property_listener_ptr(
			listener_type* listener);
		
	
		virtual ~auto_property_listener_ptr();
		
		/// observe a property
		void set_property(property_type* observed_property);

	private:
		auto_property_listener_ptr(
			const auto_property_listener_ptr&);

		auto_property_listener_ptr& operator=(
			const auto_property_listener_ptr&);

		listener_type* 	the_listener;
		property_type*	the_property;
	};


	/**
	* 	Utility function - make_property_listener_adapter() usage:
	*<P>
	*	make_property_listener_adapter(this, C::&enabled_fn, C::& value_fn);
	**/
	template<typename value_type, typename adaptee_type> inline
	property_listener<value_type>* 
	make_property_listener_adapter(
		adaptee_type adapt, 
		void (adaptee_type::* set_enabled_function)(bool),
		void (adaptee_type::* set_value_function)(value_type))
	{
		return new property_listener_adapter<value_type, adaptee_type>
			(adapt, set_enabled_function, set_value_function);
	}
	


	template<typename value_t>
	class property
	{
	public:
	
		// Utility definitions
		typedef value_t							value_type;
		typedef property_validator<value_type>	validator_type;
		typedef property_listener<value_type>	listener_type;
		
		///
		explicit property(value_type initial_value);

		///
		property(
			value_type 					initial_value, 
			auto_ptr<validator_type> 	initial_validator);
		
		///
		void add_listener(listener_type* new_listener);
		
		///
		void delete_listener(listener_type* old_listener);
		
		///
		void set_value(value_type new_value);
		
		///
		void set_enabled(bool new_enabled);
		
	private:
		value_type 						value;
		bool 							enabled;
		const auto_ptr<validator_type>	validator;
		listener_list<listener_type> 	listeners;
	};
}


// I'd like to move these implementations out-of-line, but few compilers cope.
namespace arg
{
	template<typename value_type>
	property_validator<value_type>::~property_validator()
	{
	}
	
	template<typename value_type>
	property_listener<value_type>::~property_listener()
	{
	}

	template<typename value_type>
	default_property_listener<value_type>::~default_property_listener()
	{
	}

	template<typename value_type>
	void default_property_listener<value_type>::set_enabled(bool)
	{
	}

	template<typename value_type>
	void default_property_listener<value_type>::set_value(value_type)
	{
	}

	template<typename value_type, typename adaptee_type>
	property_listener_adapter<value_type, adaptee_type>::
	~property_listener_adapter()
	{
	}

	template<typename value_type, typename adaptee_type>
	void property_listener_adapter<value_type, adaptee_type>::
	set_enabled(bool enabled)
	{
		(adaptee->*set_enabled_fn)(enabled);
	}

	template<typename value_type, typename adaptee_type>
	void property_listener_adapter<value_type, adaptee_type>::
	set_value(value_type value)
	{
		(adaptee->*set_value_fn)(value);
	}

	template<typename value_type>
	default_property_validator<value_type>::
	~default_property_validator()
	{
	}
		
	template<typename value_type>
	bool
	default_property_validator<value_type>::
	is_valid(value_type) const
	{
		return true;
	}

	template<typename value_type>
	auto_property_listener_ptr<value_type>::auto_property_listener_ptr(
			property_listener<value_type>* listener)
	:
		the_listener(listener),
		the_property(0)
	{
	}
		
	
	template<typename value_type>
	auto_property_listener_ptr<value_type>::~auto_property_listener_ptr()
	{
		if (the_property)
		{
			the_property->delete_listener(the_listener);
		}
		
		delete the_listener;
	}
	
		
	template<typename value_type>
	void auto_property_listener_ptr<value_type>::
	set_property(property<value_type>* observed_property)
	{
		if (the_property)
		{
			the_property->delete_listener(the_listener);
		}
		
		the_property = observed_property;
		
		if (the_property)
		{
			the_property->add_listener(the_listener);
		}
	}


	template<typename value_t>
	property<value_t>::property(value_type initial_value)
	:
	value(initial_value),
	enabled(true),
	validator(new default_property_validator<value_type>),
	listeners()
	{
	}

	template<typename value_t>
	property<value_t>::property(
		value_type 					initial_value, 
		auto_ptr<validator_type>	initial_validator)
	:
	value(initial_value),
	enabled(true),
	validator(initial_validator),
	listeners()
	{
	}
		
	template<typename value_t>
	void property<value_t>::add_listener(listener_type* new_listener)
	{
		listeners.add_listener(new_listener);
	}
		
	template<typename value_t>
	void property<value_t>::delete_listener(listener_type* old_listener)
	{
		listeners.delete_listener(old_listener);
	}
		
	template<typename value_t>
	void property<value_t>::set_value(value_type new_value)
	{
		if (value != new_value && validator->is_valid(new_value))
		{
			value = new_value;
			
			struct setter : listener_list<listener_type>::enumerator
			{
				setter(value_type new_value) : v(new_value) {}

#ifdef ARG_LISTENER_LIST_H
				virtual void operator()(listener_type* l) const
					{ l->set_value(v); }
#else
				virtual void operator()(listener_type& l) const
					{ l.set_value(v); }
#endif				
				const value_type v;
			};
			
			listeners.enumerate_list(setter(value));
		}
	}
		
	template<typename value_t>
	void property<value_t>::set_enabled(bool new_enabled)
	{
		if (enabled != new_enabled)
		{
			enabled = new_enabled;
			
			struct enabler : listener_list<listener_type>::enumerator
			{
				enabler(bool new_enabled) : b(new_enabled) 	{}

#ifdef ARG_LISTENER_LIST_H
				virtual void operator()(listener_type* l) const
					{ l->set_enabled(b); }
#else
				virtual void operator()(listener_type& l) const
					{ l.set_enabled(b); }
#endif				
				
				const bool b;
			};
			
			listeners.enumerate_list(enabler(enabled));
		}
	}
}

#endif
