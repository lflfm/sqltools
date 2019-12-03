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

#ifndef __CHILDFRM_H__
#define __CHILDFRM_H__
#pragma once

#include "SQLWorksheet/2PaneSplitter.h"

    class CBooklet;

class CMDIChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMDIChildFrame)
public:
    static BOOL m_MaximizeFirstDocument;

	CMDIChildFrame();
	virtual ~CMDIChildFrame();

	//{{AFX_VIRTUAL(CMDIChildFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CMDIChildFrame)
	afx_msg void OnCustomNextPane();
	afx_msg void OnCustomPrevPane();
	afx_msg void OnCustomNextTab();
	afx_msg void OnCustomPrevTab();
	afx_msg void OnCustomSplitterDown();
	afx_msg void OnCustomSplitterUp();
	afx_msg void OnCustomSplitterDefault();
	afx_msg void OnDestroy();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	//}}AFX_MSG
    afx_msg LRESULT OnSetText (WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

protected:
	C2PaneSplitter m_wndSplitter;
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};

//{{AFX_INSERT_LOCATION}}

#endif//__CHILDFRM_H__
