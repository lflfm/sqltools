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

/*
    14.08.2002 bug fix, Find/Replace dialog - garbage in combo box history
    25.09.2002 bug fix, Find/Replace dialog - [:xxxx:] exps have been replaced
    16.12.2002 bug fix, regexp replace fails on \1,...
    16.12.2002 bug fix, Find/Replace dialog auto position
    21.03.2003 bug fix, missing entry in fing what/replace with histoty
    26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
    26.05.2003 bug fix, Find/Replace dialog: "Transform backslash expressions" has beed added to handle \t\b\ddd...
    01.06.2004 bug fix, Find/Replace dialog: replace skips empty lines, e.g. "^$"
    30.06.2004 improvement/bug fix, text search/replace interface has been changed
    17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report
    30.03.2005 (ak) bug fix, hanging on trying to replace 'a' with 'aa' in the current selection 
    2017-12-03 bug fix, If you want to replace a string in a selected text with a replacement string bigger than the search string, 
                        it will not replace until the end of the original selected text.
*/

#include "stdafx.h"
#include <algorithm>
#include <COMMON/AppGlobal.h>
#include <COMMON/AppUtilities.h>
#include <COMMON/ExceptionHelper.h>
#include <COMMON/StrHelpers.h>
#include "OpenEditor/OEFindReplaceDlg.h"
#include "OpenEditor/OEView.h"
#include "Common/MyUtf.h"

#include <MultiMon.h>
extern "C" {
    extern BOOL InitMultipleMonitorStubs(void);
    extern HMONITOR (WINAPI* g_pfnMonitorFromWindow)(HWND, DWORD);
    extern BOOL     (WINAPI* g_pfnGetMonitorInfo)(HMONITOR, LPMONITORINFO);
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OpenEditor;

/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDlg dialog

CPoint CFindReplaceDlg::m_ptTopLeft(-1,-1);
BOOL CFindReplaceDlg::m_MessageOnEOF;

CFindReplaceDlg::CFindReplaceDlg(BOOL replace, COEditorView* pView)
: CDialog(!replace ? IDD_OE_EDIT_FIND : IDD_OE_EDIT_REPLACE, NULL)
{
    m_pEditorView    = pView;
    m_ReplaceMode    = replace;
    m_Modified       = TRUE;
	//{{AFX_DATA_INIT(CFindReplaceDlg)
	//}}AFX_DATA_INIT

    if (m_pEditorView)
    {
        // 26.05.2003 bug fix, Find/Replace dialog: "Transform backslash expressions" has beed added to handle \t\b\ddd...
        m_BackslashExpressions = AfxGetApp()->GetProfileInt(L"Editor", L"BackslashExpressions", FALSE);

        std::wstring buff;
        toPrintableStr(m_pEditorView->GetSearchText(), buff);
        m_SearchText = buff.c_str();

        /*
        bool backward, wholeWords, matchCase, regExpr, searchAll;
        pView->GetSearchOption(backward, wholeWords, matchCase, regExpr, searchAll);

        m_MatchCase      = matchCase;
        m_MatchWholeWord = wholeWords;
        m_AllWindows     = searchAll;
        m_RegExp         = regExpr;
        m_Direction      = backward ? 0 : 1;
        */
        m_MatchCase      = AfxGetApp()->GetProfileInt(L"Editor", L"SearchMatchCase",      FALSE);
        m_MatchWholeWord = AfxGetApp()->GetProfileInt(L"Editor", L"SearchMatchWholeWord", FALSE);
        m_AllWindows     = AfxGetApp()->GetProfileInt(L"Editor", L"SearchAllWindows",     FALSE);
        m_RegExp         = AfxGetApp()->GetProfileInt(L"Editor", L"SearchRegExp",         FALSE);
        m_MessageOnEOF   = AfxGetApp()->GetProfileInt(L"Editor", L"MessageOnEOF",         FALSE);
        // 26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
        m_Direction      = m_ReplaceMode ? 1 : AfxGetApp()->GetProfileInt(L"Editor", L"SearchDirection", 1);

        m_WhereReplace   = 1;
    }
    else
        _ASSERTE(0);
}


void CFindReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindReplaceDlg)
	//}}AFX_DATA_MAP
	DDX_CBString(pDX, IDC_EF_SEARCH_TEXT, m_SearchText);
	DDX_Control(pDX, IDC_EF_SEARCH_TEXT, m_wndSearchText);

    if (m_ReplaceMode)
    {
	    DDX_CBString(pDX, IDC_EF_REPLACE_TEXT, m_ReplaceText);
  	    DDX_Control(pDX, IDC_EF_REPLACE_TEXT, m_wndReplaceText);
	    DDX_Radio(pDX, IDC_EF_REPLACE_IN_SELECTION, m_WhereReplace);
    }
    else
    {
	    DDX_Radio(pDX, IDC_EF_UP, m_Direction);
        DDX_Check(pDX, IDC_EF_MSG_BOX_ON_EOF, m_MessageOnEOF);
    }

    DDX_Check(pDX, IDC_EF_MATCH_CASE, m_MatchCase);
	DDX_Check(pDX, IDC_EF_MATCH_WHOLE_WORD, m_MatchWholeWord);
	DDX_Check(pDX, IDC_EF_ALL_WINDOWS, m_AllWindows);
	DDX_Check(pDX, IDC_EF_REGEXP, m_RegExp);
    DDX_Check(pDX, IDC_EF_TRANSFORM_BACKSLASH_EXPR, m_BackslashExpressions);
}


