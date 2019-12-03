/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 2011 Aleksey Kochetov

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

    
    class SQLToolsSettings;

class CObjectViewerPage : public CPropertyPage
{
    SQLToolsSettings& m_settings;

public:
	CObjectViewerPage (SQLToolsSettings&);   // standard constructor

	//{{AFX_DATA(CObjectViewerPage)
	enum { IDD = IDD_PROP_OBJECT_VIEWER };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CObjectViewerPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CObjectViewerPage)
	//}}AFX_MSG
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()
};
