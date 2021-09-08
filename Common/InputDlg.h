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

// 03.01.2005 (ak) added m_password to hide input content
// 09.01.2005 (ak) created CPasswordDlg because an attrinute "password" can be changed dynamicly

#pragma once

#include "resource.h"
#include <string>

namespace Common
{

class CInputDlgImpl : public CDialog
{
// Dialog Data
	enum { IDD = IDD_INPUT };
public:
	CInputDlgImpl (UINT resID, CWnd* pParent = NULL)
        : CDialog(resID, pParent) {}

public:
    std::wstring m_title;
    std::wstring m_prompt;
    std::wstring m_value;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
};

class CInputDlg : public CInputDlgImpl
{
public:
	CInputDlg (CWnd* pParent = NULL)
        : CInputDlgImpl(IDD_INPUT, pParent) {}
};

class CPasswordDlg : public CInputDlgImpl
{
public:
	CPasswordDlg (CWnd* pParent = NULL)
        : CInputDlgImpl(IDD_INPUT_PASSWORD, pParent) {}
};

};//Common