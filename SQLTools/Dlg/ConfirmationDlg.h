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

#ifndef __CONFIRMATIONDLG_H__
#define __CONFIRMATIONDLG_H__
#pragma once


class CConfirmationDlg : public CDialog
{
public:
	CConfirmationDlg(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CConfirmationDlg)
	enum { IDD = IDD_CONFIRMATION };
	CString	m_strText;
	//}}AFX_DATA


	//{{AFX_VIRTUAL(CConfirmationDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CConfirmationDlg)
	afx_msg void OnYes();
	afx_msg void OnAll();
	afx_msg void OnNo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif//__CONFIRMATIONDLG_H__
