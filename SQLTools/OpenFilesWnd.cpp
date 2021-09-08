#include "stdafx.h"
#include "resource.h"
#include "OpenFilesWnd.h"
#include "MainFrm.h"
#include "COMMON/GUICommandDictionary.h"
//#include "COMMON/AppUtilities.h"
#include "COMMON/CustomShellContextMenu.h"

#define ACTIVATE_FILE_TIMER 777
#define ACTIVATE_FILE_DELAY 500

IMPLEMENT_DYNCREATE(OpenFilesWnd, CListCtrl)

BEGIN_MESSAGE_MAP(OpenFilesWnd, CListCtrl)
    ON_NOTIFY_REFLECT(NM_CLICK, &OpenFilesWnd::OnNMClick)
    ON_NOTIFY_REFLECT(NM_RCLICK, &OpenFilesWnd::OnNMRClick)
    ON_NOTIFY_REFLECT(LVN_KEYDOWN, &OpenFilesWnd::OnLvnKeydown)
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &OpenFilesWnd::OnLvnBegindrag)
    ON_WM_TIMER()
END_MESSAGE_MAP()

OpenFilesWnd::OpenFilesWnd ()
{
    m_ctxmenuFileProperties = 
    m_ctxmenuTortoiseGit = 
    m_ctxmenuTortoiseSvn = true;
}


void OpenFilesWnd::OpenFiles_Append (LVITEM& item)
{
    item.iItem = GetItemCount();
    VERIFY(InsertItem(&item) != -1);
}


void OpenFilesWnd::OpenFiles_UpdateByParam (LPARAM param, LVITEM& item)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
    {
        // sorting does not work for update so ...
        VERIFY(DeleteItem(nItem));
        VERIFY(InsertItem(&item) != -1);
    }
}


void OpenFilesWnd::OpenFiles_RemoveByParam (LPARAM param)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(DeleteItem(nItem));
}


void OpenFilesWnd::OpenFiles_ActivateByParam (LPARAM param)
{
    int nItem = OpenFiles_FindByParam(param);

    ASSERT(nItem != -1);

    if (nItem != -1)
        VERIFY(SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED));
}


LPARAM OpenFilesWnd::OpenFiles_GetCurSelParam ()
{
    int nItem = GetNextItem(-1, LVNI_SELECTED);

    if (nItem != -1)
    {
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = nItem;
        VERIFY(GetItem(&item));
        return item.lParam;
    }

    return 0;
}


int OpenFilesWnd::OpenFiles_FindByParam (LPARAM param)
{
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_PARAM|LVIF_TEXT;
    int nItems = GetItemCount();

    for (item.iItem = 0; item.iItem < nItems; item.iItem++) 
    {
        VERIFY(GetItem(&item));

        if (item.lParam == param)
            return item.iItem;
    }

    return -1;
}


void OpenFilesWnd::ActivateOpenFile ()
{
    POSITION pos = GetFirstSelectedItemPosition();

    if (pos)
    {
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = GetNextSelectedItem(pos);

        VERIFY(GetItem(&item));
        _ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIMainFrame)));
        _ASSERT(((CMDIChildWnd*)item.lParam)->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));

        ((CMDIMainFrame*)AfxGetMainWnd())->ActivateChild((CMDIChildWnd*)item.lParam);
    }
}

BOOL OpenFilesWnd::GetActiveDocumentPath (NMITEMACTIVATE* pItem, CString& path)
{
    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = pItem->iItem;
    VERIFY(GetItem(&item));
    ASSERT(AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CWorkbookMDIFrame)));
    ASSERT(((CMDIChildWnd*)item.lParam)->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)));
    path = ((CMDIChildWnd*)item.lParam)->GetActiveDocument()->GetPathName();

    return !path.IsEmpty();
}

void OpenFilesWnd::OnNMClick (NMHDR *, LRESULT *pResult)
{
    ActivateOpenFile();
    *pResult = 0;
}


void OpenFilesWnd::OnNMRClick (NMHDR *pNMHDR, LRESULT *pResult)
{
    ActivateOpenFile();

    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    if (pItem && pItem->iItem != -1)
    {
        *pResult = 1;
        CPoint point(pItem->ptAction);
        ClientToScreen(&point);

        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_OE_WORKBOOK_POPUP));
        CMenu* pPopup = menu.GetSubMenu(0);

        ASSERT(pPopup != NULL);
        ASSERT_KINDOF(CFrameWnd, AfxGetMainWnd());
        Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

        CString path;
        if ((m_ctxmenuFileProperties || m_ctxmenuTortoiseGit || m_ctxmenuTortoiseSvn) && GetActiveDocumentPath(pItem, path) && PathFileExists(path))
        {
            AfxGetMainWnd()->SendMessage(WM_INITMENUPOPUP, (WPARAM)pPopup->m_hMenu, 0);
            if (UINT idCommand = CustomShellContextMenu(this, point, path, pPopup, m_ctxmenuFileProperties, m_ctxmenuTortoiseGit, m_ctxmenuTortoiseSvn).ShowContextMenu())
                AfxGetMainWnd()->SendMessage(WM_COMMAND, idCommand, 0);
        }
        else
        {
            pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
        }
    }
    else
        *pResult = 0;
}


void OpenFilesWnd::OnLvnKeydown (NMHDR *, LRESULT *pResult)
{
    SetTimer(ACTIVATE_FILE_TIMER, ACTIVATE_FILE_DELAY, 0);
    *pResult = 0;
}


void OpenFilesWnd::OnLvnBegindrag (NMHDR *pNMHDR, LRESULT *pResult)
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    
    if (pItem && pItem->iItem != -1)
    {
        CString path;

        if (GetActiveDocumentPath(pItem, path))
        {
            //MessageBox(path);
            UINT uBuffSize = sizeof(DROPFILES) + path.GetLength() + 1 + 1;
            if (HGLOBAL hgDrop = GlobalAlloc(GHND|GMEM_SHARE, uBuffSize))
            {
                if (DROPFILES*  pDrop = (DROPFILES*)GlobalLock(hgDrop))
                {
                    memset(pDrop, 0, uBuffSize);
                    pDrop->pFiles = sizeof(DROPFILES);
                    memcpy((LPBYTE(pDrop) + sizeof(DROPFILES)), (LPCTSTR)path, path.GetLength());

                    GlobalUnlock(hgDrop);

                    COleDataSource data;
                    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                    data.CacheGlobalData ( CF_HDROP, hgDrop, &etc );

                    /*DROPEFFECT dwEffect  =*/ data.DoDragDrop( DROPEFFECT_COPY );
                }
            }
        }
    }

    *pResult = 0;
}


void OpenFilesWnd::OnTimer (UINT_PTR nIDEvent)
{
    if (nIDEvent == ACTIVATE_FILE_TIMER)
    {
        BOOL active = GetFocus() == this ? TRUE : FALSE;
        ActivateOpenFile();
        if (active) 
            SetFocus();
    }

    CListCtrl::OnTimer(nIDEvent);
}

LRESULT OpenFilesWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return __super::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

