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

#include <afxmt.h>
#include <arg_shared.h>

    class COEDocManager;
    class COEMultiDocTemplate;
    class COEDocument;
    namespace ThreadCommunication { template <class T, int instance> class Singleton; };
    class RecentFileList;
    class FavoritesList;

class OEWorkspaceManager : private boost::noncopyable
{
    static UINT m_workspaceClipboardFormat;
    COEDocManager* m_pDocManager;
    COEMultiDocTemplate* m_pDocTemplate;
    arg::counted_ptr<RecentFileList> m_pRecentFileList;
    arg::counted_ptr<FavoritesList>  m_pFavoritesList;
    CString m_workspaceExtension, m_snapshotExtension;
    typedef std::map<COEDocument*, unsigned> WorkspaceState;
    WorkspaceState m_lastBackupState, m_activeWorkspaceSavedState;
    std::wstring m_sUuid;
    std::wstring m_sLockFile, m_lastFolder;
    HANDLE m_hLockFile;
    std::wstring m_workspacePath;
    CFile m_workspaceFile;
    std::map<COEDocument*, unsigned> m_workspaceDocLastAcitions;

    CWinThread* m_pThread;
    typedef void (*UpdateApplicationTitle)();
    UpdateApplicationTitle m_onUpdateApplicationTitle;
    static CEvent m_eventShutdown;
    static CEvent m_eventInstantAutosave;

    static UINT ThreadProc (LPVOID);

    template <class T, int instance> friend class ThreadCommunication::Singleton;
    OEWorkspaceManager ();
    ~OEWorkspaceManager ();

public:
    static OEWorkspaceManager& Get ();
    static void Destroy ();

    void Init (COEDocManager* pDocManager, COEMultiDocTemplate* pDocTemplate, arg::counted_ptr<RecentFileList>&, arg::counted_ptr<FavoritesList>&);
    void SetWorkspaceExtetion (LPCTSTR extension) { m_workspaceExtension = extension; }
    void SetSnapshotExtetion (LPCTSTR extension)  { m_snapshotExtension = extension; }
    CWinThread* Shutdown ();
    CWinThread* ImmediateShutdown ();

    void InstantAutosave ();

    static UINT GetWorkspaceClipboardFormat ();

    int  AskToCloseAllDocuments ();
    void BuildDocumentList (std::vector<COEDocument*>&);
    void DoSave (CFile&, bool use_relative, const std::wstring& base = std::wstring());
    void DoOpen (CFile&, const std::wstring& path);

    void WorkspaceCopy  ();
    void WorkspacePaste ();

    void WorkspaceSave  (bool SaveAs);
    void WorkspaceSaveQuick ();

    void WorkspaceOpen  ();
    void WorkspaceOpenAutosaved ();
    void WorkspaceOpen  (LPCTSTR lpszPath, bool snapshot);
    void WorkspaceCloseActive ();

    void CheckAndAutosave (CFile&);

    CString GetWorkspaceFilter () const;
    CString GetSnapshotFilter () const;
    CString GetWorkspaceExtension () const;
    CString GetSnapshotExtension () const;
    CString GetWorkspaceFilename (LPCTSTR format) const;
    CString GetBackupFilename () const;

    std::wstring GetInstanceFolder () const;
    std::wstring GetBackupFolder (bool* byDefault = NULL) const;

    HANDLE GetLockFileHandle ()      { return m_hLockFile; }
    bool HasActiveWorkspace () const { return (m_workspaceFile != INVALID_HANDLE_VALUE) ? TRUE : FALSE; }
    std::wstring GetWorkspacePath () const { return m_workspacePath; }
    const WorkspaceState GetActiveWorkspaceSavedState () const { return m_activeWorkspaceSavedState; }

    void SetUpdateApplicationTitle (UpdateApplicationTitle ptr) { m_onUpdateApplicationTitle = ptr; }
};
