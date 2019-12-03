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

/***************************************************************************/
/*      Purpose: Grid implementation                                       */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003,2017 Aleksey Kochetov                           */
/***************************************************************************/

// 27.10.2003 bug fix, export settings do not affect on quick html/csv viewer launch
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
// 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 2017-11-28 implemented range selection
// 2017-11-28 added 3 new export formats

#include "stdafx.h"
#include <list>
#include <fstream>
#include <sstream>
#include <regex> // used in GridView::DoFileSave
#include "GridManager.h"
#include "GridView.h"
#include "GridSourceBase.h"
#include "Dlg/PropGridOutputPage.h"
#include "COMMON/TempFilesManager.h"
#include "COMMON\AppUtilities.h"
//#include "COMMON/GUICommandDictionary.h"
#include "PopupViewer.h"

// the following section is included only for OnGridSettings
#include "COMMON/VisualAttributesPage.h"
#include "Dlg/PropGridPage.h"
#include "Dlg/PropGridOutputPage.h"
#include "Dlg/PropHistoryPage.h"
#include "COMMON/PropertySheetMem.h"
#include "COMMON/FilenameTemplate.h"

#include "SQLWorksheet/SQLWorksheetDoc.h" // it's bad because of dependency on CPLSWorksheetDoc

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2 /* OG = OpenGrid V2 */
{

    UINT GridView::m_uWheelScrollLines; // cached value for MS Weel support
    static const char g_szClassName[] = "Kochware.OpenGrid.V2";

    IMPLEMENT_DYNCREATE(GridView, CView)
    IMPLEMENT_DYNCREATE(StrGridView, GridView)


StrGridView::StrGridView ()
{
    m_pStrSource = new GridStringSource;
    m_pManager = new GridManager(this, m_pStrSource, TRUE);
    m_pManager->m_Options.m_Editing = true;
    m_ShouldDeleteManager = true;
}

void StrGridView::Clear ()
{
    m_pStrSource->DeleteAll();
    m_pManager->Clear();
}

GridView::GridView()
: m_Erase(FALSE),
  m_pManager(0),
  m_ShouldDeleteManager(FALSE),
  m_toolbarNavigationDir(edVert),
  m_uPopupMenuId(IDR_GRID_POPUP),
  m_uExpFileCounter(0)
{
    GetSQLToolsSettings().AddSubscriber(this);
}

GridView::~GridView()
{
    try { EXCEPTION_FRAME;

        GetSQLToolsSettings().RemoveSubscriber(this);

        if (m_ShouldDeleteManager)
            delete m_pManager;

        PopupFrameWnd::DestroyPopup(this);
    }
    _DESTRUCTOR_HANDLER_;
}

bool GridView::IsEmpty () const
{
    _ASSERTE(m_pManager && GetGridSource());
    return GetGridSource()->IsEmpty();
}

void GridView::setExportSettings (const SQLToolsSettings& setting)
{
    _ASSERTE(m_pManager && GetGridSource());

#pragma warning (disable : 4189) // because of compiler failure
    GridStringSource* source = dynamic_cast<GridStringSource*>(GetGridSource());

    switch (setting.GetGridExpFormat())
    {
    case 0: source->SetOutputFormat(etfPlainText);           break;
    case 1: source->SetOutputFormat(etfQuotaDelimited);      break;
    case 2: source->SetOutputFormat(etfTabDelimitedText);    break;
    case 3: source->SetOutputFormat(etfXmlElem);             break;
    case 4: source->SetOutputFormat(etfXmlAttr);             break;
    case 5: source->SetOutputFormat(etfHtml);                break;
    case 6: source->SetOutputFormat(etfPLSQL1);              break;
    case 7: source->SetOutputFormat(etfPLSQL2);              break;
    case 8: source->SetOutputFormat(etfPLSQL3);              break;
    case 9: source->SetOutputFormat(etfPLSQL4);              break;
    default: _ASSERTE(0);
    }

    source->SetFieldDelimiterChar (setting.GetGridExpCommaChar()[0]);
    source->SetQuoteChar          (setting.GetGridExpQuoteChar()[0]);
    source->SetQuoteEscapeChar    (setting.GetGridExpQuoteEscapeChar()[0]);
    source->SetPrefixChar         (setting.GetGridExpPrefixChar()[0]);
    source->SetOutputWithHeader   (setting.GetGridExpWithHeader());
    source->SetColumnNameAsAttr   (setting.GetGridExpColumnNameAsAttr());

    source->SetXmlRootElement      (setting.GetGridExpXmlRootElement());
    source->SetXmlRecordElement    (setting.GetGridExpXmlRecordElement());
    source->SetXmlFieldElementCase (setting.GetGridExpXmlFieldElementCase());
}

void GridView::getSelectionForExport (int& row, int& nrows, int& col, int& ncols) const
{
    row = nrows = col = ncols = -1;

    if (!m_pManager->IsRangeSelectionEmpty())
    {
        int first, last;
        if (m_pManager->m_Rulers[edVert].GetNormalizedSelection(first, last))
        {
            row = first;
            nrows = last - first + 1;
        }
        if (m_pManager->m_Rulers[edHorz].GetNormalizedSelection(first, last))
        {
            col = first;
            ncols = last - first + 1;
        }
    }
    else
    {
        row = m_pManager->m_Rulers[edVert].GetCurrent();
        col = m_pManager->m_Rulers[edHorz].GetCurrent();
        nrows = ncols = 1;
    }
}

bool GridView::showExportSettings (SQLToolsSettings& settings, LPCTSTR operation, bool bLocal, bool bFileOperation)
{
    BOOL showOnShiftOnly = AfxGetApp()->GetProfileInt("showOnShiftOnly", "GridExportSettings",  FALSE);

    CPropertySheet sheet(!bLocal ? "Settings" : CString("Settings for the current \"") + operation + "\" operation ONLY!");
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    if (!bLocal || !showOnShiftOnly || HIBYTE(GetKeyState(VK_SHIFT))) 
    {
        CPropGridOutputPage1  gridOutputPage1(settings);
        gridOutputPage1.m_psp.pszTitle = "Format";
        gridOutputPage1.m_psp.dwFlags |= PSP_USETITLE;

        gridOutputPage1.m_bShowShowOnShiftOnly = bLocal ? TRUE : FALSE;
        gridOutputPage1.m_bShowOnShiftOnly = showOnShiftOnly;
        gridOutputPage1.m_bEnableFilenameTemplate = bFileOperation ? TRUE : FALSE;

        sheet.AddPage(&gridOutputPage1);

        CPropGridOutputPage2  gridOutputPage2(settings);
        gridOutputPage2.m_psp.pszTitle = "CSV/XML preferences";
        gridOutputPage2.m_psp.dwFlags |= PSP_USETITLE;

        sheet.AddPage(&gridOutputPage2);

        if (sheet.DoModal() == IDOK) 
        {
            if (bLocal)
                AfxGetApp()->WriteProfileInt("showOnShiftOnly", "GridExportSettings",  gridOutputPage1.m_bShowOnShiftOnly);
            return true;
        } 
        else
            return false;
    } 
    else
        return true;
}

BEGIN_MESSAGE_MAP(GridView, CView)
    //{{AFX_MSG_MAP(GridView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_GETDLGCODE()
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_MBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditGroup)

    ON_COMMAND(ID_GRID_COPY_AS_TEXT,             OnGridCopyAsText            )
    ON_COMMAND(ID_GRID_COPY_AS_PLSQL_VALUES,     OnGridCopyAsPlsqlValues     )
    ON_COMMAND(ID_GRID_COPY_AS_PLSQL_EXPRESSION, OnGridCopyAsPlsqlExpression )
    ON_COMMAND(ID_GRID_COPY_AS_PLSQL_IN_LIST,    OnGridCopyAsPlsqlInList     )
    ON_COMMAND(ID_GRID_COPY_AS_PLSQL_INSERTS,    OnGridCopyAsPlsqlInserts    )
    ON_UPDATE_COMMAND_UI_RANGE(ID_GRID_COPY_AS_FIRST, ID_GRID_COPY_AS_LAST, OnUpdateEditGroup)

    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEWHEEL()
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_GETFONT, OnGetFont)
    ON_WM_CONTEXTMENU()
    ON_WM_SETTINGCHANGE()
    ON_WM_INITMENUPOPUP()

    ON_COMMAND(ID_GRID_OUTPUT_OPTIONS, OnGridOutputOptions)
    ON_COMMAND(ID_GRID_OUTPUT_EXPORT, OnFileExport)
    ON_COMMAND(ID_GRID_OUTPUT_EXPORT_AND_OPEN, OnGridOutputExportAndOpen)

    ON_COMMAND(ID_GRID_COPY_COL_HEADER, OnGridCopyColHeader)
    ON_COMMAND(ID_GRID_COPY_HEADERS, OnGridCopyHeaders)
    ON_COMMAND(ID_GRID_COPY_ROW, OnGridCopyRow)
    ON_COMMAND(ID_GRID_COPY_ALL, OnGridCopyAll)
    ON_COMMAND(ID_GRID_COPY_COL, OnGridCopyCol)

    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_EXPORT, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_EXPORT_AND_OPEN, OnUpdateEditGroup)

    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_ALL, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_COL_HEADER, OnUpdate_CopyHeaders)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_HEADERS, OnUpdate_CopyHeaders)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_ROW, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_COL, OnUpdateEditGroup)

    ON_COMMAND(ID_GRID_OUTPUT_OPEN, OnGridOutputOpen)
    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_OPEN, OnUpdateEditGroup)
    ON_WM_SYSCOLORCHANGE()

    ON_COMMAND(ID_GRID_MOVE_UP,      OnMoveUp)
    ON_COMMAND(ID_GRID_MOVE_DOWN,    OnMoveDown)
    ON_COMMAND(ID_GRID_MOVE_PGUP,    OnMovePgUp)
    ON_COMMAND(ID_GRID_MOVE_PGDOWN,  OnMovePgDown)
    ON_COMMAND(ID_GRID_MOVE_END,     OnMoveEnd)
    ON_COMMAND(ID_GRID_MOVE_HOME,    OnMoveHome)

    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_UP,     OnUpdate_MoveUp    )
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_DOWN,   OnUpdate_MoveDown  )
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_PGUP,   OnUpdate_MovePgUp  )
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_PGDOWN, OnUpdate_MovePgDown)
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_END,    OnUpdate_MoveEnd   )
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_HOME,   OnUpdate_MoveHome  )

    ON_COMMAND(ID_GRID_CLEAR_RECORDS, OnGridClearRecords)
    ON_UPDATE_COMMAND_UI(ID_GRID_CLEAR_RECORDS, OnUpdate_GridClearRecords)

    ON_COMMAND(ID_GRID_ROTATE, OnRotate)
    ON_UPDATE_COMMAND_UI(ID_GRID_ROTATE, OnUpdate_Rotate)

    ON_COMMAND(ID_GRID_SETTINGS, OnGridSettings)
    ON_UPDATE_COMMAND_UI(ID_GRID_SETTINGS, OnUpdate_GridSettings)

    ON_COMMAND(ID_GRID_OPEN_WITH_IE, OnGridOpenWithIE)
    ON_COMMAND(ID_GRID_OPEN_WITH_EXCEL, OnGridOpenWithExcel)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_IE, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_EXCEL, OnUpdateEditGroup)

    ON_COMMAND(ID_GRID_POPUP, OnGridPopup)
    ON_UPDATE_COMMAND_UI(ID_GRID_POPUP, OnUpdateEditGroup)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GridView message handlers

