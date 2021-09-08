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
    12.07.2002 bug fix, drag & drop fails in the same window (stream, drop below original pos)
    26.03.2003 small improvement, added delay for scrolling in drag mode
    11.10.2004 improvement, Ruler for columns has been added
    2016.05.10 improvement, implemented Read-Only mode
    2016.06.14 improvement, Alternative Columnar selection mode 
*/

#include "stdafx.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEView.h"
#include "Common/AppUtilities.h"
#include <COMMON/AppGlobal.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RETURN_IF_LOCKED1(val) { if (IsLocked()) { Global::SetStatusText("The content is locked and cannot be modified!"); MessageBeep((UINT)-1); return val; } } 

using namespace OpenEditor;

COEditorView::CDragAndDropData COEditorView::m_DragAndDropData;

void COEditorView::PositionToPoint (OpenEditor::Position pos, CPoint& point) const
{
    if (pos.line >= m_Rulers[1].m_Topmost
    && pos.column >= m_Rulers[0].m_Topmost)
    {
        point.x = m_Rulers[0].PosToPix(pos.column) + 1;
        point.y = m_Rulers[1].PosToPix(pos.line);
    }
    else
    {
        point.x = -10000;
        point.y = -10000;
    }
}

bool COEditorView::CursorOnSelection (CPoint point) const
{
    if (point.x > m_Rulers[0].m_Indent 
    && point.y > m_Rulers[1].m_Indent
    && !IsSelectionEmpty())
    {
        Position pos = MouseToPos(point);

        Square selBlk;
        GetSelection(selBlk);
        selBlk.normalize();

        switch (GetBlockMode())
        {
        case ebtColumn:
            return selBlk.line_in_rect(pos.line) && selBlk.column_in_rect(pos.column);
        case ebtStream:
            if (selBlk.start.line == selBlk.end.line)
                return pos.line == selBlk.start.line && selBlk.column_in_rect(pos.column);
            else
                return (pos.line == selBlk.start.line && pos.column >= selBlk.start.column)
                || (pos.line == selBlk.end.line && pos.column <= selBlk.end.column)
                || (pos.line > selBlk.start.line && pos.line < selBlk.end.line);
        }
    }
    return false;
}

void COEditorView::DrawCursor (CPoint point, bool erase)
{
    Position pos = MouseToPos(point);
    CPoint newPoint;
    PositionToPoint(pos, newPoint);

    if (m_DragAndDropData.ptCaret != newPoint)
    {
        CClientDC dc(this);

        if (m_DragAndDropData.bVisible)
            dc.DrawFocusRect(&CRect(m_DragAndDropData.ptCaret, m_DragAndDropData.sCursor));

        m_DragAndDropData.ptCaret = newPoint;
        m_DragAndDropData.bVisible = !erase && !((m_DragAndDropData.pSrcWnd == this) && CursorOnSelection(point));

        if (m_DragAndDropData.bVisible)
            dc.DrawFocusRect(&CRect(m_DragAndDropData.ptCaret, m_DragAndDropData.sCursor));
    }
}

    struct COEditorViewDataSource : COleDataSource
    {
        COEditorView* m_pView;

        COEditorViewDataSource (COEditorView* pView)
            : m_pView(pView)
        {
            if (m_pView->IsAltColumnarMode())
                DelayRenderData((CLIPFORMAT)COEditorView::GetAltColumnarTextFormat());
            DelayRenderData(CF_UNICODETEXT);
        }

        virtual BOOL OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
        {
            if (lpFormatEtc->cfFormat == CF_UNICODETEXT
            || lpFormatEtc->cfFormat == (CLIPFORMAT)COEditorView::GetAltColumnarTextFormat())
            {
                CWaitCursor wait;

                std::wstring text;
                
                if (lpFormatEtc->cfFormat == CF_UNICODETEXT)
                    m_pView->GetBlock(text);
                else
                    text = L"dummy";

                HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (text.size() + 1) * sizeof(WCHAR));

                if (hGlobal)
                {
                    wchar_t* buff = (wchar_t*)GlobalLock(hGlobal);
                    if (buff)
                    {
                        memcpy(buff, text.c_str(), (text.size() + 1) * sizeof(WCHAR));
                        GlobalUnlock(hGlobal);
                        *phGlobal = hGlobal;
                        return TRUE;
                    }
                    GlobalFree(hGlobal);
                }
            }
            return FALSE;
        }
    };

