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

// 07.12.2003 bug fix, Ctrl+End causes unknown exception in data grid if query result is empty
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

GridRuler::GridRuler ()
: m_Items("GridRuler::m_Items"),
  m_cacheFistFixItems("GridRuler::m_cacheFistFixItems"),
  m_cacheLastFixItems("GridRuler::m_cacheLastFixItems")
{
}

void GridRuler::Clear ()       
{ 
    GridRulerBase::Clear(); 
    m_resizedItems.clear();
    m_cacheFistFixItems.clear();
    m_cacheLastFixItems.clear();
}

void GridRuler::OnCurrentChanging ()
{
    GetManager()->BeforeMove(m_Dir);
    GetManager()->RepaintCurent();
}

void GridRuler::OnCurrentChanged (int oldVal)
{
    if (!IsCurrentFullVisible())
        GetManager()->RecalcRuller(m_Dir, true/*adjust*/, true/*invalidate*/);
    GetManager()->RepaintCurent();
    GetManager()->AfterMove(m_Dir, oldVal);
}

void GridRuler::OnTopmostChanging ()
{
    GetManager()->BeforeScroll(m_Dir);
}

void GridRuler::OnTopmostChanged (int)
{
    GetManager()->RecalcRuller(m_Dir, false/*adjust*/, true/*invalidate*/);
    GetManager()->AfterScroll(m_Dir);
}

bool GridRuler::IsLastDataItemFullVisible() const
{
    if (m_Items.empty() || m_UnusedCount > 0)
        return true;

    int x = m_Items[m_FirstFixCount + m_NoFixCount - m_UnusedCount] - 1;

    if (!m_LastFixCount)
        return (x < GetClientSize());
    else
        return (x + m_LineSize < m_Items[m_FirstFixCount + m_NoFixCount]);
}

void GridRuler::calcRuller (int cx)
{
    if (!cx) return;

    SetClientSize(cx);
    SetCount(0);
    SetNoFixCount(0);
    SetUnusedCount(0);

    int lim = GetSourceCount();

    
    _ASSERTE(GetCurrent() >= 0); 
    if (GetCurrent() < 0) SetCurrent(0);

    _ASSERTE(!(GetCurrent() > 0 && GetCurrent() >= lim)); 
    if (GetCurrent() > 0 && GetCurrent() >= lim) SetCurrent(lim - 1);
    
    _ASSERTE(GetTopmost() >= 0);
    if (GetTopmost() < 0) setTopmostSilently(0);

    _ASSERTE(!(GetTopmost() > 0 && GetTopmost() >= lim));
    if (GetTopmost() > 0 && GetTopmost() >= lim) setTopmostSilently(lim - 1);

    SetScrollSize(cx - (m_FirstFixSize + m_LastFixSize))/* + m_LineSize*/;
    
    if (m_ScrollSize < 0)
        m_ScrollSize = 0;

    m_Items.clear();

    int itemLeft(-m_LineSize), itemWidth(0);
    for (int i = 0; i < m_FirstFixCount; i++) 
    {
        itemLeft += itemWidth + m_LineSize;
        itemWidth = GetFirstFixPixelSize(i);
        m_Items.push_back(itemLeft);
    }

    int lastLim = m_ScrollSize + m_FirstFixSize;
    for (i = GetTopmost(); i < lim; i++) 
    {
        itemLeft += itemWidth + m_LineSize;
        itemWidth = GetPixelSize(i);
        m_Items.push_back(itemLeft);
        if (itemLeft + itemWidth > lastLim) break;
    }

    if (i == lim && (itemLeft + itemWidth < lastLim)) 
    {
        if ((m_Dir == edHorz && m_Manager->m_Options.m_ExpandLastCol)
        || (m_Dir == edVert && m_Manager->m_Options.m_ExpandLastRow)) 
        {
            itemWidth = GetClientSize() - m_LastFixSize - itemLeft - m_LineSize;
        } 
        else 
        {
            m_UnusedCount = 1;
            itemLeft += itemWidth + m_LineSize;
            itemWidth = lastLim - itemLeft;
            m_Items.push_back(itemLeft);
        }
    }

    for (i = 0; i < m_LastFixCount; i++) 
    {
        if (!i) // считаем от грани
            itemLeft = GetClientSize() - m_FirstFixSize + m_LineSize;
        else
            itemLeft += itemWidth + m_LineSize;
        itemWidth = GetLastFixPixelSize(i);
        m_Items.push_back(itemLeft);
    }
    m_Count = m_Items.size();
    if (m_Count)
        m_Items.push_back(m_Items[m_Count-1] + itemWidth + m_LineSize);
    m_NoFixCount = m_Count - m_FirstFixCount - m_LastFixCount;
}

    static const int g_dirToSBar[2] = { SB_HORZ, SB_VERT };

