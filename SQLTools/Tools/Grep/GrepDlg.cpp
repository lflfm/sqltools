/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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
#include <COMMON\AppUtilities.h>
#include "SQLTools.h"
#include "GrepDlg.h"
#include "GrepView.h"
#include "GrepThread.h"
#include "COMMON\DirSelectDlg.h"
#include "GrepImplementation.h"
#include "OpenEditor/OEView.h"
#include "Common/MyUtf.h"
#include "Common/StrHelpers.h"
#include "OpenEditor/OEWorkspaceManager.h"
#include "OpenEditor/OEDocument.h"
#include "Common/ConfigFilesLocator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using namespace Common;
    using namespace GrepImplementation;

    static WINDOWPLACEMENT g_lastWindowPlacement = {};

CGrepDlg::CGrepDlg (CWnd* pParent, CGrepView* pResultView, COEditorView* pEditorView)
: CDialog(CGrepDlg::IDD, pParent),
  m_pResultView(pResultView),
  m_pEditorView(pEditorView)
{
    if (m_pResultView->IsRunning())
    {
        if (AfxMessageBox(L"Abort the running \"Find in Files\" process?", MB_ICONQUESTION|MB_YESNO) == IDYES)
            m_pResultView->AbortProcess();
        else
            AfxThrowUserException();
    }

    m_ShareSearchContext   = AfxGetApp()->GetProfileInt(L"Grep", L"ShareSearchContext", TRUE);
    if (m_ShareSearchContext)
    {
        m_MatchWholeWord       = AfxGetApp()->GetProfileInt(L"Editor", L"SearchMatchWholeWord", FALSE);
        m_MatchCase            = AfxGetApp()->GetProfileInt(L"Editor", L"SearchMatchCase",      FALSE);
        m_RegExp               = AfxGetApp()->GetProfileInt(L"Editor", L"SearchRegExp",         FALSE);
        m_BackslashExpressions = AfxGetApp()->GetProfileInt(L"Editor", L"BackslashExpressions", FALSE);
    }
    else
    {
        m_MatchWholeWord       = AfxGetApp()->GetProfileInt(L"Grep", L"MatchWholeWord",       FALSE);
        m_MatchCase            = AfxGetApp()->GetProfileInt(L"Grep", L"MatchCase",            FALSE);
        m_RegExp               = AfxGetApp()->GetProfileInt(L"Grep", L"UseRegExp",            FALSE);
        m_BackslashExpressions = AfxGetApp()->GetProfileInt(L"Grep", L"BackslashExpressions", FALSE);
    }

    m_LookInSubfolders = AfxGetApp()->GetProfileInt(L"Grep", L"LookInSubfolders", TRUE);
    m_SearchInMemory   = AfxGetApp()->GetProfileInt(L"Grep", L"SearchInMemory",   TRUE);
    m_CollapsedList    = AfxGetApp()->GetProfileInt(L"Grep", L"CollapsedList",    FALSE);

    m_workspaceFolder = ::PathFindFileName(OEWorkspaceManager::Get().GetWorkspacePath().c_str());
    m_backupFolder    = Common::wstr(COEDocument::GetSettingsManager().GetGlobalSettings()->GetFileBackupDirectoryV2());
    if (m_backupFolder.empty())
    {
        CString path;
        ::PathCombine(path.GetBuffer(MAX_PATH), ConfigFilesLocator::GetBaseFolder().c_str(), L"Backup");
        path.ReleaseBuffer();
        m_backupFolder = path;
    }

    if (m_pEditorView)
    {
        CString path = m_pEditorView->GetDocument()->GetPathName();
        LPCTSTR fname = ::PathFindFileName((LPTSTR)(LPCTSTR)path);
        if (fname != (LPCTSTR)path)
            m_currentFolder = std::wstring((LPCTSTR)path, fname - (LPCTSTR)path); 

        std::wstring text;
        OpenEditor::Square sqr;
        m_pEditorView->GetBlockOrWordUnderCursor(text, sqr, true/*onlyOneLine*/);

        if (m_BackslashExpressions)
        {
            string buff;
            Common::to_printable_str(Common::str(text).c_str(), buff);
            m_SearchText = Common::wstr(buff).c_str();
        }
        else
            m_SearchText = text.c_str();
    }
}

void CGrepDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_FF_SHARE_SEARCH_CONTEXT, m_ShareSearchContext);
    DDX_Check(pDX, IDC_FF_MATH_WHOLE_WORD, m_MatchWholeWord);
    DDX_Check(pDX, IDC_FF_MATH_CASE, m_MatchCase);
    DDX_Check(pDX, IDC_FF_REG_EXPR, m_RegExp);
    DDX_Check(pDX, IDC_FF_TRANSFORM_BACKSLASH_EXPR, m_BackslashExpressions);
    DDX_Check(pDX, IDC_FF_LOCK_IN_SUBFOLDERS, m_LookInSubfolders);
    DDX_Check(pDX, IDC_FF_SEARCH_IN_MEMORY, m_SearchInMemory);
    DDX_Check(pDX, IDC_FF_COLAPSED_LIST, m_CollapsedList);
    DDX_CBString(pDX, IDC_FF_FILE_OR_TYPE, m_MaskList);
    DDX_CBString(pDX, IDC_FF_FOLDER, m_FolderList);
    DDX_CBString(pDX, IDC_FF_TEXT, m_SearchText);
    DDX_Control(pDX, IDC_FF_REGEXP_FIND, m_wndInsertExp);
    DDX_Control(pDX, IDC_FF_FOLDER, m_wndFolderList);
    DDX_Control(pDX, IDC_FF_FILE_OR_TYPE, m_wndMaskList);
    DDX_Control(pDX, IDC_FF_TEXT, m_wndSearchText);
}

BEGIN_MESSAGE_MAP(CGrepDlg, CDialog)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_FF_REG_EXPR, OnBnClicked_RegExp)
    ON_BN_CLICKED(IDC_FF_SELECT_FOLDER, OnSelectFolder)
    ON_BN_CLICKED(IDC_FF_REGEXP_FIND, &CGrepDlg::OnBnClicked_InsertExpr)
    ON_COMMAND_RANGE(ID_REGEXP_FIND_TAB, ID_REGEXP_FIND_QUOTED, OnInsertRegexpFind)
    ON_COMMAND_RANGE(ID_FF_DIR_FIRST, ID_FF_DIR_LAST, OnChangeFolder)
    ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGrepDlg message handlers

BOOL CGrepDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    AppRestoreHistory(m_wndSearchText, L"Grep", L"TextHistory",       7);
    AppRestoreHistory(m_wndFolderList, L"Grep", L"FolderListHistory", 7);
    AppRestoreHistory(m_wndMaskList,   L"Grep", L"MaskListHistory",   7);

    if (!m_SearchText.IsEmpty())
    {
        m_wndSearchText.SetWindowText(m_SearchText);
    }

    m_wndMaskList.GetWindowText(m_MaskList);
    if (m_MaskList.IsEmpty())
    {
        m_MaskList = L"*.sql,*.txt";
        m_wndMaskList.SetWindowText(m_MaskList);
    }
    if (m_wndMaskList.GetCount() == 0)
    {
        m_wndMaskList.AddString(L"*.sql,*.txt");
        m_wndMaskList.AddString(L"*.*");
    }

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_FF_REGEXP_FIND), m_RegExp);

    if (!m_edtSearchText.SubclassComboBoxEdit(m_wndSearchText.m_hWnd))
        // hide button for Win95
        ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_FF_REGEXP_FIND), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

    if (m_hWnd && g_lastWindowPlacement.length > 0)
        SetWindowPlacement((WINDOWPLACEMENT*)&g_lastWindowPlacement);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

#include <string>
#include <algorithm>

    inline
    wchar_t to_upper (wchar_t ch)
    { 
        return (wchar_t)::toupper((int)ch); 
    }
 
    static
    size_t find_ci (std::wstring str, std::wstring what, size_t pos = 0)
    {
        std::transform(str.begin(), str.end(), str.begin(), to_upper);
        std::transform(what.begin(), what.end(), what.begin(), to_upper);
        return str.find(what, pos);
    }

    static
    void search_n_replace (std::wstring& str, const std::wstring& what, const std::wstring& to)
    {
      std::wstring::size_type pos = 0u;
      while((pos = find_ci(str, what, pos)) != std::wstring::npos)
      {
         str.replace(pos, what.length(), to);
         pos += to.length();
      }
    }
    
