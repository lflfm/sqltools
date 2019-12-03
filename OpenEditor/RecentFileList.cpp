/*
    Copyright (C) 2016 Aleksey Kochetov

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
#include <fstream>
#include <algorithm> 
#include "RecentFileList.h"
#include "Common/OsException.h"
#include "OpenEditor/OEDocument.h"
#include <tinyxml\tinyxml.h>

    using namespace OpenEditor;

    static const int LOCK_TIMEOUT = 1000;

RecentFileList::RecentFileList ()
    : m_time(time(0)),
    m_modified(false),
    m_fileAccessMutex(FALSE, "GNU.OpenEditor.History"),
    m_pRecentFilesListCtrl(0)
{
}


RecentFileList::~RecentFileList ()
{
}

#include "Common\RecentFilesListCtrl.h"

void RecentFileList::AttachControl (RecentFilesListCtrl* pRecentFilesListCtrl) 
{ 
    m_pRecentFilesListCtrl = pRecentFilesListCtrl; 

    if (m_pRecentFilesListCtrl)
    {
        std::deque<file_info>::const_iterator it = m_list.begin();

        for (; it != m_list.end(); ++it)
            m_pRecentFilesListCtrl->LoadEntry(it->path, it->time);

        if (m_pRecentFilesListCtrl->IsInitialized())
            m_pRecentFilesListCtrl->Refresh(true);
    }
}

void RecentFileList::DoOpen (const string& path, std::deque<file_info>& list, std::deque<wks_info>& wksList, unsigned __int64& time)
{
    if (!::PathFileExists(path.c_str()))
        return;

    TiXmlDocument doc;

    doc.LoadFile(path);

    if (doc.Error())
    {
        if (AfxMessageBox(CString("The file \"") + path.c_str() + "\" is corruted!\r\nWould you like to backup it and cointinue?", MB_ICONEXCLAMATION|MB_OKCANCEL) == IDOK)
        {
            ::DeleteFile((path + ".bad.3").c_str());
            ::MoveFile((path + ".bad.2").c_str(), (path + ".bad.3").c_str());
            ::MoveFile((path + ".bad.1").c_str(), (path + ".bad.2").c_str());
            ::MoveFile( path            .c_str(), (path + ".bad.1").c_str());
            doc.Clear();
        }
        else
            THROW_APP_EXCEPTION("RecentFileList: xml parsing error!");
    }

    if (const TiXmlElement* rootEl = doc.FirstChildElement("History"))
    {
        time = _atoi64(rootEl->Attribute("time"));

        if (const TiXmlElement* filesEl = rootEl->FirstChildElement("Files"))
        {
            if (const TiXmlElement* fileEl = filesEl->FirstChildElement("File"))
            {
                do 
                {
                    file_info fi;
                    fi.time = _atoi64(fileEl->Attribute("time"));
                    fi.path = fileEl->Attribute("path");

                    if (!fileEl->Attribute("used", &fi.counter))
                        fi.counter = 1;

                    if (!fileEl->Attribute("pos_x", &fi.cursor_position.column))
                        fi.cursor_position.column = 0;

                    if (!fileEl->Attribute("pos_y", &fi.cursor_position.line))
                        fi.cursor_position.line = 0;
                
                    if (!fileEl->Attribute("topmost_y", &fi.topmost_line))
                        fi.topmost_line = 0;

                    if (!fileEl->Attribute("topmost_x", &fi.topmost_column))
                        fi.topmost_column = 0;

                    if (!fileEl->Attribute("sel_md", &fi.block_mode))
                        fi.block_mode = 0;
                
                    if (!fileEl->Attribute("alt_col", &fi.alt_columnar))
                        fi.alt_columnar = 0;

                    bool dflt = false;
                    dflt |= !fileEl->Attribute("sel_beg_x", &fi.selection.start.column) ?  true : false;
                    dflt |= !fileEl->Attribute("sel_beg_y", &fi.selection.start.line  ) ?  true : false;
                    dflt |= !fileEl->Attribute("sel_end_x", &fi.selection.end.column  ) ?  true : false;
                    dflt |= !fileEl->Attribute("sel_end_y", &fi.selection.end.line    ) ?  true : false;
                    if (dflt)
                        fi.selection.clear();

                    if (const TiXmlElement* bookmarksEl = fileEl->FirstChildElement("Bmks"))
                    {
                        if (const TiXmlElement* bookmarkEl = bookmarksEl->FirstChildElement("Bmk"))
                        {
                            while (bookmarkEl)
                            {
                                int line = -1;
                                if (bookmarkEl->Attribute("line", &line) && line != -1)
                                    fi.bookmarks.push_back(line);

                                bookmarkEl = bookmarkEl->NextSiblingElement("Bmk");
                            }
                        }
                    }

                    if (const TiXmlElement* bookmarksEl = fileEl->FirstChildElement("RndmBmks"))
                    {
                        if (const TiXmlElement* bookmarkEl = bookmarksEl->FirstChildElement("Bmk"))
                        {
                            while (bookmarkEl)
                            {
                                int id = -1, line = -1;
                                if (bookmarkEl->Attribute("id", &id) && id != -1
                                && bookmarkEl->Attribute("line", &line) && line != -1)
                                    fi.random_bookmarks.push_back(std::make_pair(id, line));

                                bookmarkEl = bookmarkEl->NextSiblingElement("Bmk");
                            }
                        }
                    }

                    list.push_back(fi);

                    fileEl = fileEl->NextSiblingElement("File");
                }
                while (fileEl);
            }
        }

        if (const TiXmlElement* filesEl = rootEl->FirstChildElement("Workspaces"))
        {
            if (const TiXmlElement* fileEl = filesEl->FirstChildElement("Workspace"))
            {
                do 
                {
                    wks_info wi;
                    wi.time = _atoi64(fileEl->Attribute("time"));
                    wi.path = fileEl->Attribute("path");
                    wksList.push_back(wi);

                    fileEl = fileEl->NextSiblingElement("Workspace");
                }
                while (fileEl);
            }
        }
    }
}

void RecentFileList::Open (const string& path)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_path + "\"!");

    DoOpen(path, m_list, m_wksList, m_time);
    m_path = path;
    m_time = min<unsigned __int64>(m_time, time(0));
    m_modified = false;
}

/*
    in case if we want to save the list everytime on application, 
    local_counter has to be reset to 0 in order to keep correct counting 
*/
void RecentFileList::Save ()
{
    if (m_modified)
    {
        CSingleLock lock(&m_fileAccessMutex);
        BOOL locked = lock.Lock(LOCK_TIMEOUT);
        if (!locked) 
            THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_path + "\"!");

        // repare what to save
        std::deque<file_info> deltaList;
        {
            std::deque<file_info>::const_iterator it = m_list.begin();
            for (; it != m_list.end(); ++ it)
                if (it->time > m_time)
                    deltaList.push_back(*it);
        }
        std::deque<wks_info> deltaWksList;
        {
            std::deque<wks_info>::const_iterator it = m_wksList.begin();
            for (; it != m_wksList.end(); ++ it)
                if (it->time > m_time)
                    deltaWksList.push_back(*it);
        }
        if (!deltaList.empty() || !deltaWksList.empty())
        {
            // make lock
            // open a copy from disc
            std::deque<file_info> list;
            std::deque<wks_info>  wksList;
            unsigned __int64 tm = 0;
            DoOpen(m_path, list, wksList, tm);

            // merging
            {
                std::deque<file_info>::const_iterator it = deltaList.begin();
                for (; it != deltaList.end(); ++ it)
                {
                    std::deque<file_info>::iterator destIt = std::find(list.begin(), list.end(), *it);
                    if (destIt == list.end())
                    {
                        if (!it->deleted)
                        {
                            list.push_back(*it);
                            list.rbegin()->counter += list.rbegin()->local_counter;
                        }
                    }
                    else if (destIt->time < it->time)
                    {
                        if (!it->deleted)
                        {
                            (*destIt) = *it;
                            destIt->counter += destIt->local_counter; 
                        }
                        else
                        {
                            list.erase(destIt);
                        }
                    }
                }
            }
            {
                std::deque<wks_info>::const_iterator it = deltaWksList.begin();
                for (; it != deltaWksList.end(); ++ it)
                {
                    std::deque<wks_info>::iterator destIt = std::find(wksList.begin(), wksList.end(), *it);
                    if (destIt == wksList.end())
                    {
                        if (!it->deleted)
                            wksList.push_back(*it);
                    }
                    else if (destIt->time < it->time)
                    {
                        if (!it->deleted)
                            (*destIt) = *it;
                        else
                            wksList.erase(destIt);
                    }
                }
            }

            // sorting
            std::sort(list.begin(), list.end(), more_recent_file);
            std::sort(wksList.begin(), wksList.end(), more_recent_wks);

            //cutting to the max size
            unsigned int nMaxFiles = COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryFiles();
            if (list.size() > nMaxFiles) list.resize(nMaxFiles);

            unsigned int nMaxWorkspaces = COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryWorkspaces();
            if (wksList.size() > nMaxWorkspaces) wksList.resize(nMaxWorkspaces);

            m_list.swap(list); 
            m_wksList.swap(wksList);

            m_time = time(0);
            DoSave(m_path, m_list, m_wksList, m_time);
            m_modified = false;
        }
    }
}

    static
    void SetAttribute (TiXmlElement* elem, const char* attr, unsigned __int64 value)
    {
        char buff[80];
        _i64toa(value, buff, 10);
        elem->SetAttribute(attr, buff);
    }