BOOL GridView::PreCreateWindow(CREATESTRUCT& cs)
{
    WNDCLASS wndClass;
    BOOL bRes = CView::PreCreateWindow(cs);
    HINSTANCE hInstance = AfxGetInstanceHandle();

    // see if the class already exists
    if (!::GetClassInfo(hInstance, g_szClassName, &wndClass))
    {
        // get default stuff
        ::GetClassInfo(hInstance, cs.lpszClass, &wndClass);
        wndClass.lpszClassName = g_szClassName;
        wndClass.style &= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wndClass.hbrBackground = 0;
        // register a new class
        if (!AfxRegisterClass(&wndClass))
            AfxThrowResourceException();
    }
    cs.lpszClass = g_szClassName;

    return bRes;
}

LRESULT GridView::OnGetFont (WPARAM, LPARAM)
{
    if (m_paintAccessories.get() && (HFONT)m_paintAccessories->m_Font)
    {
        return (LRESULT)(HFONT)m_paintAccessories->m_Font;
    }
    else if (GetParent())
    {
        return GetParent()->SendMessage(WM_GETFONT, 0, 0L);
    }
    else
        return (LRESULT)::GetStockObject(DEFAULT_GUI_FONT);
}

void GridView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    CView::OnPrepareDC(pDC, pInfo);

    if (m_paintAccessories.get() && (HFONT)m_paintAccessories->m_Font)
    {
        pDC->SelectObject(&m_paintAccessories->m_Font);
    }
    else if (GetParent())
    {
        HFONT hfont = (HFONT)GetParent()->SendMessage(WM_GETFONT, 0, 0L);
        if (hfont)
            pDC->SelectObject(hfont);
    }
    else
        pDC->SelectObject(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
}