void CGrepDlg::OnOK ()
{
    UpdateData();

    if (m_pEditorView)
    {
        std::wstring text;

        if (m_BackslashExpressions)
        {
            string buff;
            Common::to_unprintable_str(Common::str(m_SearchText).c_str(), buff);
            text = Common::wstr(buff);
        }
        else
            text = m_SearchText;

        m_pEditorView->SetSearchText(text.c_str());
        m_pEditorView->SetSearchOption(
            false, // forfard
            m_MatchWholeWord ? true : false,
            m_MatchCase      ? true : false,
            m_RegExp         ? true : false,
            AfxGetApp()->GetProfileInt(L"Editor", L"SearchAllWindows",  FALSE) ? true : false
        );
    }

    AfxGetApp()->WriteProfileInt(L"Grep", L"ShareSearchContext",   m_ShareSearchContext    );
    AfxGetApp()->WriteProfileInt(L"Grep", L"MatchWholeWord",       m_MatchWholeWord         );
    AfxGetApp()->WriteProfileInt(L"Grep", L"MatchCase",            m_MatchCase              );
    AfxGetApp()->WriteProfileInt(L"Grep", L"UseRegExp",            m_RegExp                 );
    AfxGetApp()->WriteProfileInt(L"Grep", L"BackslashExpressions", m_BackslashExpressions   );
    AfxGetApp()->WriteProfileInt(L"Grep", L"LookInSubfolders",     m_LookInSubfolders       );
    AfxGetApp()->WriteProfileInt(L"Grep", L"SearchInMemory",       m_SearchInMemory         );
    AfxGetApp()->WriteProfileInt(L"Grep", L"CollapsedList",        m_CollapsedList          );

    if (m_ShareSearchContext)
    {
        AfxGetApp()->WriteProfileInt(L"Editor", L"SearchMatchWholeWord", m_MatchWholeWord      );
        AfxGetApp()->WriteProfileInt(L"Editor", L"SearchMatchCase",      m_MatchCase           );
        AfxGetApp()->WriteProfileInt(L"Editor", L"SearchRegExp",         m_RegExp              );
        AfxGetApp()->WriteProfileInt(L"Editor", L"BackslashExpressions", m_BackslashExpressions);
    }

    AppSaveHistory(m_wndSearchText, L"Grep", L"TextHistory",       7);
    AppSaveHistory(m_wndFolderList, L"Grep", L"FolderListHistory", 7);
    AppSaveHistory(m_wndMaskList,   L"Grep", L"MaskListHistory",   7);

    SavePlacement();
    CDialog::OnOK();

    if (m_pResultView->IsRunning())
    {
        AfxMessageBox(L"The previous \"Find in Files\" process is still running. Please try later", MB_ICONERROR|MB_OK);
        m_pResultView->AbortProcess();
        AfxThrowUserException();
    }

    //m_finalFolderList.clear();
    //vector<wstring> list = make_list(wstring(m_FolderList), true);
    //for (auto it = list.begin(); it != list.end(); ++it)
    //    if (!it->empty())
    //    {
    //        wstring folder;
    //        if (!_wcsnicmp(L"<CUR_DIR>\\", it->c_str(), it->length()))
    //            folder = m_currentFolder;
    //        else if (!_wcsnicmp(L"<WKS_DIR>\\", it->c_str(), it->length()))
    //            folder = m_workspaceFolder;
    //        else if (!_wcsnicmp(L"<BKP_DIR>\\", it->c_str(), it->length()))
    //            folder = m_backupFolder;
    //        else
    //            folder = *it;
    //        
    //        if (!folder.empty())
    //        {
    //            m_finalFolderList += folder;
    //            if (*folder.rbegin() != '\\')
    //                m_finalFolderList += '\\';
    //            m_finalFolderList += ';';
    //        }
    //    }

    wstring openDirs;
    auto templateList = theApp.GetPLSDocTemplate();
    if (templateList)
    {
        POSITION pos = templateList->GetFirstDocPosition();
        while (pos != NULL)
        {
            auto pDoc = templateList->GetNextDoc(pos);
            if (pDoc)
            {
                auto path = pDoc->GetPathName();
                if (!path.IsEmpty())
                {
                    LPCWSTR buff = path;
                    LPCWSTR name = PathFindFileName(buff);
                    if (name != buff)
                    {
                        // ignore possible dupicates, they will be removed in make_list later
                        openDirs += wstring(buff, name-buff);
                        openDirs += L';';
                    }
                }
            }
        }
    }

    m_finalFolderList = m_FolderList;
    search_n_replace(m_finalFolderList, L"<CD>", L"<CURRENT_DIR>");
    search_n_replace(m_finalFolderList, L"<CURRENT_DIR>", !m_currentFolder.empty() ? m_currentFolder : m_workspaceFolder);
    search_n_replace(m_finalFolderList, L"<WD>", L"<WORKSTACE_DIR>");
    search_n_replace(m_finalFolderList, L"<WORKSTACE_DIR>", m_workspaceFolder);
    search_n_replace(m_finalFolderList, L"<BD>", L"<BACKUP_DIR>");
    search_n_replace(m_finalFolderList, L"<BACKUP_DIR>", m_backupFolder);
    search_n_replace(m_finalFolderList, L"<OD>", L"<OPEN_DIRS>");
    search_n_replace(m_finalFolderList, L"<OPEN_DIRS>", openDirs);

    CGrepThread* pThread =
        (CGrepThread*)AfxBeginThread(RUNTIME_CLASS(CGrepThread), THREAD_PRIORITY_ABOVE_NORMAL);

    if (pThread)
        pThread->RunGrep(this, m_pResultView);
}

