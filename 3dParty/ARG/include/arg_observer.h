/** \file arg_observer.h
*   
*   Templated collection of listeners
* 
*   Modification History
*<PRE>
*   06-Apr-00 : Reworked documentation following user feedback  Alan Griffiths
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   01-Oct-99 : Original version                                Alan Griffiths
*</PRE>
**/

#ifndef ARG_OBSERVER_H
#define ARG_OBSERVER_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

#ifndef ARG_VECTOR_H
#include ARG_STLHDR(vector)
#define ARG_VECTOR_H
#endif

#ifndef ARG_ALGORITHM_H
#include ARG_STLHDR(algorithm)
#define ARG_ALGORITHM_H
#endif

#ifndef ARG_FUNCTIONAL_H
#include ARG_STLHDR(functional)
#define ARG_FUNCTIONAL_H
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
    *   Implementation classes supporting arg::subject template.
    *   This probably wants rationalising later...  
    *
    *   The following was written after getting fed up with trying to 
    *   pursuade egcs and MSVC to both approve the same code that used
    *   std::mem_fun with operator()
    *                                           arg {01-Oct-99}
    *
    *   @author alan@octopull.demon.co.uk
    **/
    namespace functional
    {
        /**
        *   Implementation class supporting arg::subject template.
        *
        *   Provides a copy constructable wrapper for a reference to a 
        *   const functor object with a (const) function call operator.
        **/
        template<class functor>
        class const_functor_ref_t
        {
        public:
            const_functor_ref_t(const functor& ff) : f(ff) {}
            
            template<typename parm>
            void operator() (parm p) { f(p); }

        private:
            const functor& f;
        };

        /**
        *   Implementation class supporting arg::subject template.
        *
        *   Provides a copy constructable wrapper for a reference to a 
        *   non-const functor object with a (non-const) function call operator.
        **/
        template<class functor>
        class functor_ref_t
        {
        public:
            functor_ref_t(functor& ff) : f(ff) {}
            
            template<typename parm>
            void operator() (parm p) { f(p); }

        private:
            functor& f;
        };
        
        
        /// Utility to build a "reference to a functor"
        template<class functor>
        const_functor_ref_t<functor> functor_ref(const functor& ff)
        {
            return const_functor_ref_t<functor>(ff);
        }

// MSVC doesn't reliably overload templates on const        
#ifndef _MSC_VER
        /// Utility to build a "reference to a functor"
        template<class functor>
        functor_ref_t<functor> functor_ref(functor& ff)
        {
            return functor_ref_t<functor>(ff);
        }
#endif        
    }

    /**
    *   Support class "subject" for the GOF observer pattern - the 
    *   observer interface is supplied as a template parameter.
    *
    *   The user code is responsible for ensuring that all observer pointers
    *   are valid when <CODE>notify</CODE> is invoked.
    *   
	*	All methods support the strong exception safety guarantee.
	*
    *                                           @author Alan Griffiths
    **/
    template<class observer>
    class subject
    {
    public:
    
        subject() : refs() {}
    
        /**
        *   @param addee    pointer to observer, must remain valid 
        *                   while broadcasts are possible.
        **/
        subject& add_observer(observer* addee)
        {
			refs.push_back(addee);
			return *this;
        }

        /**
        *   @param removee  pointer to observer.
        **/
        subject& remove_observer(observer* removee)
        {
            using namespace std;
			refs.erase(partition(
                refs.begin(), 
                refs.end(), 
                bind2nd(not_equal_to<observer*>(), removee)),
                refs.end());
			return *this;
        }
        
        /**
        *   Interface class to support notifying observers.  The user 
        *   provides a derived class (implementing the function call
        *   operator) to the subject::notify.  This user class is responsible
        *   for passing the notification message to each supplied observer.
        **/
        class notify_strategy
        {
         public:
            /**
            *   @param obs  an observer to be notified
            **/
            virtual void operator()(observer* obs) const = 0;
        };
        
		/**
		*	Enumerate elements of list passing them to the <code>notify</code>
        *   functor.
		**/
		const subject& notify(const notify_strategy& notify) const
		{
			std::for_each(refs.begin(), refs.end(), 
                    functional::functor_ref(notify));
                
			return *this;
		}
        
        
    private:
        typedef std::vector<observer*> observers;
        observers refs;
    };
}
#endif