void RecentFileList::DoSave (const string& path, const std::deque<file_info>& list, const std::deque<wks_info>& wksList, unsigned __int64 time)
{
    TiXmlDocument doc;
    doc.LinkEndChild(new TiXmlDeclaration("1.0","","yes"));

    if (TiXmlElement* rootEl = new TiXmlElement("History"))
    {
        SetAttribute(rootEl, "time", time);

        if (TiXmlElement* filesEl = new TiXmlElement("Files"))
        {
            std::deque<file_info>::const_iterator it = list.begin();
            for (; it != list.end(); ++it)
            {
                if (TiXmlElement* fileEl = new TiXmlElement("File"))
                {
                    fileEl->SetAttribute("path", it->path);
                    SetAttribute(fileEl, "time", it->time);
                    
                    if (it->counter > 1)
                        SetAttribute(fileEl, "used", it->counter);

                    if (it->cursor_position.column != 0)
                        fileEl->SetAttribute("pos_x", it->cursor_position.column);

                    if (it->cursor_position.line != 0)
                        fileEl->SetAttribute("pos_y", it->cursor_position.line);
                    
                    if (it->topmost_line != 0)
                        fileEl->SetAttribute("topmost_y", it->topmost_line);
                                                       
                    if (it->topmost_column != 0)
                        fileEl->SetAttribute("topmost_x", it->topmost_column);
                                                       
                    if (it->block_mode != 0)
                        fileEl->SetAttribute("sel_md", it->block_mode);
                    
                    if (it->block_mode == ebtColumn && it->alt_columnar != 0)
                        fileEl->SetAttribute("alt_col", it->alt_columnar);

                    if (!it->selection.is_empty())
                    {
                        fileEl->SetAttribute("sel_beg_x", it->selection.start.column);
                        fileEl->SetAttribute("sel_beg_y", it->selection.start.line  );
                        fileEl->SetAttribute("sel_end_x", it->selection.end.column  );
                        fileEl->SetAttribute("sel_end_y", it->selection.end.line    );
                    }

                    if (!it->bookmarks.empty())
                    {
                        if (TiXmlElement* bookmarksEl = new TiXmlElement("Bmks"))
                        {
                            vector<int>::const_iterator bmkIt = it->bookmarks.begin();
                            for (; bmkIt != it->bookmarks.end(); ++bmkIt)
                            {
                                if (TiXmlElement* bookmarkEl = new TiXmlElement("Bmk"))
                                {
                                    bookmarkEl->SetAttribute("line", *bmkIt);  
                                    bookmarksEl->LinkEndChild(bookmarkEl);
                                }
                            }
                            fileEl->LinkEndChild(bookmarksEl);
                        }
                    }

                    if (!it->random_bookmarks.empty())
                    {
                        if (TiXmlElement* bookmarksEl = new TiXmlElement("RndmBmks"))
                        {
                            vector<std::pair<int,int>>::const_iterator bmkIt = it->random_bookmarks.begin();
                            for (; bmkIt != it->random_bookmarks.end(); ++bmkIt)
                            {
                                if (TiXmlElement* bookmarkEl = new TiXmlElement("Bmk"))
                                {
                                    bookmarkEl->SetAttribute("id", bmkIt->first);
                                    bookmarkEl->SetAttribute("line", bmkIt->second);
                                    bookmarksEl->LinkEndChild(bookmarkEl);
                                }
                            }
                            fileEl->LinkEndChild(bookmarksEl);
                        }
                    }

                    filesEl->LinkEndChild(fileEl);
                }
            }
            rootEl->LinkEndChild(filesEl);
        }

        if (TiXmlElement* filesEl = new TiXmlElement("Workspaces"))
        {
            std::deque<wks_info>::const_iterator it = wksList.begin();
            for (; it != wksList.end(); ++it)
                if (TiXmlElement* fileEl = new TiXmlElement("Workspace"))
                {
                    fileEl->SetAttribute("path", it->path);
                    SetAttribute(fileEl, "time", it->time);
                    filesEl->LinkEndChild(fileEl);
                }
            rootEl->LinkEndChild(filesEl);
        }

        doc.LinkEndChild(rootEl);
    }

    doc.SaveFile(path);
}

