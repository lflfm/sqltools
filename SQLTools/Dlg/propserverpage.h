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

#ifndef __PROPSERVERPAGE_H__
#define __PROPSERVERPAGE_H__
#pragma once

#include "SQLToolsSettings.h"

class CPropServerPage : public CPropertyPage
{
    SQLToolsSettings& m_settings;

public:
	CPropServerPage (SQLToolsSettings&);

	//{{AFX_DATA(CPropServerPage)
	enum { IDD = IDD_PROP_SERVER };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CPropServerPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
public:
    virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}

#endif//__PROPSERVERPAGE_H__
