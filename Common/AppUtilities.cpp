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
#include "COMMON\AppUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

namespace Common
{

void AppRestoreHistory (CComboBox& wndList, const char* szSection, const char* szEntry, int nSize)
{
    BOOL useRegistry = AfxGetApp()->m_pszRegistryKey ?  TRUE : FALSE;

    wndList.ResetContent();

    for (int i(0); i < nSize; i++)
    {
        CString strEntry, strBuffer;
        strEntry.Format(useRegistry ? "%s_%d" : "%s_v2_%d", (const char*)szEntry, i);
        
        if (useRegistry)
            strBuffer = AfxGetApp()->GetProfileString(szSection, strEntry);
        else
        {
            LPBYTE data; 
            UINT   bytes;
            if (AfxGetApp()->GetProfileBinary(szSection, strEntry, &data, &bytes))
                strBuffer.SetString((LPCSTR)data, bytes);
        }

        if (!i)
            wndList.SetWindowText(strBuffer);

        if (!strBuffer.IsEmpty())
            wndList.AddString(strBuffer);
        else 
            if (i) break; // only the first string can be null
    }
}

void AppSaveHistory (CComboBox& wndList, const char* szSection, const char* szEntry, int nSize)
{
    BOOL useRegistry = AfxGetApp()->m_pszRegistryKey ?  TRUE : FALSE;

    int i(0);
    CString strText, strEntry;
    wndList.GetWindowText(strText);

    strEntry.Format(useRegistry ? "%s_%d" : "%s_v2_%d", (const char*)szEntry, i);
    if (useRegistry)
        AfxGetApp()->WriteProfileString(szSection, strEntry, strText);
    else
        AfxGetApp()->WriteProfileBinary(szSection, strEntry, (LPBYTE)(LPCSTR)strText, strText.GetLength());
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

        strEntry.Format(useRegistry ? "%s_%d" : "%s_v2_%d", (const char*)szEntry, i);
        if (useRegistry)
            AfxGetApp()->WriteProfileString(szSection, strEntry, strBuffer);
        else
            AfxGetApp()->WriteProfileBinary(szSection, strEntry, (LPBYTE)(LPCSTR)strBuffer, strBuffer.GetLength());
    }
}

void AppWalkDir (const char* szPath, const char* szFileMask,
                 StringList& listFiles, BOOL bSortFiles, StringList* pListSubdir)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfdFile;
    memset(&wfdFile, 0, sizeof wfdFile);

    String strBuffer;
    String strPath(szPath);
    if (strPath.length() > 0 && strPath[strPath.length()-1] != '\\')
        strPath += "\\";

    StringList listLocal;

    strBuffer = strPath;
    strBuffer += szFileMask;
    hFile = FindFirstFile(strBuffer.c_str(), &wfdFile);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(wfdFile.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
                listLocal.push_back(wfdFile.cFileName);
        }
        while(FindNextFile(hFile, &wfdFile));

        if(hFile != INVALID_HANDLE_VALUE)
            FindClose(hFile);
    }

    if (bSortFiles)
        listLocal.sort();

    StringListIt it;
    for(it = listLocal.begin(); it != listLocal.end(); ++it)
    {
        strBuffer = strPath;
        strBuffer.append(*it);
        listFiles.push_back(strBuffer);
    }

    memset(&wfdFile, 0, sizeof wfdFile);
    strBuffer = strPath;
    strBuffer += "*.*";

    listLocal.clear();
    hFile = FindFirstFile(strBuffer.c_str(), &wfdFile);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(wfdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
            && strcmp(".", wfdFile.cFileName)
            && strcmp("..", wfdFile.cFileName))
                listLocal.push_back(wfdFile.cFileName);
        }
        while(FindNextFile(hFile, &wfdFile));

        if(hFile != INVALID_HANDLE_VALUE)
        FindClose(hFile);
    }

    if (bSortFiles)
        listLocal.sort();

    for(it = listLocal.begin(); it != listLocal.end(); ++it)
    {
        strBuffer = strPath;
        strBuffer.append(*it);

        AppWalkDir(strBuffer.c_str(), szFileMask, listFiles, bSortFiles, pListSubdir);

        if (pListSubdir)
            pListSubdir->push_back(strBuffer);
    }

    listLocal.clear();
}

BOOL AppDeleteFiles (const char* szPath, const char* szFileMask, BOOL bAndSubdir)
{
    BOOL bRetVal = TRUE;
    StringList listFiles, listSubdir;

    AppWalkDir(szPath, szFileMask, listFiles, FALSE, &listSubdir);

    StringListIt it = listFiles.begin();
    for(; it != listFiles.end(); ++it)
        if (DeleteFile((*it).c_str()) == FALSE)
            bRetVal = FALSE;

    if (bAndSubdir)
        for(it = listSubdir.begin(); it != listSubdir.end(); ++it)
            if (RemoveDirectory((*it).c_str()) == FALSE)
                bRetVal = FALSE;

    return bRetVal;
}


