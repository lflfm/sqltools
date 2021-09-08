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

// 14.08.2002 bug fix, empty string cannot be saved in in combo box history
// 24.09.2007 some improvements taken from sqltools++ by Randolf Geist
// 2016.06.28 cleaned up AppCopyTextToClipboard / AppSetClipboardText

#include "stdafx.h"
#include <lmerr.h>
#include <sstream>
#include <afxole.h>
#include "AppUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

namespace Common
{

void AppRestoreHistory (CComboBox& wndList, LPCTSTR szSection, LPCTSTR szEntry, int nSize)
{
    BOOL useRegistry = AfxGetApp()->m_pszRegistryKey ?  TRUE : FALSE;

    wndList.ResetContent();

    for (int i(0); i < nSize; i++)
    {
        CString strEntry, strBuffer;
        strEntry.Format(useRegistry ? _T("%s_%d") : _T("%s_v2_%d"), (LPCTSTR)szEntry, i);
        
        if (useRegistry)
            strBuffer = AfxGetApp()->GetProfileString(szSection, strEntry);
        else
        {
            LPBYTE data; 
            UINT   bytes;
            if (AfxGetApp()->GetProfileBinary(szSection, strEntry, &data, &bytes))
                strBuffer = wstr(string((const char*)data, bytes)).c_str();
        }

        if (!i)
            wndList.SetWindowText(strBuffer);

        if (!strBuffer.IsEmpty())
            wndList.AddString(strBuffer);
        else 
            if (i) break; // only the first string can be null
    }
}

void AppSaveHistory (CComboBox& wndList, LPCTSTR szSection, LPCTSTR szEntry, int nSize)
{
    BOOL useRegistry = AfxGetApp()->m_pszRegistryKey ?  TRUE : FALSE;

    int i(0);
    CString strText, strEntry;
    wndList.GetWindowText(strText);

    strEntry.Format(useRegistry ? L"%s_%d" : L"%s_v2_%d", szEntry, i);
    if (useRegistry)
        AfxGetApp()->WriteProfileString(szSection, strEntry, strText);
    else
    {
        string utf8buffer = str(strText);
        AfxGetApp()->WriteProfileBinary(szSection, strEntry, (LPBYTE)utf8buffer.data(), utf8buffer.length());
    }
    i++;

    int nCount = wndList.GetCount();
    for (int j(0); i < nSize && j < nCount; i++, j++)
    {
        CString strBuffer;

        while (j < nCount)
        {
            wndList.GetLBText(j, strBuffer);

            if (strBuffer == strText) j++;
            else break;
        }

        if (strBuffer.IsEmpty()
        || strBuffer == strText) break;

        strEntry.Format(useRegistry ? L"%s_%d" : L"%s_v2_%d", szEntry, i);
        if (useRegistry)
            AfxGetApp()->WriteProfileString(szSection, strEntry, strBuffer);
        else
        {
            string utf8buffer = str(strBuffer);
            AfxGetApp()->WriteProfileBinary(szSection, strEntry, (LPBYTE)utf8buffer.data(), utf8buffer.length());
        }
    }
}

void AppWalkDir (LPCTSTR szPath, LPCTSTR szFileMask,
                 CStringList& fileList, BOOL bSortFiles, CStringList* folderList)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfdFile;
    memset(&wfdFile, 0, sizeof wfdFile);

    wstring buffer;
    wstring path(szPath);
    if (path.length() > 0 && path[path.length()-1] != '\\')
        path += L"\\";

    list<wstring> local;

    buffer = path;
    buffer += szFileMask;
    hFile = FindFirstFile(buffer.c_str(), &wfdFile);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(wfdFile.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
                local.push_back(wfdFile.cFileName);
        }
        while(FindNextFile(hFile, &wfdFile));

        if(hFile != INVALID_HANDLE_VALUE)
            FindClose(hFile);
    }

    if (bSortFiles)
        local.sort();

    for(auto it = local.begin(); it != local.end(); ++it)
    {
        buffer = path;
        buffer.append(*it);
        fileList.AddTail(buffer.c_str());
    }

    memset(&wfdFile, 0, sizeof wfdFile);
    buffer = path;
    buffer += L"*.*";

    local.clear();
    hFile = FindFirstFile(buffer.c_str(), &wfdFile);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(wfdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
            && wcscmp(L".", wfdFile.cFileName)
            && wcscmp(L"..", wfdFile.cFileName))
                local.push_back(wfdFile.cFileName);
        }
        while(FindNextFile(hFile, &wfdFile));

        if(hFile != INVALID_HANDLE_VALUE)
        FindClose(hFile);
    }

    if (bSortFiles)
        local.sort();

    for(auto it = local.begin(); it != local.end(); ++it)
    {
        buffer = path;
        buffer.append(*it);

        AppWalkDir(buffer.c_str(), szFileMask, fileList, bSortFiles, folderList);

        if (folderList)
            folderList->AddTail(buffer.c_str());
    }

    local.clear();
}

