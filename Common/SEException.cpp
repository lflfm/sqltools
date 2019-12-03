/*
    Copyright (C) 2004 Aleksey Kochetov

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

#include "stdafx.h"
#include <COMMON/StackTracer.h>
#include <COMMON/SEException.h>


namespace Common
{
    using namespace std;

    static __declspec(thread) char g_lastStack[4096] = { 0 };

inline
SEException::SEException (unsigned int se_code)
: runtime_error(GetSEName(se_code))
{
}

    static _se_translator_function s_org_se_translator_function = NULL;
    static void se_translator_function (unsigned int, EXCEPTION_POINTERS*);

void SEException::InstallSETranslator ()
{
    _se_translator_function f = _set_se_translator(se_translator_function);
    
    if (!s_org_se_translator_function)
        s_org_se_translator_function = f;
}

void SEException::UninstallSETranslator ()
{
    if (s_org_se_translator_function)
    {
        _set_se_translator(s_org_se_translator_function);
        s_org_se_translator_function = NULL;
    }
}

const char* SEException::GetLastStackDump ()
{
    return g_lastStack;
}

string SEException::GetSEName (unsigned int code)
{
    switch(code)
    {
    case EXCEPTION_ACCESS_VIOLATION         : return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_BREAKPOINT               : return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT    : return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_SINGLE_STEP              : return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED    : return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DENORMAL_OPERAND     : return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO       : return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT       : return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION    : return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW             : return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK          : return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW            : return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO       : return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW             : return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION         : return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION : return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    }

    char buff[100];
    _snprintf(buff, sizeof(buff)-1, "Unknown SE exception(0x%x)", code);

    return buff;
}

static
void se_translator_function (unsigned int code, EXCEPTION_POINTERS* p)
{
    string result;
    StackTracer::Trace(result, p);
    size_t length = min(sizeof(g_lastStack)-1, result.length());
    memcpy(g_lastStack, result.data(), length);
    g_lastStack[length] = 0;

    throw SEException(code);
}

};//namespace Common