int GridView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (m_pManager == 0 || CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_paintAccessories = CGridViewPaintAccessories::GetPaintAccessories(GetVisualAttributeSetName());
    m_paintAccessories->OnSettingsChanged(GetSQLToolsSettings().GetVASet(GetVisualAttributeSetName()));

    CRect rect;
    GetClientRect(rect);

    TEXTMETRIC tm;
    CClientDC dc(this);
    OnPrepareDC(&dc, 0);
    dc.GetTextMetrics(&tm);

    CSize size((2 * tm.tmAveCharWidth + tm.tmMaxCharWidth) / 3, tm.tmHeight);
    m_pManager->EvOpen(rect, size);

    SetTimer(1, 100, NULL);

    return 0;
}

void GridView::OnDestroy()
{
    m_pManager->EvClose();

    KillTimer(1);

    CView::OnDestroy();
}

void GridView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && (nType == SIZE_MAXIMIZED || nType == SIZE_RESTORED))
    {
        m_pManager->EvSize(CSize(cx, cy));
        Invalidate(TRUE);
        m_Erase = TRUE;
    }
}

void GridView::OnHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar*)
{
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_TRACKPOS;
    GetScrollInfo(SB_HORZ, &scrollInfo);

    m_pManager->EvScroll(edHorz, nSBCode, scrollInfo.nTrackPos);
}

