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
#include "afxwin.h"
#include <set>
#include <string>
#include "ManagedListCtrlFilterDlg.rh"
#include "ListCtrlDataProvider.h"

namespace Common
{
    class ListCtrlManager;

class CManagedListCtrlFilterDlg : public CDialog
{
    CRect m_colHeaderRect;
    ListCtrlManager& m_listCtrlManager;
    ListCtrlManager::FilterCollection m_filter;
    int m_filterColumn; 

    CString m_value;
    int m_operation;

    CComboBox   m_valueList; 
    CComboBox   m_operationList;
    CComboBoxEx m_columnList;
    CImageList  m_filterImage;

    void updateColumnListIcons ();

public:
    CManagedListCtrlFilterDlg(CWnd* pParent, const CRect& colHeaderRect, ListCtrlManager&, int filterColumn);
	virtual ~CManagedListCtrlFilterDlg();

// Dialog Data
	enum { IDD = IDD_MLIST_FILTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

public:
    virtual BOOL OnInitDialog();
    afx_msg void OnCbnDblclk_List();
    afx_msg void OnCbnEditChange_List();
    afx_msg void OnCbnSelChange_List();
    afx_msg void OnBnClicked_Clear();
    afx_msg void OnBnClicked_ClearAll();
    afx_msg void OnCbnSelChange_ColList();
    afx_msg void OnBnClicked_Apply();
protected:
    virtual void OnOK();
};

}//namespace Common
