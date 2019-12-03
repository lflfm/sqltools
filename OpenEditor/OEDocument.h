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

// 17.01.2005 (ak) simplified settings class hierarchy 

#pragma once

#ifndef __OEDocument_h__
#define __OEDocument_h__

#include "OpenEditor/OEStorage.h"
#include "OpenEditor/OESettings.h"
#include <COMMON/MemoryMappedFile.h>
#include "FileWatch/FileWatch.h"
#include "OpenEditor/OEDocManager.h"


    class COEditorView;
    namespace Common { class CPropertySheetMem; }
    class RecentFileList;

class COEDocument : public CDocument, public CFileWatchClient
{
    // for a modified sign '*' in a title bar
    std::string m_orgTitle, m_extension;
    bool m_orgModified, m_extensionInitialized;
    __int64 m_orgFileTime, m_orgFileSize;
    static bool m_enableOpenUnexisting;

    // for backup processing only
    bool m_newOrSaveAs;
    std::string m_lastBackupPath;
    void backupFile (LPCTSTR lpszPathName);

    // settings container
    OpenEditor::Settings m_settings;
    // the main object
    OpenEditor::Storage m_storage;

    // memory mapped file support
    Common::MemoryMappedFile m_mmfile;
    // allocated memory when file locking is disabled
    LPVOID m_vmdata;
    void loadFile (const char* path, bool reload = false, bool external = false, bool modified = false);

    static OpenEditor::SettingsManager* m_pSettingsManager;
    static OpenEditor::Searcher m_searcher;
    static OpenEditor::SettingsManager& getSettingsManager ();

    virtual void OnWatchedFileChanged ();

protected:
    virtual void GetNewPathName (CString& newName) const { newName = m_strPathName; }

    void SetText (LPVOID, unsigned long);
    LPCVOID GetVMData () const;

    COEDocument();
    virtual ~COEDocument();
    DECLARE_DYNCREATE(COEDocument)

public:
    void SetText (const char*, unsigned long);

    const::string& GetOrgTitle () const { return m_orgTitle; }

    static bool GetEnableOpenUnexisting () { return m_enableOpenUnexisting; }
    static void SetEnableOpenUnexisting (bool enable) { m_enableOpenUnexisting = enable; }
    struct EnableOpenUnexisting
    {
        EnableOpenUnexisting ()  { SetEnableOpenUnexisting(true); }
        ~EnableOpenUnexisting () { SetEnableOpenUnexisting(false); }
    };
    
    struct SettingsDialogCallback
    {
        virtual void BeforePageAdding (Common::CPropertySheetMem&) = 0;
        virtual void AfterPageAdding  (Common::CPropertySheetMem&) = 0;
        virtual void OnOk     () = 0;
        virtual void OnCancel () = 0;
    };

    static void LoadSettingsManager ();
    static void ReloadTemplates ();
    static void SaveSettingsManager ();
    static bool ShowSettingsDialog (SettingsDialogCallback* = 0);
    static void ShowTemplateDialog (OpenEditor::TemplatePtr, int index, const string& = string());
    static const OpenEditor::SettingsManager& GetSettingsManager ();
    static void DestroySettingsManager ();

    OpenEditor::Storage& GetStorage ()               { return m_storage; }
    const OpenEditor::Settings& GetSettings () const { return m_settings; }
    OpenEditor::Settings& GetSettings ()             { return m_settings; }

    void SetClassSetting (const char* name)          { if (name) getSettingsManager().SetClassSettingsByLang(name, m_settings); }
    void DefaultFileFormat ()                        { m_storage.SetFileFormat((OpenEditor::EFileFormat)GetSettings().GetFileCreateAs(), false); }

    bool IsLocked  () const;
    virtual void Lock (bool readOnly);

    bool IsNewDocument () const                     { return m_strPathName.IsEmpty() ? true : false; }

    virtual void OnContextMenuInit (CMenu*)         {}