void CGrepDlg::OnCancel ()
{
    SavePlacement();
    CDialog::OnCancel();
}

void CGrepDlg::OnClose()
{
    SavePlacement();
    CDialog::OnClose();
}

void CGrepDlg::SavePlacement ()
{
    g_lastWindowPlacement.length = sizeof g_lastWindowPlacement;
    if (GetWindowPlacement(&g_lastWindowPlacement))
    {
        if (g_lastWindowPlacement.showCmd == SW_MINIMIZE)
        {
            g_lastWindowPlacement.showCmd = SW_SHOWNORMAL;
            g_lastWindowPlacement.flags   = 0;
        }
    }
}

void CGrepDlg::OnSelectFolder () 
{
    CMenu menu;
    menu.CreatePopupMenu();

    UpdateData();
    m_wndFolderList.GetWindowText(m_FolderList);
    vector<wstring> list = make_list(wstring(m_FolderList), true);

    if (!list.empty())
    {
        ASSERT(ID_FF_DIR_LAST-ID_FF_DIR_FIRST == 9);

        menu.AppendMenu(MF_STRING, ID_FF_DIR_FIRST + 9, L"Replace all with ..." );

        menu.AppendMenu(MF_SEPARATOR);

        if (list.size() > 1)
        {
            // replace items
            int index = 0;
            for (auto it = list.begin(); it != list.end() && index < 4; ++it, ++index)
            {
                wstring text = L"Replace \"" + *it + L"\" with ...";
                menu.AppendMenu(MF_STRING, ID_FF_DIR_FIRST + index, text.c_str());
            }

            menu.AppendMenu(MF_SEPARATOR);

            index = 4;
            for (auto it = list.begin(); it != list.end() && index <= 8; ++it, ++index)
            {
                wstring text = L"Remove \"" + *it + L"\"";
                menu.AppendMenu(MF_STRING, ID_FF_DIR_FIRST + index, text.c_str());
            }

            menu.AppendMenu(MF_SEPARATOR);
        }
    }

    menu.AppendMenu(MF_STRING, ID_FF_DIR_FIRST + 8, L"Append a folder...");

    ShowPopupMenu(IDC_FF_SELECT_FOLDER, &menu);
}

static
bool chooseFolder (CWnd* parent, wstring& folder)
{
    // alternativly MFC CFolderPickerDialog can be used here
    CDirSelectDlg dirDlg(L"Look In", parent, folder.c_str());

    if (dirDlg.DoModal() == IDOK) 
    {
        CString buffer;
        dirDlg.GetPath(buffer);
        folder = buffer;
        return true;
    }
    return false;
}

