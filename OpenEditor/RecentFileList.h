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
#pragma once

#include <deque>
#include <afxmt.h>
#include "OpenEditor/OEView.h"

    class COEDocument;
    class RecentFilesListCtrl;

class RecentFileList
{
    struct file_info
    {
        unsigned __int64 time;
        int codepage;
        int topmost_line, topmost_column;
        OpenEditor::Position cursor_position;
        OpenEditor::Square selection;
        int block_mode, alt_columnar;
        int counter, local_counter;
        std::wstring path;
        std::vector<int> bookmarks;
        std::vector<std::pair<int,int>> random_bookmarks;
        bool deleted;

        file_info ()
        {
            codepage = -1;
            time = 0;
            topmost_line = topmost_column = 0;
            block_mode = alt_columnar = 0;
            counter = local_counter = 0;
            cursor_position.column = 
            cursor_position.line = 0;
            deleted = false;
            selection.clear();
        }

        bool operator == (const file_info& other) { return path == other.path; }
    };

    static bool more_recent_file (const file_info& left, const file_info& right);

    struct wks_info
    {
        unsigned __int64 time;
        std::wstring path;
        bool deleted;
        wks_info () : time(0), deleted(false) {}
        bool operator == (const wks_info& other) { return path == other.path; }
    };

    static bool more_recent_wks (const wks_info& left, const wks_info& right);

    std::deque<file_info> m_list;
    std::deque<wks_info> m_wksList;

    unsigned __int64 m_time;
    std::wstring m_path;
    bool m_modified;
    CMutex m_fileAccessMutex;   // tinyxml uses fstream access
                                // so we have to use the mutex to lock the file 
                                // for concurrent access

    RecentFilesListCtrl* m_pRecentFilesListCtrl;

    file_info& find (const std::wstring& path);
    wks_info& find_wks (const std::wstring& path);

    static 
    void DoOpen (const std::wstring& path, std::deque<file_info>& list, std::deque<wks_info>& wksList, unsigned __int64& time);
    static 
    void DoSave (const std::wstring& path, const std::deque<file_info>& list, const std::deque<wks_info>& wksList, unsigned __int64 time);

public:
    RecentFileList ();
    virtual ~RecentFileList ();

    void AttachControl (RecentFilesListCtrl* pRecentFilesListCtrl);

    void Open (const std::wstring& path);
    void Save ();

    void OnOpenDocument (COEDocument*, bool restoreState = true);
    void OnSaveDocument (COEDocument*);
    void OnCloseDocument (COEDocument*);
    void RemoveDocument (const std::wstring& path);

    void OnOpenWorkspace (const std::wstring& path);
    void OnSaveWorkspace (const std::wstring& path);
    void RemoveWorkspace (const std::wstring& path);

    void UpdateFileMenu (CCmdUI* pCmdUI);
    bool GetFileName (int, CString&);

    void UpdateWorkspaceMenu (CCmdUI* pCmdUI);
    bool GetWorkspaceName (int, CString&);

private:
    // the code below has been borrowed from the original CRecentFileList
    struct MenuHandler
    {
        CString m_strOriginal;
        CStringArray m_arrNames;
        int m_nSize, m_nMaxSize;
        int m_nStart, m_nMaxDisplayLength;

        MenuHandler ()
        {
            m_nMaxSize = m_nSize = 10;
            m_arrNames.SetSize(m_nSize);
            m_nMaxDisplayLength = 64;
            m_nStart = 0;
        }

        ~MenuHandler ()
        {
        }

        int GetSize() const { return m_nSize; }
        void Init (const std::vector<std::wstring>&, int size);
        bool GetName (int, CString&);
        BOOL GetDisplayName(CString& strName, int nIndex, LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
        void UpdateMenu (CCmdUI* pCmdUI);
    }
    m_fileMenuHandler, m_wksMenuHandler;

};

