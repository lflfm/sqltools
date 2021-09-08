/* 
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

// 30.03.2005 (ak) highlighting for a dragged item
// 17.04.2005 (ak) added auto-scrolling


#include "stdafx.h"
#include "oetabctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(COETabCtrl, CTabCtrl)
    //{{AFX_MSG_MAP(CMyTabCtrl)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
    ON_WM_TIMER()
END_MESSAGE_MAP()
//	ON_WM_PAINT()
//	ON_WM_MOUSEWHEEL()


/** @fn COETabCtrl::COETabCtrl(void)
 * @brief Constructor. Default button flag to FALSE and load pointers to cursors (ARROW & IDC_NO).
 */
COETabCtrl::COETabCtrl(void)
: 
m_button_is_down(false),
m_first_selected_item(-1),
m_highlighting_for_dragged(true),
m_highlighting_threshold_exceeded(false),
m_scrolling(SD_NONE)
{
    g_hCurArrow = LoadCursor(NULL, IDC_ARROW);
    g_hCurNo = LoadCursor(NULL, IDC_NO);
}

/** @fn COETabCtrl::~COETabCtrl(void)
 * @brief Destructior. Do nothing now.
 */
COETabCtrl::~COETabCtrl(void)
{
}

/** void COETabCtrl::OnLButtonDown(UINT nFlags, CPoint point)
 * @brief If there is more than 1 tab, on the left mouse button - find where we are and set the 'moving' flag.
 *
 * @arg nFlags Ctrl&Shift indicators.
 * @arg point Position of the cursor.
 */
void COETabCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    CRect item_rect;
    if (GetItemCount() > 1) {
        TCHITTESTINFO ht;
        ht.pt = point;
        int inx = HitTest(&ht);
        // find where we are now (often we aren't in the selected item!)
        if (ht.flags != TCHT_NOWHERE) // do nothing if the point is nowhere
        {
            m_first_selected_item = inx; 

        // if everything is ok set flag
        m_button_is_down = true;
            m_button_down_point = point;
            m_highlighting_threshold_exceeded = false;
        //SetCursor(g_hCurSize);// set DragCursor
    }
    }

    CTabCtrl::OnLButtonDown(nFlags, point);
}

    // helper function
    static
    BOOL change_item (CTabCtrl* pTabWnd, int inx)
    {
            NMHDR hmhdr;
            hmhdr.hwndFrom = pTabWnd->m_hWnd;
            hmhdr.idFrom = GetWindowLong(pTabWnd->m_hWnd, GWL_ID);
            hmhdr.code = TCN_SELCHANGING;

            // request owner for confirmation
            if (!pTabWnd->GetOwner()->SendMessage(WM_NOTIFY, (WPARAM)hmhdr.idFrom, (LPARAM)&hmhdr))
            {
                // cange selection if confirmation is positive
                pTabWnd->SetCurSel(inx);
                hmhdr.code = TCN_SELCHANGE;
                pTabWnd->GetOwner()->SendMessage(WM_NOTIFY, (WPARAM)hmhdr.idFrom, (LPARAM)&hmhdr);
                return TRUE;
            }

        return FALSE;
    }

/** void COETabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
 * @brief On the left mouse button up - unset flag and change the cursor to a normal IDC_ARROW.
 *
 * @arg nFlags Ctrl&Shift indicators.
 * @arg point Position of the cursor.
 */
void COETabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    CTabCtrl::OnLButtonUp(nFlags, point);

    if (m_button_is_down)
    {
        if (m_first_selected_item != -1)
    {
        if (m_highlighting_for_dragged)
            HighlightItem(m_first_selected_item, FALSE);
        
            change_item(this, m_first_selected_item);
    }

    KillTimer(DRAG_MOUSE_TIMER);
    m_first_selected_item = -1;
    m_button_is_down = false;
        m_scrolling = SD_NONE;
    SetCursor(g_hCurArrow);
    }
}

    // helper function
    static
    BOOL move_item (CTabCtrl* pTabWnd, int oldInx, int newInx, bool highlight)
    {
        TCITEM item;
        TCHAR item_buf[255];
        item.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM; // get all params
        item.pszText = item_buf;
        item.cchTextMax = sizeof(item_buf)/sizeof(TCHAR);

        if (highlight)
            pTabWnd->HighlightItem(oldInx, FALSE);
                    
        if(pTabWnd->GetItem(oldInx, &item) == TRUE) {
            pTabWnd->SetCurSel(newInx);
            pTabWnd->DeleteItem(oldInx);
            pTabWnd->InsertItem(newInx, &item);
            pTabWnd->SetCurSel(newInx);
            if (highlight)
                pTabWnd->HighlightItem(newInx);
            return TRUE;
        }

        if (highlight)
            pTabWnd->HighlightItem(oldInx);

        return FALSE;
    }

/** void COETabCtrl::OnMouseMove(UINT nFlags, CPoint point)
 * @brief If a flag is set, on mouse move check cursor position, set the cursor & eventually move the item.
 *
 * @arg nFlags Ctrl&Shift indicators.
 * @arg point Position of the cursor.
 */
void COETabCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    TRACE("COETabCtrl::OnMouseMove\n");

    if(m_button_is_down == true) 
    {
        if (m_highlighting_for_dragged)
        {
            if (!m_highlighting_threshold_exceeded)
            {
                if (abs(m_button_down_point.x - point.x) > HIGHLIGHTING_THRESHOLD_CX
                || abs(m_button_down_point.y - point.y) > HIGHLIGHTING_THRESHOLD_CY)
                    m_highlighting_threshold_exceeded = true;
            }

            if (m_highlighting_threshold_exceeded)
                HighlightItem(m_first_selected_item, TRUE);
        }

        // set apropriate cursor inside/outside Bar
        CRect client_rect;
        GetClientRect(client_rect);

        if (client_rect.PtInRect(point))
            SetCursor(g_hCurArrow);            
        else
            SetCursor(g_hCurNo);

        TCHITTESTINFO ht;
        ht.pt = point;
        int item_new_pos = HitTest(&ht);
        
        if (ht.flags != TCHT_NOWHERE) 
        {
            m_scrolling = SD_NONE; // reset scrolling

            ht.pt.x = client_rect.right-1;
            ht.pt.y = point.y;
            int item_last = HitTest(&ht);

            if (item_new_pos != m_first_selected_item
            && (item_new_pos != item_last)
            && move_item(this, m_first_selected_item, item_new_pos, m_highlighting_for_dragged))
                m_first_selected_item = item_new_pos;
            else
            {
                if (m_first_selected_item)
                {
                    CRect rc;
                    GetItemRect(m_first_selected_item-1, &rc);
                    if (rc.left < 0
                    && GetItemRect(m_first_selected_item, &rc)
                    && point.x < rc.left + rc.Width()/3)
                    {
                        m_scrolling = SD_RIGHT;
                        TRACE("move to left, RIGHT scrolling activated\n");
                    }
                }

                if (!m_scrolling && m_first_selected_item < GetItemCount()-1)
                {
                    if (HWND hSpin = ::GetWindow(m_hWnd, GW_CHILD))
                    {
                        CRect spin_rect;
                        ::GetWindowRect(hSpin, &spin_rect);
                        ScreenToClient(&spin_rect);
                        if (client_rect.CenterPoint().x < spin_rect.CenterPoint().x)
                            client_rect.right = min(client_rect.right, spin_rect.left);
                        else
                            client_rect.left = max(client_rect.left, spin_rect.right);
                    }

                    CRect rc;
                    GetItemRect(m_first_selected_item+1, &rc);
                    if (rc.right > client_rect.right
                    && GetItemRect(m_first_selected_item, &rc)
                    && point.x > rc.right - rc.Width()/3)
                    {
                        m_scrolling = SD_LEFT;
                        TRACE("move to right, LEFT scrolling activated\n");
                    }
        }

                if (m_scrolling)
                    SetTimer(DRAG_MOUSE_TIMER, DRAG_MOUSE_TIMER_ELAPSE, NULL);
            }
        }
    }
    CTabCtrl::OnMouseMove(nFlags, point);
}

