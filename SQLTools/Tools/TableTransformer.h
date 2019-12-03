/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#ifndef _TABLETRANSFORMER_H_
#define _TABLETRANSFORMER_H_
#pragma once
#include "Dlg/SchemaList.h"


class CTableTransformer : public CDialog
{
	CSchemaList  m_wndSchemaList;
	CComboBox	 m_wndTableList;
	CString      m_strSchema, m_strTable;
public:
    static void MakeScript (const string& schema, const string& table);

	CTableTransformer(CWnd* pParent = NULL);

	//{{AFX_DATA(CTableTransformer)
	enum { IDD = IDD_TABLE_TRANSFOMER };
	//}}AFX_DATA


	//{{AFX_VIRTUAL(CTableTransformer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CTableTransformer)
	afx_msg void OnSelChangeSchema();
    afx_msg void OnSchemaListLoaded (NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
#endif//_TABLETRANSFORMER_H_
