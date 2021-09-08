#include "stdafx.h"
#include "ManagedListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Common
{

CManagedListCtrl::CManagedListCtrl (ListCtrlDataProvider& adapter)
: m_manager(*this, adapter)
{
}

BEGIN_MESSAGE_MAP(CManagedListCtrl, CListCtrl)
    ON_WM_CREATE()
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnclick)
    ON_NOTIFY(HDN_FILTERCHANGE, 0, OnHdnFilterChange)
    ON_NOTIFY(HDN_FILTERBTNCLICK, 0, OnHdnFilterBtnClick)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfo)
END_MESSAGE_MAP()

void CManagedListCtrl::Init ()
{
    m_manager.OnCreate();
}

int CManagedListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CListCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_manager.OnCreate();

    return 0;
}

void CManagedListCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    m_manager.OnSort(pNMLV->iSubItem);

    *pResult = 0;
}

void CManagedListCtrl::OnHdnFilterChange(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

    m_manager.OnFilterChange(phdr->iItem);
    
    *pResult = 0;
}

void CManagedListCtrl::OnHdnFilterBtnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMHDFILTERBTNCLICK phdr = reinterpret_cast<LPNMHDFILTERBTNCLICK>(pNMHDR);

    m_manager.OnFilterBtnClick(phdr->iItem);

    *pResult = 0;
}

void CManagedListCtrl::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    try
    {
        if (pDispInfo->item.mask & LVIF_TEXT)
        {
            wcsncpy(pDispInfo->item.pszText, 
                m_manager.GetString((int)pDispInfo->item.lParam, pDispInfo->item.iSubItem), 
                pDispInfo->item.cchTextMax);
            pDispInfo->item.pszText[pDispInfo->item.cchTextMax-1] = 0;
        }

        if (pDispInfo->item.mask & LVIF_STATE)
        {
            // +1 for convertion to 1 based index
            pDispInfo->item.state = (m_manager.GetStateImageIndex((int)pDispInfo->item.lParam)+1) << (8 + 4);
        }
    }
    catch (...)
    {
        wcsncpy(pDispInfo->item.pszText, L"error", pDispInfo->item.cchTextMax);
        pDispInfo->item.state = 0;
    }
    
    *pResult = 0;
}

LRESULT CManagedListCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CListCtrl::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

}//namespace Common