RecentFileList::file_info& RecentFileList::find (const string& path)
{
    std::deque<file_info>::iterator it = m_list.begin();

    for (; it != m_list.end(); ++it) 
        if (it->path == path) 
            return *it;
    
    m_list.insert(m_list.begin(), file_info());

    m_list.begin()->path = path;

    return *m_list.begin();
}


void RecentFileList::OnOpenDocument (COEDocument* doc, bool restoreState /*= true*/)
{
    if (doc)
    {
        string path = doc->GetPathName();
        if (!path.empty())
        {
            RecentFileList::file_info& fi = find(path);
            fi.time = time(0);
            ++fi.local_counter; 
            m_modified = true;

            if (m_pRecentFilesListCtrl)
                m_pRecentFilesListCtrl->UpdateEntry(fi.path, fi.time);

            if (restoreState 
            && COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryRestoreEditorState())
            {
                {
                    vector<int>::const_iterator it = fi.bookmarks.begin();
                    for (; it != fi.bookmarks.end(); ++it)
                        doc->GetStorage().SetBookmark(*it, eBmkGroup1, true);
                }
                {
                    vector<pair<int,int>>::const_iterator it = fi.random_bookmarks.begin();
                    for (; it != fi.random_bookmarks.end(); ++it)
                        doc->GetStorage().SetRandomBookmark(RandomBookmark((unsigned char)it->first), it->second, true);
                }

                POSITION vpos = doc->GetFirstViewPosition();
                CView* pView = doc->GetNextView(vpos);
                if (COEditorView* pOEView = DYNAMIC_DOWNCAST(COEditorView, pView))
                {
                    pOEView->MoveTo(fi.cursor_position, true);

                    pOEView->SetBlockMode(fi.block_mode == ebtColumn ? ebtColumn : ebtStream, 
                        fi.block_mode == ebtColumn ? (fi.alt_columnar ? true : false) : false);

                    pOEView->SetSelection(fi.selection);

                    pOEView->SetTopmostLine(fi.topmost_line);
                    pOEView->SetTopmostColumn(fi.topmost_column);
                }
            }
        }
    }
}