BEGIN_MESSAGE_MAP(CFindReplaceDlg, CDialog)
	//{{AFX_MSG_MAP(CFindReplaceDlg)
	ON_BN_CLICKED(IDC_EF_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_EF_REGEXP, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_ALL_WINDOWS, OnAllWindows)
	ON_CBN_EDITCHANGE(IDC_EF_SEARCH_TEXT, OnSearchText)
	ON_CBN_SELCHANGE(IDC_EF_SEARCH_TEXT, OnSearchText)
	ON_CBN_EDITCHANGE(IDC_EF_REPLACE_TEXT, OnChangeSettings)
	ON_CBN_SELCHANGE(IDC_EF_REPLACE_TEXT, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_REPLACE, OnReplace)
	ON_BN_CLICKED(IDC_EF_REPLACE_ALL, OnReplaceAll)
	ON_BN_CLICKED(IDC_EF_MATCH_CASE, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_MATCH_WHOLE_WORD, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_UP, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_DOWN, OnChangeSettings)
    ON_BN_CLICKED(IDC_EF_REPLACE_IN_SELECTION, OnReplaceWhere)
	ON_BN_CLICKED(IDC_EF_REPLACE_IN_WHOLE_FILE, OnReplaceWhere)
	ON_BN_CLICKED(IDC_EF_COUNT, OnCount)
	ON_BN_CLICKED(IDC_EF_MARK_ALL, OnMarkAll)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_EF_REGEXP_FIND, OnBnClicked_RegexpFind)
    ON_BN_CLICKED(IDC_EF_REGEXP_REPLACE, OnBnClicked_RegexpReplace)
    ON_COMMAND_RANGE(ID_REGEXP_FIND_TAB, ID_REGEXP_FIND_QUOTED, OnInsertRegexpFind)
    ON_COMMAND_RANGE(ID_REGEXP_REPLACE_WHAT_FIND, ID_REGEXP_REPLACE_TAGEXP_9, OnInsertRegexpReplace)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDlg message handlers

void CFindReplaceDlg::toPrintableStr (const wchar_t* from, std::wstring& _to)
{
    if (m_BackslashExpressions)
    {
        string buff;
        Common::to_printable_str(Common::str(from).c_str(), buff);
        _to = Common::wstr(buff);
    }
    else
        _to = from;
}

void CFindReplaceDlg::toUnprintableStr (const wchar_t* from, std::wstring& _to, bool skipEscDgt)
{
    if (m_BackslashExpressions)
    {
        string buff;
        Common::to_unprintable_str(Common::str(from).c_str(), buff, skipEscDgt);
        _to = Common::wstr(buff);
    }
    else
        _to = from;
}

