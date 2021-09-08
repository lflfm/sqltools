/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2017 Aleksey Kochetov

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
2017-12-10 Popuip Viewer is based on the code from sqltools++ by Randolf Geist
*/

#pragma once

#include <afxhtml.h>


namespace OG2 /* OG = OpenGrid V2 */
{

class GridPopupEdit : public CEdit
{
	CFont m_Font;
	HACCEL m_accelTable;

public:
	GridPopupEdit();
	virtual ~GridPopupEdit();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd * pWnd, CPoint pos);
    afx_msg void OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnEditCopy();
	afx_msg void OnEditSelectAll();
	afx_msg void OnGridPopupClose();
    afx_msg void OnGridPopupWordWrap();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

class HtmlView : public CHtmlView
{
public:
    HtmlView () {}
    void LoadHTMLfromString (const std::wstring& wtext, const std::string& text);
    virtual void PostNcDestroy() {} // remove self-destructor

    DECLARE_MESSAGE_MAP()
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

class PopupFrameWnd : public CFrameWnd
{
    friend GridPopupEdit;
	DECLARE_DYNCREATE(PopupFrameWnd)
    CToolBar      m_toolbar;
	GridPopupEdit m_EditBox;
    HtmlView      m_hmlViewer;

    static PopupFrameWnd*  m_theOne;
    static CWnd*           m_lastClient;
    static WINDOWPLACEMENT m_lastWindowPlacement;
    static int             m_currentTab;
    static bool            m_bWordWrap;

	PopupFrameWnd();
	~PopupFrameWnd();
public:
    static void ShowPopup  (CWnd* client, const string& text);
    static void DestroyPopup (CWnd* client);

    void SetPopupText (const std::string& text);
    void ResoreLastWindowPlacement ();
    void SetEditorWordWrap (bool bWordWrap);
    bool GetEditorWordWrap () { return m_bWordWrap; };
    

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnGridPopupText();
	afx_msg void OnGridPopupHtml();
	afx_msg void OnGridPopupWordWrap();
    afx_msg void OnUpdate_GridPopupWordWrap (CCmdUI* pCmdUI);
};

}//namespace OG2
