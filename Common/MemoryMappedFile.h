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

#pragma once
#ifndef __MemoryMappedFile_h__
#define __MemoryMappedFile_h__

#include "ExceptionHelper.h"


namespace Common
{

    enum EMemoryMappedOptions {
        emmoNone          =    0,
        emmoRead          = 0x01,
        emmoWrite         = 0x02,
        emmoShareRead     = 0x04,
        emmoShareWrite    = 0x08,
        emmoSequential    = 0x10,
        emmoCreateAlways  = 0x20
    };

class MemoryMappedFile
{
public:
    MemoryMappedFile ();
    ~MemoryMappedFile ();

    void Open (const wchar_t* filename, unsigned options, unsigned long length = 0);
    void Close ();

    bool IsOpen () const                    { return m_hFile == INVALID_HANDLE_VALUE ? false : true; }
    void* GetData () const                  { return m_data; }
    unsigned long GetDataLength () const    { return m_length; }

    static void Swap (MemoryMappedFile&, MemoryMappedFile&);
private:
    // not supported operations
    MemoryMappedFile (const MemoryMappedFile&);
    void operator = (const MemoryMappedFile&);

    HANDLE m_hFile, m_hMMFile;
    LPVOID m_data;
    unsigned long m_length;
};

}; // Common

#endif//__MemoryMappedFile_h__
