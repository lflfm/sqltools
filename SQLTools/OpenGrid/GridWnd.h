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

#pragma once
#ifndef __GridWnd_h__
#define __GridWnd_h__

/***************************************************************************/
/*      Purpose: Grid implementation                                       */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003,2017 Aleksey Kochetov                           */
/***************************************************************************/

#include "stdafx.h"
#include <list>
#include <COMMON/VisualAttributes.h>
#include "GridManager.h"
#include "GridViewPaintAccessories.h"

// TODO: it should be replaced with grid own settings
#include <OpenEditor/OESettings.h>
#include "SQLTools.h" 

    class CPLSWorksheetDoc;

namespace OG2 /* OG = OpenGrid */
{
    enum ETextFormat;
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;

class GridWnd : public CWnd, protected OpenEditor::SettingsSubscriber
{
protected:
    GridWnd();
    virtual ~GridWnd();
    DECLARE_DYNCREATE(GridWnd)

    void setExportSettings (const SQLToolsSettings& setting);
    void getSelectionForExport (int& row, int& nrows, int& col, int& ncols) const;
    static bool showExportSettings (SQLToolsSettings& settings, LPCTSTR operation, bool bLocal, bool bFileOperation);

public:
    const AGridManager* GetGridManager () const;
    AGridManager* GetGridManager ();
    const AGridSource* GetGridSource () const;
    AGridSource* GetGridSource ();
    int GetGridSourceCount (eDirection dir) const;

    void SetVScroller (HWND hBarWnd);
    void SetHScroller (HWND hBarWnd);
    void RecalcVScroller ();
    void RecalcHScroller ();

    virtual bool IsEmpty () const;
    bool CanMoveUp () const;
    bool CanMoveDown () const;

    //{{AFX_VIRTUAL(GridWnd)
    public:
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnDraw(CDC*) {};
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(GridWnd)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg UINT OnGetDlgCode();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnPaint();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    void DoEditCopy (const SQLToolsSettings& setting, int);
    afx_msg void OnEditCopy();
    afx_msg void OnGridCopyAsText           ();
    afx_msg void OnGridCopyAsPlsqlValues    ();
    afx_msg void OnGridCopyAsPlsqlExpression();
    afx_msg void OnGridCopyAsPlsqlInList    ();
    afx_msg void OnGridCopyAsPlsqlInserts   ();
    afx_msg void OnEditPaste();
    afx_msg void OnEditCut();
    afx_msg void OnUpdateEditGroup(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_CopyHeaders(CCmdUI* pCmdUI);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    //}}AFX_MSG
    afx_msg void OnFileExport();
    afx_msg LRESULT OnGetFont (WPARAM, LPARAM);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pos);
    afx_msg void OnSettingChange (UINT uFlags, LPCTSTR lpszSection);

    DECLARE_MESSAGE_MAP()

protected:
    static UINT m_uWheelScrollLines;   // cached value for MS Weel support

    CGridViewPaintAccessoriesPtr m_paintAccessories;
    GridManager* m_pManager;
    BOOL m_ShouldDeleteManager;
    eDirection m_toolbarNavigationDir;
    std::string m_visualAttributeSetName;
    UINT m_uPopupMenuId;
    UINT m_uExpFileCounter;

    // SettingsSubscriber
    virtual void OnSettingsChanged ();

private:
    BOOL m_Erase;
    //CToolTipCtrl m_wndToolTip;
    
protected:
    void DoOpen (ETextFormat frm, const char* ext);
    void DoFileSave (const SQLToolsSettings&, int format, bool shellOpen);
    CPLSWorksheetDoc* DoOpenInEditor (const SQLToolsSettings&, int format);

public:
    void SetVisualAttributeSetName (const std::string&);
    const std::string& GetVisualAttributeSetName () const;

    afx_msg void OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnGridOutputOptions ();
    afx_msg void OnGridOutputExportAndOpen ();
    afx_msg void OnGridCopyAll();
    afx_msg void OnGridCopyColHeader();
    afx_msg void OnGridCopyHeaders();
    afx_msg void OnGridCopyRow();
    afx_msg void OnGridCopyCol();

    afx_msg void OnGridOutputOpen();
    afx_msg void OnSysColorChange();

    afx_msg void OnMoveUp ();
    afx_msg void OnMoveDown ();   
    afx_msg void OnMovePgUp ();   
    afx_msg void OnMovePgDown (); 
    afx_msg void OnMoveEnd ();    
    afx_msg void OnMoveHome ();   

    afx_msg void OnUpdate_MoveUp    (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_MoveDown  (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_MovePgUp  (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_MovePgDown(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_MoveEnd   (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_MoveHome  (CCmdUI* pCmdUI);

    afx_msg void OnGridClearRecords ();   
    afx_msg void OnUpdate_GridClearRecords (CCmdUI* pCmdUI);

    afx_msg void OnRotate ();   
    afx_msg void OnUpdate_Rotate (CCmdUI* pCmdUI);

    afx_msg void OnGridSettings ();
    afx_msg void OnUpdate_GridSettings (CCmdUI* pCmdUI);

    afx_msg void OnGridOpenWithIE ();
    afx_msg void OnGridOpenWithExcel ();
    
    afx_msg void OnGridPopup ();
};

/////////////////////////////////////////////////////////////////////////////

class GridStringSource;

class StrGridWnd : public GridWnd
{
protected:
    GridStringSource* m_pStrSource;

public:
    StrGridWnd();

    void Clear ();

    DECLARE_DYNCREATE(StrGridWnd);
};

/////////////////////////////////////////////////////////////////////////////

inline
const AGridManager* GridWnd::GetGridManager () const
    { return m_pManager; }

inline
AGridManager* GridWnd::GetGridManager ()
    { return m_pManager; }

inline
const AGridSource* GridWnd::GetGridSource () const
    { return m_pManager->m_pSource; }

inline
AGridSource* GridWnd::GetGridSource ()
    { return m_pManager->m_pSource; }

inline
int GridWnd::GetGridSourceCount (eDirection dir) const
    { return GetGridSource()->GetCount(dir); }

inline
void GridWnd::SetVisualAttributeSetName (const std::string& name)
    { m_visualAttributeSetName = name; }

inline
const std::string& GridWnd::GetVisualAttributeSetName () const
    { return m_visualAttributeSetName; }

inline
void GridWnd::SetVScroller (HWND hBarWnd)
    { _ASSERTE(m_pManager); m_pManager->m_Rulers[edVert].SetScroller(hBarWnd); }

inline
void GridWnd::SetHScroller (HWND hBarWnd)
    { _ASSERTE(m_pManager); m_pManager->m_Rulers[edHorz].SetScroller(hBarWnd); }

inline
void GridWnd::RecalcVScroller ()
    { _ASSERTE(m_pManager); m_pManager->CalcScroller(edVert); }

inline
void GridWnd::RecalcHScroller ()
    { _ASSERTE(m_pManager); m_pManager->CalcScroller(edHorz); }

inline
bool GridWnd::CanMoveUp () const     
    { return !IsEmpty() && m_pManager->m_Rulers[m_toolbarNavigationDir].GetCurrent() > 0; }

inline
bool GridWnd::CanMoveDown () const   
    { return !IsEmpty() && m_pManager->m_Rulers[m_toolbarNavigationDir].GetCurrent() < GetGridSource()->GetCount(m_toolbarNavigationDir)-1; }

//{{AFX_INSERT_LOCATION}}

}//namespace OG2

#endif//__GridWnd_h__
