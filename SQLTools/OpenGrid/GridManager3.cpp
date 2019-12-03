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

/***************************************************************************/
/*      Purpose: Grid manager implementation for grid component            */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

// 2017-11-28 implemented range selection

#include "stdafx.h"
#include "GridManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{
    class EditNotValid : public std::exception 
    {
    public:
        EditNotValid () : std::exception("Invalid input") {}
    };

EditGridManager::EditGridManager (CWnd* client , AGridSource* source, bool shouldDeleteSource) 
: PaintGridManager(client, source, shouldDeleteSource),
  m_pEditor(0),
  m_State(esClear)
{
}

void EditGridManager::EvClose ()
{
    EndEditMode(false/*save*/, false/*focusToClient*/);
    PaintGridManager::EvClose();
}

void EditGridManager::Clear ()
{
    EndEditMode(false/*save*/, false/*focusToClient*/);
    PaintGridManager::Clear();
}

void EditGridManager::SetEditMode ()
{
    _ASSERTE(!IsEditMode());

    if (m_Options.m_Editing
    //&& m_pSource->CanModify()
    && !m_pSource->IsEmpty()
    && m_pSource->GetCount(edHorz) 
    && m_pSource->GetCount(edVert)) 
    {
        SetFlag(esEditMode);
        OpenEditor();
    } else
        MessageBeep(MB_ICONHAND);
}

void EditGridManager::EndEditMode (bool save, bool focusToClient)
{
    if (IsEditMode()) 
    {
        CloseEditor(save, focusToClient);
        ClearFlag(esEditMode);
    }
}

int EditGridManager::AppendRow ()
{
    endScrollMode();
    int row = m_pSource->Append();
    m_Rulers[edVert].SetCurrent(row);
    return row;
}

int EditGridManager::InsertRow ()
{
    endScrollMode();
    int row = m_Rulers[edVert].GetCurrent();
    m_pSource->Insert(row);
    m_Rulers[edVert].SetCurrent(row);
    return row;
}

int EditGridManager::DeleteRow ()
{
    endScrollMode();
    int row = m_Rulers[edVert].GetCurrent();
    m_pSource->Delete(row);
    //m_Rulers[edVert].SetCurrent(row);
    return row;
}

void EditGridManager::BeforeMove (eDirection)
{
    if (IsEditMode()) CloseEditor();
}

void EditGridManager::AfterMove (eDirection dir, int oldVal)
{
    __super::AfterMove(dir, oldVal);
}

void EditGridManager::BeforeScroll (eDirection)
{
    if (IsEditMode() && m_pEditor) 
    {
        m_pEditor->SetWindowPos(0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOREDRAW);
        SetFlag(esHiddenEdit);
    }
}

void EditGridManager::AfterScroll (eDirection)
{
}

void EditGridManager::OpenEditor ()
{
    if (IsEditMode() && !m_pEditor && IsCurrentVisible()) 
    {
        CRect rc;
        GetCurRect(rc);
        rc.InflateRect(-1, -1);
        m_pEditor = m_pSource->BeginEdit(m_Rulers[edVert].GetCurrent(), m_Rulers[edHorz].GetCurrent(), m_pClientWnd, rc);

        if (m_pEditor) 
        {
            m_pClientWnd->InvalidateRect(rc, FALSE);
            m_pEditor->SetFont(m_pClientWnd->GetFont());
            m_pEditor->SetFocus();
        }
    }
}

void EditGridManager::CloseEditor (bool save, bool focusToClient)
{
    ASSERT_EXCEPTION_FRAME;

    if (IsEditMode() && m_pEditor)
    {
        if (save) 
        {
            if (m_pEditor->UpdateData(TRUE))
                m_pSource->PostEdit();
            else
                _RAISE(EditNotValid());
        }
        else 
            m_pSource->CancelEdit();

        m_pEditor = 0;
        
        if (focusToClient)
            m_pClientWnd->SetFocus();
    }
}

bool EditGridManager::IdleAction ()
{
    if (CheckFlag(esHiddenEdit) && IsEditMode()
        && m_pEditor && IsCurrentVisible()) 
    {
        ClearFlag(esHiddenEdit);
        CRect rc;
        GetCurRect(rc);
        rc.InflateRect(-1,-1);
        m_pEditor->SetWindowPos(&CWnd::wndTop, rc.left, rc.top, rc.Width(), rc.Height(), SWP_SHOWWINDOW);
    }

    if (CheckFlag(esRefreshEdit) && IsEditMode()) 
    {
        ClearFlag(esRefreshEdit);
        CloseEditor(false/*save*/);
    }

    if (!m_pEditor && IsEditMode()) 
        OpenEditor();

    return false;
}

void EditGridManager::EvSetFocus (bool focus)
{
    PaintGridManager::EvSetFocus(focus);

    if (focus && m_pEditor) m_pEditor->SetFocus();
}

bool EditGridManager::EvKeyDown (UINT key, UINT repeatCount, UINT flags)
{
    bool retVal = true;
    try {
        if (!PaintGridManager::EvKeyDown(key, repeatCount, flags))
            switch (key) 
            {
            case VK_F2:
                if (!IsEditMode())  SetEditMode();
                else                EndEditMode();
                break;
            case VK_ESCAPE:
                if (IsEditMode())
                    EndEditMode(false/*save*/);  
                if (!m_Rulers[edHorz].IsSelectionEmpty() || !m_Rulers[edVert].IsSelectionEmpty())
                {
                    m_Rulers[edVert].ResetSelection();
                    m_Rulers[edHorz].ResetSelection();
                    RepaintAll();
                }
                break;
            default:
                retVal = false;
            }
    }
    catch (const EditNotValid&) {}
    return retVal;
}

void EditGridManager::EvLButtonDblClk (UINT /*modKeys*/, const CPoint& point)
{
    int lim[4];
    m_Rulers[edHorz].GetDataLoc(lim);
    m_Rulers[edVert].GetDataLoc(lim+2);

    if (lim[0] <= point.x && point.x < lim[1]
    && lim[2] <= point.y && point.y < lim[3]
    && !IsEditMode()) 
    {
        SetEditMode();
    }
}

void EditGridManager::InvalidateEdit ()
{
    SetFlag(esRefreshEdit);
}

}//namespace OG2
