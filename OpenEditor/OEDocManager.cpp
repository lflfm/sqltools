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

/*
    23.03.2003 improvement, save files dialog appears when the program is closing
    31.03.2003 bug fix, the programm fails on attempt to open multiple file selection in Open dialog
    31.03.2003 bug fix, "Save all" command is not silent since "Save all on exit" dialog was created
    26.05.2003 bug fix, no file name on "Save As" or "Save" for a new file
    17.01.2005 (ak) simplified settings class hierarchy 
    2016.06.28 improvement, added open Workspace
*/

#include "stdafx.h"
#include "OpenEditor/OEDocManager.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OESaveModifiedDlg.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OENewFileDlg.h"
#include <Common/AppUtilities.h>
#include "OpenEditor/RecentFileList.h"
#include "OpenEditor/OEWorkspaceManager.h"



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

    using namespace OpenEditor;

static 
bool convert_extensions (const string& extensions, string& ofn_extensions, const char* def_ext)
{
    bool retVal = false;

    string buffer;
    string::const_iterator it = extensions.begin();
    for (; it != extensions.end(); ++it)
    {
        if (isspace(*it) || *it == ',' && !buffer.empty())
        {
            if (def_ext && !stricmp(buffer.c_str(), def_ext)) 
                retVal = true;

            if (!ofn_extensions.empty()) 
                ofn_extensions += ';';
            ofn_extensions += "*." + buffer;
            buffer.erase();
        }
        else
            buffer += *it;
    }
    if (!buffer.empty())
    {
        if (def_ext && !stricmp(buffer.c_str(), def_ext)) 
            retVal = true;

        if (!ofn_extensions.empty()) 
            ofn_extensions += ';';
        ofn_extensions += "*." + buffer;
        buffer.erase();
    }

    return retVal;
}

static 
void make_filter (CString& filter, OPENFILENAME& ofn, const char* def_ext)
{
	// append the "*.*" all files filter
	filter += "All Files (*.*)";
	filter += (TCHAR)'\0';   // next string please
	filter += _T("*.*");
	filter += (TCHAR)'\0';   // last string
    ofn.nFilterIndex = 1;

    const SettingsManager& settingMgrl = COEDocument::GetSettingsManager();
    for (int i(0), count(settingMgrl.GetClassCount()); i < count; i++)
    {
        const ClassSettingsPtr classSettings = settingMgrl.GetClassByPos(i);
        const string& extensions = classSettings->GetExtensions();
        const string& name = classSettings->GetName();

        string ofn_extensions;
        
        if (convert_extensions(extensions, ofn_extensions, def_ext))
            ofn.nFilterIndex = i + 2; // this index is 1 based + 1 for "All files"

        filter += name.c_str();
        filter += " Files (";
        filter += ofn_extensions.c_str();
        filter += ')';
        filter += '\0';
        filter += ofn_extensions.c_str();
        filter += '\0';
    }

    filter += '\0'; // close
	ofn.lpstrFilter = filter;
}

IMPLEMENT_DYNAMIC(COEDocManager, CDocManager)

COEDocManager::COEDocManager ()
{
    m_nFilterIndex = 0;
}

BOOL COEDocManager::doPromptFileName (
    LPCSTR fromDir, CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* /*pTemplate*/
)
{
    // it would be better to append a class default extension (for example .sql for pl/sql class)
    // but it's not available right here so extract an extension from fileName and use it as default
    LPCSTR defExt = ::PathFindExtension(fileName);
    if (defExt && *defExt == '.') defExt++;

   CFileDialog dlgFile(bOpenFileDialog, NULL, NULL,
        (bOpenFileDialog ? OFN_ALLOWMULTISELECT : 0)
            | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT/* | OFN_EXPLORER*/ | OFN_ENABLESIZING | OFN_NOCHANGEDIR,
        NULL, NULL, 0);

	CString title;
	VERIFY(title.LoadString(nIDSTitle));

	dlgFile.m_ofn.Flags |= lFlags;

	CString strFilter;
    make_filter(strFilter, dlgFile.m_ofn, defExt);

	dlgFile.m_ofn.lpstrTitle = title;
    
    if (bOpenFileDialog)
        dlgFile.m_ofn.nFilterIndex = m_nFilterIndex;

// 31.03.2003 bug fix, the programm fails on attempt to open multiple file selection in Open dialog
#if _MAX_PATH*256 < 64*1024-1
#define PATH_BUFFER _MAX_PATH*256
#else
#define PATH_BUFFER 64*1024-1
#endif
    dlgFile.m_ofn.nMaxFile = PATH_BUFFER;
	dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(dlgFile.m_ofn.nMaxFile);
    //dlgFile.m_ofn.lpstrFile[0] = 0; // 26.05.2003 bug fix, no file name on "Save As" or "Save" for a new file
	
    // overwrite initial dir
    dlgFile.m_ofn.lpstrInitialDir = fromDir;

    INT_PTR nResult = dlgFile.DoModal();
    
    if (bOpenFileDialog)
        m_nFilterIndex = dlgFile.m_ofn.nFilterIndex;

	fileName.ReleaseBuffer();

	return nResult == IDOK;
}

