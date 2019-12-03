/** \file arg_wrapped.h
*   
*   A macro for implementing the "wrapped value" idiom.
* 
*   Modification History
*<PRE>
*   04-Nov-99 : Revised copyright notice						Alan Griffiths
*   16-Jun-99 : Original version                                Alan Griffiths
*</PRE>
**/
/*-----------------------------------------------------------------------------
*  arglib: A collection of utilities. (E.g. smart pointers).
*  Copyright (C) 1999, 2000 Alan Griffiths.
* 
*   This code is part of the 'arglib' library. The latest version of
*   this library is available from:
* 
*       http:* www.octopull.demon.co.uk/arglib/
* 
*   This code may be used for any purpose provided that:
* 
*     1. This complete notice is retained unchanged in any version
*        of this code.
* 
*     2. If this code is incorporated into any work that displays
*        copyright notices then the copyright for the 'arglib'
*        library must be included.
* 
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* 
*   You may contact the author at: alan@octopull.demon.co.uk
* -----------------------------------------------------------------------------	
*/

#ifndef ARG_WRAPPED_H
#define ARG_WRAPPED_H

#ifndef ARG_COMPILER_H
#include "arg_compiler.h"
#endif

/**
*   A macro for implementing the "wrapped value" idiom.
*   This provides additional type safety e.g.
*<pre><code>
*   ARG_WRAPPED_VALUE(int, hour);
*   ARG_WRAPPED_VALUE(int, minute);
*   class time { public : time(hour hh, minute mm); ... };
*   . . .
*   time t(hour(10), minute(30));
*</code></pre>
*   Making the code easier to follow and an accidental transposition of 
*   the parameters a compilation error.
**/
#define ARG_WRAPPED_VALUE(type, name)\
class name {\
public:\
    explicit name(type value) : v(value) {}\
    type get_value() const { return v; }\
private:\
    type v;\
}

#endif
