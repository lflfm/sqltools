// PropTreeItemEdit.cpp : implementation file
//
//  Copyright (C) 1998-2001 Scott Ramsay
//	sramsay@gonavi.com
//	http://www.gonavi.com
//
//  This material is provided "as is", with absolutely no warranty expressed
//  or implied. Any use is at your own risk.
// 
//  Permission to use or copy this software for any purpose is hereby granted 
//  without fee, provided the above notices are retained on all copies.
//  Permission to modify the code and to distribute modified code is granted,
//  provided the above notices are retained, and a notice that the code was
//  modified is included with the above copyright notice.
// 
//	If you use this code, drop me an email.  I'd like to know if you find the code
//	useful.

#include "stdafx.h"
#include "proptree.h"
#include "PropTreeItemEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemEdit

CPropTreeItemEdit::CPropTreeItemEdit() :
	m_sEdit(_T("")),
	m_nFormat(ValueFormatText),
	m_bPassword(FALSE)
{
}

CPropTreeItemEdit::~CPropTreeItemEdit()
{
}


BEGIN_MESSAGE_MAP(CPropTreeItemEdit, CEdit)
	//{{AFX_MSG_MAP(CPropTreeItemEdit)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemEdit message handlers

void CPropTreeItemEdit::DrawAttribute(CDC* pDC, const RECT& rc)
{
	ASSERT(m_pProp!=NULL);


    // (ak)
	//pDC->SelectObject(IsReadOnly() ? m_pProp->GetNormalFont() : m_pProp->GetBoldFont());
	//pDC->SetTextColor(RGB(0,0,0));
    pDC->SelectObject(m_pProp->GetNormalFont());
    pDC->SetTextColor(GetSysColor(!IsReadOnly() ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
    // (ak)
	pDC->SetBkMode(TRANSPARENT);

	CRect r = rc;
    r.InflateRect(-2,-2); // (ak)

	TCHAR ch;

	// can't use GetPasswordChar(), because window may not be created yet
	ch = (m_bPassword) ? '*' : '\0';

	if (ch)
	{
		CString s;

		s = m_sEdit;
		for (LONG i=0; i<s.GetLength();i++)
			s.SetAt(i, ch);

		pDC->DrawText(s, r, DT_SINGLELINE|DT_TOP|DT_NOPREFIX);
	}
	else
	{
		pDC->DrawText(m_sEdit, r, DT_SINGLELINE|DT_TOP|DT_NOPREFIX);
	}
}



void CPropTreeItemEdit::SetAsPassword(BOOL bPassword)
{
	m_bPassword = bPassword;
}


void CPropTreeItemEdit::SetValueFormat(ValueFormat nFormat)
{
	m_nFormat = nFormat;
}


// Retrieve the item's attribute value
LPCSTR CPropTreeItemEdit::GetItemValue () const
{
	return m_sEdit;
}

void CPropTreeItemEdit::GetItemValue (int& val) const
{
	ASSERT(m_nFormat == ValueFormatInteger);
    val = _ttoi(m_sEdit);
}

void CPropTreeItemEdit::GetItemValue (double& val) const
{
	ASSERT(m_nFormat == ValueFormatFloat);
    val = _tstof(m_sEdit);
}

// Set the item's attribute value
void CPropTreeItemEdit::SetItemValue (LPCSTR str)
{
	m_nFormat = ValueFormatText;
    m_sEdit = str;
}

void CPropTreeItemEdit::SetItemValue (int val)
{
	m_nFormat = ValueFormatInteger;
    m_sEdit.Format(_T("%d"), val);
}

void CPropTreeItemEdit::SetItemValue (double val)
{
	m_nFormat = ValueFormatFloat;
    m_sEdit.Format(_T("%f"), val);
}

void CPropTreeItemEdit::OnMove()
{
	if (IsWindow(m_hWnd))
        // (ak)
	    SetWindowPos(NULL, m_rc.left+2, m_rc.top+2, m_rc.Width()-4, m_rc.Height()-4, SWP_NOZORDER|SWP_NOACTIVATE);
}


void CPropTreeItemEdit::OnRefresh()
{
	if (IsWindow(m_hWnd))
		SetWindowText(m_sEdit);
}


void CPropTreeItemEdit::OnCommit()
{
	// hide edit control
	ShowWindow(SW_HIDE);

	// store edit text for GetItemValue
	GetWindowText(m_sEdit);
}


void CPropTreeItemEdit::OnActivate()
{
	// Check if the edit control needs creation
	if (!IsWindow(m_hWnd))
	{
		DWORD dwStyle;

		dwStyle = WS_CHILD|ES_AUTOHSCROLL;
		Create(dwStyle, m_rc, m_pProp->GetCtrlParent(), GetCtrlID());
		SendMessage(WM_SETFONT, (WPARAM)m_pProp->GetNormalFont()->m_hObject);
	}

    SetMargins(0, 0); // (ak)
	SetPasswordChar((TCHAR)(m_bPassword ? '*' : 0));
	SetWindowText(m_sEdit);
	SetSel(0, -1);

    // (ak)
	SetWindowPos(NULL, m_rc.left+2, m_rc.top+2, m_rc.Width()-4, m_rc.Height()-4, SWP_NOZORDER|SWP_SHOWWINDOW);
	SetFocus();
}

// ak
void CPropTreeItemEdit::OnCancel()
{
	// hide edit control
	if (m_hWnd) ShowWindow(SW_HIDE);
}

UINT CPropTreeItemEdit::OnGetDlgCode() 
{
	return CEdit::OnGetDlgCode()|DLGC_WANTALLKEYS;
}


void CPropTreeItemEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar==VK_RETURN)
		CommitChanges();
	
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CPropTreeItemEdit::OnKillfocus() 
{
	CommitChanges();	
}
