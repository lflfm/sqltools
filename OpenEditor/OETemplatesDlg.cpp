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
    2016.06.09 Improvement, made Template dialog resizable
    2017-12-28 improvement, added name uniqueness validation
*/
#include "stdafx.h"
#include "resource.h"
#include "OpenEditor/OETemplatesDlg.h"
#include "OpenEditor/OETemplatesPage.h"

using Common::VisualAttribute;

// COETemplatesDlg dialog

COETemplatesDlg::COETemplatesDlg (CWnd* pParent, Mode mode, const OpenEditor::Template& templ, OpenEditor::Template::Entry& entry, const VisualAttribute& textAttr)
	: CDialog(COETemplatesDlg::IDD, pParent),
    m_mode(mode),
    m_template(templ),
    m_entry(entry),
    m_textAttr(textAttr),
    m_cursorPosMarker('^')
{
    m_name         = m_entry.name.c_str();
    m_keyword      = m_entry.keyword.c_str();
    m_minKeyLength = m_entry.minLength;
    m_text         = m_entry.text.c_str();

    const char* ptr, *buff;
    buff = ptr = m_text.LockBuffer();
    
    for (int line = m_entry.curLine; ptr && line >= 0; ptr++)
    {
        if (*ptr == '\r') line--;
        if (!line) break;
    }

    ptr += m_entry.curPos + (m_entry.curLine ? 2 : 0);
    m_text.UnlockBuffer();
    m_text.Insert(ptr - buff, m_cursorPosMarker);
}

COETemplatesDlg::~COETemplatesDlg()
{
}

void COETemplatesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_OETF_NAME, m_name);

    if (pDX->m_bSaveAndValidate)
    {
        if (!m_template.ValidateUniqueness(string(m_name), m_entry.uuid))
        {
            AfxMessageBox(CString("The name \"") + m_name + "\" is not unique!", MB_OK|MB_ICONSTOP);
            pDX->Fail();
        }
    }

    DDX_Text(pDX, IDC_OETF_KEYWORD, m_keyword);
    DDX_Text(pDX, IDC_OETF_TEXT, m_text);

    DDX_Text(pDX, IDC_OETF_MIN_LENGTH, m_minKeyLength);
    DDV_MinMaxInt(pDX, m_minKeyLength, 0, 999);
    SendDlgItemMessage(IDC_OETF_MIN_LENGTH_SPIN, UDM_SETRANGE32, 0, 999);
}


BEGIN_MESSAGE_MAP(COETemplatesDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnOk)
    ON_WM_GETMINMAXINFO()
    ON_WM_SIZE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()


// COETemplatesDlg message handlers

    static CSize get_item_offset (CDialog* dlg, UINT id, bool left = true, bool top = true)
    {
        CRect client;
        dlg->GetClientRect(client);

        CRect item;
        ::GetWindowRect(::GetDlgItem(*dlg, id), item);
        dlg->ScreenToClient(item);

        return CSize(
            (left ? client.Width() - item.left : client.Width() - item.right), 
            (top ? client.Height() - item.top : client.Height() - item.bottom)
            );
    }

    static CPoint get_top_left (CDialog* dlg, UINT id)
    {
        CRect rc;
        ::GetWindowRect(::GetDlgItem(*dlg, id), rc);
        dlg->ScreenToClient(rc);
        return rc.TopLeft();
    }

