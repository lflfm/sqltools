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

#include "stdafx.h"
#include "GridManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{

bool GridManager::IsFixedCorner (const CPoint& point) const
{
    for (int k(0); k < 2; k++) 
    {
        int i = m_Rulers[k].PointToItem(!k ? point.x : point.y);
        if (!(i < m_Rulers[k].GetFirstFixCount()))
            return false;
    }
    return true;
}

bool GridManager::IsFixedCol (int x) const
{
    return (m_Rulers[0].PointToItem(x) < m_Rulers[0].GetFirstFixCount()) ? true : false;
}

bool GridManager::IsFixedRow (int y) const
{
    return (m_Rulers[1].PointToItem(y) < m_Rulers[1].GetFirstFixCount()) ? true : false;
}

bool GridManager::IsDataCell (const CPoint& point) const
{
    for (int k(0); k < 2; k++) 
    {
        int i = m_Rulers[k].PointToItem(!k ? point.x : point.y);
        if (!m_Rulers[k].IsDataItem(i))
            return false;
    }
    return true;
}

}// namespace OG2