void GridRuler::calcScroller ()
{
    bool enabled[2] = { m_Manager->m_Options.m_HorzScroller, m_Manager->m_Options.m_VertScroller };
    bool always[2]  = { m_Manager->m_Options.m_HorzScrollerAlways, m_Manager->m_Options.m_VertScrollerAlways };

    if (enabled[m_Dir])
    {
        SCROLLINFO scrollInfo;
        scrollInfo.cbSize    = sizeof SCROLLINFO;
        scrollInfo.fMask     = SIF_POS|SIF_RANGE|SIF_PAGE | (always[m_Dir] ? SIF_DISABLENOSCROLL : 0);
        scrollInfo.nMin      = 0;
        scrollInfo.nMax      = GetSourceCount() - 1;
        scrollInfo.nPage     = GetFullVisibleDataItemCount();
        scrollInfo.nPos      = GetTopmost();
        scrollInfo.nTrackPos = 0;

        if (m_Scroller)
            ::SetScrollInfo(m_Scroller, SB_CTL, &scrollInfo, TRUE);
        else 
            GetClientWnd()->SetScrollInfo(g_dirToSBar[m_Dir], &scrollInfo, TRUE);
    }
}

void GridRuler::adjustTopmostByPos (int pos, bool full)
{
    int topmost = 0;

    if (pos > 0)
    {
        topmost = min(pos, GetSourceCount() - 1);
        for (int size = 0; topmost >= 0; topmost--) 
        {
            size += GetPixelSize(topmost) + m_LineSize;
            if (size > m_ScrollSize) 
            {
                if (full && topmost != pos)
                    topmost++;
                break;
            }
        }
    }
    
    return SetTopmost((topmost < 0) ? 0 : topmost);
}

int GridRuler::PointToItem (int x) const
{
    //_ASSERTE(x >= 0);

    for (int i = 0; i < m_Count; i++)
        if (x < m_Items[i])
            break;

    return (i) ? (i - 1) : (0);
}

bool GridRuler::IsCurrentFullVisible () const
{
    if (IsCurrentVisible())
    {
        int data[2], curr[2];

        GetDataArea(data);
        GetCurrentLoc(curr);

        return (curr[0] < data[0] || curr[1]-1 > data[1]) ? false : true; // ignoring 1px difference on the right/bottom
    }
    return false;
}

EFixType GridRuler::GetItemType (int i) const
{
    EFixType type;
    
    if (i < m_FirstFixCount) 
        type = efFirst;
    else if (i < m_FirstFixCount + m_NoFixCount)    
        type = efNone;
    else 
        type = efLast;
    
    return type;
}


// Cursor navigation
bool GridRuler::MoveToLeft ()
{
    if (GetCurrent() > 0)
    {
        DecCurrent();
        return true;
    }
    return false;
}

bool GridRuler::MoveToRight ()
{
    int last = GetSourceCount() - 1;

    if (GetCurrent() < last)
    {
        IncCurrent();
        return true;
    }
    return false;
}

bool GridRuler::MoveToHome ()
{
    if (GetCurrent())
    {
        SetCurrent(0);
        return true;
    }
    return false;
}

bool GridRuler::MoveToEnd ()
{
    int last = GetSourceCount() - 1;

    // 07.12.2003 bug fix, Ctrl+End causes unknown exception in data grid if query result is empty
    if (last > 0 && GetCurrent() != last)
    {
        SetCurrent(last);
        return true;
    }
    return false;
}