void RecentFileList::OnSaveDocument (COEDocument* doc)
{
    if (doc)
    {
        string path = doc->GetPathName();
        if (!path.empty())
        {
            string path = doc->GetPathName();
            RecentFileList::file_info& fi = find(path);

            m_modified = true;

            if (!fi.counter) // it is either save new or save as 
            {
                fi.time = time(0);
                if (m_pRecentFilesListCtrl)
                    m_pRecentFilesListCtrl->UpdateEntry(fi.path, fi.time);
            }
        }
    }
}

void RecentFileList::OnCloseDocument (COEDocument* doc)
{
    if (doc)
    {
        string path = doc->GetPathName();
        if (!path.empty())
        {
            RecentFileList::file_info& fi = find(path);
            //fi.time = time(0);
            m_modified = true;

            fi.bookmarks.clear();
            doc->GetStorage().GetBookmarkedLines(fi.bookmarks, eBmkGroup1);

            fi.random_bookmarks.clear();
            const RandomBookmarkArray& bookmarks = doc->GetStorage().GetRandomBookmarks();
            for (unsigned char i = RandomBookmark::FIRST; i < RandomBookmark::LAST; ++i)
            {
                int line = bookmarks.GetLine(RandomBookmark(i));
                if (line != -1)
                    fi.random_bookmarks.push_back(std::make_pair/*<int, int>*/(i, line));
            }

            POSITION vpos = doc->GetFirstViewPosition();
            CView* pView = doc->GetNextView(vpos);
            if (COEditorView* pOEView = DYNAMIC_DOWNCAST(COEditorView, pView))
            {
                fi.cursor_position = pOEView->GetPosition();
                fi.topmost_line = pOEView->GetTopmostLine();
                fi.topmost_column = pOEView->GetTopmostColumn();
                fi.block_mode = (int)pOEView->GetBlockMode();
                fi.alt_columnar = pOEView->IsAltColumnarMode() ? 1 : 0; 
                pOEView->GetSelection(fi.selection);
            }
        }
    }
}