void COEditorView::StartDrag ()
{
    m_DragAndDropData.pSrcWnd = this;
    m_DragAndDropData.pDstWnd = NULL;
    COEditorViewDataSource data(this);
    DROPEFFECT de = data.DoDragDrop(IsLocked() ? DROPEFFECT_COPY: DROPEFFECT_COPY|DROPEFFECT_MOVE);

    if (de == DROPEFFECT_MOVE && m_DragAndDropData.pDstWnd != this && !IsLocked())
        DeleteBlock();

    m_DragAndDropData.pSrcWnd =
    m_DragAndDropData.pDstWnd = NULL;

    if (de == DROPEFFECT_NONE)
    {
        CPoint point;
        GetCursorPos(&point);
        ScreenToClient(&point);

        CRect rect;
        GetClientRect(&rect);

        if (rect.PtInRect(point))
        {
            ClearSelection(true);
            MoveTo(MouseToPos(point));
        }
    }
}

DROPEFFECT COEditorView::OnDragEnter (COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
    try { EXCEPTION_FRAME;

        if (CFrameWnd* frame = GetParentFrame()) 
        {
            if (frame->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
                static_cast<CMDIChildWnd*>(frame)->MDIActivate();
            frame->SetActiveView(this);
        }

        m_DragAndDropData.bVisible = false;
        m_DragAndDropData.sCursor.cx = 2;
        m_DragAndDropData.sCursor.cy = m_Rulers[1].m_CharSize;
    
    } _OE_DEFAULT_HANDLER_;

    return OnDragOver(pDataObject, dwKeyState, point);
}

void COEditorView::OnDragLeave ()
{
    try { EXCEPTION_FRAME;

        m_bDragScrolling = FALSE;
        DrawCursor(CPoint(-1, -1), true);
        CView::OnDragLeave();
    
    } _OE_DEFAULT_HANDLER_;
}

/*
    26.03.2003 small improvement, added delay for scrolling in drag mode
*/
DROPEFFECT COEditorView::OnDragOver (COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
    DROPEFFECT de = DROPEFFECT_NONE;

    try { EXCEPTION_FRAME;
        
        RETURN_IF_LOCKED1(de);

        if (pDataObject 
        && (pDataObject->IsDataAvailable(CF_UNICODETEXT) || pDataObject->IsDataAvailable(CF_TEXT)))
        {
            if ((dwKeyState & MK_CONTROL) == MK_CONTROL)
                de = DROPEFFECT_COPY;
            else
                de = DROPEFFECT_MOVE;

            if (m_DragAndDropData.ptCursor == point)  // no cursor motion
            {
                enum { nothing, start, redraw } status = nothing;

                if (m_Rulers[1].m_Topmost > 0 
                && point.y - m_Rulers[1].m_Indent < m_Rulers[1].m_CharSize / 2)
                {
                    if (m_bDragScrolling)
                    {
                        status = redraw;
                        DrawCursor(point, true);
                        SendMessage(WM_VSCROLL, SB_LINELEFT);
                    }
                    else 
                        status = start;
                } 
                else if (point.y - m_Rulers[1].m_Indent 
                > m_Rulers[1].m_ClientSize - m_Rulers[1].m_CharSize / 2)
                {
                    if (m_bDragScrolling)
                    {
                        status = redraw;
                        DrawCursor(point, true);
                        SendMessage(WM_VSCROLL, SB_LINERIGHT);
                    }
                    else 
                        status = start;
                }

                if (m_Rulers[0].m_Topmost > 0 
                && point.x - m_Rulers[0].m_Indent < m_Rulers[1].m_CharSize / 2)
                {
                    if (m_bDragScrolling)
                    {
                        status = redraw;
                        DrawCursor(point, true);
                        SendMessage(WM_HSCROLL, SB_LINELEFT);
                    }
                    else 
                        status = start;
                } 
                else if (point.x - m_Rulers[0].m_Indent 
                > m_Rulers[0].m_ClientSize - m_Rulers[1].m_CharSize / 2)
                {
                    if (m_bDragScrolling)
                    {
                        status = redraw;
                        DrawCursor(point, true);
                        SendMessage(WM_HSCROLL, SB_LINERIGHT);
                    }
                    else 
                        status = start;
                }

                switch (status)
                {
                case start:
                    SetTimer(DRAG_MOUSE_TIMER, DRAG_MOUSE_TIMER_ELAPSE, NULL);
                    break;
                case redraw:
                    UpdateWindow();
                    m_DragAndDropData.bVisible = false;
                    m_DragAndDropData.ptCaret = CPoint(-2, -2);
                    DrawCursor(point);
                    break;
                }
            } 
            else
            {
                m_DragAndDropData.ptCursor = point;
                DrawCursor(point);
            }
        }

        // the following "if" has been added to visualize
        // 2 different source formats of "Object Viewer" 
        if (pDataObject 
        && pDataObject->IsDataAvailable(CF_PRIVATEFIRST) // assume it is "{code}"
        && (de == DROPEFFECT_COPY || de == DROPEFFECT_MOVE)
        && (dwKeyState & MK_SHIFT) == MK_SHIFT)
        {
            de = DROPEFFECT_LINK;
        }
        
    } _OE_DEFAULT_HANDLER_;

    return de;
}

// 12/07/2002 bug fix, drag & drop fails in the same window (stream, drop below original pos)
BOOL COEditorView::OnDrop (COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
    try { EXCEPTION_FRAME;

    m_bDragScrolling = FALSE;

    DrawCursor(CPoint(), true);

    RETURN_IF_LOCKED1(FALSE);

    if (pDataObject 
    && (pDataObject->IsDataAvailable(CF_UNICODETEXT) || pDataObject->IsDataAvailable(CF_TEXT))
    && !((m_DragAndDropData.pSrcWnd == this) && CursorOnSelection(point))) 
    {
        std::wstring text;
        std::string info;

        bool thereIsData = Common::AppGetDragAndDroData(text, pDataObject, CF_UNICODETEXT);

        if (!thereIsData)
        {
            string utfText;
            thereIsData = Common::AppGetDragAndDroData(utfText, pDataObject, CF_TEXT);
            if (thereIsData)
                text = Common::wstr(utfText);
        }

        if (thereIsData) 
        {
            Common::AppGetDragAndDroData(info, pDataObject, CF_PRIVATEFIRST);
           
            m_DragAndDropData.pDstWnd = this;

            // save a original selection
            Square orgSel;
            GetSelection(orgSel);
            orgSel.normalize();
         
            // start undo transaction
            UndoGroup undoGroup(*this);
            Position pos = GetPosition();
            PushInUndoStack(pos);

            // get a drop position
            pos = MouseToPos(point);

            // is it a move operation in this view ?
            bool move = (dropEffect == DROPEFFECT_MOVE 
                         && m_DragAndDropData.pSrcWnd == this) ? true : false;
            
            bool after = false; // where's the drop position ? after the STREAM selection postion? 

            if (move)
            {
                // if moving and the drop position is before the selection
                switch (GetBlockMode())
                {
                case ebtColumn:
                    {
                        int inx = -1;
                        if (orgSel.line_in_rect(pos.line) && pos.column > orgSel.end.column)
                        {
                            int len = pos2inx(pos.line, orgSel.end.column) - pos2inx(pos.line, orgSel.start.column);
                            inx = pos2inx(pos.line, pos.column) - len;
                        }
                        DeleteBlock();
                        if (inx != -1)
                            pos.column = inx2pos(pos.line, inx);
                    }
                    break;
                case ebtStream:
                    if (pos.line < orgSel.start.line 
                    || pos.line == orgSel.start.line && pos.column <= orgSel.start.column)
                    {
                        after = false;
                        DeleteBlock();
                    }
                    else
                       after = true;
                }
            }

            // move a caret to drop a position
            MoveTo(pos);

            // insert
            if (!text.empty()) 
            {
                string dummy;
                bool isAltColumnarText = Common::AppGetDragAndDroData(dummy, pDataObject, (CLIPFORMAT)m_AltColumnarTextFormat);

                if (isAltColumnarText && GetBlockMode() != ebtColumn)
                    SetBlockMode(ebtColumn, true); 

                if (info == "{code}")
                    AlignCodeFragment(text.c_str(), text);

                InsertBlock(text.c_str(), !GetBlockKeepMarkingAfterDragAndDrop()/*hideSelection*/);
            }
            else
                return FALSE;

            // if moving and the drop position is after the STREAM selection
            if (move && after && GetBlockMode() == ebtStream)
            {
                Square newSel;
                newSel.start = pos;
                newSel.end = GetPosition();
                
                // restore the original selection and delete it
                SetSelection(orgSel);
                int _orgSel_start_column = pos2inx(orgSel.start.line, orgSel.start.column);
                int _orgSel_end_column   = pos2inx(orgSel.end.line, orgSel.end.column);
                int _newSel_start_column = pos2inx(newSel.start.line, newSel.start.column);
                int _newSel_end_column   = pos2inx(newSel.end.line, newSel.end.column);
                DeleteBlock();

                // calculate offset for the new selection
                int lines = orgSel.end.line - orgSel.start.line;
                
                if (orgSel.end.line == newSel.start.line) // "near" drag and drop
                {
                    int len = _orgSel_end_column - _orgSel_start_column;
                    newSel.start.column = inx2pos(newSel.start.line, _newSel_start_column - len);
                    if (!lines) // once line
                        newSel.end.column   = inx2pos(newSel.end.line, _newSel_end_column - len);
                }
                newSel.start.line -= lines;
                newSel.end.line   -= lines;

                SetSelection(newSel);
                MoveTo(newSel.end);
                PushInUndoStack(newSel.end);
                PushInUndoStack(GetBlockMode(), IsAltColumnarMode(), newSel);
            }

            SetFocus();
            return TRUE;
        }
        else
            MessageBeep((UINT)-1);
    }
    
    } _OE_DEFAULT_HANDLER_;

    return FALSE;
}