void GridRuler::ClearCacheItems ()                                     
{ 
    m_resizedItems.clear(); 
    m_cacheFistFixItems.clear(); 
    m_cacheLastFixItems.clear(); 
}

/*
  the following method was tested only for quantity is equal to 1 or -1 !!!
*/
void GridRuler::OnChangedQuantity (int pos, int quantity)
{
    if (quantity > 0) // inserting
    {
        bool recalc = false;

        // shift cached items
        std::map<int, int> resizedItems;
        std::map<int, int>::const_iterator it = m_resizedItems.begin();
        for (; it != m_resizedItems.end(); ++it)
            if (it->first < pos)
                resizedItems[it->first] = it->second;
            else
                resizedItems[it->first + quantity] = it->second;
        m_resizedItems.swap(resizedItems);

        // the first inserted pos is above viewport
        if (pos < GetTopmost()) {
            setTopmostSilently(GetTopmost() + quantity);
            GetManager()->RepaintAll(); // actually only numbers have to be repainted
        }
        // the first inserted pos is above of viewport bottom
        else if (pos <= GetTopmost() + GetDataItemCount())
            recalc = true;
        // else 
            // don't care about lines below viewport
        
        // adjust the cursor pos
        if (pos <= GetCurrent())
        {
            int curPos = //(quantity == GetSourceCount()) ? 0 : // this is not initial population
                std::min<int>(GetCurrent() /*+ quantity*/, GetSourceCount()-1);

            if (curPos != GetCurrent())
            {
                recalc = true;
                setCurrentSilently(curPos);
            }
        }

        if (recalc)
            GetManager()->RecalcRuller(m_Dir, false/*adjust*/, true/*invalidate*/);
        else
            calcScroller(); // always recalc scroller
    }
    else if (quantity < 0) // deleting
    {
        bool recalc = false;

        // shift cached items
        std::map<int, int> resizedItems;
        std::map<int, int>::const_iterator it = m_resizedItems.begin();
        for (; it != m_resizedItems.end(); ++it)
            if (it->first < pos)
                resizedItems[it->first] = it->second;
            else if (it->first < pos + quantity)
                ; // skipping
            else
                resizedItems[it->first - quantity] = it->second;
        m_resizedItems.swap(resizedItems);

        // the first deleted pos is below viewport
        if (pos >= GetTopmost() + GetDataItemCount())
        {
            // do nothing
        }
        // the last deleted pos is above viewport
        else if (pos - quantity <= GetTopmost())
        {
            // just shift topmost
            int topmost = GetTopmost() + quantity;
            setTopmostSilently(topmost < 0 ? 0 : topmost);
            GetManager()->RepaintAll(); // actually only numbers have to be repainted
        }
        // the first deleted pos is above topmost
        else if (pos < GetTopmost())
        {
            // set topmost to the first record after deleted 
            // if it exists
            int topmost = pos - quantity + 1;
            int count = GetSourceCount();
            topmost = std::min<int>(topmost, count ? count - 1 : 0);
            setTopmostSilently(topmost < 0 ? 0 : topmost);
            recalc = true;
        }
        // exact match, the first deleted pos is equal to topmost
        else if (pos == GetTopmost())
        {
            int count = GetSourceCount();
            
            // it's the last record
            if (pos > count - 1)
                setTopmostSilently(pos <= 0 ? 0 : pos - 1);

            recalc = true;
        }
        // the first deleted pos is below topmost
        else if (pos > GetTopmost())
        {
            // grid does not support partial invalidation
            recalc = true;
        }
        // we should not reach this line
        else
            ASSERT(0);

        // adjust the current pos
        int curLine = GetCurrent();
        
        // the deleted pos is below the cur pos
        if (curLine < pos)
        {
            // do nothing
        }
        // the current pos is in the deleted lines
        else if (pos <= curLine && curLine < pos - quantity) 
        {
            curLine = pos;
            int count = GetSourceCount();
            curLine = std::min<int>(curLine, count ? count - 1 : 0);
        }
        // the current pos is below the last deleted pos
        else if (curLine >= pos - quantity) 
        {
            curLine += quantity;
            if (curLine < 0) curLine = 0;
        }
        // we should not reach this line
        else
            ASSERT(0);
        
        if (curLine != GetCurrent())
        {
            recalc = true;
            setCurrentSilently(curLine);
        }

        if (recalc)
            GetManager()->RecalcRuller(m_Dir, false/*adjust*/, true/*invalidate*/);        
        else
            calcScroller(); // always recalc scroller
    }
}

