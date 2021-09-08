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

/*
    2015-07-26 bug fix, added FlushViewOfFile & FlushFileBuffers to force Windows to flush data to disk
*/

#include "stdafx.h"
#include "MemoryMappedFile.h"
#include "OsException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Common
{

#define CHECK_FILE_OPERATION(r) if(!(r)) OsException::CheckLastError();


void MemoryMappedFile::Swap (MemoryMappedFile& first, MemoryMappedFile& second)
{
    std::swap(first.m_hFile,   second.m_hFile);
    std::swap(first.m_hMMFile, second.m_hMMFile);
    std::swap(first.m_data,    second.m_data);
    std::swap(first.m_length,  second.m_length);
}

MemoryMappedFile::MemoryMappedFile ()
{
    m_hFile   = INVALID_HANDLE_VALUE;
    m_hMMFile = NULL;
    m_data    = NULL;
    m_length  = 0;
}

MemoryMappedFile::~MemoryMappedFile ()
{
    try { 
        Close(); 
    }         
    _DESTRUCTOR_HANDLER_;
}

void MemoryMappedFile::Open (const wchar_t* filename, unsigned options, unsigned long length)
{
    try 
    {
        _ASSERTE(m_hFile == INVALID_HANDLE_VALUE);

        if (m_hFile != INVALID_HANDLE_VALUE)
            Close();

        m_hFile = CreateFile(
                filename,                           // file name
                GENERIC_READ //(options & emmoRead ? GENERIC_READ : 0) // access mode
                | (options & emmoWrite ? GENERIC_WRITE : 0),
                (options & emmoShareRead ? FILE_SHARE_READ : 0) // share mode
                | (options & emmoShareWrite ? FILE_SHARE_WRITE : 0),
                0,                                  // security
                                                    // how to create
                options & emmoCreateAlways ? CREATE_ALWAYS : OPEN_EXISTING,                      
                                                    // file attributes
                (options & emmoSequential ? FILE_FLAG_SEQUENTIAL_SCAN : 0),
                0                                   // handle to template file
            );
    
        CHECK_FILE_OPERATION(m_hFile != INVALID_HANDLE_VALUE);
    
        if (length)
        {
            m_length = length;
            CHECK_FILE_OPERATION(SetFilePointer(m_hFile, m_length, NULL, FILE_BEGIN) != -1 && SetEndOfFile(m_hFile));
        }
        else
        {
            BY_HANDLE_FILE_INFORMATION fileInformation;
            CHECK_FILE_OPERATION(GetFileInformationByHandle(m_hFile, &fileInformation));
            _CHECK_AND_THROW_(!fileInformation.nFileSizeHigh, "File size is too big");
            m_length = fileInformation.nFileSizeLow;
        }

        if (m_length > 0)
        {
            m_hMMFile = CreateFileMapping(
                    m_hFile,                            // handle to file
                    0,                                  // security
                                                        // protection
                    options & emmoWrite ? PAGE_READWRITE : PAGE_READONLY,                      
                    0,                                  // high-order DWORD of size
                    0,                                  // low-order DWORD of size
                    0                                   // object name
                );

            CHECK_FILE_OPERATION(m_hMMFile != NULL);

            m_data = MapViewOfFile(
                    m_hMMFile,                          // handle to file-mapping object
                                                        // access mode
                    options & emmoWrite ? FILE_MAP_WRITE : FILE_MAP_READ,                      
                    0,                                  // high-order DWORD of offset
                    0,                                  // low-order DWORD of offset
                    0                                   // number of bytes to map
                );
	        
            CHECK_FILE_OPERATION(m_data != NULL);
        }
    }
    catch (...)   
    { 
        try { Close(); } catch (...) {}
        throw;
    }
}

void MemoryMappedFile::Close ()
{
    if (m_data != NULL)
    {
        // 2015-07-26 bug fix, added FlushViewOfFile & FlushFileBuffers to force Windows to flush data to disk
        FlushViewOfFile(m_data, m_length);
        UnmapViewOfFile(m_data);
        m_data = NULL;
    }
    if (m_hMMFile != NULL)
    {
        FlushFileBuffers(m_hMMFile);
        CloseHandle(m_hMMFile);
        m_hMMFile = NULL;
    }
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

};

