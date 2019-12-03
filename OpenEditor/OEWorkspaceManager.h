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
    string m_workspaceExtension, m_snapshotExtension;
    typedef std::map<COEDocument*, unsigned> WorkspaceState;
    WorkspaceState m_lastBackupState, m_activeWorkspaceSavedState;
    string m_sUuid, m_sLockFile, m_lastFolder;
    HANDLE m_hLockFile;
    string m_workspacePath;
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
    void SetWorkspaceExtetion (const string& extension) { m_workspaceExtension = extension; }
    void SetSnapshotExtetion (const string& extension)  { m_snapshotExtension = extension; }
    CWinThread* Shutdown ();
    CWinThread* ImmediateShutdown ();

    void InstantAutosave ();

    static UINT GetWorkspaceClipboardFormat ();

    int  AskToCloseAllDocuments ();
    void BuildDocumentList (std::vector<COEDocument*>&);
    void DoSave (CFile&, bool use_relative, const string& base = string());
    void DoOpen (CFile&, const string& path);

    void WorkspaceCopy  ();
    void WorkspacePaste ();

    void WorkspaceSave  (bool SaveAs);
    string WorkspaceSaveQuick ();

    void WorkspaceOpen  ();
    void WorkspaceOpenAutosaved ();
    void WorkspaceOpen  (LPCTSTR lpszPath, bool snapshot);
    void WorkspaceCloseActive ();

    void CheckAndAutosave (CFile&);

    CString GetWorkspaceFilter () const;
    CString GetSnapshotFilter () const;
    CString GetWorkspaceExtension () const;
    CString GetSnapshotExtension () const;
    CString GetWorkspaceFilename (const char* format) const;
    CString GetBackupFilename () const;

    string GetInstanceFolder () const;
    string GetBackupFolder (bool* byDefault = NULL) const;

    HANDLE GetLockFileHandle ()      { return m_hLockFile; }
    bool HasActiveWorkspace () const { return (m_workspaceFile != INVALID_HANDLE_VALUE) ? TRUE : FALSE; }
    string GetWorkspacePath () const { return m_workspacePath; }
    const WorkspaceState GetActiveWorkspaceSavedState () const { return m_activeWorkspaceSavedState; }

    void SetUpdateApplicationTitle (UpdateApplicationTitle ptr) { m_onUpdateApplicationTitle = ptr; }
};