void CFindReplaceDlg::AdjustPosition ()
{
    CRect rc, newRc, orgRc;
    GetWindowRect(rc);
    newRc = rc;
    orgRc = rc;

    if (m_pEditorView)
    {
        CRect caretRc;
        if (m_pEditorView->GetCaretPosition(caretRc))
        {
            m_pEditorView->ClientToScreen(&caretRc);

            CRect viewRc;
            m_pEditorView->GetClientRect(&viewRc);
            m_pEditorView->ClientToScreen(&viewRc);

            if (rc.top <= caretRc.top && rc.bottom >= caretRc.bottom)
            {
                // so the current line overlapped with the dialog position
                if ((caretRc.top + caretRc.bottom) / 2 < (viewRc.top + viewRc.bottom) / 2) // top window half
                {
                    newRc.top   = (viewRc.top + viewRc.bottom) / 2;
                    newRc.bottom = newRc.top + rc.Height();
                }
                else // bottom window half
                {
                    newRc.bottom = (viewRc.top + viewRc.bottom) / 2;
                    newRc.top    = newRc.bottom - rc.Height();
                }
                rc = newRc;
            }
        }
    }

    {
        // lets make sure that the dialog is visiable on any monitor
        CRect desktopRc;

        if (g_pfnMonitorFromWindow)
        {
            HMONITOR hmonitor = g_pfnMonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            memset(&monitorInfo, 0, sizeof(monitorInfo));
            monitorInfo.cbSize = sizeof(monitorInfo);

            if (g_pfnGetMonitorInfo(hmonitor, &monitorInfo))
            {
                desktopRc = monitorInfo.rcMonitor;
            }
            else
                GetDesktopWindow()->GetWindowRect(desktopRc);
        }
        else
            GetDesktopWindow()->GetWindowRect(desktopRc);

        newRc = rc;
        // vert position adjustment
        if (rc.top < desktopRc.top)                  
            newRc.MoveToY(desktopRc.top);
        if (rc.bottom > desktopRc.bottom)               
            newRc.MoveToY(desktopRc.bottom - rc.Height());
        // horz position adjustment
        if (rc.left < desktopRc.left)                  
            newRc.MoveToX(desktopRc.left);
        if (rc.right > desktopRc.right)               
            newRc.MoveToX(desktopRc.right - rc.Width());
    }

    if (orgRc != newRc)
        MoveWindow(newRc, TRUE);
}

void CFindReplaceDlg::SaveOption ()
{
    if (m_Modified)
    {

        Common::AppSaveHistory(m_wndSearchText, L"Editor", L"SearchText", 10);
        // 21.03.2003 bug fix, missing entry in fing what/replace with histoty
        Common::AppRestoreHistory(m_wndSearchText, L"Editor", L"SearchText", 10);

        if (m_ReplaceMode)
        {
            Common::AppSaveHistory(m_wndReplaceText, L"Editor", L"ReplaceText", 10);
            // 21.03.2003 bug fix, missing entry in fing what/replace with histoty
            Common::AppRestoreHistory(m_wndReplaceText, L"Editor", L"ReplaceText", 10);
        }

        if (m_pEditorView)
        {
            std::wstring buff;
            toUnprintableStr(m_SearchText, buff, false);

            m_pEditorView->SetSearchText(buff.c_str());
            m_pEditorView->SetSearchOption(m_Direction == 0 ? true : false,
                                     m_MatchWholeWord ? true : false,
                                     m_MatchCase      ? true : false,
                                     m_RegExp         ? true : false,
                                     m_AllWindows     ? true : false);

            AfxGetApp()->WriteProfileInt(L"Editor", L"SearchMatchCase",      m_MatchCase     );
            AfxGetApp()->WriteProfileInt(L"Editor", L"SearchMatchWholeWord", m_MatchWholeWord);
            AfxGetApp()->WriteProfileInt(L"Editor", L"SearchAllWindows",     m_AllWindows    );
            AfxGetApp()->WriteProfileInt(L"Editor", L"SearchRegExp",         m_RegExp        );
            AfxGetApp()->WriteProfileInt(L"Editor", L"MessageOnEOF",         m_MessageOnEOF  );
            // 26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
            if (!m_ReplaceMode)
                AfxGetApp()->WriteProfileInt(L"Editor", L"SearchDirection", m_Direction);

            AfxGetApp()->WriteProfileInt(L"Editor", L"BackslashExpressions", m_BackslashExpressions);
        }

        m_Modified = FALSE;
    }
}

