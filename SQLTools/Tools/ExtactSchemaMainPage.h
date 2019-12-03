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

#ifndef __EXTRACTSCHEMAMAINPAGE_H__
#define __EXTRACTSCHEMAMAINPAGE_H__
#pragma once

#include "resource.h"
#include "Dlg\SchemaList.h"

    class ExtractDDLSettings;

class CExtactSchemaMainPage : public CPropertyPage
{
public:
	CExtactSchemaMainPage (ExtractDDLSettings&);

	//{{AFX_DATA(CExtactSchemaMainPage)
	enum { IDD = IDD_ES_MAIN_PAGE };
	//}}AFX_DATA

    ExtractDDLSettings& m_DDLSettings;

	CSchemaList m_wndSchemaList;
	CComboBox   m_wndFolder,
                m_wndTableTablespace,
                m_wndIndexTablespace;

    void ShowFullPath ();
    void SetStatusText (const char*);
    void UpdateDataAndSaveHistory ();

	//{{AFX_VIRTUAL(CExtactSchemaMainPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CExtactSchemaMainPage)
	afx_msg void OnSelectFolder();
	virtual BOOL OnInitDialog();
	afx_msg void OnFolderOptions();
	afx_msg void OnInvalidateFullPath();
    afx_msg void OnSchemaListLoaded (NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	afx_msg void OnUseDbAlias ();
	afx_msg void OnUseDbAliasAs (UINT);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
#endif//__EXTRACTSCHEMAMAINPAGE_H__