BOOL COEDocManager::DoPromptFileName (
    CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate
    )
{
    return doPromptFileName(NULL, fileName, nIDSTitle, lFlags, bOpenFileDialog, pTemplate);
}

void COEDocManager::doFileOpen (LPCSTR initialDir)
{
	// prompt the user (with all document templates)
	CString newName;
	
    if (!doPromptFileName(initialDir, newName, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, NULL))
		return; // open cancelled

    const char* pszBuffer = newName.LockBuffer();
    const char* pszName = strchr(pszBuffer, 0);
  
    _ASSERTE(pszName);

    if (*(++pszName) == 0)
	    AfxGetApp()->OpenDocumentFile(newName);
    else 
    {
        do 
        {
            CString strFileName(pszBuffer);
   	        OpenDocumentFile(strFileName + '\\' + pszName);
            pszName = strchr(pszName, 0);
        } 
        while (pszName && (*(++pszName) != 0));
    }
}

/*
  overwritten to support multiselection
*/
void COEDocManager::OnFileOpen ()
{
    doFileOpen(NULL);
}

CDocument* COEDocManager::OpenDocumentFile (LPCTSTR lpszPath)
{
    if (!(!lpszPath || ::PathFileExists(lpszPath))) // added to fix not removing dead entries from recent file list
        return NULL;

    if (lpszPath
    && !Common::AppIsFolder(lpszPath, false))
    {
        if (const char* ext = ::PathFindExtension(lpszPath))
        {
            if (!stricmp(OEWorkspaceManager::Get().GetWorkspaceExtension(), ext))
            {
                OEWorkspaceManager::Get().WorkspaceOpen(lpszPath, false);
                return NULL;
            }
            else if (!stricmp(OEWorkspaceManager::Get().GetSnapshotExtension(), ext))
            {
                OEWorkspaceManager::Get().WorkspaceOpen(lpszPath, true);
                return NULL;
            }
        }

        return CDocManager::OpenDocumentFile(lpszPath);
    }

    doFileOpen(lpszPath);
    return NULL;
}

BOOL COEDocManager::SaveAllModified ()
{
    AfxMessageBox("COEDocManager::SaveAllModified is obsolete. \n\nPlease notify the developer!", MB_OK|MB_ICONERROR);
    return FALSE;
}

BOOL COEDocManager::SaveAllModified (bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit)
{
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
		ASSERT_KINDOF(CDocTemplate, pTemplate);

        if (pTemplate->IsKindOf(RUNTIME_CLASS(COEMultiDocTemplate)))
        {
		    if (!((COEMultiDocTemplate*)pTemplate)->SaveAllModified(skipNew, saveSilently, canUseWorksapce, onExit))
			    return FALSE;
        }
        else
        {
		    if (!pTemplate->SaveAllModified())
			    return FALSE;
        }
	}
	return TRUE;
}

BOOL COEDocManager::SaveAndCloseAllDocuments ()
{
    BOOL retVal = TRUE;
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
		
        ASSERT_KINDOF(CDocTemplate, pTemplate);
		
        if (pTemplate->IsKindOf(RUNTIME_CLASS(COEMultiDocTemplate)))
        {
            if (!((COEMultiDocTemplate*)(pTemplate))->SaveAndCloseAllDocuments(NULL))
                retVal = FALSE;
        }
        else
        {
            if (pTemplate->SaveAllModified())
		        pTemplate->CloseAllDocuments(FALSE);
            else
                retVal = FALSE;
        }
	}
	return retVal;
}


IMPLEMENT_DYNAMIC(COEMultiDocTemplate, CMultiDocTemplate)

COEMultiDocTemplate::COEMultiDocTemplate (UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass, counted_ptr<RecentFileList>& recentFileList)
: CMultiDocTemplate (nIDResource, pDocClass, pFrameClass, pViewClass), 
  m_pickDocType(FALSE),
  m_disableEditStateRestoring(FALSE),
  m_pRecentFileList(recentFileList)
{
}

CDocument* COEMultiDocTemplate::OpenWorkspaceFile (LPCTSTR lpszPathName)
{
    DisableEditStateRestoring disabler(*this);
    return OpenDocumentFile(lpszPathName, TRUE);
}

