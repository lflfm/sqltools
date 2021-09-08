/*
    Copyright (C) 2004,2020 Aleksey Kochetov

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

    HANDLE hProcess =::GetCurrentProcess();
    HANDLE hThread = ::GetCurrentThread();

    STACKFRAME stackFrame;
    ZeroMemory(&stackFrame, sizeof(stackFrame));
    stackFrame.AddrPC.Mode      = AddrModeFlat;
    stackFrame.AddrStack.Mode   = AddrModeFlat;
    stackFrame.AddrFrame.Mode   = AddrModeFlat;

    if (exInfo)
    {
        stackFrame.AddrPC.Offset    = exInfo->ContextRecord->Eip;
        stackFrame.AddrStack.Offset = exInfo->ContextRecord->Esp;
        stackFrame.AddrFrame.Offset = exInfo->ContextRecord->Ebp;
    }
    else
    {
        CONTEXT context;
        ZeroMemory(&context, sizeof(context));
        context.ContextFlags = CONTEXT_FULL;

        __asm
        {
        Label:
          mov [context.Ebp], ebp;
          mov [context.Esp], esp;
          mov eax, [Label];
          mov [context.Eip], eax;
        }

        stackFrame.AddrPC.Offset    = context.Eip;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrFrame.Offset = context.Ebp;
    }

    if (::SymInitialize(hProcess, NULL, TRUE))
    {
        while (::StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread,
                            &stackFrame, NULL, NULL,
                            SymFunctionTableAccess, SymGetModuleBase, NULL)
            && stackFrame.AddrFrame.Offset)
        {
            out << stackFrame.AddrPC.Offset << '\t';

            IMAGEHLP_MODULE moduleInfo;
            memset(&moduleInfo, 0, sizeof(moduleInfo) );
            moduleInfo.SizeOfStruct = sizeof(moduleInfo);

            DWORD dwModBase = SymGetModuleBase(hProcess, stackFrame.AddrPC.Offset);
            if (dwModBase && SymGetModuleInfo(hProcess, stackFrame.AddrPC.Offset, &moduleInfo))
                out << '\t' << moduleInfo.ModuleName;

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