BOOL AppDeleteDirectory (const char* szPath)
{
    AppDeleteFiles(szPath, "*.*", TRUE);
    return RemoveDirectory(szPath);
}

bool AppGetFileAttrs (
        const char* szPath, DWORD* attrs, __int64* fileSize,
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

bool AppGetFileAttrs (const char* szPath, BY_HANDLE_FILE_INFORMATION* pInfo)
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

void AppGetPath (std::string& path)
{
    char szBuff[_MAX_PATH + 1];
    VERIFY(::GetModuleFileName(AfxGetApp()->m_hInstance, szBuff, _MAX_PATH));
    szBuff[_MAX_PATH] = 0;
    path = szBuff;
    String::size_type pathLen = path.find_last_of('\\');
    path.resize((pathLen != String::npos) ? pathLen : 0);
}

void AppGetFullPathName (const String& path, String& fullPath)
{
	char *filename;
    int length = GetFullPathName(path.c_str(), 0, 0, &filename);
    char *buffer = new char[length + 1];
    GetFullPathName(path.c_str(), length + 1, buffer, &filename);
    fullPath = buffer;
    delete buffer;
}

bool AppShellOpenFile (const char* file)
{
    HINSTANCE result = ShellExecute(NULL, "open", file, NULL, NULL, SW_SHOW);

#pragma warning(push)
#pragma warning(disable : 4311)
    if ((DWORD)result <= HINSTANCE_ERROR)
#pragma warning(pop)
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((String("Cannot open file \"") + file + "\" with default viewer.").c_str(), MB_OK|MB_ICONSTOP);
        return false;
    }

    return true;
}

bool AppIsFolder (const char* path, bool nothrow)
{
    ASSERT_EXCEPTION_FRAME;

    DWORD dwFileAttributes = GetFileAttributes(path);

    if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

    if (!nothrow)
        THROW_APP_EXCEPTION(CString("Cannot read folder or file \"") + path + "\".");

    return false;
}

void AppCreateFolderHierarchy (const char* _path)
{
    String path = _path; 

    if (path.size())
    {
        StringList listDir;
        // check existent path part
        while (!path.empty() && *(path.rbegin()) != ':'
        && GetFileAttributes(path.c_str()) == 0xFFFFFFFF) {
            listDir.push_front(path);

            size_t len = path.find_last_of('\\');
            if (len != std::string::npos)
                path.resize(len);
            else
                break;
        }

        // create non-existent path part
        StringList::const_iterator it(listDir.begin()), end(listDir.end());
        for (; it != end; it++)
        {
            if (!::CreateDirectory((*it).c_str(), NULL))
            {
                String lastError;
                AppGetLastError(lastError);
                String message("Can't create folder \"");
                message += *it;
                message += "\"\n";
                message += lastError;
                THROW_APP_EXCEPTION(message);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//  the following code is based on CWinException
//	Copyright(c) Armen Hakobyan 2001
void AppGetLastError (String& lastError)
{
	HMODULE 	hModule 	= NULL; 
	LPTSTR		lpszMessage	= NULL;
	DWORD		cbBuffer	= NULL;
    DWORD       dwErrCode   = ::GetLastError();

	lastError.clear();

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

    if (lastError.empty())
    {
        ostringstream o;
        o << "Unknown system error - " << dwErrCode;
        lastError = o.str();
    }
}

void AppCopyTextToClipboard (const String& theText)
{
	if (::OpenClipboard(NULL))
	{
		CWaitCursor wait;
		::EmptyClipboard();

		if (theText.size())
		{
			HGLOBAL hDataDest = ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, theText.size() + 1);
			
            if (hDataDest)
			{
				char* dest = (char*)::GlobalLock(hDataDest);

                if (dest)
					memcpy(dest, theText.c_str(), theText.size() + 1);

				::GlobalUnlock(hDataDest);
				::SetClipboardData(CF_TEXT, hDataDest);
			}
		}
		::CloseClipboard();
	}
}

bool AppLoadTextFromResources (const char* name, const char* type, String& text)
{
	text.clear();

    if (HRSRC hrsrc = ::FindResource(AfxGetResourceHandle(), name, type))
        if (HGLOBAL handle = ::LoadResource(AfxGetResourceHandle(), hrsrc))
		{
            const char* ptr = (const char*)::LockResource(handle);
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
        if (char* mem = (char*)GlobalLock(hData)) 
            memcpy(mem, text, length + 1);

    return SetClipboardData(format, hData) ? true : false;
}

bool AppGetClipboardText (string& buffer, UINT format /*= CF_TEXT*/)
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

bool AppGetDragAndDroData (string& buffer, COleDataObject* pDataObject, CLIPFORMAT format /*= CF_TEXT*/)
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

}//namespace Common
