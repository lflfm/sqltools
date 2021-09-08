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

// 24.09.2007 some improvements taken from sqltools++ by Randolf Geist

#pragma once
#ifndef __AppUtilities_h__
#define __AppUtilities_h__

#pragma warning ( disable : 4786 )
#include <list>
#include <string>


namespace Common
{

    void AppRestoreHistory (CComboBox& wndList, LPCTSTR szSection, LPCTSTR szEntry, int nSize);
    void AppSaveHistory (CComboBox& wndList, LPCTSTR szSection, LPCTSTR szEntry, int nSize);


    void AppWalkDir (LPCTSTR szPath, LPCTSTR szFileMask,
                     CStringList& fileList, BOOL bSortFiles = TRUE, CStringList* folderList = 0);
    BOOL AppDeleteFiles (LPCTSTR szPath, LPCTSTR szFileMask, BOOL bAndSubdir);
    BOOL AppDeleteDirectory (LPCTSTR szPath);
    void AppGetPath (CString& path);
    void AppGetFullPathName (LPCTSTR path, CString& fullPath);

    bool AppGetFileAttrs (
            LPCTSTR szPath, DWORD* attrs, __int64* fileSize = 0,
            __int64* creationTime = 0, __int64* lastWriteTime = 0
        );
    bool AppGetFileAttrs (LPCTSTR szPath, BY_HANDLE_FILE_INFORMATION*);

    bool AppShellOpenFile (LPCTSTR file);
    bool AppIsFolder (LPCTSTR path, bool nothrow);
    void AppCreateFolderHierarchy (LPCTSTR path);
    void AppGetLastError (CString&);
    void AppCopyTextToClipboard (const CString& theText);

    bool AppLoadTextFromResources (LPCTSTR name, LPCTSTR type, std::string& text);

    bool AppSetClipboardText (const char*, int, UINT = CF_TEXT);
    bool AppSetClipboardText (const wchar_t*, int, UINT = CF_UNICODETEXT);
    bool AppGetClipboardText (std::string&, UINT = CF_TEXT);
    bool AppGetClipboardText (CString&, UINT = CF_UNICODETEXT);
    bool AppGetDragAndDroData (std::string&, COleDataObject*, CLIPFORMAT = CF_TEXT);
    bool AppGetDragAndDroData (std::wstring&, COleDataObject*, CLIPFORMAT = CF_UNICODETEXT);
    bool AppGetDragAndDroData (CString&, COleDataObject*, CLIPFORMAT = CF_UNICODETEXT);

}//namespace Common

#endif//__AppUtilities_h__