CDocument* COEMultiDocTemplate::OpenDocumentFile (LPCTSTR lpszPathName, BOOL bMakeVisible)
{
    if (lpszPathName)
        return CMultiDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);

    CDocument* pDocument = NULL;

    try { EXCEPTION_FRAME;

        if (m_pickDocType)
        {
            COENewFileDlg dlg(*this);
            
            if (dlg.DoModal() != IDOK)
                return NULL;

            m_filename = dlg.GetFilename();
            m_category = dlg.GetCategory();

            bool modified = false;
            GlobalSettingsPtr globalSerringsPtr = COEDocument::GetSettingsManager().GetGlobalSettings();
            if (globalSerringsPtr->GetDefaultClass() != m_category)
            {
                globalSerringsPtr->SetDefaultClass(m_category);
                modified = true;
            }

            OpenEditor::ClassSettings& classSettings = COEDocument::GetSettingsManager().GetByName(m_category);
            if (classSettings.GetFilenameTemplate() != dlg.GetTemplate())
            {
                classSettings.SetFilenameTemplate(dlg.GetTemplate());
                modified = true;
            }

            if (modified)
                COEDocument::SaveSettingsManager();
        }
        else
        {
            m_category = COEDocument::GetSettingsManager().GetGlobalSettings()->GetDefaultClass();
        }

        pDocument = CMultiDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);
    
        m_filename.clear();
        m_category.clear();
    }
    _OE_DEFAULT_HANDLER_;

    return pDocument;
}

CDocument* COEMultiDocTemplate::OpenDocumentFile (LPCTSTR lpszPathName, BOOL bAddToMRU, BOOL bMakeVisible)
{
    CDocument* pDocument = __super::OpenDocumentFile(lpszPathName, bAddToMRU, bMakeVisible);

    if (pDocument)
    {
        try { EXCEPTION_FRAME;
            if (m_pRecentFileList.get())
                m_pRecentFileList->OnOpenDocument(dynamic_cast<COEDocument*>(pDocument), !m_disableEditStateRestoring);
        }
        _OE_DEFAULT_HANDLER_;
    }

    return pDocument;
}

void COEMultiDocTemplate::SetDefaultTitle (CDocument* pDocument) 
{
    try { EXCEPTION_FRAME;
    
        ASSERT_KINDOF(COEDocument, pDocument);

        if (COEDocument* pOEDocument = DYNAMIC_DOWNCAST(COEDocument, pDocument))
        {
            pOEDocument->SetClassSetting(m_category.c_str());
            if (m_filename.empty())
                FormatTitle(pOEDocument->GetSettings().GetFilenameTemplate().c_str(), m_filename);
            pOEDocument->SetTitle(m_filename.c_str());
        }
        else
            CMultiDocTemplate::SetDefaultTitle(pDocument);
    }
    _OE_DEFAULT_HANDLER_;
}

BOOL COEMultiDocTemplate::SaveAllModified ()
{
    AfxMessageBox("COEMultiDocTemplate::SaveAllModified is obsolete. \n\nPlease notify the developer!", MB_OK|MB_ICONERROR);
    return FALSE;
}

/*
    we want to save all individal filese then canUseWorksapce is false ("Save All" command / autosave on task switching)
*/
BOOL COEMultiDocTemplate::SaveAllModified (bool skipNew, bool saveSilently, bool canUseWorksapce, bool onExit)
{
    if (onExit 
    && COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceFileSaveInActiveOnExit()
    && OEWorkspaceManager::Get().HasActiveWorkspace())
    {
        //TODO#2: consider saving workspace even there is no mofified document, just for editor state
        OEWorkspaceManager::Get().WorkspaceSave(false);
    }

    if (canUseWorksapce
    && OEWorkspaceManager::Get().HasActiveWorkspace())
    {
        WorkspaceState lastState;
        GetWorkspaceState(lastState);
        if (WorkspaceStateEqual(OEWorkspaceManager::Get().GetActiveWorkspaceSavedState(), lastState))
            return TRUE;
    }

    // 31.03.2003 bug fix, "Save all" command is not silent since "Save all on exit" dialog was created
    if (onExit || !saveSilently)
    {
        BOOL failed = FALSE;

        DocSavingList saveList;

	    POSITION pos = GetFirstDocPosition();
    	
        while (pos != NULL)
	    {
		    CDocument* pDoc = GetNextDoc(pos);
    		
            if (pDoc->IsModified())
                saveList.push_back(std::pair<CDocument*, bool>(pDoc, true));
	    }

        if (saveList.size())
        {
            BOOL thereIsActiveWorkspace =  OEWorkspaceManager::Get().HasActiveWorkspace();

            BOOL quickSaveInWorkspace = AfxGetApp()->GetProfileInt("SaveModifiedDlg", "quickSaveInWorkspace",  FALSE);

            switch (COESaveModifiedDlg(saveList, quickSaveInWorkspace, thereIsActiveWorkspace).DoModal())
            {
            case IDOK:
			    {
                    AfxGetApp()->WriteProfileInt("SaveModifiedDlg", "quickSaveInWorkspace",  quickSaveInWorkspace);

                    if (!quickSaveInWorkspace)
                    {
				        for (DocSavingListConst_Iterator it = saveList.begin(); it != saveList.end(); ++it)
					        if (it->second) 
						        failed |= !it->first->DoFileSave();

                        if (thereIsActiveWorkspace)
                            OEWorkspaceManager::Get().WorkspaceSave(false);
                    }
                    else
                    {
                        if (thereIsActiveWorkspace)
                            OEWorkspaceManager::Get().WorkspaceSave(false);
                        else
                            OEWorkspaceManager::Get().WorkspaceSaveQuick();
                    }
			    }
                break;
            case IDCANCEL:
                failed = TRUE;
            }
        }

        return !failed;
    }
    else // it is saveSilently
    {
	    POSITION pos = GetFirstDocPosition();
	    while (pos != NULL)
	    {
		    COEDocument* pDoc = (COEDocument*)GetNextDoc(pos);

            if (!pDoc->IsModified()) 
                continue;
        
            if (skipNew && pDoc->IsNewDocument())
                continue;

            if (saveSilently && !pDoc->IsNewDocument())
            {
                if (!pDoc->DoFileSave())
                    return FALSE;
            }
            else if (!pDoc->SaveModified())
                return FALSE;
	    }
    }
	return TRUE;
}

