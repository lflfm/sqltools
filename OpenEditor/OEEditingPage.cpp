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

// 29.06.2003 improvement, "Restrict cursor" has been replaced with "Cursor beyond EOL" and "Cursor beyond EOF"
// 17.01.2005 (ak) simplified settings class hierarchy 

#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEEditingPage.h"
#include "Common/MyUtf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using OpenEditor::GlobalSettings;

/////////////////////////////////////////////////////////////////////////////
// COEEditingPage property page

COEEditingPage::COEEditingPage(SettingsManager& manager)
: CPropertyPage(COEEditingPage::IDD),
  m_manager(manager)
{
    m_psp.dwFlags &= ~PSP_HASHELP;

    const OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
    m_UndoMemLimit    = settings->GetUndoMemLimit() / 1024;
    m_MaxUndoCount    = settings->GetUndoLimit();
    m_UndoAfterSaving = settings->GetUndoAfterSaving() ? TRUE : FALSE;
    m_TruncateSpaces  = settings->GetTruncateSpaces() ? TRUE : FALSE;
	m_CursorBeyondEOL = settings->GetCursorBeyondEOL() ? TRUE : FALSE;
	m_CursorBeyondEOF = settings->GetCursorBeyondEOF() ? TRUE : FALSE;
    m_LineNumbers     = settings->GetLineNumbers() ? TRUE : FALSE;
    m_VisibleSpaces   = settings->GetVisibleSpaces() ? TRUE : FALSE;
    m_EOFMark         = settings->GetEOFMark() ? TRUE : FALSE;
    m_UseSmartHome    = settings->GetUseSmartHome() ? TRUE : FALSE;
    m_UseSmartEnd     = settings->GetUseSmartEnd() ? TRUE : FALSE;
    m_TimestampFormat = settings->GetTimestampFormat().c_str();
}

COEEditingPage::~COEEditingPage()
{
}

void COEEditingPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COEEditingPage)
    DDX_Text(pDX, IDC_OE_MAX_UNDO_COUNT, m_MaxUndoCount);
    DDX_Check(pDX, IDC_OE_TRUNCATE_SPACES, m_TruncateSpaces);
    //}}AFX_DATA_MAP
    DDX_Check(pDX, IDC_OE_UNDO_AFTER_SAVING, m_UndoAfterSaving);
    DDX_Text(pDX, IDC_OE_MAX_UNDO_MEM, m_UndoMemLimit);

    DDV_MinMaxUInt(pDX, m_MaxUndoCount, 0, 1000000);
    SendDlgItemMessage(IDC_OE_MAX_UNDO_COUNT_SPIN, UDM_SETRANGE32, 0, 1000000);
    DDV_MinMaxUInt(pDX, m_UndoMemLimit, 0, 1000000);
    SendDlgItemMessage(IDC_OE_MAX_UNDO_MEM_SPIN, UDM_SETRANGE32, 0, 1000000);

    DDX_Check(pDX, IDC_OE_CURSOR_BEYOND_EOL, m_CursorBeyondEOL);
    DDX_Check(pDX, IDC_OE_CURSOR_BEYOND_EOF, m_CursorBeyondEOF);

    if (!pDX->m_bSaveAndValidate)
    {
        ::EnableWindow(
            ::GetDlgItem(m_hWnd, IDC_OE_CURSOR_BEYOND_EOF),
            m_CursorBeyondEOL
            );
    }

    DDX_Check(pDX, IDC_OE_LINE_NUMBERS,   m_LineNumbers);
    DDX_Check(pDX, IDC_OE_VISIBLE_SPACES, m_VisibleSpaces);
    DDX_Check(pDX, IDC_OE_EOF_MARK, m_EOFMark);
    DDX_Check(pDX, IDC_OE_USE_SMART_HOME, m_UseSmartHome);
    DDX_Check(pDX, IDC_OE_USE_SMART_END,  m_UseSmartEnd);
    DDX_Text(pDX,  IDC_OE_TIMESTAMP_FORMAT, m_TimestampFormat);
}


BEGIN_MESSAGE_MAP(COEEditingPage, CPropertyPage)
	//{{AFX_MSG_MAP(COEEditingPage)
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_OE_CURSOR_BEYOND_EOL, OnBnClicked_CursorBeyondEol)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COEEditingPage message handlers

BOOL COEEditingPage::OnApply()
{
    try { EXCEPTION_FRAME;

        OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
        settings->SetUndoMemLimit   (m_UndoMemLimit * 1024,            false /*notify*/);
        settings->SetUndoLimit      (m_MaxUndoCount,                   false /*notify*/);
        settings->SetUndoAfterSaving(m_UndoAfterSaving ? true : false, false /*notify*/);
        settings->SetTruncateSpaces (m_TruncateSpaces ? true : false,  false /*notify*/);
        settings->SetCursorBeyondEOL(m_CursorBeyondEOL ? true : false, false /*notify*/);
        settings->SetCursorBeyondEOF(m_CursorBeyondEOL && m_CursorBeyondEOF 
                                        ? true : false,                false /*notify*/);
        settings->SetLineNumbers    (m_LineNumbers ? true : false,     false /*notify*/);
        settings->SetVisibleSpaces  (m_VisibleSpaces ? true : false,   false /*notify*/);
        settings->SetEOFMark        (m_EOFMark ? true : false,         false /*notify*/);
        settings->SetUseSmartHome   (m_UseSmartHome ? true : false,    false /*notify*/);
        settings->SetUseSmartEnd    (m_UseSmartEnd ? true : false,     false /*notify*/);
        settings->SetTimestampFormat(Common::str(m_TimestampFormat),false /*notify*/);
    }
    _OE_DEFAULT_HANDLER_;

	return TRUE;
}

void COEEditingPage::OnBnClicked_CursorBeyondEol ()
{
    ::EnableWindow(
        ::GetDlgItem(m_hWnd, IDC_OE_CURSOR_BEYOND_EOF),
        ::IsDlgButtonChecked(m_hWnd, IDC_OE_CURSOR_BEYOND_EOL) == BST_CHECKED
        );
}
