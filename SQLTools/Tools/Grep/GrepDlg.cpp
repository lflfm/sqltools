/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace Common;

CGrepDlg::CGrepDlg(CWnd* pParent, CGrepView* pView)
: CDialog(CGrepDlg::IDD, pParent),
  m_pView(pView)
{
    ASSERT(m_pView);
    ASSERT(!m_pView->IsRunning());
    //{{AFX_DATA_INIT(CGrepDlg)
    //}}AFX_DATA_INIT
    m_bMathWholeWord    = AfxGetApp()->GetProfileInt("Grep", "MathWholeWord",    FALSE);
    m_bMathCase         = AfxGetApp()->GetProfileInt("Grep", "MathCase",         FALSE);
    m_bUseRegExpr       = AfxGetApp()->GetProfileInt("Grep", "UseRegExpr",       FALSE);
    m_bLookInSubfolders = AfxGetApp()->GetProfileInt("Grep", "LookInSubfolders", TRUE);
    m_bSaveFiles        = AfxGetApp()->GetProfileInt("Grep", "SaveFiles",        FALSE);
    m_bCollapsedList    = AfxGetApp()->GetProfileInt("Grep", "CollapsedList",    FALSE);
}

void CGrepDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGrepDlg)
	DDX_Check(pDX, IDC_FF_MATH_WHOLE_WORD, m_bMathWholeWord);
	DDX_Check(pDX, IDC_FF_MATH_CASE, m_bMathCase);
	DDX_Check(pDX, IDC_FF_REG_EXPR, m_bUseRegExpr);
	DDX_Check(pDX, IDC_FF_LOCK_IN_SUBFOLDERS, m_bLookInSubfolders);
	DDX_Check(pDX, IDC_FF_SAVE_FILES, m_bSaveFiles);
	DDX_Check(pDX, IDC_FF_COLAPSED_LIST, m_bCollapsedList);
	DDX_CBString(pDX, IDC_FF_FILE_OR_TYPE, m_strFileOrType);
	DDX_CBString(pDX, IDC_FF_FOLDER, m_strFolder);
	DDX_CBString(pDX, IDC_FF_TEXT, m_strWhatFind);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_FF_INSERT_EXPR, m_wndInsertExpr);
	DDX_Control(pDX, IDC_FF_FOLDER, m_wndFolder);
	DDX_Control(pDX, IDC_FF_FILE_OR_TYPE, m_wndFileOrType);
	DDX_Control(pDX, IDC_FF_TEXT, m_wndWhatFind);
}

BEGIN_MESSAGE_MAP(CGrepDlg, CDialog)
	//{{AFX_MSG_MAP(CGrepDlg)
	ON_BN_CLICKED(IDC_FF_SELECT_FOLDER, OnSelectFolder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGrepDlg message handlers

BOOL CGrepDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	AppRestoreHistory(m_wndWhatFind,   "Grep", "TextHistory",       7);
	AppRestoreHistory(m_wndFolder,     "Grep", "FolderHistory",     7);
	AppRestoreHistory(m_wndFileOrType, "Grep", "FileOrTypeHistory", 7);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGrepDlg::OnOK ()
{
    UpdateData();
    AfxGetApp()->WriteProfileInt("Grep", "MathWholeWord",    m_bMathWholeWord);
    AfxGetApp()->WriteProfileInt("Grep", "MathCase",         m_bMathCase);
    AfxGetApp()->WriteProfileInt("Grep", "UseRegExpr",       m_bUseRegExpr);
    AfxGetApp()->WriteProfileInt("Grep", "LookInSubfolders", m_bLookInSubfolders);
    AfxGetApp()->WriteProfileInt("Grep", "SaveFiles",        m_bSaveFiles);
    AfxGetApp()->WriteProfileInt("Grep", "CollapsedList",    m_bCollapsedList);

    AppSaveHistory(m_wndWhatFind,   "Grep", "TextHistory",       7);
    AppSaveHistory(m_wndFolder,     "Grep", "FolderHistory",     7);
    AppSaveHistory(m_wndFileOrType, "Grep", "FileOrTypeHistory", 7);

    CDialog::OnOK();

    CGrepThread* pThread =
        (CGrepThread*)AfxBeginThread(RUNTIME_CLASS(CGrepThread), THREAD_PRIORITY_ABOVE_NORMAL);

    if (pThread)
        pThread->RunGrep(this, m_pView);
}


void CGrepDlg::OnSelectFolder() 
{
    m_wndFolder.GetWindowText(m_strFolder);
    CDirSelectDlg dirDlg("Look In", this, m_strFolder);

    if (dirDlg.DoModal() == IDOK) 
	{
        dirDlg.GetPath(m_strFolder);
		m_wndFolder.SetWindowText(m_strFolder);
		m_wndFolder.SetEditSel(0, -1);
    }
    m_wndFolder.SetFocus();
}

LRESULT CGrepDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