void CFindReplaceDlg::OnFindNext()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        UpdateData();
        SaveOption();

        if (m_pEditorView && !m_SearchText.IsEmpty())
        {
            COEditorView* pView;
            if (m_pEditorView->RepeadSearch(esdDefault, pView))
            {
                m_pEditorView = pView;
                AdjustPosition();

                if (!m_ReplaceMode)
                    OnOK();
            }
        }
    }
    _OE_DEFAULT_HANDLER_;
}

void CFindReplaceDlg::OnReplace ()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        UpdateData();
        SaveOption();

        Square blk;

        if (m_pEditorView)
        {
            m_pEditorView->GetSelection(blk);
            blk.normalize();

            // 01.06.2004 bug fix, Find/Replace dialog: replace skips empty lines, e.g. "^$"
            if (/*!blk.is_empty() &&*/ blk.start.line == blk.end.line)
            {
                FindCtx ctx;
                // 16.12.2002 bug fix, regexp replace fails on \1,...
                toUnprintableStr(m_ReplaceText, ctx.m_text, m_RegExp ? true : false /*skipEscDgt*/);
                ctx.m_line  = blk.start.line;
                ctx.m_start = blk.start.column;
                ctx.m_end   = blk.end.column;
                m_pEditorView->Replace(ctx);
            }
        }

        OnFindNext();
    }
    _OE_DEFAULT_HANDLER_;
}

void CFindReplaceDlg::OnReplaceAll ()
{
    SearchBatch(esbReplace);
}

void CFindReplaceDlg::OnCount ()
{
    SearchBatch(esbCount);
}

void CFindReplaceDlg::OnMarkAll ()
{
    SearchBatch(esbMark);
}

void CFindReplaceDlg::SearchBatch (OpenEditor::ESearchBatch mode)
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        int counter = 0;

        UpdateData();
        SaveOption();

        if (m_pEditorView)
        {
            // replace in file(s) or count/mark
            if (m_WhereReplace == 1 || mode == esbCount || mode == esbMark )
            {
                if (!m_AllWindows || mode == esbCount || mode == esbMark
                || AfxMessageBox(L"All occurances will be replaced for all windows! Do You confirm?", MB_OKCANCEL|MB_ICONWARNING) == IDOK)
                {
                    std::wstring buff;
                    toUnprintableStr(m_ReplaceText, buff, false);
                    counter = m_pEditorView->SearchBatch(buff.c_str(), mode);
                }
            }
            else // replace in selection
            {
                Square blk;
                m_pEditorView->GetSelection(blk);
                bool normBlk = (blk.start < blk.end);
                if (!normBlk) blk.normalize();
                Position orgPos = m_pEditorView->GetPosition();

                bool _backward = m_pEditorView->IsBackwardSearch();
                m_pEditorView->SetBackwardSearch(false);

                try
                {
                    //Square blk;
                    //m_pEditorView->GetSelection(blk);

                    if (!blk.is_empty())
                    {
                        blk.normalize();
                        switch (m_pEditorView->GetBlockMode())
                        {
                        default:
                            THROW_APP_EXCEPTION("Unsupported block type for counting or replacing.");
                        case ebtStream:
                            ;
                        }

                        COEditorView* pView = 0;

                        COEditorView::UndoGroup undoGroup(*m_pEditorView);
                        Position pos = m_pEditorView->GetPosition();
                        m_pEditorView->PushInUndoStack(pos);

                        FindCtx ctx;
                        ctx.m_line = blk.start.line;
                        ctx.m_start = blk.start.column;
                        ctx.m_end = blk.end.column;
                        ctx.m_skipFirstNull = false;
                        // 27.05.2002 bug fix, regexp replace fails on \1,... for ReplaceAll
                        toUnprintableStr(m_ReplaceText, ctx.m_text, m_RegExp ? true : false /*skipEscDgt*/);

                        while (m_pEditorView->Find((const COEditorView *&)pView, ctx)
                        && !ctx.m_eofPassed && pView == m_pEditorView)
                        {
                            if ((ctx.m_line >= blk.start.line && ctx.m_line < blk.end.line)
                            || (ctx.m_line == blk.end.line && ctx.m_end <= blk.end.column))
                            {
                                if (mode == esbReplace)
                                {
                                    int orgLen = ctx.m_end - ctx.m_start;
                                    m_pEditorView->Replace(ctx);
                                    int newLen = ctx.m_end - ctx.m_start;
                                    // 30.03.2005 (ak) bug fix, hanging on trying to replace 'a' with 'aa' in the current selection 
                                    ctx.m_start = ctx.m_end;
                                    // 2017-12-03 bug fix, If you want to replace a string in a selected text with a replacement string bigger than the search string, 
                                    //                     it will not replace until the end of the original selected text.
                                    if (ctx.m_line == blk.end.line)
                                        blk.end.column += newLen - orgLen;
                                }
                                counter++;
                            }
                            else
                                break;
                        }
                    }
                }
                catch (...)
                {
                    m_pEditorView->SetBackwardSearch(_backward);
                    throw;
                }

                m_pEditorView->SetBackwardSearch(_backward);
                if (!normBlk) 
                    std::swap(blk.start, blk.end);
                else
                    orgPos = blk.end;
                m_pEditorView->MoveTo(orgPos);
                m_pEditorView->SetSelection(blk);
            }

            if (mode == esbReplace && counter)
                AdjustPosition();
        }

        char buff[100];
        itoa(counter, buff, 10);

        switch (mode)
        {
        case esbReplace:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been replaced!"));
            break;
        case esbMark:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been marked!"));
            break;
        case esbCount:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been found!"));
            MessageBox(Common::wstr(buff).c_str(), L"Find");
            break;
        }
    }
    _OE_DEFAULT_HANDLER_;
}

