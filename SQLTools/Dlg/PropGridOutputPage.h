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
#ifndef __PropGridOutputPage_H__
#define __PropGridOutputPage_H__

#include "resource.h"
#include "SQLToolsSettings.h"
#include <COMMON/EditWithSelPos.h>

// CPropGridOutputPage dialog

class CPropGridOutputPage1 : public CPropertyPage
{
    CEditWithSelPos m_edtTemplate;
    SQLToolsSettings& m_settings;
    void setupItems ();
public:
    BOOL m_bShowShowOnShiftOnly;
    BOOL m_bShowOnShiftOnly;
    BOOL m_bEnableFilenameTemplate;

    CPropGridOutputPage1(SQLToolsSettings&);

// Dialog Data
    enum { IDD = IDD_PROP_GRID_OUTPUT };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
public:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnExpFormatChanged();
    afx_msg void OnBnClicked_Substitution ();
    afx_msg void OnSubstitution (UINT id);
};

class CPropGridOutputPage2 : public CPropertyPage
{
    SQLToolsSettings& m_settings;
    void setupItems ();

public:
    CPropGridOutputPage2(SQLToolsSettings&);

// Dialog Data
    enum { IDD = IDD_PROP_GRID_OUTPUT_CSV };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
public:
    DECLARE_MESSAGE_MAP()
};

#endif//__PropGridOutputPage_H__