BOOL AppDeleteFiles (LPCTSTR szPath, LPCTSTR szFileMask, BOOL bAndSubdir)
{
    BOOL bRetVal = TRUE;
    CStringList listFiles, listSubdir;

    AppWalkDir(szPath, szFileMask, listFiles, FALSE, &listSubdir);

    for (POSITION pos = listFiles.GetHeadPosition(); pos != NULL;)
    {
        CString file = listFiles.GetNext(pos);
        if (DeleteFile(file) == FALSE)
            bRetVal = FALSE;
    }

    if (bAndSubdir)
        for (POSITION pos = listSubdir.GetHeadPosition(); pos != NULL;)
        {
            CString folder = listFiles.GetNext(pos);
            if (RemoveDirectory(folder) == FALSE)
                bRetVal = FALSE;
        }

    return bRetVal;
}


BOOL AppDeleteDirectory (LPCTSTR szPath)
{
    ASSERT(0);
    AppDeleteFiles(szPath, L"*.*", TRUE);
    return RemoveDirectory(szPath);
}

bool AppGetFileAttrs (
        LPCTSTR szPath, DWORD* attrs, __int64* fileSize,
        __int64* creationTime, __int64* lastWriteTime
    )
{
    BY_HANDLE_FILE_INFORMATION info;

    if (AppGetFileAttrs (szPath, &info))
    {
        if (attrs)
            *attrs = info.dwFileAttributes;
        if (fileSize)
            *fileSize = (__int64(info.nFileSizeHigh) << 32) 
                + info.nFileSizeLow;
        if (creationTime)
            *creationTime = (__int64(info.ftCreationTime.dwHighDateTime) << 32) 
                + info.ftCreationTime.dwLowDateTime;
        if (lastWriteTime)
            *lastWriteTime = (__int64(info.ftLastWriteTime.dwHighDateTime) << 32) 
                + info.ftLastWriteTime.dwLowDateTime;
        return true;
    }

    return false;
}

bool AppGetFileAttrs (LPCTSTR szPath, BY_HANDLE_FILE_INFORMATION* pInfo)
{
    HANDLE hFile = ::CreateFile(
            szPath, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, 
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
        );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        // it can be replaced with GetFileAttributesEx, but it'll not be compatible with win95
        if (GetFileInformationByHandle(hFile, pInfo))
        {
            ::CloseHandle(hFile);
            return true;
        }
        ::CloseHandle(hFile);
    }

    TRACE("Common::AppGetFileAttrs failed for %s", szPath);
    return false;
}

void AppGetPath (CString& path)
{
    TCHAR szBuff[_MAX_PATH + 1];
    VERIFY(::GetModuleFileName(AfxGetApp()->m_hInstance, szBuff, _MAX_PATH));
    szBuff[_MAX_PATH] = 0;
    VERIFY(::PathRemoveFileSpec(szBuff));
    path = szBuff;
}

// TODO: test
void AppGetFullPathName (LPCTSTR path, CString& fullPath)
{
    int length = GetFullPathName(path, 0, 0, 0);
    TCHAR* buffer = fullPath.GetBuffer(length + 1);
    GetFullPathName(path, length + 1, buffer, 0);
    fullPath.ReleaseBuffer();
}

bool AppShellOpenFile (LPCTSTR file)
{
    HINSTANCE result = ShellExecute(NULL, L"open", file, NULL, NULL, SW_SHOW);

#pragma warning(push)
#pragma warning(disable : 4311)
    if ((DWORD)result <= HINSTANCE_ERROR)
#pragma warning(pop)
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((CString(L"Cannot open file \"") + file + L"\" with default viewer."), MB_OK|MB_ICONSTOP);
        return false;
    }

    return true;
}

bool AppIsFolder (LPCTSTR path, bool _nothrow)
{
    ASSERT_EXCEPTION_FRAME;

    DWORD dwFileAttributes = GetFileAttributes(path);

    if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

    if (!_nothrow)
        THROW_APP_EXCEPTION("Cannot read folder or file \"" + Common::str(path) + "\".");

    return false;
}

void AppCreateFolderHierarchy (LPCTSTR path)
{
    SHCreateDirectoryEx( NULL, path, NULL );
}

