/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#ifndef __DBSOURCEWND_H__
#define __DBSOURCEWND_H__
#pragma once

#include <list>
#include <OCI8/Connect.h>
#include <COMMON/FixedArray.h>
#include "DbBrowser\DbBrowserList.h"
#include "LabelWithWaitBar.h"


    class SQLToolsSettings;

    class SchemaComboBox : public CComboBox
    {
        bool m_busy;
    public:
        SchemaComboBox () : m_busy(false) {}

        bool IsBusy () const { return m_busy; }
        void SetBusy (bool busy);

        DECLARE_MESSAGE_MAP();
        afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    };

class CDbSourceWnd : public CDialog
{
    static CPtrList m_thisDialogs;

// Construction
public:
	CDbSourceWnd ();
	virtual ~CDbSourceWnd();

// Attributes
public:
    CImageList m_Images;
    HACCEL m_accelTable;

    Common::FixedArray<DbBrowserList*, 22> m_wndTabLists;
    int m_nItems, m_nSelItems;

    DbBrowserList* GetCurSel () const;

// Dialog Data
	//{{AFX_DATA(CDbSourceWnd)
	enum { IDD = IDD_DB_SOURCE };
	CTabCtrl	m_wndTab;
	BOOL	m_bValid;
	BOOL	m_bInvalid;
	CString	m_strSchema;
	int		m_nViewAs;
	//}}AFX_DATA
	
    SchemaComboBox   m_wndSchemaList;
    LabelWithWaitBar m_wndStatus;

	BOOL m_AllSchemas;
    BOOL m_bShowTabTitles;
	string m_defSchema;
	std::list<std::wstring> m_recentSchemas;

    enum SchemaListStatus { SLS_EMPTY, SLS_CURRENT_ONLY, SLS_LOADING, SLS_LOADED } m_schemaListStatus;

	void addToRecentSchemas (const wchar_t*);

// Operations
public:
    BOOL IsVisible () const; 
    BOOL AreTabTitlesVisible () const   { return m_bShowTabTitles; }
    BOOL Create (CWnd* pParentWnd);
    void InitSchemaList ();
    void PopulateSchemaList ();
    void SetSchemaForObjectLists ();
    void OnSwitchTab (unsigned int);

    void EvOnConnect ();
    void EvOnDisconnect ();
    void OnLoad (bool bAsOne);

// Overrides
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL OnInitDialog();
    virtual void OnOK ();
    virtual void OnCancel();

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDropDownSchema();
    afx_msg void OnSetFocusSchema();
    afx_msg void OnSchemaChanged();
    afx_msg void OnToolbarFilterChanged();
    afx_msg void OnRefresh();
    afx_msg void OnShowAsList();
    afx_msg void OnShowAsReport();
    afx_msg void OnDestroy();
    afx_msg void OnSettings();
    afx_msg void OnFilter();
    afx_msg void OnTabTitles();
    afx_msg void OnSelChangeTab(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnIdleUpdateCmdUI ();
    afx_msg LRESULT OnHelpHitTest(WPARAM, LPARAM lParam);
    afx_msg BOOL OnToolTipText (UINT, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

//{{AFX_INSERT_LOCATION}}
#endif//__DBSOURCEWND_H__
