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

#ifndef _SCHEMALIST_H_
#define _SCHEMALIST_H_
#pragma once

class CSchemaList : public CComboBox
{
    friend struct GetDefSchemaTask;
    friend struct PopulateSchemaListTask;
    CString m_defSchema;
    BOOL    m_allSchemas;
    BOOL    m_busy; 
public:
    static const int NM_LIST_LOADED = 1;

	CSchemaList (BOOL allSchemas = TRUE);

    void UpdateSchemaList ();

protected:
	//{{AFX_VIRTUAL(CSchemaList)
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CSchemaList)
	afx_msg void OnSetFocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};

#endif//_SCHEMALIST_H_