/////////////////////////////////////////////////////////////////////////////
//  the following code is based on CWinException
//	Copyright(c) Armen Hakobyan 2001
void AppGetLastError (CString& lastError)
{
    HMODULE 	hModule 	= NULL; 
    LPTSTR		lpszMessage	= NULL;
    DWORD		cbBuffer	= NULL;
    DWORD       dwErrCode   = ::GetLastError();

    lastError.Empty();

    if ((dwErrCode >= NERR_BASE) && (dwErrCode <= MAX_NERR /*NERR_BASE+899*/))
    {
        hModule = ::LoadLibraryEx( _T("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
    }
    
    cbBuffer = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                               FORMAT_MESSAGE_IGNORE_INSERTS | 
                               FORMAT_MESSAGE_FROM_SYSTEM | // always consider system table
                               ((hModule != NULL) ? FORMAT_MESSAGE_FROM_HMODULE : 0), 
                               hModule, // module to get message from (NULL == system)
                               dwErrCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
                               (LPTSTR) &lpszMessage, 0, NULL );
    
    //ERROR_RESOURCE_LANG_NOT_FOUND

    if (cbBuffer)
        lastError = (LPCTSTR)lpszMessage;

    ::LocalFree(lpszMessage);
    if (hModule) ::FreeLibrary(hModule);

    if (lastError.IsEmpty())
        lastError.Format(L"Unknown system error - %d", dwErrCode);
}

void AppCopyTextToClipboard (const CString& theText)
{
    if (::OpenClipboard(NULL))
    {
        CWaitCursor wait;
        ::EmptyClipboard();

        if (theText.GetLength())
        {
            size_t size = sizeof(TCHAR) * (theText.GetLength() + 1);
            HGLOBAL hDataDest = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size);
            
            if (hDataDest)
            {
                void* dest = ::GlobalLock(hDataDest);

                if (dest)
                    memcpy(dest, (LPCTSTR)theText, size);

                ::GlobalUnlock(hDataDest);
                ::SetClipboardData(CF_UNICODETEXT, hDataDest);
            }
        }
        ::CloseClipboard();
    }
}

bool AppLoadTextFromResources (LPCTSTR name, LPCTSTR type, std::string& text)
{
    text.clear();

    if (HRSRC hrsrc = ::FindResource(AfxGetResourceHandle(), name, type))
        if (HGLOBAL handle = ::LoadResource(AfxGetResourceHandle(), hrsrc))
        {
            LPCSTR ptr = (LPCSTR)::LockResource(handle);
            int length = (int)::SizeofResource(AfxGetResourceHandle(), hrsrc);
            text.assign(ptr, length);
            return true;
        }

    return false;
}

bool AppSetClipboardText (const char* text, int length, UINT format /*= CF_TEXT*/)
{
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, length + 1);
    
    if (hData)
    {
        if (void* mem = GlobalLock(hData)) 
        {
            memcpy(mem, text, length + 1);
            ::GlobalUnlock(hData);
        }
    }
    return ::SetClipboardData(format, hData) ? true : false;
}

bool AppSetClipboardText (const wchar_t* text, int length, UINT format /*= CF_UNICODETEXT*/)
{
    int size = sizeof(wchar_t) * (length + 1);
    HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size);
    if (hData)
    {
        if (void* dest = ::GlobalLock(hData))
        {
            memcpy(dest, text, size);
            ::GlobalUnlock(hData);
        }
    }
    return ::SetClipboardData(format, hData);
}

bool AppGetClipboardText (std::string& buffer, UINT format /*= CF_TEXT*/)
{
    if (HGLOBAL hData = ::GetClipboardData(format))
        if (char* text = (char*)GlobalLock(hData)) 
        {
            buffer = text;
            GlobalUnlock(hData);
            return true;
        }
    return false;
}

bool AppGetClipboardText (CString& buffer, UINT format /*= CF_UNICODETEXT*/)
{
    if (HGLOBAL hData = ::GetClipboardData(format))
        if (TCHAR* text = (TCHAR*)GlobalLock(hData)) 
        {
            buffer = text;
            GlobalUnlock(hData);
            return true;
        }
    return false;
}

bool AppGetDragAndDroData (std::string& buffer, COleDataObject* pDataObject, CLIPFORMAT format /*= CF_TEXT*/)
{
    if (HGLOBAL hData = pDataObject->GetGlobalData(format))
        if (char* text = (char*)GlobalLock(hData)) 
        {
            buffer = text;
            GlobalUnlock(hData);
            return true;
        }
    return false;
}

bool AppGetDragAndDroData (std::wstring& buffer, COleDataObject* pDataObject, CLIPFORMAT format /*= CF_UNICODETEXT*/)
{
    if (HGLOBAL hData = pDataObject->GetGlobalData(format))
        if (wchar_t* text = (wchar_t*)GlobalLock(hData)) 
        {
            buffer = text;
            GlobalUnlock(hData);
            return true;
        }
    return false;
}

bool AppGetDragAndDroData (CString& buffer, COleDataObject* pDataObject, CLIPFORMAT format /*= CF_UNICODETEXT*/)
{
    if (HGLOBAL hData = pDataObject->GetGlobalData(format))
        if (TCHAR* text = (TCHAR*)GlobalLock(hData)) 
        {
            buffer = text;
            GlobalUnlock(hData);
            return true;
        }
    return false;
}

}//namespace Common
