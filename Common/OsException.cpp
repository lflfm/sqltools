/*
    Copyright (C) 2005 Aleksey Kochetov

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
#include "OsException.h"
#include "MyUtf.h"

namespace Common
{

OsException::OsException (int err, const wchar_t* msg) 
    : m_err(err), std::exception(str(msg).c_str()) 
{
}

void OsException::CheckLastError () /* throw OsException */
{
    ASSERT_EXCEPTION_FRAME;

    std::wstring desc;

    if (int err = GetLastError(desc))
    {
        TRACE("Last error: [%d] %s", err, desc.c_str());
        _RAISE(OsException(err, desc.c_str()));
    }
}

int OsException::GetLastError (std::wstring& desc)
{
    if (int err = ::GetLastError())
        return GetError(err, desc);

    return 0;
}

int OsException::GetError (int err, std::wstring& desc)
{
    if (err)
    {
        LPCTSTR sysmsg = 0;
        
        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR)&sysmsg,
            0,
            NULL 
        );
        
        desc = sysmsg;

        LocalFree((LPVOID)sysmsg);
    }
    return err;
}

}; // Common