BOOL CFindReplaceDlg::OnInitDialog ()
{
    BOOL enableWhereReplace =
        (!m_pEditorView->IsSelectionEmpty()
         && m_pEditorView->GetBlockMode() != ebtColumn
         && !m_AllWindows) ? TRUE : FALSE;

    Square blk;
    m_pEditorView->GetSelection(blk);
    m_WhereReplace = (enableWhereReplace
        && blk.start.line != blk.end.line) ? 0 : 1;

    CDialog::OnInitDialog();

    Common::AppRestoreHistory(m_wndSearchText, L"Editor", L"SearchText", 10);

    if (!m_SearchText.IsEmpty())
    {
        // 14/08/2002 bug fix, Find/Replace dialog - garbage in combo box history
        m_wndSearchText.SetWindowText(m_SearchText);
    }

    if (m_ReplaceMode)
    {
        Common::AppRestoreHistory(m_wndReplaceText, L"Editor", L"ReplaceText", 10);

        GetDlgItem(IDC_EF_REPLACE_IN_SELECTION)->EnableWindow(enableWhereReplace);
    }

    SetupButtons();

    if (m_ptTopLeft.x != -1 && m_ptTopLeft.y != -1)
    {
        CRect rc;
        GetWindowRect(&rc);
        rc.MoveToXY(m_ptTopLeft);
        MoveWindow(rc);
    }
    else
    {
        CRect rc, frmRc;
        GetWindowRect(&rc);
        AfxGetMainWnd()->GetWindowRect(&frmRc);
        rc.MoveToX(frmRc.right - rc.Width() - 6);
        MoveWindow(rc);
    }

    AdjustPosition();

    if (!m_edtSearchText.SubclassComboBoxEdit(m_wndSearchText.m_hWnd))
        // hide button for Win95
        ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

    if (m_wndReplaceText.m_hWnd)
    {
        if (!m_edtReplaceText.SubclassComboBoxEdit(m_wndReplaceText.m_hWnd))
            // hide button for Win95
            ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
    }

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), m_RegExp);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), m_RegExp);

    return TRUE;
}

void CFindReplaceDlg::OnChangeSettings ()
{
    UpdateData();
    m_Modified = TRUE;
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), m_RegExp);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), m_RegExp);
}

