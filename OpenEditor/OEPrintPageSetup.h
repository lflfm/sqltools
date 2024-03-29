/* 
    Copyright (C) 2002 Aleksey Kochetov

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEPrintPageSetup_h__
#define __OEPrintPageSetup_h__

#include "resource.h"
#include "OpenEditor/OESettings.h"


    using OpenEditor::SettingsManager;

class COEPrintPageSetup : public CDialog
{
public:
    COEPrintPageSetup (OpenEditor::SettingsManager&);

    OpenEditor::SettingsManager& m_manager;

    BOOL	m_BlackAndWhite, m_LineNumbers;
    std::string m_Header, m_Footer;
    double
        m_LeftMargin, m_RightMargin,
        m_TopMargin, m_BottomMargin;

    //{{AFX_DATA(COEPrintPageSetup)
    enum { IDD = IDD_OE_PRINT_PAGE_SETUP };
    //}}AFX_DATA

    //{{AFX_VIRTUAL(COEPrintPageSetup)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL

protected:

    //{{AFX_MSG(COEPrintPageSetup)
    //}}AFX_MSG
    //DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}

#endif//__OEPrintPageSetup_h__
