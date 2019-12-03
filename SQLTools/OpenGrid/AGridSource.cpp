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
/*      Purpose: Superbase data source class for grid component            */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

#include "stdafx.h"
#include "AGridSource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2 {

    using Common::nvector;

AGridSource::AGridSource () 
    : m_empty(true), 
    m_disableNotifications(false),
    m_GridManagers("AGridSource::m_GridManagers")
{
}

void AGridSource::EvAttach (AGridManager* mgr)                               
{
    if (mgr)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end && *it != mgr; it++);

        if (it == end)
           m_GridManagers.push_back(mgr);
    }
}

void AGridSource::EvDetach (AGridManager* mgr)                               
{
    if (mgr)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end && *it != mgr; it++);

        if (*it == mgr)
           m_GridManagers.erase(it, it + 1);
    }
}

void AGridSource::Notify_ChangedRows (int startRow, int endRow)
{
    if (!m_disableNotifications)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end; it++)
            (*it)->OnChangedRows(startRow, endRow);
    }
}

void AGridSource::Notify_ChangedCols (int startCol, int endCol)
{
    if (!m_disableNotifications)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end; it++)
            (*it)->OnChangedCols(startCol, endCol);
    }
}

void AGridSource::Notify_ChangedRowsQuantity (int row, int quantity)
{
    if (!m_disableNotifications)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end; it++)
            (*it)->OnChangedRowsQuantity(row, quantity);
    }
}

void AGridSource::Notify_ChangedColsQuantity (int col, int quantity)
{
    if (!m_disableNotifications)
    {
        nvector<AGridManager*>::iterator
            it(m_GridManagers.begin()), end(m_GridManagers.end());

        for (; it != end; it++)
            (*it)->OnChangedColsQuantity(col, quantity);
    }
}

AGridSource::NotificationDisabler::NotificationDisabler (AGridSource* owner)
: m_pOwner(owner)
{
    _ASSERTE(!m_pOwner->m_disableNotifications); // recursion is not supported
    m_pOwner->m_disableNotifications = true;
}

void AGridSource::NotificationDisabler::Enable ()
{
    m_pOwner->m_disableNotifications = false;
    //if (int rows = m_pOwner->GetCount(edVert))
    //    m_pOwner->Notify_ChangedRows(0, rows);
    //if (int cols = m_pOwner->GetCount(edHorz))
    //    m_pOwner->Notify_ChangedCols(0, cols);
}

}//namespace OG2

