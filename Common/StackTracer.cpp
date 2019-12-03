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

#include "StdAfx.h"
#include <sstream>
#include <iomanip>
#include <process.h>
#include <imagehlp.h>
#include "StackTracer.h"

#define MAX_NAME_LENGTH 1024

namespace Common {

    using namespace std;

    static const char* failure = "StackTracer::trace failed.";

void StackTracer::Trace (string& result, EXCEPTION_POINTERS* exInfo)
{
    std::ostringstream out;
    out.setf(ios_base::showbase);
    out.unsetf(ios_base::dec);
    out.setf(ios_base::hex);

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx((OSVERSIONINFO*)&osvi))
    {
        result = failure;
        return;
    }

    HANDLE hProcess =
        (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) \
            ? (HANDLE) ::GetCurrentProcessId() : ::GetCurrentProcess();

    HANDLE hThread = ::GetCurrentThread();

    CONTEXT context;
    CONTEXT* pcontext;

    STACKFRAME stackFrame;
	ZeroMemory(&stackFrame, sizeof(stackFrame));
	stackFrame.AddrPC.Mode      = AddrModeFlat;
	stackFrame.AddrStack.Mode   = AddrModeFlat;
	stackFrame.AddrFrame.Mode   = AddrModeFlat;

    if (exInfo)
    {
        pcontext = exInfo->ContextRecord;
	    stackFrame.AddrPC.Offset    = exInfo->ContextRecord->Eip;
	    stackFrame.AddrStack.Offset = exInfo->ContextRecord->Esp;
	    stackFrame.AddrFrame.Offset = exInfo->ContextRecord->Ebp;
    }
    else
    {
        pcontext = &context;
	    ZeroMemory(&context, sizeof(context));
        context.ContextFlags = CONTEXT_CONTROL;

        if (::GetThreadContext(hThread, &context))
        {
	        stackFrame.AddrPC.Offset    = context.Eip;
	        stackFrame.AddrStack.Offset = context.Esp;
	        stackFrame.AddrFrame.Offset = context.Ebp;
        }
        else
        {
            result = failure;
            return;
        }
    }

    if (::SymInitialize(hProcess, NULL, TRUE))
    {
        while (::StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread,
			                &stackFrame, &context, NULL,
                            SymFunctionTableAccess, SymGetModuleBase, NULL)
            && stackFrame.AddrFrame.Offset)
        {
            out << stackFrame.AddrPC.Offset << '\t';

	        IMAGEHLP_MODULE Mod;
            DWORD dwModBase = SymGetModuleBase(hProcess, stackFrame.AddrPC.Offset);
		    if (dwModBase && SymGetModuleInfo(hProcess, stackFrame.AddrPC.Offset, &Mod))
                out << '\t' << Mod.ModuleName;

            char buff[sizeof(IMAGEHLP_SYMBOL) + MAX_NAME_LENGTH];
	        ZeroMemory(&buff, sizeof(buff));
	        PIMAGEHLP_SYMBOL pSym = (PIMAGEHLP_SYMBOL)&buff;
		    pSym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL) + MAX_NAME_LENGTH;
		    pSym->MaxNameLength = MAX_NAME_LENGTH;

            DWORD displacement;
            if (::SymGetSymFromAddr(hProcess, stackFrame.AddrPC.Offset, &displacement, pSym))
                out << '\t' << pSym->Name << "() + " << displacement;
            else if (dwModBase)
                out << '\t' << stackFrame.AddrPC.Offset - dwModBase;

            out << endl;
	    }
        result = out.str();

        ::SymCleanup(hProcess);
    }
    else
        result = failure;
}

}//namespace Common
