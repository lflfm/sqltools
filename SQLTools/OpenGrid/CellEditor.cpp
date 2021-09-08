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
/*      Purpose: Cell text editor implementation for grid component        */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

#include "stdafx.h"
#include <algorithm>
#include <afxole.h>         // for Drag & Drop
#include "CellEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OG2
{

BEGIN_MESSAGE_MAP(CellEditor, CEdit)
	//{{AFX_MSG_MAP(CellEditor)
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
    ON_COMMAND(ID_EDIT_COPY,  CEdit::Copy)
    ON_COMMAND(ID_EDIT_CUT,   CEdit::Cut)
    ON_COMMAND(ID_EDIT_PASTE, CEdit::Paste)
    ON_COMMAND(ID_EDIT_UNDO,  OnUndo)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEMOVE()   // for Drag & Drop
	ON_WM_LBUTTONDOWN() // for Drag & Drop
END_MESSAGE_MAP()

int CellEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//SetMargins(2, 2);
    SetFont(GetParent()->GetFont());

	return 0;
}

void CellEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    int i, j;
    BOOL sendToParent(FALSE);
    //BOOL shiftDown   = 0xFF00 & GetKeyState(VK_SHIFT);
    //BOOL controlDown = 0xFF00 & GetKeyState(VK_CONTROL);
    //BOOL menuDown    = 0xFF00 & GetKeyState(VK_MENU);

    switch (nChar)
    {
    case VK_TAB:
    case VK_F2:
    case VK_ESCAPE:
        sendToParent = TRUE;
        break;
    //case VK_INSERT: 
    //if (!(0xFF00 & GetKeyState(VK_SHIFT))) 
    //  sendToParent = TRUE;
    //if (!shiftDown && controlDown && !menuDown) {
    //  CEdit::Copy();
    //  return;
    //}
    //if (shiftDown && !controlDown && !menuDown) {
    //  CEdit::Paste();
    //  return;
    //}
    //break;
    case VK_UP:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_RETURN:
        if ((0xFF00 & GetKeyState(VK_CONTROL))
        || !(::GetWindowLong(*this, GWL_STYLE) & ES_MULTILINE))
            sendToParent = TRUE;
        break;
    case VK_HOME:
    case VK_LEFT:
        GetSel(i, j);
        if ((0xFF00 & GetKeyState(VK_CONTROL) 
        || (!i && !j))
        && !(::GetWindowLong(*this, GWL_STYLE) & ES_MULTILINE))
            sendToParent = TRUE;
    break;
    case VK_END:
    case VK_RIGHT:
        GetSel(i, j);
        if ((0xFF00 & GetKeyState(VK_CONTROL)
        || ((!i || i == j) && j == GetWindowTextLength()))
        && !(::GetWindowLong(*this, GWL_STYLE) & ES_MULTILINE))
            sendToParent = TRUE;
        break;
    //case VK_DELETE:
    //if (0xFF00 & GetKeyState(VK_CONTROL))
    //  sendToParent = TRUE;
    //if (shiftDown && !controlDown && !menuDown) {
    //  CEdit::Cut();
    //  return;
    //}
    //break;
    }

    if (sendToParent) 
    {
        if(GetParent()) 
        {
            const MSG* msg = GetCurrentMessage();
            GetParent()->SendMessage(msg->message,msg->wParam,msg->lParam);
        }
    } else
        CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT CellEditor::OnGetDlgCode() 
{
    UINT retVal = CEdit::OnGetDlgCode();
    if (GetParent()) 
    {
        const MSG* msg = GetCurrentMessage();
        retVal |= GetParent()->SendMessage(msg->message,msg->wParam,msg->lParam);
    }
    return retVal;
}

// for Drag & Drop
bool CellEditor::CursorOnSelText (CPoint point)
{
    int beg, end;
    GetSel(beg, end);
    if (beg != end) 
    {
        if (beg > end)
            std::swap(beg, end);
        int pos = LOWORD(CharFromPos(point));
        if (pos >= beg && pos <=end)
            return true;
    }
    return false;
}

void CellEditor::OnMouseMove (UINT nFlags, CPoint point) 
{
    if (!(MK_LBUTTON & nFlags) && CursorOnSelText(point))
        SetCursor(LoadCursor(NULL, IDC_ARROW));

    CEdit::OnMouseMove(nFlags, point);
}

struct CKEditDataSource : COleDataSource 
{
    CEdit* m_pEdit;

    CKEditDataSource (CEdit* pEdit) 
    { 
        m_pEdit = pEdit; 
        DelayRenderData(CF_TEXT);
    }

// TODO: test!
    virtual BOOL OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
    {
        if (lpFormatEtc->cfFormat == CF_TEXT) 
        {
            int beg, end;
            CString buffer;
            wchar_t *src;
            m_pEdit->GetSel(beg, end);
            int size = end - beg + 1;
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size * sizeof(wchar_t));
            if (hGlobal) 
            {
                if (wchar_t* dest = (wchar_t*)GlobalLock(hGlobal)) 
                {
                    dest[0] = 0;
                    HGLOBAL hMem = 0;
                    if (ES_MULTILINE & ::GetWindowLong(*m_pEdit, GWL_STYLE)) 
                    {
                        hMem = m_pEdit->GetHandle();
                        src = (wchar_t*)GlobalLock(hMem);
                    } 
                    else 
                    {
                        m_pEdit->GetWindowText(buffer);
                        src = buffer.LockBuffer();
                    }
                    if (src) 
                    {
                        wcsncpy(dest, src + beg, end - beg);
                        dest[end - beg] = 0;
                    }
                    if (hMem) 
                    {
                        GlobalUnlock(hMem);
                        m_pEdit->SetHandle(hMem); // why?
                    }
          
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

void CellEditor::OnLButtonDown (UINT nFlags, CPoint point) 
{
    if (CursorOnSelText(point)) 
    {
        DROPEFFECT de = CKEditDataSource(this).DoDragDrop(DROPEFFECT_COPY);
        switch (de) 
        {
        case DROPEFFECT_NONE:
            {
                CPoint pt;
                GetCursorPos(&pt);
                ScreenToClient(&pt);
                if (pt == point) 
                {
                    int pos = LOWORD(CharFromPos(point));
                    SetSel(pos, pos);
                }
                break;
            }
        }
    } 
    else
        CEdit::OnLButtonDown(nFlags, point);
}

}//namespace OG2