BOOL COETemplatesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    {
        CRect rc;
        GetWindowRect(rc);
        m_minSize = rc.Size();
    }

    m_okOffset = get_item_offset(this, IDOK);
    m_cancelOffset = get_item_offset(this, IDCANCEL);
    m_textOffset = get_item_offset(this, IDC_OETF_TEXT, false, false);
    m_textTopLeft = get_top_left(this, IDC_OETF_TEXT);
    m_hintOffset = get_item_offset(this, IDC_OETF_HINT, false, true);
    m_hintTopLeft = get_top_left(this, IDC_OETF_HINT);

    LOGFONT logfont;
    memset(&logfont, 0, sizeof(logfont));

    logfont.lfHeight         = -10 * m_textAttr.m_FontSize;
    logfont.lfCharSet        = DEFAULT_CHARSET;
    logfont.lfOutPrecision   = CLIP_DEFAULT_PRECIS;
    logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality        = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily = FIXED_PITCH;
    strncpy(logfont.lfFaceName, m_textAttr.m_FontName.c_str(), LF_FACESIZE-1);


    CWnd* pWndText = GetDlgItem(IDC_OETF_TEXT);
    CClientDC dc(pWndText);
    m_textFont.CreatePointFontIndirect(&logfont, &dc);
    pWndText->SetFont(&m_textFont);

    switch (m_mode)
    {
    case emInsert:
        SetWindowText("Insert Template");
        break;
    case emEdit:
        SetWindowText("Edit Template");
        break;
    case emDelete:
        SetWindowText("Delete Template");
        ::EnableWindow(::GetDlgItem(*this, IDC_OETF_NAME), FALSE);
        ::EnableWindow(::GetDlgItem(*this, IDC_OETF_KEYWORD), FALSE);
        ::EnableWindow(::GetDlgItem(*this, IDC_OETF_MIN_LENGTH), FALSE);
        ::EnableWindow(::GetDlgItem(*this, IDC_OETF_TEXT), FALSE);
        break;
    }

    {
        BYTE *pRc = 0;
        UINT size = 0;
        if (AfxGetApp()->GetProfileBinary("TemplatesDlg", "RECT", &pRc, &size)
        && size == sizeof(RECT))
        {
            CRect rc = *(RECT*)pRc;
            CWnd* parent = GetParent();
            if (!parent) 
                parent = AfxGetMainWnd();
            if (parent) 
            {
                CRect parentRc;
                parent->GetWindowRect(parentRc);
                rc.OffsetRect(parentRc.TopLeft());
                SetWindowPos(NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER);
            }
        }
    }

    return TRUE;
}

void COETemplatesDlg::OnOk()
{
    OnOK();

    m_entry.name = m_name;
    m_entry.keyword = m_keyword;
    m_entry.minLength = m_minKeyLength;
    
    m_entry.curPos = m_entry.curLine = 0;
    const char* buff = m_text.LockBuffer();
    const char* pos = strchr(buff, m_cursorPosMarker);
    
    for (const char* ptr = buff; pos && ptr < pos; ptr++, m_entry.curPos++)
    {
        if (*ptr == '\r')
        {
            m_entry.curLine++;
            m_entry.curPos = 0;
        }
    }
    if (m_entry.curLine) m_entry.curPos -=2;
    m_text.UnlockBuffer();
    
    if (pos)
        m_text.Delete(pos - buff, 1);

    m_entry.text = m_text;
}

LRESULT COETemplatesDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}


void COETemplatesDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CDialog::OnGetMinMaxInfo(lpMMI);

    if (lpMMI->ptMinTrackSize.x < m_minSize.cx)
        lpMMI->ptMinTrackSize.x = m_minSize.cx;

    if (lpMMI->ptMinTrackSize.y < m_minSize.cy)
        lpMMI->ptMinTrackSize.y = m_minSize.cy;
}


void COETemplatesDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    HDWP hDefer = BeginDeferWindowPos(4);

    ::DeferWindowPos(hDefer, ::GetDlgItem(*this, IDOK), NULL, 
        cx - m_okOffset.cx, cy - m_okOffset.cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    ::DeferWindowPos(hDefer, ::GetDlgItem(*this, IDCANCEL), NULL, 
        cx - m_cancelOffset.cx, cy - m_cancelOffset.cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    ::DeferWindowPos(hDefer, ::GetDlgItem(*this, IDC_OETF_TEXT), NULL, 
        0, 0, cx - m_textTopLeft.x - m_textOffset.cx, cy - m_textTopLeft.y - m_textOffset.cy, SWP_NOMOVE|SWP_NOZORDER);
    ::DeferWindowPos(hDefer, ::GetDlgItem(*this, IDC_OETF_HINT), NULL, 
        m_hintTopLeft.x , cy - m_hintOffset.cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

    EndDeferWindowPos(hDefer);
}


void COETemplatesDlg::OnDestroy()
{
    CWnd* parent = GetParent();
    if (!parent) 
        parent = AfxGetMainWnd();
    if (parent) 
    {
        CRect rc, parentRc;
        GetWindowRect(rc);
        parent->GetWindowRect(parentRc);
        rc.OffsetRect(-parentRc.left, -parentRc.top);
        AfxGetApp()->WriteProfileBinary("TemplatesDlg", "RECT", (LPBYTE)(RECT*)&rc, sizeof(RECT));
    }

    CDialog::OnDestroy();
}
