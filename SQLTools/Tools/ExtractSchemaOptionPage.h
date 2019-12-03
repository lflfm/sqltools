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
#ifndef _EXTRACTSCHEMAOPTIONPAGE_H_
#define _EXTRACTSCHEMAOPTIONPAGE_H_

    class ExtractDDLSettings;

class CExtractSchemaOptionPage : public CPropertyPage
{
	enum { IDD = IDD_ES_OPTION_PAGE };
    ExtractDDLSettings& m_DDLSettings;

public:
	CExtractSchemaOptionPage (ExtractDDLSettings&);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);
protected:
	afx_msg void OnCommentsAfterColumn();

	DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif//_EXTRACTSCHEMAOPTIONPAGE_H_
