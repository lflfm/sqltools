/*
    Copyright (C) 2005-2015 Aleksey Kochetov

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
#include "ListCtrlDataProvider.h"

namespace Common
{

class CManagedListCtrl : public CListCtrl
{
    ListCtrlManager m_manager;
public:
	CManagedListCtrl (ListCtrlDataProvider&);

    void SetSortColumn (int col, ListCtrlManager::ESortDir dir = ListCtrlManager::ASC);
    void GetSortColumn (int& col, ListCtrlManager::ESortDir& dir);
    void SetFilter (const ListCtrlManager::FilterCollection&);
    void GetFilter (ListCtrlManager::FilterCollection&);

    void Init (); // call it after subclassing

    void OnAppendEntry ()          { m_manager.OnAppendEntry(); }
    void OnUpdateEntry (int entry) { m_manager.OnUpdateEntry(entry); }
    void OnDeleteEntry (int entry, bool moveLastSelected = true) { m_manager.OnDeleteEntry(entry, moveLastSelected); }

    void Refresh (bool autosizeColumns = false) { m_manager.OnRefresh(autosizeColumns); }
    void Resort ()                              { m_manager.Resort(); }

    void SelectEntry (int entry);

    void ShowFilterDialog (int col = 0) { m_manager.OnFilterBtnClick(col); }

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnHdnFilterChange(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnHdnFilterBtnClick(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

inline
void CManagedListCtrl::SetSortColumn (int col, ListCtrlManager::ESortDir dir) 
{ 
    m_manager.SetSortColumn(col, dir); 
}

inline
void CManagedListCtrl::GetSortColumn (int& col, ListCtrlManager::ESortDir& dir)
{
    m_manager.GetSortColumn(col, dir); 
}

inline
void CManagedListCtrl::SetFilter (const ListCtrlManager::FilterCollection& filter)
{
    m_manager.SetFilter(filter);
}

inline
void CManagedListCtrl::GetFilter (ListCtrlManager::FilterCollection& filter)
{
    m_manager.GetFilter(filter);
}

inline
void CManagedListCtrl::SelectEntry (int entry)
{
    m_manager.SelectEntry(entry);
}


}//namespace Common
