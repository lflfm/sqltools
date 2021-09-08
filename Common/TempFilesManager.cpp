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
#include "TempFilesManager.h"
#include <WinException/WinException.h>

using namespace std;

TempFilesManager TempFilesManager::gManager;

TempFilesManager::~TempFilesManager ()
{
    try {
        while (!m_files.empty())
            pop();
    }
    _DESTRUCTOR_HANDLER_;
}

void TempFilesManager::push (const wstring& file)
{
    while (m_files.size() > 1024)
        pop();

    m_files.push_back(file);
}

void TempFilesManager::pop ()
{
    DeleteFile(m_files.front().c_str());
    m_files.pop_front();
}

std::wstring TempFilesManager::CreateFile (const wchar_t* ext)
{
    size_t size = GetTempPath(0, 0);
    wstring file, path(size-1, '_');
    GetTempPath(path.length()+1, const_cast<wchar_t*>(path.data()));

    if (!path.empty() && *path.rbegin() != '\\') path += '\\';

    for (int i = 0; i < 1024; i++)
    {
        wchar_t buff[80]; 
        const int buff_size = sizeof(buff)/sizeof(buff[0]);

        buff[buff_size-1] = 0;
        static int fileIndex = 0;
        _snwprintf(buff, buff_size-1, L"SQLT%04d.", ++fileIndex);
        file = path + buff + ext;

        HANDLE hFile = ::CreateFile(
                file.c_str(), 0, FILE_SHARE_READ,
                NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            gManager.push(file);
            ::CloseHandle(hFile);
            break;
        }
        else
        {
            CWinException x;
            if (x != ERROR_FILE_EXISTS)
            {
                file.clear();
                x.ReportError();
                break;
            }

        }

        file.clear();
    }
    return file;
}