void CGrepDlg::OnChangeFolder (UINT nID)
{
    m_wndFolderList.GetWindowText(m_FolderList);
    vector<wstring> list = make_list(wstring(m_FolderList), true);

    wstring folder;
    int index, command = nID - ID_FF_DIR_FIRST;
    switch (command)
    {
    case 9: // replace all
        if (!list.empty())
            folder = *list.begin();
        if (!chooseFolder(this, folder))
            return;
        list.clear();
        list.push_back(folder);
        break;
    case 0: // replace (command-1)
    case 1:
    case 2:
    case 3:
        index = command;
        folder = list.at(index);
        if (!chooseFolder(this, folder))
            return;
        list.at(index) = folder;
        break;
    case 4: // remove (command-1)
    case 5:
    case 6:
    case 7:
        index = command - 4;
        list.erase(list.begin() + index);
        break;
    case 8: // append
        if (!list.empty())
            folder = *list.rbegin();
        if (!chooseFolder(this, folder))
            return;
        list.push_back(folder);
        break;
    }
    folder.clear();
    for (auto it = list.begin(); it != list.end(); ++it)
        if (!it->empty())
        {
            folder += *it;
            if (*it->rbegin() != '\\')
                folder += '\\';
            folder += ';';
        }
    m_FolderList = folder.c_str();
    m_wndFolderList.SetWindowText(m_FolderList);
    m_wndFolderList.SetFocus();
}

LRESULT CGrepDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}


void CGrepDlg::OnBnClicked_RegExp ()
{
    UpdateData();
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_FF_REGEXP_FIND), m_RegExp);
}

void CGrepDlg::OnBnClicked_InsertExpr ()
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_OE_REGEXP_FIND_POPUP));

    CMenu* pPopup = menu.GetSubMenu(0);
    _ASSERTE(pPopup != NULL);

    ShowPopupMenu(IDC_FF_REGEXP_FIND, pPopup);
}

void CGrepDlg::ShowPopupMenu (UINT btnId, CMenu* pPopup)
{
    HWND hButton = ::GetDlgItem(m_hWnd, btnId);

    _ASSERTE(hButton);

    if (hButton
    && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED)
    {
        ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

        CRect rect;
        ::GetWindowRect(hButton, &rect);

        TPMPARAMS param;
        param.cbSize = sizeof param;
        param.rcExclude.left   = 0;
        param.rcExclude.top    = rect.top;
        param.rcExclude.right  = USHRT_MAX;
        param.rcExclude.bottom = rect.bottom;

        if (::TrackPopupMenuEx(*pPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               rect.left, rect.bottom + 1, *this, &param))
        {
              MSG msg;
              ::PeekMessage(&msg, hButton, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
        }

        ::PostMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0L);
    }
}

void CGrepDlg::OnInsertRegexpFind (UINT nID)
{
    if (nID > ID_REGEXP_FIND_TAB)
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EF_REGEXP), BM_SETCHECK, BST_CHECKED, 0L);

    static
    struct { const wchar_t* text; int offset; }
        expr[] = {
            { L"\\t", -1 },
            { L".",  -1 },
            { L"*",  -1 },
            { L"+",  -1 },
            { L"^",  -1 },
            { L"$",  -1 },
            { L"[]",  1 },
            { L"[^]", 2 },
            { L"|",  -1 },
            { L"\\", -1 },
            { L"([a-zA-Z0-9])",              -1 },
            { L"([a-zA-Z])",                 -1 },
            { L"([0-9])",                    -1 },
            { L"([0-9a-fA-F]+)",             -1 },
            { L"([a-zA-Z_$][a-zA-Z0-9_$]*)", -1 },
            { L"(([0-9]+\\.[0-9]*)|([0-9]*\\.[0-9]+)|([0-9]+))", -1 },
            { L"((\"[^\"]*\")|('[^']*'))",   -1 },
        };

    _ASSERTE((nID - ID_REGEXP_FIND_TAB) < (sizeof(expr)/sizeof(expr[0])));

    if (((nID - ID_REGEXP_FIND_TAB) < (sizeof(expr)/sizeof(expr[0]))))
    {
        m_edtSearchText.InsertAtCurPos(expr[nID - ID_REGEXP_FIND_TAB].text, expr[nID - ID_REGEXP_FIND_TAB].offset);
    }
}

void CGrepDlg::OnHelp ()
{
    SendMessage(WM_COMMANDHELP);
}

