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

    void AppRestoreHistory (CComboBox& wndList, const char* szSection, const char* szEntry, int nSize);
    void AppSaveHistory (CComboBox& wndList, const char* szSection, const char* szEntry, int nSize);


    typedef std::string          String;
    typedef std::list<String>    StringList;
    typedef StringList::iterator StringListIt;

    void AppWalkDir (const char* szPath, const char* szFileMask,
                     StringList& listFiles, BOOL bSortFiles = TRUE, StringList* pListSubdir = 0);
    BOOL AppDeleteFiles (const char* szPath, const char* szFileMask, BOOL bAndSubdir);
    BOOL AppDeleteDirectory (const char* szPath);
    void AppGetPath (String& path);
    void AppGetFullPathName (const String& path, String& fullPath);

    bool AppGetFileAttrs (
            const char* szPath, DWORD* attrs, __int64* fileSize = 0,
            __int64* creationTime = 0, __int64* lastWriteTime = 0
        );
    bool AppGetFileAttrs (const char* szPath, BY_HANDLE_FILE_INFORMATION*);

    bool AppShellOpenFile (const char* file);
    bool AppIsFolder (const char* path, bool nothrow);
    void AppCreateFolderHierarchy (const char* path);
    void AppGetLastError (String&);
	void AppCopyTextToClipboard (const String& theText);

	bool AppLoadTextFromResources (const char* name, const char* type, String& text);

    bool AppSetClipboardText (const char*, int, UINT = CF_TEXT);
    bool AppGetClipboardText (string&, UINT = CF_TEXT);
    bool AppGetDragAndDroData (string&, COleDataObject*, CLIPFORMAT = CF_TEXT);

}//namespace Common

#endif//__AppUtilities_h__

