/*
    Copyright (C) 2002,2005 Aleksey Kochetov

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
#include <arg_shared.h>
#include <Common/FixedArray.h>
#include <Common/FilenameTemplate.h>

    class COEDocument;
    class RecentFileList;
    using arg::counted_ptr;

/*
  COEDocManager supports multiple extensions for one document type:
  * extended syntax - ... SQL Files|PL/SQL Files\n*.SQL|*.PLS;*.PLB ...;
  * backward compatibility according MFC documentation (... .SQL ...);

  The following methods nave not been implemented yet:
    void RegisterShellFileTypes (BOOL bCompat);
    void UnregisterShellFileTypes ();
*/
class COEDocManager : public CDocManager
{
public:
    DECLARE_DYNAMIC(COEDocManager);
    COEDocManager ();

    using CDocManager::GetDocumentCount;

    // helper for standard commdlg dialogs
    virtual BOOL DoPromptFileName (CString&, UINT, DWORD, BOOL, CDocTemplate*);

    virtual void OnFileOpen ();
    
    virtual CDocument* OpenDocumentFile (LPCWSTR);

    virtual BOOL SaveAllModified(); // obsolete
    BOOL SaveAllModified (bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit);
    virtual BOOL SaveAndCloseAllDocuments ();

private:
    void doFileOpen (LPCWSTR initialDir);
    BOOL doPromptFileName (LPCWSTR initialDir, CString& fileName, UINT nIDSTitle, 
                           DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
    DWORD m_nFilterIndex;
};

class COEMultiDocTemplate : public CMultiDocTemplate
{
public:
    DECLARE_DYNAMIC(COEMultiDocTemplate);

    COEMultiDocTemplate (UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass, counted_ptr<RecentFileList>&);

    int GetDocumentCount () const { return m_docList.GetCount(); }

    virtual CDocument* OpenWorkspaceFile (LPCWSTR lpszPathName);
    virtual CDocument* OpenDocumentFile (LPCWSTR lpszPathName, BOOL bMakeVisible);
    virtual CDocument* OpenDocumentFile (LPCWSTR lpszPathName, BOOL bAddToMRU, BOOL bMakeVisible);
    virtual void SetDefaultTitle (CDocument*);

    virtual BOOL SaveAllModified(); // obsolete
    BOOL SaveAllModified(bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit);
    virtual BOOL SaveAndCloseAllDocuments (const CDocument* pButThis);

    BOOL OnIdle (LONG, CFrameWnd*);

    typedef Common::FilenameTemplateFields FormatFields;
    const FormatFields& GetFormatFields () const { return Common::FilenameTemplate::GetFormatFields(); }
    void FormatTitle (const wchar_t* format, CString& title) const; 

    struct EnablePickDocType
    {
        COEMultiDocTemplate& m_templ;
        EnablePickDocType (COEMultiDocTemplate& templ) : m_templ(templ) { m_templ.m_pickDocType = true; }
        ~EnablePickDocType () { m_templ.m_pickDocType = false; }
    };

    struct DisableEditStateRestoring
    {
        COEMultiDocTemplate& m_templ;
        DisableEditStateRestoring (COEMultiDocTemplate& templ) : m_templ(templ) { m_templ.m_disableEditStateRestoring = true; }
        ~DisableEditStateRestoring () { m_templ.m_disableEditStateRestoring = false; }
    };

    RecentFileList* GetRecentFileList() { return m_pRecentFileList.get(); }

    typedef std::map<COEDocument*, unsigned> WorkspaceState;

    void GetWorkspaceState (WorkspaceState&);
    static bool WorkspaceStateEqual (const WorkspaceState& previous, const WorkspaceState& current, bool ignoreClosed = false);

    int GetUntitledCount () const { return (int)m_nUntitledCount; }
    void SetUntitledCount (int nUntitledCount) { m_nUntitledCount = (UINT)nUntitledCount; }

private:
    bool m_pickDocType, m_disableEditStateRestoring;
    CString m_category, m_filename;
    counted_ptr<RecentFileList> m_pRecentFileList;
    friend EnablePickDocType;
};
