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

// 11.09.2002 improvement, string representation for NULL value in data grid
// 06.10.2002 changes, some simplifying

#pragma once
#ifndef __PROPGRIDPAGE_H__
#define __PROPGRIDPAGE_H__

#include "resource.h"
#include "SQLToolsSettings.h"

class CPropGridPage1 : public CPropertyPage
{
    SQLToolsSettings& m_settings;
public:
	CPropGridPage1 (SQLToolsSettings&);

	//{{AFX_VIRTUAL(CPropGridPage1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL
public:
    //DECLARE_MESSAGE_MAP()
};

class CPropGridPage2 : public CPropertyPage
{
    SQLToolsSettings& m_settings;
public:
	CPropGridPage2 (SQLToolsSettings&);

	//{{AFX_VIRTUAL(CPropGridPage1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL
public:
    //DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif//__PROPGRIDPAGE_H__
