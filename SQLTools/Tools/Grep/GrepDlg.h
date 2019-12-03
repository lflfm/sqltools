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

#ifndef __GREPDLG_H__
#define __GREPDLG_H__
#pragma once

    class CGrepView;


class CGrepDlg : public CDialog
{
	HICON m_hIcon;
    CGrepView* m_pView;

public:
	CGrepDlg (CWnd* pParent, CGrepView* pView);

	//{{AFX_DATA(CGrepDlg)
	enum { IDD = IDD_FILE_FIND };
	BOOL	m_bMathWholeWord;
	BOOL	m_bMathCase;
	BOOL	m_bUseRegExpr;
	BOOL	m_bLookInSubfolders;
	BOOL	m_bSaveFiles;
	BOOL	m_bCollapsedList;
	CString	m_strFileOrType;
	CString	m_strFolder;
	CString	m_strWhatFind;
	//}}AFX_DATA
	CButton	  m_wndInsertExpr;
	CComboBox m_wndFolder;
	CComboBox m_wndFileOrType;
	CComboBox m_wndWhatFind;

	//{{AFX_VIRTUAL(CGrepDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL
	virtual void OnOK ();

protected:
	//{{AFX_MSG(CGrepDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectFolder();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}

#endif//__GREPDLG_H__