void COETabCtrl::OnTimer (UINT nIDEvent)
{
    if (nIDEvent == DRAG_MOUSE_TIMER)
    {
        if (m_button_is_down)
        {
            switch (m_scrolling)
            {
            case SD_RIGHT:
                {
                    if (m_first_selected_item > 0) {
                        if (move_item(this, m_first_selected_item, m_first_selected_item - 1, m_highlighting_for_dragged)) {
                            --m_first_selected_item;
                            TRACE("Scrolled\n");
                            return;
                        }
                    }
                }
                break;
            case SD_LEFT:
                {
                    int item_count = GetItemCount();
                    if (m_first_selected_item < item_count - 1) {
                        if (move_item(this, m_first_selected_item, m_first_selected_item + 1, m_highlighting_for_dragged)) {
                            ++m_first_selected_item;
                            TRACE("Scrolled\n");
                            return;
                        }
                    }
                }
                break;
            }
        }
        TRACE("Scrolling deactivated\n");
        m_scrolling = SD_NONE;
        KillTimer(DRAG_MOUSE_TIMER);
    }
    else
        CTabCtrl::OnTimer(nIDEvent);
}

/** void COETabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
 * @brief If preferences are set, on double click close tab.
 *
 * @arg nFlags Ctrl&Shift indicators.
 * @arg point Position of the cursor.
 */
void COETabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // "SysTabControl32" does not send WM_NOTIFY on DblClick,
    // but we can send this message so a parent window will handle it

    NMHDR hmhdr;
    hmhdr.hwndFrom = m_hWnd;
    hmhdr.idFrom = GetWindowLong(m_hWnd, GWL_ID);
    hmhdr.code = NM_DBLCLK;

    GetOwner()->SendMessage(WM_NOTIFY, (WPARAM)hmhdr.idFrom, (LPARAM)&hmhdr);  

    CTabCtrl::OnLButtonDblClk(nFlags, point);
}

/* some experiments with Whell - it doesn't work
BOOL COETabCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
    int current_sel=GetCurSel();
    if(zDelta > 0) {
        if(current_sel+1 >= GetItemCount())
            current_sel=0;
        SetCurSel(current_sel);
    } else if(zDelta < 0) {
        if(current_sel-1 <= 0)
            current_sel=GetItemCount();
        SetCurSel(current_sel);
    }
    return FALSE;
}*/

/* some experiments with custom OnPaint
void COETabCtrl::OnPaint()
{
    //GetWindowRect(rcBar);
    CPaintDC dc(this);
    CPen penDiv, *oldPen;

    int oldBkMode, drawColor;
    CRect rectItem;
    TC_ITEM tcItem;
    char tcItemBuf[255];

    tcItem.mask=TCIF_TEXT;
    tcItem.pszText=tcItemBuf;
    tcItem.cchTextMax=255;

    penDiv.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
    oldPen=dc.SelectObject(&penDiv);
    oldBkMode=dc.SetBkMode(TRANSPARENT);

    // for each item get info & draw
    for(int i=0; i < this->GetItemCount(); i++) {
        this->GetItemRect(i, rectItem);
        this->SetItemSize(CSize(120,20));
        if(this->GetItem(i,&tcItem) == TRUE) {

            // selected or not
            if(i == this->GetCurSel()) {
                drawColor=::GetSysColor(COLOR_BTNFACE);
                dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
            } else {
                drawColor=::GetSysColor(COLOR_BTNFACE);
                drawColor += (24+(24<<8)+(24<<16));
                dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
            }
            // fill rect
            dc.FillSolidRect(rectItem, drawColor);
            // divider
            dc.MoveTo(rectItem.right, rectItem.top+2);
            dc.LineTo(rectItem.right, rectItem.bottom);
            // text inside
            dc.DrawText(tcItem.pszText, strlen(tcItem.pszText), rectItem, DT_CENTER|DT_VCENTER);
        }
    }
    dc.SetBkMode(oldBkMode);
    dc.SelectObject(oldPen);
}
*/