void GridView::OnVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar*)
{
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_TRACKPOS;
    GetScrollInfo(SB_VERT, &scrollInfo);

    m_pManager->EvScroll(edVert, nSBCode, scrollInfo.nTrackPos);
}

UINT GridView::OnGetDlgCode()
{
    return ((!m_pManager->m_Options.m_Tabs) ? 0 : DLGC_WANTTAB)
             | DLGC_WANTARROWS | DLGC_WANTCHARS
             | CView::OnGetDlgCode();
}

HBRUSH GridView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_EDIT
    || nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetBkColor(RGB(255,255,255));
        return (HBRUSH)::GetStockObject(WHITE_BRUSH);
    }
    else
        return CView::OnCtlColor(pDC, pWnd, nCtlColor);
}

void GridView::OnPaint()
{
    CPaintDC dc(this);
    OnPrepareDC(&dc, 0);

    m_pManager->Paint(dc, (m_Erase && dc.m_ps.fErase), dc.m_ps.rcPaint);

    m_Erase = FALSE;
}

void GridView::OnSetFocus(CWnd* pOldWnd)
{
    m_pManager->EvSetFocus(TRUE);

    CView::OnSetFocus(pOldWnd);

    SendMessage(WM_NCACTIVATE, TRUE);

    // don't recall why I added this but it looks wrong now
    //if (GetParent())
    //    GetParent()->SendNotifyMessage(GetDlgCtrlID(), 1, (LPARAM)this);
}

void GridView::OnKillFocus(CWnd* pNewWnd)
{
    m_pManager->EvSetFocus(FALSE);

    CView::OnKillFocus(pNewWnd);

    SendMessage(WM_NCACTIVATE, FALSE);

    // don't recall why I added this but it looks wrong now
    //if (GetParent())
    //    GetParent()->SendNotifyMessage(GetDlgCtrlID(), 2, (LPARAM)this);
}

void GridView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (!m_pManager->EvKeyDown(nChar, nRepCnt, nFlags))
        CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void GridView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    m_pManager->EvLButtonDblClk(nFlags, point);
}

    static CRect s_lastClickRect;
    const int DRAGGING_THRESHOLD = 3;

void GridView::OnLButtonDown (UINT nFlags, CPoint point)
{
    s_lastClickRect.TopLeft() = point;
    s_lastClickRect.TopLeft().Offset(-DRAGGING_THRESHOLD, -DRAGGING_THRESHOLD);
    s_lastClickRect.BottomRight() = point;
    s_lastClickRect.BottomRight().Offset(DRAGGING_THRESHOLD, DRAGGING_THRESHOLD);

    if (!m_pManager->IsPointOverRangeSelection(point))
    {
        m_pManager->EvLButtonDown(nFlags, point);
        
        if ((0xFF00 & GetKeyState(VK_MENU)))
            OnGridPopup();
    }
}

void GridView::OnMButtonDown (UINT /*nFlags*/, CPoint point)
{
    s_lastClickRect.TopLeft() = point;
    s_lastClickRect.TopLeft().Offset(-DRAGGING_THRESHOLD, -DRAGGING_THRESHOLD);
    s_lastClickRect.BottomRight() = point;
    s_lastClickRect.BottomRight().Offset(DRAGGING_THRESHOLD, DRAGGING_THRESHOLD);

    if (!m_pManager->IsPointOverRangeSelection(point))
    {
        m_pManager->EvLButtonDown(0, point);
        
        OnGridPopup();
    }
}

void GridView::OnLButtonUp(UINT nFlags, CPoint point)
{
    CView::OnLButtonUp(nFlags, point);

    if (m_pManager->IsPointOverRangeSelection(point)
    && s_lastClickRect.PtInRect(point))
        m_pManager->NavGridManager::EvLButtonDown(nFlags, point);

    m_pManager->EvLButtonUp(nFlags, point);
}

// check the history for tooltips support 
void GridView::OnMouseMove (UINT nFlags, CPoint point)
{
    m_pManager->EvMouseMove(nFlags, point);

    if ((nFlags & MK_LBUTTON) && !m_pManager->ResizingProcess()) // Drag & Drop
    {
        if (/*m_pManager->IsPointOverRangeSelection(point)
        && */!s_lastClickRect.PtInRect(point))
        {
            setExportSettings(GetSQLToolsSettings()); // to beginning of drag

            int row, nrows, col, ncols;
            getSelectionForExport(row, nrows, col, ncols);

            if (GetGridSource())
            {
                if (m_pManager->IsFixedCorner(point))
                    GetGridSource()->DoDragDrop(row, 1, col, 1, ecDragTopLeftCorner);
                else if (m_pManager->IsFixedCol(point.x))
                {
                    if (row < GetGridSource()->GetCount(edVert))
                        GetGridSource()->DoDragDrop(row, nrows, -1, -1, ecDragRowHeader);
                }
                else if (m_pManager->IsFixedRow(point.y))
                {
                    if (col < GetGridSource()->GetCount(edHorz))
                        GetGridSource()->DoDragDrop(-1, -1, col, ncols, ecDragColumnHeader);
                }
                else if (m_pManager->IsDataCell(point))
                    GetGridSource()->DoDragDrop(row, nrows, col, ncols, ecDataCell);
            }
        }
    }

    CView::OnMouseMove(nFlags, point);
}