void CFindReplaceDlg::OnAllWindows ()
{
    OnChangeSettings();

    if (m_ReplaceMode)
    {
        GetDlgItem(IDC_EF_REPLACE_IN_SELECTION)
            ->EnableWindow(!m_pEditorView->IsSelectionEmpty()
                           && m_pEditorView->GetBlockMode() != ebtColumn
                           && !m_AllWindows);

        if (m_AllWindows && m_WhereReplace == 0)
        {
            m_WhereReplace = 1;
            UpdateData(FALSE);
        }
    }

    SetupButtons();
}

void CFindReplaceDlg::OnReplaceWhere ()
{
    OnChangeSettings();
    SetupButtons();
}

void CFindReplaceDlg::OnSearchText ()
{
    OnChangeSettings();
    SetupButtons();
}

void CFindReplaceDlg::SetupButtons ()
{
    // it works not precisely but maybe later...
    BOOL hasSearchText = m_wndSearchText.GetWindowTextLength() > 0
        || m_wndSearchText.GetCurSel() != -1 && m_wndSearchText.GetLBTextLen(m_wndSearchText.GetCurSel());

    if (!m_ReplaceMode)
    {
        GetDlgItem(IDC_EF_MARK_ALL)->EnableWindow(hasSearchText);
        GetDlgItem(IDC_EF_FIND_NEXT)->EnableWindow(hasSearchText);
        GetDlgItem(IDC_EF_COUNT)->EnableWindow(hasSearchText);
    }
    else
    {
        GetDlgItem(IDC_EF_FIND_NEXT)->EnableWindow(m_WhereReplace && hasSearchText);
        GetDlgItem(IDC_EF_REPLACE)->EnableWindow(m_WhereReplace && hasSearchText);
        GetDlgItem(IDC_EF_REPLACE_ALL)->EnableWindow(hasSearchText);
    }
}
void CFindReplaceDlg::OnBnClicked_RegexpFind()
{
    ShowPopupMenu(IDC_EF_REGEXP_FIND, IDR_OE_REGEXP_FIND_POPUP);
}

void CFindReplaceDlg::OnBnClicked_RegexpReplace()
{
    ShowPopupMenu(IDC_EF_REGEXP_REPLACE, IDR_OE_REGEXP_REPLACE_POPUP);
}

void CFindReplaceDlg::ShowPopupMenu (UINT btnId, UINT menuId)
{
    HWND hButton = ::GetDlgItem(m_hWnd, btnId);

    _ASSERTE(hButton);

    if (hButton
    && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED)
    {
        ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

        CRect rect;
        ::GetWindowRect(hButton, &rect);

        CMenu menu;
        VERIFY(menu.LoadMenu(menuId));

        CMenu* pPopup = menu.GetSubMenu(0);
        _ASSERTE(pPopup != NULL);

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


static DWORD start, end;

void CFindReplaceDlg::OnInsertRegexpFind (UINT nID)
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
        OnChangeSettings();
        SetupButtons();
    }
}

void CFindReplaceDlg::OnInsertRegexpReplace (UINT nID)
{
    ::SendMessage(::GetDlgItem(m_hWnd, IDC_EF_REGEXP), BM_SETCHECK, BST_CHECKED, 0L);

    static
    const wchar_t* expr[]
        = { L"\\0", L"\\1", L"\\2", L"\\3", L"\\4", L"\\5", L"\\6", L"\\7", L"\\8", L"\\9" };

    _ASSERTE((nID - ID_REGEXP_REPLACE_WHAT_FIND) < (sizeof(expr)/sizeof(expr[0])));

    if (((nID - ID_REGEXP_REPLACE_WHAT_FIND) < (sizeof(expr)/sizeof(expr[0]))))
    {
        m_edtReplaceText.InsertAtCurPos(expr[nID - ID_REGEXP_REPLACE_WHAT_FIND], -1);
        OnChangeSettings();
        SetupButtons();
    }

}

void CFindReplaceDlg::OnCancel()
{
    CRect rc;
    GetWindowRect(&rc);
    m_ptTopLeft = rc.TopLeft();

    CDialog::OnCancel();
}

void CFindReplaceDlg::OnOK()
{
    CRect rc;
    GetWindowRect(&rc);
    m_ptTopLeft = rc.TopLeft();

    CDialog::OnOK();
}

LRESULT CFindReplaceDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}
