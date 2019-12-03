/*
    Copyright (C) 2002 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// 25.12.2004 (AK) Added AppException - an untraceable exception
// 17.01.2005 (ak) added THROW_APP_EXCEPTION macro

#pragma once
#ifndef __ExceptionHelper_h__
#define __ExceptionHelper_h__

#include <string>
#include <exception>
#include <COMMON/SEException.h>


namespace Common
{
    // AppException is an untraceable exception,
    // it means that
    //      we don't call TraceStackOnThrow
    //      we don't show a bug report dialog in catch block
    // We should use it to break execution of a procedure and an show error message
    class AppException : public std::exception
    {
    public:
        explicit AppException (const char* what)         : std::exception(what)          {}
        explicit AppException (const std::string& what)  : std::exception(what.c_str())  {}
    };

    typedef void (*AppInfoFn)(std::string& programName, std::string& infoText, std::string& reportPrefix);
    void SetAppInfoFn (AppInfoFn);

    void DefaultHandler (const char* file, int line); // for ...
    void DefaultHandler (const std::exception&, const char* file, int line);
#ifdef _AFX
    void DefaultHandler (CException*, const char* file, int line);
#endif//_AFX

    void TraceStackOnThrow ();

    class ExceptionFrame {
    public:
        ExceptionFrame ();
        ~ExceptionFrame ();
    };

    void CheckExceptionFrame (const char* file, int line);

     __declspec(noreturn) void __cdecl terminate(void);

     class StackOverflowGuard {
        int& m_counter;
     public:
        StackOverflowGuard (int& counter, int limit, const char* file, int line) 
        : m_counter(counter) { 
             if (++m_counter > limit) 
                 Throw(file, line);
        }
        ~StackOverflowGuard () { m_counter--; }

        void Throw (const char* file, int line);
     };
}
#define STACK_OVERFLOW_GUARD(limit) \
    static int __LINE__##counter; \
    Common::StackOverflowGuard __LINE__##StackOverflowGuard(__LINE__##counter, limit, __FILE__, __LINE__);

#define THROW_APP_EXCEPTION(msg)    do { ASSERT_EXCEPTION_FRAME; throw Common::AppException((msg)); } while(false)
#define THROW_STD_EXCEPTION(msg)    do { ASSERT_EXCEPTION_FRAME; throw std::exception((msg)); } while(false)
#define _CHECK_AND_THROW_(x, msg)   { ASSERT_EXCEPTION_FRAME; if (!(x)) { _ASSERTE(0); throw std::exception((msg)); } }

#define DEFAULT_HANDLER(x)          Common::DefaultHandler(x, __FILE__, __LINE__);
#define DEFAULT_HANDLER_ALL         Common::DefaultHandler(__FILE__, __LINE__);

#ifdef _AFX
#define _COMMON_DEFAULT_HANDLER_ \
    catch (CException* x)           { DEFAULT_HANDLER(x); } \
    catch (const std::exception& x) { DEFAULT_HANDLER(x); } \
    catch (...)                     { DEFAULT_HANDLER_ALL; }
#else
#define _COMMON_DEFAULT_HANDLER_ \
    catch (const std::exception& x) { DEFAULT_HANDLER(x); } \
    catch (...)                     { DEFAULT_HANDLER_ALL; }
#endif//_AFX

#define _DESTRUCTOR_HANDLER_ catch (...) {}

#if !defined(NDEBUG) && !defined(_CONSOLE)
#define EXCEPTION_FRAME Common::ExceptionFrame frame##__LINE__
#define ASSERT_EXCEPTION_FRAME Common::CheckExceptionFrame(__FILE__, __LINE__)
#else
#define EXCEPTION_FRAME
#define ASSERT_EXCEPTION_FRAME
#endif

#endif//__ExceptionHelper_h__