void GridView::OnTimer (UINT)
{
    m_pManager->IdleAction();
}

void GridView::OnSettingsChanged ()
{
    m_paintAccessories = CGridViewPaintAccessories::GetPaintAccessories(GetVisualAttributeSetName());
    m_paintAccessories->OnSettingsChanged(GetSQLToolsSettings().GetVASet(GetVisualAttributeSetName()));

    TEXTMETRIC tm;
    CClientDC dc(this);
    OnPrepareDC(&dc, 0);
    dc.GetTextMetrics(&tm);

    CSize size((2 * tm.tmAveCharWidth + tm.tmMaxCharWidth) / 3, tm.tmHeight);
    m_pManager->EvFontChanged(size);
}

LRESULT GridView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CView::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_;
    return 0;
}

BOOL GridView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    try { EXCEPTION_FRAME;

        CWnd* pInplaceEditor = m_pManager->GetEditor();

        if (pInplaceEditor != NULL && pInplaceEditor->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
            return TRUE;

        // then pump through frame
        if (CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
            return TRUE;

        return FALSE;
    }
    _DEFAULT_HANDLER_;

    return TRUE;
}

void GridView::OnContextMenu (CWnd*, CPoint pos)
{
    if (GetGridSource())
    {
        if (pos.x == -1 && pos.y == -1) // invoked by keyboard
        {
            if (m_pManager->IsCurrentVisible()) 
            {
                CRect rc;
                m_pManager->GetCurRect(rc);
                ClientToScreen(rc);
                pos.x = rc.left + 3;
                pos.y = rc.bottom;
            }
            else
            {
                int loc[2];
                m_pManager->m_Rulers[edHorz].GetDataArea(loc);
                pos.x = loc[0];
                m_pManager->m_Rulers[edVert].GetDataArea(loc);
                pos.x = loc[0];
                ClientToScreen(&pos);
            }
        }
        CMenu menu;
        VERIFY(menu.LoadMenu(m_uPopupMenuId));
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);
        //Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
    }
}

void GridView::OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0)
    {
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_COL_HEADER, MF_BYCOMMAND | MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_HEADERS, MF_BYCOMMAND | MF_ENABLED);

        if (GetGridSourceCount(edVert) > 0)
        {
            pPopupMenu->EnableMenuItem(ID_EDIT_COPY, MF_BYCOMMAND|MF_ENABLED);

            if (m_pManager->IsRangeSelectionEmpty()) // in assumption mouse is over selection
            {
                pPopupMenu->EnableMenuItem(ID_GRID_COPY_COL, MF_BYCOMMAND | MF_ENABLED);
                pPopupMenu->EnableMenuItem(ID_GRID_COPY_ROW, MF_BYCOMMAND | MF_ENABLED);
            }

            pPopupMenu->EnableMenuItem(ID_GRID_COPY_ALL, MF_BYCOMMAND | MF_ENABLED);
            pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_OPEN, MF_BYCOMMAND | MF_ENABLED);
            pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_EXPORT, MF_BYCOMMAND | MF_ENABLED);
            pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_EXPORT_AND_OPEN, MF_BYCOMMAND | MF_ENABLED);
        }
    }
}

    struct ClipboardGuard
    {
        CWnd* m_pWnd;
        bool  m_open;

        ClipboardGuard (CWnd* pWnd) : m_pWnd(pWnd), m_open(false) {}
        ~ClipboardGuard () { CloseClipboard(); }

        bool OpenClipboard ()
        {
            if (!m_open)
            {
                if (m_pWnd->OpenClipboard())
                    m_open = true;
            }
            return m_open;
        }

        bool CloseClipboard ()
        {
            if (m_open)
            {
                ::CloseClipboard();
                m_open = false;
            }
            return m_open;
        }
    };

void GridView::DoEditCopy (const SQLToolsSettings& setting, int format)
{
    CWaitCursor wait;
    ClipboardGuard cg(this);

    if (GetGridSource()
    && cg.OpenClipboard())
    {
        setExportSettings(setting);

        int row, nrows, col, ncols;
        getSelectionForExport(row, nrows, col, ncols);

        GetGridSource()->DoEditCopy(row, nrows, col, ncols, ecDataCell, format);
        cg.CloseClipboard();
    }
}

void GridView::OnEditCopy ()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Copy", true, false))
    {
        DoEditCopy(settings, etfDefault);
    }
}

void GridView::OnGridCopyAsText ()
{
    DoEditCopy(GetSQLToolsSettings(), etfPlainText);
}

void GridView::OnGridCopyAsPlsqlValues ()
{
    DoEditCopy(GetSQLToolsSettings(), etfPLSQL1);
}