void RecentFileList::RemoveDocument (const string& path)
{
    RecentFileList::file_info& fi = find(path);
    fi.time = time(0);
    fi.deleted = true;
    m_modified = true;
}

RecentFileList::wks_info& RecentFileList::find_wks (const string& path)
{
    std::deque<wks_info>::iterator it = m_wksList.begin();

    for (; it != m_wksList.end(); ++it) 
        if (it->path == path) 
            return *it;
    
    m_wksList.insert(m_wksList.begin(), wks_info());

    m_wksList.begin()->path = path;

    return *m_wksList.begin();
}

void RecentFileList::OnOpenWorkspace (const string& path)
{
    if (!path.empty())
    {
        wks_info& wks = find_wks(path);
        wks.time = time(0);
        m_modified = true;
    }
}

void RecentFileList::OnSaveWorkspace (const string& path)
{
    if (!path.empty())
    {
        wks_info& wks = find_wks(path);

        if (!wks.time) // new one
        {
            wks.time = time(0);
            m_modified = true;
        }
    }
}

void RecentFileList::RemoveWorkspace (const string& path)
{
    RecentFileList::wks_info& wks = find_wks(path);
    wks.time = time(0);
    wks.deleted = true;
    m_modified = true;
}

bool RecentFileList::more_recent_file (const file_info& left, const file_info& right)
{
    return left.time > right.time;
}