//////////////////////////////////////////////////////////////////////

CalcGridManager::CalcGridManager (CWnd* client, AGridSource* source, bool shouldDeleteSource)
    : m_pClientWnd(client),
    m_pSource(source),
    m_ShouldDeleteSource(shouldDeleteSource)
{
    for (int dir = 0; dir < 2 ; dir++)
    {
        m_Rulers[dir].SetDir((eDirection)dir);
        m_Rulers[dir].SetManager(this);
    }
}

AGridSource* CalcGridManager::SetSource (AGridSource* source, bool shouldDeleteSource)
{
    m_pSource->EvDetach(this);
    
    std::swap(source, m_pSource);
    std::swap(shouldDeleteSource, m_ShouldDeleteSource);
    
    if (shouldDeleteSource) 
    {
        delete source;
        source = 0;
    }

    CRect rect(0, 0, m_Rulers[edHorz].GetClientSize(), m_Rulers[edVert].GetClientSize());
    CSize charSize(m_Rulers[edHorz].GetCharSize(), m_Rulers[edVert].GetCharSize());
    
    CalcGridManager::EvOpen(rect, charSize);

    return source;
}

void CalcGridManager::SetupRuller (eDirection dir, int charSize, bool resetPos)
{
    if (resetPos)
        m_Rulers[dir].Reset();

    m_Rulers[dir].Clear();
    m_Rulers[dir].SetCharSize(charSize);
    m_Rulers[dir].SetFirstFixCount(m_pSource->GetFirstFixCount(dir));
    m_Rulers[dir].SetLastFixCount(m_pSource->GetLastFixCount(dir));
    m_Rulers[dir].SetFirstFixSize(0);
    m_Rulers[dir].SetLastFixSize(0);
    m_Rulers[dir].SetCellMargin((dir == edHorz) ? 3 : 1);

    if (dir == edHorz) 
        m_Rulers[edHorz].SetLineSize(m_Options.m_VertLine ? 1 : 0);
    else  
        m_Rulers[edVert].SetLineSize(m_Options.m_HorzLine ? 1 : 0);

    m_Rulers[dir].CalcFirstFixSize();
    m_Rulers[dir].CalcLastFixSize();
}

void CalcGridManager::EvOpen (CRect& rect, CSize& charSize)
{
    m_pSource->EvAttach(this);
    SetupRuller(edHorz, charSize.cx, true);
    SetupRuller(edVert, charSize.cy, true);
    CalcRuller(edHorz, rect.Width(),  true, false/*invalidate*/);
    CalcRuller(edVert, rect.Height(), true, false/*invalidate*/);
    m_pClientWnd->Invalidate(FALSE);
}

void CalcGridManager::EvFontChanged (CSize& charSize)
{
    if (m_Rulers[edHorz].GetCharSize() != charSize.cx
    || m_Rulers[edVert].GetCharSize() != charSize.cy)
    {
        CSize clisenSize(m_Rulers[edHorz].GetClientSize(), m_Rulers[edVert].GetClientSize());
        SetupRuller(edHorz, charSize.cx, false);
        SetupRuller(edVert, charSize.cy, false);
        m_Rulers[edHorz].SetClientSize(clisenSize.cx); 
        m_Rulers[edVert].SetClientSize(clisenSize.cy);
        Refresh();
    }
}

void CalcGridManager::EvClose ()
{
    m_pSource->EvDetach(this);
}

void CalcGridManager::DoEvSize (const CSize& size, bool adjust, bool force)
{
    if (size.cy && (force || m_Rulers[edVert].GetClientSize() != size.cy))
        CalcRuller(edVert, size.cy, adjust, false/*invalidate*/);

    if (size.cx && (force || m_Rulers[edHorz].GetClientSize() != size.cx)) 
        CalcRuller(edHorz, size.cx, adjust, false/*invalidate*/);
}