void GridView::OnGridCopyAsPlsqlExpression ()
{
    DoEditCopy(GetSQLToolsSettings(), etfPLSQL2);
}

void GridView::OnGridCopyAsPlsqlInList ()
{
    DoEditCopy(GetSQLToolsSettings(), etfPLSQL3);
}

void GridView::OnGridCopyAsPlsqlInserts ()
{
    DoEditCopy(GetSQLToolsSettings(), etfPLSQL4);
}

void GridView::OnEditPaste ()
{
    //GetGridSource()->DoEditPaste();
}

void GridView::OnEditCut ()
{
    //GetGridSource()->DoEditCut();
}

void GridView::OnUpdateEditGroup(CCmdUI* pCmdUI)
{
    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);
}

void GridView::OnUpdate_CopyHeaders(CCmdUI* pCmdUI)
{
    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);
}

void GridView::OnRButtonDown(UINT nFlags, CPoint point)
{
    bool hit = m_pManager->IsPointOverRangeSelection(point);

    if (!hit)
        m_pManager->EvLButtonDown(nFlags, point);

    CView::OnRButtonDown(nFlags, point);
}

void GridView::OnFileExport ()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "File Save", true, true))
        DoFileSave(settings, etfDefault, false);
}

void GridView::OnGridOutputExportAndOpen()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "File Save and Shell Open", true, true))
        DoFileSave(settings, etfDefault, true);
}

void GridView::DoFileSave (const SQLToolsSettings& settings, int _format, bool shellOpen)
{
    if (GetGridSource())
    {
        setExportSettings(settings);

        const char* filter = 0;
        const char* extension = 0;

        string filename;
        Common::FilenameTemplate::Format(settings.GetGridExpFilenameTemplate().c_str(), filename, m_uExpFileCounter);

        if (CDocument* doc = GetDocument())
        {
            CString title = doc->GetTitle();
            title.TrimRight("* ");
            filename = std::regex_replace(filename, std::regex("&script"), string(title));
        }

        int format = (_format != etfDefault) ? _format : settings.GetGridExpFormat();
        switch (format)
        {
        case 0:
            filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";
            extension = "txt";
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            filter = "SQL Files (*.sql)|*.sql|All Files (*.*)|*.*||";
            extension = "sql";
            break;
        case 1:
            filter = "CSV Files (Comma delimited) (*.csv)|*.csv|All Files (*.*)|*.*||";
            extension = "csv";
            break;
        case 2:
            filter = "Text Files (Tab delimited) (*.txt)|*.txt|All Files (*.*)|*.*||";
            extension = "txt";
            break;
        case 3:
        case 4:
            filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*||";
            extension = "xml";
            break;
        case 5:
            filter = "HTML Files (*.html; *.htm)|*.html;*.htm|All Files (*.*)|*.*||";
            extension = "html";
            break;
        }

        CFileDialog dial(FALSE, extension, filename.c_str(),
                        OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ENABLESIZING,
                        filter, this, 0);

        if (dial.DoModal() == IDOK)
        {
            m_uExpFileCounter++;
            CString path = dial.GetPathName();
            std::ofstream of(path);
            
            int row, nrows, col, ncols;
            getSelectionForExport(row, nrows, col, ncols);
            if (!(nrows != 1 || ncols != 1)) // it is a single cell
                row = nrows = col = ncols = -1;

            GetGridSource()->ExportText(of, row, nrows, col, ncols, format);
            of.close();

            if (shellOpen)
                Common::AppShellOpenFile(path);
        }
    }
}

BOOL GridView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (!m_uWheelScrollLines)
        ::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_uWheelScrollLines, 0);

    if (m_uWheelScrollLines == WHEEL_PAGESCROLL)
    {
        OnVScroll(zDelta > 0 ? SB_PAGEDOWN : SB_PAGEUP , 0, 0);
    }
    else
    {
        int nToScroll = ::MulDiv(zDelta, m_uWheelScrollLines, WHEEL_DELTA);

        if (zDelta > 0)
            while (nToScroll--)
                OnVScroll(SB_LINELEFT, 0, 0);
        else
            while (nToScroll++)
                OnVScroll(SB_LINERIGHT, 0, 0);
    }

    return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void GridView::OnSettingChange (UINT, LPCTSTR)
{
    m_uWheelScrollLines = 0;
}

void GridView::OnGridOutputOptions ()
{
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Grid copy/export", false, true))
    {
        GetSQLToolsSettingsForUpdate() = settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
    }
}

void GridView::OnGridCopyAll ()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Copy All", true, false))
    {
        CWaitCursor wait;
        ClipboardGuard cg(this);

        if (GetGridSource() && cg.OpenClipboard())
        {
            setExportSettings(settings);
            GetGridSource()->DoEditCopy(-1, -1, -1, -1, ecEverything, etfDefault);
            cg.CloseClipboard();
        }
    }
}