void RecentFileList::UpdateFileMenu (CCmdUI* pCmdUI)
{
    std::sort(m_list.begin(), m_list.end(), more_recent_file);

    std::deque<file_info>::const_iterator it = m_list.begin();

    int size = COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryFilesInRecentMenu();

    vector<string> list;
    for (int i = 0; i < size && it != m_list.end(); ++i, ++it)
        if (!it->deleted)
            list.push_back(it->path);
     
    m_fileMenuHandler.m_nMaxDisplayLength = 
        COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryOnlyFileNameInRecentMenu() ? 0 : 64;
    m_fileMenuHandler.Init(list, size);
    m_fileMenuHandler.UpdateMenu(pCmdUI);
}

bool RecentFileList::GetFileName (int index, CString& fname)
{
    return m_fileMenuHandler.GetName(index, fname);
}

bool RecentFileList::more_recent_wks (const wks_info& left, const wks_info& right)
{
    return left.time > right.time;
}

void RecentFileList::UpdateWorkspaceMenu (CCmdUI* pCmdUI)
{
    std::sort(m_wksList.begin(), m_wksList.end(), more_recent_wks);

    std::deque<wks_info>::const_iterator it = m_wksList.begin();

    int size = COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryWorkspacesInRecentMenu();

    vector<string> list;
    for (; list.size() <= (unsigned)size && it != m_wksList.end(); ++it)
        if (!it->deleted)
            list.push_back(it->path);

    m_wksMenuHandler.m_nMaxDisplayLength = 
        COEDocument::GetSettingsManager().GetGlobalSettings()->GetHistoryOnlyWorkspaceNameInRecentMenu() ? 0 : 64;
    m_wksMenuHandler.Init(list, size);
    m_wksMenuHandler.UpdateMenu(pCmdUI);
}

bool RecentFileList::GetWorkspaceName (int index, CString& fname)
{
    return m_wksMenuHandler.GetName(index, fname);
}

void RecentFileList::MenuHandler::Init (const vector<string>& list, int size)
{
    m_arrNames.SetSize(size);
    m_nSize = size;
    m_nMaxSize = max(size, m_nMaxSize);

    vector<string>::const_iterator it = list.begin();

    for (int i = 0; i < m_nSize; ++i)
    {
        if (it != list.end())
        {
            m_arrNames[i] = it->c_str();
            ++it;
        }
        else
            m_arrNames[i].Empty();
    }
}

bool RecentFileList::MenuHandler::GetName (int index, CString& fname)
{
    /*
    check if path is valid and mark to delete if not
    */
    if (index < m_nSize && !m_arrNames[index].IsEmpty())
    {
        fname = m_arrNames[index];
        return true;
    }
    return false;
}

// the code below has been borrowed from the original CRecentFileList

BOOL AFXAPI AfxFullPath(_Pre_notnull_ _Post_z_ LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, _Out_cap_(nMax) LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, _Out_opt_cap_(nMax) LPTSTR lpszTitle, UINT nMax);

/////////////////////////////////////////////////////////////////////////////
// lpszCanon = C:\MYAPP\DEBUGS\C\TESWIN.C
//
// cchMax   b   Result
// ------   -   ---------
//  1- 7    F   <empty>
//  1- 7    T   TESWIN.C
//  8-14    x   TESWIN.C
// 15-16    x   C:\...\TESWIN.C
// 17-23    x   C:\...\C\TESWIN.C
// 24-25    x   C:\...\DEBUGS\C\TESWIN.C
// 26+      x   C:\MYAPP\DEBUGS\C\TESWIN.C

