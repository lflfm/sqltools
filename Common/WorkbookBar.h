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

/*
    11.10.2006 bug fix, invalid path in tooltip after tab reorganization 
*/

#pragma once
#ifndef __WORKBOOKBAR_H__
#define __WORKBOOKBAR_H__

#include "resource.h"
#include "OETabCtrl.h"
#include <map>
#include <string>

class CWorkbookBar : public CDialogBar
{
protected:
	//{{AFX_DATA(CWorkbookBar)
	enum { IDD = IDD_OE_WORKBOOK_BAR };
	COETabCtrl m_wndTab;
	//}}AFX_DATA
    typedef std::map<LPARAM, std::string> TooltipMap;
    TooltipMap m_tooltips;

public:
    BOOL Create (CWnd* pParentWnd, UINT uID);
    void SetImageList (CImageList* pImgList) { m_wndTab.SetImageList(pImgList); }

    void AppendItem (TCITEM&);
    void UpdateItemByParam (LPARAM param, TCITEM&, LPCSTR tooltipText);
    void RemoveItemByParam (LPARAM param);
    void ActivateItemByParam (LPARAM param);
    LPARAM GetCurSelParam ();
    BOOL IsEpmty() const { return !m_wndTab.GetItemCount() ? TRUE : FALSE; }
    void GetAllTabParams (std::vector<LPARAM>&) const;
protected:
    int FindTabByParam (LPARAM param) const;
    void SetToolTipsText (int nItem, LPARAM param, LPCSTR tooltipText);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual CSize CalcFixedLayout (BOOL bStretch, BOOL bHorz);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()

    afx_msg LRESULT OnInitDialog (WPARAM, LPARAM);
	afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTab_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnCbar_Hide();
    afx_msg BOOL OnTab_TTNNeedText (UINT id, NMHDR* pNMHDR, LRESULT* pResult);
};

//{{AFX_INSERT_LOCATION}}

#endif//__WORKBOOKBAR_H__