BOOL COEMultiDocTemplate::SaveAndCloseAllDocuments (const CDocument* pButThis)
{
    BOOL retVal = SaveAllModified(false/*skipNew*/, false/*saveSilently*/, true/*canUseWorksapce*/, false/*onExit*/);

    if (retVal)
    {
	    POSITION pos = GetFirstDocPosition();

	    while (pos != NULL)
        {
            CDocument* pDoc = GetNextDoc(pos);
            if (pButThis != pDoc)
		        pDoc->OnCloseDocument();
        }
    }

	return retVal;
}

BOOL COEMultiDocTemplate::OnIdle (LONG lCount, CFrameWnd* pFrameWnd)
{
    /*
        calling OnIdle only for an active document
        It might be changed later
    */
    if (!pFrameWnd) 
    {
		ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
        pFrameWnd = ((CFrameWnd*)AfxGetMainWnd());
    }

    if (CDocument* pActiveDoc = pFrameWnd->GetActiveDocument())
    {
		ASSERT_KINDOF(COEDocument, pActiveDoc);
        if (pActiveDoc->GetDocTemplate() == this)
        {
		    ASSERT_KINDOF(COEDocument, pActiveDoc);
            return ((COEDocument*)pActiveDoc)->OnIdle(lCount);
        }
    }

    return FALSE;

    //BOOL retVal = FALSE;

	//POSITION pos = GetFirstDocPosition();

	//while (pos != NULL)
	//{
	//	COEDocument* pDoc = (COEDocument*)GetNextDoc(pos);
	//	ASSERT_VALID(pDoc);
	//	ASSERT_KINDOF(COEDocument, pDoc);
	//	retVal |= pDoc->OnIdle(lCount);
	//}

    //return retVal;
}

void COEMultiDocTemplate::FormatTitle (const char* format, std::string& title) const
{
    Common::FilenameTemplate::Format(format, title, m_nUntitledCount);
}

void COEMultiDocTemplate::GetWorkspaceState (WorkspaceState& lastState)
{
    WorkspaceState state;

	POSITION pos = this->GetFirstDocPosition();
	while (pos != NULL)
    {
        CDocument* pDoc = this->GetNextDoc(pos);
        if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
        {
            unsigned int action = pOEDoc->GetStorage().GetActionId().ToUInt();
            if (action != Sequence::START_VAL) // skipping new emty documents
                state[pOEDoc] = action; 
        }
    }

    lastState.swap(state);
}

//static 
bool COEMultiDocTemplate::WorkspaceStateEqual (const WorkspaceState& previous, const WorkspaceState& current, bool ignoreClosed)
{
    if (!ignoreClosed)
    {
        return previous.size() == current.size()
            && std::equal(current.begin(), current.end(), previous.begin());
    }
    else
    {
        // the following comparison ingnores documens that were closed since the last backup
        std::map<COEDocument*, unsigned>::const_iterator it = current.begin();
        for (; it != current.end(); ++it)
        {
            std::map<COEDocument*, unsigned>::const_iterator mit = previous.find(it->first);
            if (mit == previous.end() || mit->second != it->second)
            {
                return false;
            }
        }
    }

    return true;
}