void GridView::OnGridCopyColHeader ()
{
    CWaitCursor wait;
    ClipboardGuard cg(this);

    if (GetGridSource() && cg.OpenClipboard())
    {
        setExportSettings(GetSQLToolsSettings());

        int row, nrows, col, ncols;
        getSelectionForExport(row, nrows, col, ncols);

        GetGridSource()->DoEditCopy(-1, -1, col, ncols, ecColumnHeader, etfDefault);
        cg.CloseClipboard();
    }
}

void GridView::OnGridCopyHeaders()
{
    CWaitCursor wait;
    ClipboardGuard cg(this);

    if (GetGridSource() && cg.OpenClipboard())
    {
        setExportSettings(GetSQLToolsSettings());

        GetGridSource()->DoEditCopy(-1, -1, -1, -1, ecFieldNames, etfDefault);
        cg.CloseClipboard();
    }
}

void GridView::OnGridCopyRow()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Copy Row", true, false))
    {
        CWaitCursor wait;
        ClipboardGuard cg(this);

        if (GetGridSource() && cg.OpenClipboard())
        {
            setExportSettings(settings);
            int row = m_pManager->m_Rulers[edVert].GetCurrent();
            GetGridSource()->DoEditCopy(row, 1, -1, -1, ecRow, etfDefault);
            cg.CloseClipboard();
        }
    }
}

void GridView::OnGridCopyCol ()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Copy Column", true, false))
    {
        CWaitCursor wait;
        ClipboardGuard cg(this);

        if (GetGridSource() && cg.OpenClipboard())
        {
            setExportSettings(settings);
            int row = m_pManager->m_Rulers[edVert].GetCurrent();
            int col = m_pManager->m_Rulers[edHorz].GetCurrent();
            GetGridSource()->DoEditCopy(-1, -1, col, 1, ecColumn, etfDefault);
            cg.CloseClipboard();
        }
    }
}

void GridView::OnGridOutputOpen()
{
    // local copy
    SQLToolsSettings settings = GetSQLToolsSettings();

    if (showExportSettings(settings, "Open in Editor", true, false))
        DoOpenInEditor(settings, etfDefault);
}

CPLSWorksheetDoc* GridView::DoOpenInEditor (const SQLToolsSettings& settings, int _format)
{
    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0)
    {
        POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();

        if (CDocTemplate* pDocTemplate = AfxGetApp()->GetNextDocTemplate(pos))
        {
            if (CDocument* doc = pDocTemplate->OpenDocumentFile(NULL))
            {
                int format = (_format != etfDefault) ? _format : settings.GetGridExpFormat();
                setExportSettings(settings);

                int row, nrows, col, ncols;
                getSelectionForExport(row, nrows, col, ncols);
                if (!(nrows != 1 || ncols != 1)) // it is a single cell
                    row = nrows = col = ncols = -1;

                std::ostringstream of;
                GetGridSource()->ExportText(of, row, nrows, col, ncols, format);
                string text = of.str();

                CMemFile mf((BYTE*)text.c_str(), text.length());
                CArchive ar(&mf, CArchive::load);
                doc->Serialize(ar);

                // it's bad because of dependency on CPLSWorksheetDoc
                ASSERT_KINDOF(CPLSWorksheetDoc, doc);
                CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)doc;

                // 03.06.2003 bug fix, "Open In Editor" does not set document type
                // 29.06.2003 bug fix, "Open In Editor" fails on comma/tab delimited formats
                const char* name = "Text";

                switch (format)
                {
                case 0: //etfPlainText
                case 6: //etfPLSQL1
                case 7: //etfPLSQL2
                case 8: //etfPLSQL3
                case 9: //etfPLSQL3
                    name = "PL/SQL";
                    break;
                case 1: //etfQuotaDelimited
                case 2: //etfTabDelimitedText
                    name = "Text";
                    break;
                case 3: //etfXmlElem 
                case 4: //etfXmlAttr
                case 5: //etfHtml
                    name = "XML";
                    break;
                }

                pDoc->SetClassSetting(name);
                // 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
                pDoc->DefaultFileFormat();
                return pDoc;
            }
        }
    }
    return NULL;
}

void GridView::OnSysColorChange()
{
    CView::OnSysColorChange();

    m_pManager->InitColors();
}


void GridView::OnMoveUp ()
{
    if (!IsEmpty())
        m_pManager->MoveToLeft(m_toolbarNavigationDir);
}

void GridView::OnMoveDown ()
{
    if (!IsEmpty())
        m_pManager->MoveToRight(m_toolbarNavigationDir);
}

void GridView::OnMovePgUp ()
{
    if (!IsEmpty())
        m_pManager->MoveToPageUp(m_toolbarNavigationDir);
}

void GridView::OnMovePgDown ()
{
    if (!IsEmpty())
        m_pManager->MoveToPageDown(m_toolbarNavigationDir);
}

void GridView::OnMoveEnd ()
{
    if (!IsEmpty())
        m_pManager->MoveToEnd(m_toolbarNavigationDir);
}

