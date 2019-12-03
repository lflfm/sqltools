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

#pragma once


// CTableTruncateOptionsDlg dialog

class CTableTruncateOptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(CTableTruncateOptionsDlg)

public:
	CTableTruncateOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTableTruncateOptionsDlg();

// Dialog Data
	enum { IDD = IDD_DB_TRUNCATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//DECLARE_MESSAGE_MAP()
public:
    BOOL m_checkDependencies;
    BOOL m_disableFKs;
    BOOL m_scriptOnly;
    BOOL m_keepAllocated;
protected:
    virtual void OnOK();
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};