void CalcGridManager::GetCurRect (CRect& rc) const
{
    int loc[4];
    m_Rulers[edHorz].GetCurrentInnerLoc(loc);
    m_Rulers[edVert].GetCurrentInnerLoc(loc+2);
    rc.left   = loc[0];
    rc.top    = loc[2];
    rc.right  = loc[1];
    rc.bottom = loc[3];
}

void CalcGridManager::Rotate ()
{
    rotate(m_Rulers[edHorz], m_Rulers[edVert]);

    for (int dir = 0; dir < 2; dir++)
    {
        ClearCacheItems((eDirection)dir);
        int clientSize = m_Rulers[dir].GetClientSize();
        //int sizeAsOne = m_Rulers[dir].GetSizeAsOne();
        SetupRuller((eDirection)dir, m_Rulers[dir].GetCharSize(), false/*resetPos*/);
        //m_Rulers[dir].SetSizeAsOne(sizeAsOne);
        CalcRuller((eDirection)dir, clientSize, true/*adjust*/, false/*invalidate*/);
    }

    m_pClientWnd->Invalidate(FALSE);
    InvalidateEdit();
}

void CalcGridManager::Clear ()
{
    m_Rulers[edHorz].Reset();
    m_Rulers[edVert].Reset();
    Refresh();
}

void CalcGridManager::Refresh ()
{
    RecalcRuller(edHorz, true, false/*invalidate*/);
    RecalcRuller(edVert, true, false/*invalidate*/);

    m_pClientWnd->Invalidate(FALSE);
    InvalidateEdit();
}

void CalcGridManager::CalcRuller (eDirection dir, int cx, bool adjust, bool invalidate)
{
    m_Rulers[dir].calcRuller(cx);

    bool fullVisible = m_Rulers[dir].IsCurrentFullVisible();
    bool unusedSpace = m_Rulers[dir].GetUnusedCount() > 0;

    if (adjust && (!fullVisible || unusedSpace))
    {
        if (m_Rulers[dir].GetTopmost() > m_Rulers[dir].GetCurrent()) // curremt is above topmost
        {
            m_Rulers[dir].SetTopmost(m_Rulers[dir].GetCurrent());
            invalidate = true;
        }
        else
        {
            if (fullVisible && unusedSpace)
                m_Rulers[dir].adjustTopmostByPos(m_pSource->GetCount(dir), true);
            else
                m_Rulers[dir].adjustTopmostByPos(m_Rulers[dir].GetCurrent(), true);
            invalidate = true;
        }
    }

    CalcScroller(dir);

    if (invalidate)
        m_pClientWnd->Invalidate();
}

bool CalcGridManager::IsPointOverRangeSelection(const CPoint& point) const
{
    if (point.x >= 0 && point.x < m_Rulers[edHorz].GetClientSize()
        && point.y >= 0 && point.y < m_Rulers[edVert].GetClientSize())
    {
        int  sel[2] = { false };
        bool hit[2] = { false };

        for (int dir = 0; dir < 2; dir++)
        {
            if (!m_Rulers[dir].IsSelectionEmpty())
            {
                sel[dir] = true;

                int inx = m_Rulers[dir].PointToItem(!dir ? point.x : point.y)
                    - m_Rulers[dir].GetFirstFixCount();

                if (inx >= 0)
                {
                    int pos = inx + m_Rulers[dir].GetTopmost();
                    int first, last;
                    if (m_Rulers[dir].GetNormalizedSelection(first, last)
                    && first <= pos && pos <= last)
                        hit[dir] = true;
                }
            }
        }

        if (sel[edHorz] && sel[edVert]) // rectangular selection
            return hit[edHorz] && hit[edVert];

        if (sel[edHorz] && !sel[edVert]) // column selection
            return hit[edHorz];

        else if (!sel[edHorz] && sel[edVert]) // row selection
            return hit[edVert];
    }

    return false;
}

} // namespace OG2