#if !defined(_DLL)
AFX_STATIC void AFXAPI _AfxAbbreviateName(_Inout_z_ LPTSTR lpszCanon, int cchMax, BOOL bAtLeastName);
#else
AFX_STATIC void AFXAPI _AfxAbbreviateName(_Inout_z_ LPTSTR lpszCanon, int cchMax, BOOL bAtLeastName)
{
	ENSURE_ARG(AfxIsValidString(lpszCanon));

	int cchFullPath, cchFileName, cchVolName;
	const TCHAR* lpszCur;
	const TCHAR* lpszBase;
	const TCHAR* lpszFileName;

	lpszBase = lpszCanon;
	cchFullPath = lstrlen(lpszCanon);

	cchFileName = AfxGetFileName(lpszCanon, NULL, 0) - 1;
	lpszFileName = lpszBase + (cchFullPath-cchFileName);

	// If cchMax is more than enough to hold the full path name, we're done.
	// This is probably a pretty common case, so we'll put it first.
	if (cchMax >= cchFullPath)
		return;

	// If cchMax isn't enough to hold at least the basename, we're done
	if (cchMax < cchFileName)
	{
		if (!bAtLeastName)
			lpszCanon[0] = _T('\0');
		else
			Checked::tcscpy_s(lpszCanon, cchFullPath + 1, lpszFileName);
		return;
	}

	// Calculate the length of the volume name.  Normally, this is two characters
	// (e.g., "C:", "D:", etc.), but for a UNC name, it could be more (e.g.,
	// "\\server\share").
	//
	// If cchMax isn't enough to hold at least <volume_name>\...\<base_name>, the
	// result is the base filename.

	lpszCur = lpszBase + 2;                 // Skip "C:" or leading "\\"

	if (lpszBase[0] == '\\' && lpszBase[1] == '\\') // UNC pathname
	{
		// First skip to the '\' between the server name and the share name,
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	// if a UNC get the share name, if a drive get at least one directory
	ASSERT(*lpszCur == '\\');
	// make sure there is another directory, not just c:\filename.ext
	if (cchFullPath - cchFileName > 3)
	{
		lpszCur = _tcsinc(lpszCur);
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	ASSERT(*lpszCur == '\\');

	cchVolName = int(lpszCur - lpszBase);
	if (cchMax < cchVolName + 5 + cchFileName)
	{
		Checked::tcscpy_s(lpszCanon, cchFullPath + 1, lpszFileName);
		return;
	}

	// Now loop through the remaining directory components until something
	// of the form <volume_name>\...\<one_or_more_dirs>\<base_name> fits.
	//
	// Assert that the whole filename doesn't fit -- this should have been
	// handled earlier.

	ASSERT(cchVolName + (int)lstrlen(lpszCur) > cchMax);
	while (cchVolName + 4 + (int)lstrlen(lpszCur) > cchMax)
	{
		do
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
		while (*lpszCur != '\\');
	}

	// Form the resultant string and we're done.
	int cch;
	if (cchVolName >= 0 && cchVolName < cchMax)
		cch = cchVolName;
	else cch = cchMax;
	Checked::memcpy_s(lpszCanon + cch, cchFullPath + 1 - cch, _T("\\..."), sizeof(_T("\\...")) );
	Checked::tcscat_s(lpszCanon, cchFullPath + 1, lpszCur);
}
#endif

BOOL RecentFileList::MenuHandler::GetDisplayName(CString& strName, int nIndex, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const
{
	ENSURE_ARG(lpszCurDir == NULL || AfxIsValidString(lpszCurDir, nCurDir));

	//ASSERT(m_arrNames != NULL);
	ENSURE_ARG(nIndex < m_nSize);
	if (lpszCurDir == NULL || m_arrNames[nIndex].IsEmpty())
		return FALSE;

	int nLenName = m_arrNames[nIndex].GetLength();
	LPTSTR lpch = strName.GetBuffer( nLenName + 1);
	if (lpch == NULL)
	{
		AfxThrowMemoryException();
	}
	Checked::tcsncpy_s(lpch, nLenName + 1, m_arrNames[nIndex], _TRUNCATE);
	// nLenDir is the length of the directory part of the full path
	int nLenDir = nLenName - (AfxGetFileName(lpch, NULL, 0) - 1);
	BOOL bSameDir = FALSE;
	if (nLenDir == nCurDir)
	{
		TCHAR chSave = lpch[nLenDir];
		lpch[nCurDir] = 0;  // terminate at same location as current dir
		bSameDir = ::AfxComparePath(lpszCurDir, lpch);
		lpch[nLenDir] = chSave;
	}
	// copy the full path, otherwise abbreviate the name
	if (bSameDir)
	{
		// copy file name only since directories are same
		TCHAR szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nCurDir, szTemp, _countof(szTemp));
		Checked::tcsncpy_s(lpch, nLenName + 1, szTemp, _TRUNCATE);
	}
	else if (m_nMaxDisplayLength != -1)
	{
		// strip the extension if the system calls for it
		TCHAR szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nLenDir, szTemp, _countof(szTemp));
		Checked::tcsncpy_s(lpch+nLenDir, nLenName + 1 - nLenDir, szTemp, _TRUNCATE);

		// abbreviate name based on what will fit in limited space
		_AfxAbbreviateName(lpch, m_nMaxDisplayLength, bAtLeastName);
	}
	strName.ReleaseBuffer();
	return TRUE;
}

void RecentFileList::MenuHandler::UpdateMenu (CCmdUI* pCmdUI)
{
	ENSURE_ARG(pCmdUI != NULL);
	//ASSERT(m_arrNames != NULL);

	CMenu* pMenu = pCmdUI->m_pMenu;
	if (m_strOriginal.IsEmpty() && pMenu != NULL)
		pMenu->GetMenuString(pCmdUI->m_nID, m_strOriginal, MF_BYCOMMAND);

	if (m_arrNames[0].IsEmpty())
	{
		// no MRU files
		if (!m_strOriginal.IsEmpty())
			pCmdUI->SetText(m_strOriginal);
		pCmdUI->Enable(FALSE);
		return;
	}

	if (pCmdUI->m_pMenu == NULL)
		return;

	int iMRU;
    for (iMRU = 0; iMRU < m_nMaxSize/*m_nSize*/; iMRU++)
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID + iMRU, MF_BYCOMMAND);

	TCHAR szCurDir[_MAX_PATH];
	DWORD dwDirLen = GetCurrentDirectory(_MAX_PATH, szCurDir);
	if( dwDirLen == 0 || dwDirLen >= _MAX_PATH )
		return;	// Path too long

	int nCurDir = lstrlen(szCurDir);
	ASSERT(nCurDir >= 0);
	szCurDir[nCurDir] = '\\';
	szCurDir[++nCurDir] = '\0';

	CString strName;
	CString strTemp;
	for (iMRU = 0; iMRU < m_nSize; iMRU++)
	{
		if (!GetDisplayName(strName, iMRU, szCurDir, nCurDir))
			break;

		// double up any '&' characters so they are not underlined
		LPCTSTR lpszSrc = strName;
		LPTSTR lpszDest = strTemp.GetBuffer(strName.GetLength()*2);
		while (*lpszSrc != 0)
		{
			if (*lpszSrc == '&')
				*lpszDest++ = '&';
			if (_istlead(*lpszSrc))
				*lpszDest++ = *lpszSrc++;
			*lpszDest++ = *lpszSrc++;
		}
		*lpszDest = 0;
		strTemp.ReleaseBuffer();

		// insert mnemonic + the file name
		TCHAR buf[10];
		int nItem = ((iMRU + m_nStart) % _AFX_MRU_MAX_COUNT) + 1;


		// number &1 thru &9, then 1&0, then 11 thru ...
		if (nItem > 10)
			_stprintf_s(buf, _countof(buf), _T("%d "), nItem);
		else if (nItem == 10)
			Checked::tcscpy_s(buf, _countof(buf), _T("1&0 "));
		else
			_stprintf_s(buf, _countof(buf), _T("&%d "), nItem);

		pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++,
			MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++,
			CString(buf) + strTemp);
	}

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
	pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();

	pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
}