    // used in by OEWorkspaceManager
    void GetOrgFileTimeAndSize (__int64& fileTime, __int64& fileSize) const { fileTime = m_orgFileTime; fileSize = m_orgFileSize; }
    void SetOrgFileTimeAndSize (__int64 fileTime, __int64 fileSize)         { m_orgFileTime = fileTime; m_orgFileSize = fileSize; }

public:
    virtual BOOL IsModified();
    RecentFileList* GetRecentFileList();
    virtual BOOL OnNewDocument();
    virtual void SetTitle(LPCTSTR lpszTitle);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL SaveModified (); 
    void SetModifiedFlag (BOOL bModified = TRUE);
    virtual BOOL DoFileSave();
    virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
    virtual void UpdateTitle ();
    virtual BOOL OnIdle(LONG);
    virtual void OnTimer (UINT /*nIDEvent*/) {};
    virtual void OnCloseDocument ();

protected:
    BOOL doSave  (LPCTSTR lpszPathName, BOOL bReplace, BOOL bMove);
    BOOL doSave2 (LPCTSTR lpszPathName, BOOL bReplace = TRUE, BOOL bMove = FALSE);
    //{{AFX_MSG(COEDocument)
    afx_msg void OnEditPrintPageSetup();
    afx_msg void OnEditFileSettings();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnFileReload();
    afx_msg void OnUpdate_FileReload(CCmdUI *pCmdUI);
    afx_msg void OnUpdate_FileLocation(CCmdUI *pCmdUI);
    afx_msg void OnUpdate_FileType  (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_FileLines (CCmdUI* pCmdUI);

    afx_msg void OnEditViewLineNumbers ();
    afx_msg void OnUpdate_EditViewLineNumbers (CCmdUI *pCmdUI);
    afx_msg void OnEditViewSyntaxGutter ();
    afx_msg void OnUpdate_EditViewSyntaxGutter (CCmdUI *pCmdUI);
    afx_msg void OnEditViewWhiteSpace ();
    afx_msg void OnUpdate_EditViewWhiteSpace (CCmdUI* pCmdUI);
    afx_msg void OnEditViewRuler ();
    afx_msg void OnUpdate_EditViewRuler (CCmdUI* pCmdUI);
    afx_msg void OnEditViewColumnMarkers ();
    afx_msg void OnUpdate_EditViewColumnMarkers (CCmdUI* pCmdUI);
    afx_msg void OnEditViewIndentGuide ();
    afx_msg void OnUpdate_EditViewIndentGuide (CCmdUI* pCmdUI);

    afx_msg void OnFileOpen ();

    afx_msg void OnFileCloseOthers ();
    afx_msg void OnUpdate_FileCloseOthers (CCmdUI* pCmdUI);
    afx_msg void OnFileRename ();
    afx_msg void OnFileCopyLocation();
    afx_msg void OnUpdate_FileCopyLocation (CCmdUI* pCmdUI);

    afx_msg void OneditFileFormatWinows ();
    afx_msg void OneditFileFormatUnix   ();
    afx_msg void OneditFileFormatMac    ();
    afx_msg void OnUpdate_FileFormat (CCmdUI* pCmdUI);
};

inline
    LPCVOID COEDocument::GetVMData () const { return m_vmdata; }

inline
    OpenEditor::SettingsManager& COEDocument::getSettingsManager ()
    {
        if (!m_pSettingsManager) 
            m_pSettingsManager = new OpenEditor::SettingsManager;
        return *m_pSettingsManager;
    }

inline
    void COEDocument::DestroySettingsManager ()
    {
        if (m_pSettingsManager) 
        {
            delete m_pSettingsManager;
            m_pSettingsManager = 0;
        }
    }

inline
    const OpenEditor::SettingsManager& 
        COEDocument::GetSettingsManager () { return getSettingsManager(); }

inline
    bool COEDocument::IsLocked  () const
    {
        return m_storage.IsLocked();
    }

inline
    void COEDocument::Lock (bool lock)
    {
        m_storage.Lock(lock);
    }



//{{AFX_INSERT_LOCATION}}

#endif//__OEDocument_h__
