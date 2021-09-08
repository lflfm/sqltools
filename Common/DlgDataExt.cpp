/*
    Copyright (C) 2004 Aleksey Kochetov

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

// 2011.09.14 bug fix, taken from sqltools++ by Randolf Geist

#include "stdafx.h"
#include "DlgDataExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void DDX_Check (CDataExchange* pDX, int nIDC, bool& value)
{
    pDX->PrepareCtrl(nIDC);
    HWND hWndCtrl;
    pDX->m_pDlgWnd->GetDlgItem(nIDC, &hWndCtrl);

    if (pDX->m_bSaveAndValidate)
        value = ::SendMessage(hWndCtrl, BM_GETCHECK, 0, 0L) == BST_CHECKED ? true : false;
    else
        ::SendMessage(hWndCtrl, BM_SETCHECK, (WPARAM)value ? BST_CHECKED : BST_UNCHECKED, 0L);
}

void DDX_Text (CDataExchange* pDX, int nIDC, std::string& value)
{
    if (pDX->m_bSaveAndValidate)
    {
        std::wstring buff;
        DDX_Text(pDX, nIDC, buff);
        value = Common::str(buff);
    }
    else
    {
        std::wstring buff = Common::wstr(value);
        DDX_Text(pDX, nIDC, buff);
    }
}

void DDX_Text (CDataExchange* pDX, int nIDC, std::wstring& value)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if (pDX->m_bSaveAndValidate)
    {
        CString buffer;
        int nLen = ::GetWindowTextLength(hWndCtrl);
        ::GetWindowText(hWndCtrl, buffer.GetBufferSetLength(nLen), nLen+1);
        buffer.ReleaseBuffer();
        value = buffer;
    }
    else
    {
        AfxSetWindowText(hWndCtrl, value.c_str());
    }
}

void DDV_MaxChars (CDataExchange* pDX, const std::string& _value, int nChars)
{
    ASSERT(nChars >= 1);        // allow them something

    std::wstring value = Common::wstr(_value);

    if (pDX->m_bSaveAndValidate
    && static_cast<int>(value.length()) > nChars)
    {
        TCHAR szT[32];
        wsprintf(szT, _T("%d"), nChars);
        CString prompt;
        AfxFormatString1(prompt, AFX_IDP_PARSE_STRING_SIZE, szT);
        AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_STRING_SIZE);
        prompt.Empty(); // exception prep
        pDX->Fail();
    }
    else if (pDX->m_idLastControl != 0 && pDX->m_bEditLastControl)
    {
        HWND hWndLastControl;
        pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl, &hWndLastControl);
        // limit the control max-chars automatically
        ::SendMessage(hWndLastControl, EM_LIMITTEXT, nChars, 0);
    }
}

void DDV_MaxChars (CDataExchange* pDX, const std::wstring& value, int nChars)
{
    ASSERT(nChars >= 1);        // allow them something


    if (pDX->m_bSaveAndValidate
    && static_cast<int>(value.length()) > nChars)
    {
        TCHAR szT[32];
        wsprintf(szT, _T("%d"), nChars);
        CString prompt;
        AfxFormatString1(prompt, AFX_IDP_PARSE_STRING_SIZE, szT);
        AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_STRING_SIZE);
        prompt.Empty(); // exception prep
        pDX->Fail();
    }
    else if (pDX->m_idLastControl != 0 && pDX->m_bEditLastControl)
    {
        HWND hWndLastControl;
        pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl, &hWndLastControl);
        // limit the control max-chars automatically
        ::SendMessage(hWndLastControl, EM_LIMITTEXT, nChars, 0);
    }
}

void DDX_CBString (CDataExchange* pDX, int nIDC, std::wstring& _value)
{
    pDX->PrepareCtrl(nIDC);
    HWND hWndCtrl;
    pDX->m_pDlgWnd->GetDlgItem(nIDC, &hWndCtrl);

    if (pDX->m_bSaveAndValidate)
    {
        CString value;
        // just get current edit item text (or drop list static)
        int nLen = ::GetWindowTextLength(hWndCtrl);
        if (nLen > 0)
        {
            // get known length
            ::GetWindowText(hWndCtrl, value.GetBufferSetLength(nLen), nLen+1);
        }
        else
        {
            // for drop lists GetWindowTextLength does not work - assume
            //  max of 255 characters
            ::GetWindowText(hWndCtrl, value.GetBuffer(255), 255+1);
        }
        value.ReleaseBuffer();
        _value = value;
    }
    else
    {
        // 2011.09.14 bug fix, taken from sqltools++ by Randolf Geist
        LRESULT lResult;
        // set current selection based on model string
        if ((lResult = ::SendMessage(hWndCtrl, CB_FINDSTRINGEXACT, (WPARAM)-1,
            (LPARAM)(LPCTSTR)_value.c_str())) == CB_ERR)
        {
            // just set the edit text (will be ignored if DROPDOWNLIST)
            AfxSetWindowText(hWndCtrl, _value.c_str());
        }
        else
        {
            ::SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)lResult, 0);
        }
    }
}