void GridView::OnMoveHome ()
{
    if (!IsEmpty())
        m_pManager->MoveToHome(m_toolbarNavigationDir);
}

void GridView::OnUpdate_MoveUp (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnUpdate_MoveDown (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MovePgUp (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnUpdate_MovePgDown (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MoveEnd (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MoveHome (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnRotate ()
{
    if (m_pManager->m_Options.m_Rotation && !IsEmpty())
    {
        if (GridSourceBase* source = dynamic_cast<GridSourceBase*>(GetGridSource()))
        {
            source->SetOrientation(source->GetOrientation() == eoVert ? eoHorz : eoVert);
            m_pManager->m_Options.m_ColSizingAsOne = source->GetOrientation() == eoHorz;
            m_pManager->Rotate();

            m_toolbarNavigationDir = source->GetOrientation() == eoVert ? edVert : edHorz;

            if (m_pManager->m_Options.m_ColSizingAsOne)
            {
                int sizeAsOne = m_pManager->m_Rulers[edHorz].GetScrollSize() / 2 - 1;
                m_pManager->m_Rulers[edHorz].SetSizeAsOne(sizeAsOne);
                m_pManager->RecalcRuller(edHorz, true/*adjust*/, true/*invalidate*/);
            }
        }
    }
}

void GridView::OnUpdate_Rotate (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pManager->m_Options.m_Rotation && !IsEmpty());
}


void GridView::OnGridClearRecords()
{
    if (m_pManager->m_Options.m_ClearRecords)
        GetGridSource()->DeleteAll();
}

void GridView::OnUpdate_GridClearRecords(CCmdUI* pCmdUI)
{
    if (m_pManager->m_Options.m_ClearRecords
    && !IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0) {
        pCmdUI->Enable(true);
    } else {
        pCmdUI->Enable(false);
    }
}

void GridView::OnGridSettings()
{
    SQLToolsSettings settings = GetSQLToolsSettings();

    CPropGridPage1   gridPage1(settings);
    CPropGridPage2   gridPage2(settings);
    CPropGridOutputPage1 gridOutputPage1(settings);
    CPropGridOutputPage2 gridOutputPage2(settings);
    CPropHistoryPage histPage(settings);

    CVisualAttributesPage vaPage(settings.GetVASets(), "");
    vaPage.m_psp.pszTitle = "SQLTools::Font and Color";
    vaPage.m_psp.dwFlags |= PSP_USETITLE;

    static UINT gStarPage = 0;
    Common::CPropertySheetMem sheet("Grid settings", gStarPage);
    sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
    sheet.SetTreeWidth(240);

    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    sheet.AddPage(&gridPage1);
    sheet.AddPage(&gridPage2);
    sheet.AddPage(&gridOutputPage1);
    sheet.AddPage(&gridOutputPage2);
    sheet.AddPage(&histPage);
    sheet.AddPage(&vaPage);

    int retVal = sheet.DoModal();

    if (retVal == IDOK)
    {
        GetSQLToolsSettingsForUpdate() = settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
    }
}

void GridView::OnUpdate_GridSettings (CCmdUI* pCmdUI)
{
    pCmdUI->Enable();
}

void GridView::DoOpen (ETextFormat frm, const char* ext)
{
    if (GridStringSource* source = dynamic_cast<GridStringSource*>(GetGridSource()))
    {
        // 27.10.2003 bug fix, export settings do not affect on quick html/csv viewer launch
        setExportSettings(GetSQLToolsSettings());

        string file = TempFilesManager::CreateFile(ext);

        if (!file.empty())
        {
            std::ofstream of(file.c_str());

            int row, nrows, col, ncols;
            getSelectionForExport(row, nrows, col, ncols);
            if (!(nrows != 1 || ncols != 1)) // it is a single cell
                row = nrows = col = ncols = -1;

            GetGridSource()->ExportText(of, row, nrows, col, ncols, frm);

            of.close();

            Common::AppShellOpenFile(file.c_str());
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Cannot generate temporary file name for export.", MB_OK|MB_ICONSTOP);
        }
    }
}

void GridView::OnGridOpenWithIE ()
{
    DoOpen(etfHtml, "HTM");
}

void GridView::OnGridOpenWithExcel ()
{
    DoOpen(etfQuotaDelimited, "CSV");
}

/*
2017-12-10 Popuip Viewer is based on the code from sqltools++ by Randolf Geist
*/
void GridView::OnGridPopup ()
{
    if (GridStringSource* source = dynamic_cast<GridStringSource*>(GetGridSource()))
    {
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();

        string text, line;
        source->GetCellStr(text, row, col);

        istringstream in(text);
        ostringstream out;

        while (getline(in, line))
        {
            if (line.size() > 0 && *line.rbegin() == '\r')
                out.write(line.c_str(), line.size()-1);
            else
                out.write(line.c_str(), line.size());
            out << '\r' << '\n';
        }

        PopupFrameWnd::ShowPopup (this, out.str());
    }
}

}//namespace OG2



