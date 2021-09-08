/* 
    Copyright (C) 2005 Aleksey Kochetov

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

#include <string>
#include "resource.h"
#include <COMMON/EditWithSelPos.h>
#include "OpenEditor/OEDocManager.h"

class COENewFileDlg : public CDialog
{
// Dialog Data
    enum { IDD = IDD_OE_NEW_FILE };
    CString m_category, m_template, m_filename;
    CEditWithSelPos m_edtTemplate;
    COEMultiDocTemplate& m_docTemplate;

public:
    COENewFileDlg (COEMultiDocTemplate&, CWnd* pParent = NULL);
    virtual ~COENewFileDlg();

    const CString& GetFilename () const { return m_filename; }  
    const CString& GetCategory () const { return m_category; }  
    const CString& GetTemplate () const { return m_template; }  

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()
    afx_msg void OnCbnSelchange_Category ();
    afx_msg void OnBnClicked_Substitution ();
    afx_msg void OnEnChange_Template ();
    afx_msg void OnSubstitution (UINT id);
};
