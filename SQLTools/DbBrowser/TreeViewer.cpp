/*
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
    09.05.2016 Count query (Ctrl+F11) fails from Object Viewer if table/view selected w/o columns
    2018.02.24 Object Viewer "Query" and "Count" commands work with partitions2 and subpartitions 
    2018.02.24 Added help for Data grid and Object Viewer
*/
#include "stdafx.h"
#include "SQLTools.h"
#include "TreeViewer.h"
#include <COMMON\AppUtilities.h>
#include <COMMON\ExceptionHelper.h>
#include "MainFrm.h"
#include "OpenEditor/OEView.h"
#include "SQLWorksheet\SQLWorksheetDoc.h"
#include "COMMON\InputDlg.h"
#include "ObjectTree_Builder.h"
#include "COMMON/PropertySheetMem.h"
#include "Dlg/ObjectViewerPage.h"
#include "SessionCache.h"
#include "COMMON/GUICommandDictionary.h"
#include "SQLUtilities.h"
#include <Common/StrHelpers.h>
#include "ServerBackgroundThread\TaskQueue.h"
#include "ConnectionTasks.h"
#include "ActivePrimeExecutionNote.h"

using namespace ObjectTree;
using namespace ServerBackgroundThread;
using ObjectTree::TreeNodePoolPtr;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    class SafeTreeNodePtr : private boost::noncopyable
    {
        TreeNode* m_node;
    public:
        SafeTreeNodePtr (const ObjectTree::TreeNodePoolPtr& pool, TreeNode* node)
            : m_node(0) { if (pool.get()) m_node = pool->ValidatePtr(node); }

        SafeTreeNodePtr (const ObjectTree::TreeNodePoolPtr& pool, DWORD_PTR ptr)
            : m_node(0) { if (pool.get()) m_node = pool->ValidatePtr(reinterpret_cast<TreeNode*>(ptr)); }

        operator TreeNode* ()    { return m_node; }
        TreeNode* operator -> () { return m_node; }
        TreeNode* get ()         { return m_node; }
    };


/////////////////////////////////////////////////////////////////////////////
// CTreeViewer
CTreeViewer::CTreeViewer() 
    : m_classicTreeLook(false), m_accelTable(NULL), m_requestCounter(0)
{
}

CTreeViewer::~CTreeViewer() 
{
    try { EXCEPTION_FRAME;

	    if (m_accelTable)
		    DestroyAcceleratorTable(m_accelTable);
    }
    _DESTRUCTOR_HANDLER_
}

void CTreeViewer::SetClassicTreeLook (bool classicTreeLook)
{
    m_classicTreeLook = classicTreeLook;
    if (m_hWnd)
    {
        if (m_classicTreeLook)
            ModifyStyle(TVS_FULLROWSELECT, TVS_HASLINES);
        else
            ModifyStyle(TVS_HASLINES, TVS_FULLROWSELECT);
    }
}

void CTreeViewer::LoadAndSetImageList (UINT nResId)
{
    m_Images.Create(nResId, 16, 64, RGB(0,255,0));
    SetImageList(&m_Images, TVSIL_NORMAL);
}

BEGIN_MESSAGE_MAP(CTreeViewer, CTreeCtrlEx)
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &CTreeViewer::OnTvnItemexpanding)
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CTreeViewer::OnNMCustomdraw)
    ON_WM_SETCURSOR()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_COMMAND(ID_OV_SETTING, OnSettings)
    ON_COMMAND(ID_OV_REFRESH, OnRefresh)
    ON_COMMAND(ID_OV_PURGE_CACHE, OnPurgeCache)
    ON_COMMAND(ID_OV_SORT_NATURAL, OnSortNatural)
    ON_COMMAND(ID_OV_SORT_ALPHABETICAL, OnSortAlphabetical)
    ON_COMMAND(ID_SQL_LOAD, OnSqlLoad)
    ON_COMMAND(ID_SQL_LOAD_WITH_DBMS_METADATA, OnSqlLoadWithDbmsMetadata)
    ON_COMMAND(ID_SQL_QUICK_QUERY, OnSqlQuickQuery)
    ON_COMMAND(ID_SQL_QUICK_COUNT, OnSqlQuickCount)
END_MESSAGE_MAP()

void CTreeViewer::OnInit ()
{
	if (!m_accelTable)
	{
		CMenu menu;
        menu.CreatePopupMenu();

        //BuildContexMenu(menu, 0);
        menu.AppendMenu(MF_STRING, ID_EDIT_COPY,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_SQL_LOAD,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_SQL_LOAD_WITH_DBMS_METADATA,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_SQL_QUICK_QUERY,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_SQL_QUICK_COUNT,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_OV_PURGE_CACHE,  L"dummy");
        menu.AppendMenu(MF_STRING, ID_OV_SETTING,  L"dummy");

		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(menu.m_hMenu);
	}

    ModifyStyleEx(0, 0x0004/*TVS_EX_DOUBLEBUFFER*/);
    //ModifyStyle(0, /*TVS_FULLROWSELECT|*/TVS_HASBUTTONS|TVS_HASLINES, 0);
    SetClassicTreeLook(m_classicTreeLook);
}

string CTreeViewer::GetTooltipText (HTREEITEM hItem)
{
    SafeTreeNodePtr ptr(m_pNodePool, GetItemData(hItem));

    if (ptr)
        return ptr->GetTooltipText();

    return string();
}

    /*
    this task does not do anything in backgrund, 
    cteated for synchronization with async object loading
    */
	struct BackgroundTask_Drilldown : Task
    {
        CTreeViewer& m_viewer;
        HTREEITEM m_hFrom;
        string m_drildowTo;
        vector<string> m_drilldown;

        BackgroundTask_Drilldown (CTreeViewer& viewer, HTREEITEM hFrom, const vector<string>& drilldown)
            : Task("Drilldown to node", 0),
            m_viewer(viewer),
            m_hFrom(hFrom),
            m_drilldown(drilldown)
        {
            SetSilent(true);
        }

        void DoInBackground (OciConnect&)
        {
        }

        void ReturnInForeground ()
        {
            if (!m_drilldown.empty() 
            && m_viewer.GetItemState(m_hFrom, TVIS_EXPANDED) & TVIS_EXPANDED)
            {
                HTREEITEM hChild = m_viewer.GetChildItem(m_hFrom);
  
                while (hChild != NULL)
                {
                    SafeTreeNodePtr node(m_viewer.m_pNodePool, m_viewer.GetItemData(hChild));

                    if (node->GetName() == m_drilldown.rbegin()->c_str()
                    || node->GetLabel(NULL) == m_drilldown.rbegin()->c_str())
                    {
                        m_viewer.Expand(hChild, TVE_EXPAND);
                        m_viewer.Select(hChild, TVGN_CARET);

                        m_drilldown.pop_back();
                        if (!m_drilldown.empty())
                            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_Drilldown(m_viewer, hChild, m_drilldown)));

                        break;
                    }
                    hChild = m_viewer.GetNextSiblingItem(hChild);
                }
            }
        }
    };

void CTreeViewer::ShowObject (const ObjectDescriptor& objDesc, const std::vector<std::string>& drilldown)
{
    m_requestCounter = 0;
    SetCursor(NULL);

    DeleteAllItems();

    if (GetSQLToolsSettings().GetObjectCache()
    && GetSQLToolsSettings().GetObjectViewerUseCache())
        m_pNodePool = SessionCache::GetObjectPool();
    else
        m_pNodePool.reset(new TreeNodePool);

    if (!m_pNodePool.get())
        THROW_STD_EXCEPTION("Access to uninitialized m_pNodePool at CTreeViewer::ShowObject!");

    if (Object* object = m_pNodePool->CreateObject(objDesc))
    {
        std::wstring label = Common::wstr(object->GetLabel(0));

        TV_INSERTSTRUCT  tvstruct;
        memset(&tvstruct, 0, sizeof(tvstruct));
        tvstruct.hParent      = TVI_ROOT;
        tvstruct.hInsertAfter = TVI_LAST;
        tvstruct.item.mask    = TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
        tvstruct.item.pszText = (LPWSTR)label.c_str();
        tvstruct.item.lParam  = reinterpret_cast<LPARAM>(static_cast<TreeNode*>(object));
        tvstruct.item.iImage  =
        tvstruct.item.iSelectedImage = object->GetImage();
        tvstruct.item.cChildren = 1;

        if (HTREEITEM hItem = InsertItem(&tvstruct))
        {
            Expand(hItem, TVE_EXPAND);
            Select(hItem, TVGN_CARET);

            if (!drilldown.empty())
                BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_Drilldown(*this, hItem, drilldown)));
        }
    }
}

	struct BackgroundTask_LoadTreeNodeOnCopy : Task
    {
        CEvent&    m_event;
        vector<TreeNode*> m_nodes, m_cloned;
        enum Status { INIT, LOADED, COPIED } m_status;

        BackgroundTask_LoadTreeNodeOnCopy (CEvent& event, vector<TreeNode*> nodes)
            : Task("Load object description(s)", 0),
            m_event(event),
            m_nodes(nodes),
            m_status(INIT)

        {
            vector<TreeNode*>::iterator it = m_nodes.begin();
            for (; it != m_nodes.end(); ++it)
                if (*it)
                    m_cloned.push_back((*it)->Clone());
        }

        ~BackgroundTask_LoadTreeNodeOnCopy ()
        {
            vector<TreeNode*>::iterator it = m_cloned.begin();
            for (; it != m_cloned.end(); ++it)
                if (*it)
                    delete *it;
        }

        void DoInBackground (OciConnect& connect)
        {
            // cannot use ActivePrimeExecutionOnOff onOff; here bewcause of messaging on drag&drop

            vector<TreeNode*>::iterator it = m_cloned.begin();
            for (; it != m_cloned.end(); ++it)
                if (*it)
                    (*it)->Load(connect);

            //Sleep(1000);

            m_status = LOADED;
            m_event.SetEvent();
        }

        void ReturnInForeground ()
        {
            if (m_status == LOADED)
            {
                vector<TreeNode*>::iterator it = m_nodes.begin();
                vector<TreeNode*>::iterator it2 = m_cloned.begin();

                for (; it != m_nodes.end() && it2 != m_cloned.end(); ++it, ++it2)
                    (*it)->Copy(**it2);

                m_status = COPIED;
            }
        }
    };

/*
DRAG & DROP from "Object Viewer" window 

Depends on <Shift> and <Ctrl>, a drag and drop operation might use one of 3 formats:

1) simple     - typically just a word
2) MULTI-LINE - multi-line text
2) COLUMN     - a word per line

the table of drag & drop formats
---------------------------------------------------------------------
| Object            | Default    | <Shift> pressed | <Ctrl> pressed |
|-------------------------------------------------------------------|
| Column            | simple     | simple          | simple         |
| Constraint        | simple     | MULTI-LINE      | COLUMN         |
| Container*        | simple     | MULTI-LINE      | COLUMN         |
| GrantedPrivelege  | simple     | simple          | simple         |
| Index             | simple     | MULTI-LINE      | COLUMN         |
| Package           | simple     | simple          | simple         |
| PackageBody       | simple     | simple          | simple         |
| PackageProcedure  | MULTI-LINE | MULTI-LINE      | COLUMN         |
| Procedure         | MULTI-LINE | MULTI-LINE      | COLUMN         |
| Sequence          | simple     | simple          | simple         |
| Synonym           | simple     | simple          | simple         |
| TableBase         | simple     | MULTI-LINE      | COLUMN         |
| Trigger           | simple     | simple          | simple         |
| Unknown           | simple     | simple          | simple         |
---------------------------------------------------------------------

* Under "Container" here: Constraints, DependentObjects, DependentOn,
GrantedPriveleges, Indexes, ReferencedBy, References, Triggers


if mutltiple objects are selected for a drag and drop operation
then "simple" format is applied to every item 
and the result can be formatted in COLUMN with <Ctrl> pressed.
<Shift> will be ignored in this case.
*/

const string CTreeViewer::GetSelectedItemsAsText (bool shift /*= false*/, bool ctrl /*= false*/)
{
    if (CObjectViewerWnd* parent = dynamic_cast<CObjectViewerWnd*>(GetParent()))
    {
        CWaitCursor wait;

        int itemCount = GetSelectedCount();

        TextCtx ctx;
        ctx.m_shift = false;
        ctx.m_form = etfDEFAULT;

        if (itemCount > 1)
            ctx.m_form = etfSHORT;
        else if (shift || ctrl)
            ctx.m_form = !ctrl ? etfLINE : etfCOLUMN;

        TextFormatter out(ctx.m_form == etfCOLUMN ? etfCOLUMN : etfLINE);

        bool use_alias = false;
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
        {
            static std::wstring table_alias;
            Common::CInputDlg dlg;
            dlg.m_title  = L"SQLTools";
            dlg.m_prompt = L"&Input table alias:";
            dlg.m_value  = table_alias;

            if (dlg.DoModal() == IDOK)
            {
                use_alias = true;
                table_alias = dlg.m_value;
                ctx.m_table_alias = Common::str(dlg.m_value);
            }
        }

        if (ctx.m_form != etfSHORT)
        {
            vector<TreeNode*> nodes;

            for (HTREEITEM hItem = GetFirstSelectedItem(); hItem != NULL; hItem = GetNextSelectedItem(hItem))
            {
                SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

                if (node && !node->IsComplete())
                    nodes.push_back(node.get());
            }

            m_bkgrSyncEvent.ResetEvent();
            TaskPtr task(new BackgroundTask_LoadTreeNodeOnCopy(m_bkgrSyncEvent, nodes));

            BkgdRequestQueue::Get().Push(task);
            if (WaitForSingleObject(m_bkgrSyncEvent, 3000) == WAIT_OBJECT_0)
                task->ReturnInForeground();
            else
                AfxMessageBox(L"Timeout error while loading object(s).\nPlease try again.", MB_ICONERROR | MB_OK); 

        }

        for (HTREEITEM hItem = GetFirstSelectedItem(); hItem != NULL; hItem = GetNextSelectedItem(hItem))
        {
            SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

            if (node)
            {
                node->GetTextToCopy(ctx);
                out.Put(ctx.m_text);
            }
        }

        return out.GetResult();
    }
    return string();
}

void CTreeViewer::EvOnConnect ()
{
}

void CTreeViewer::EvOnDisconnect ()
{
    m_requestCounter = 0;
    DeleteAllItems();
    m_pNodePool.reset(0);
}

	struct BackgroundTask_LoadTreeNodeOnExpand : Task
    {
        CTreeViewer&    m_viewer;
        HTREEITEM       m_hItem;
        TreeNode*       m_cloned;

        BackgroundTask_LoadTreeNodeOnExpand (CTreeViewer& viewer, HTREEITEM hItem)
            : Task("Load object description", 0),
            m_viewer(viewer),
            m_hItem(hItem),
            m_cloned(0)

        {
            m_viewer.ShowWaitCursor(true);

            SafeTreeNodePtr node(m_viewer.m_pNodePool, m_viewer.GetItemData(m_hItem));

            if (node)
                m_cloned = node->Clone();
        }

        ~BackgroundTask_LoadTreeNodeOnExpand ()
        {
            if (m_cloned)
                delete m_cloned;
        }

        void DoInBackground (OciConnect& connect)
        {
            ActivePrimeExecutionOnOff onOff;

            if (m_cloned)
                m_cloned->Load(connect);

            //Sleep(1000);
        }

        void ReturnInForeground ()
        {
            m_viewer.ShowWaitCursor(false);

            SafeTreeNodePtr node(m_viewer.m_pNodePool, m_viewer.GetItemData(m_hItem));

            if (node && m_cloned && m_cloned->IsComplete())
            {
                node->Copy(*m_cloned);
                m_viewer.Expand(m_hItem, TVE_EXPAND);
            }
            else if (node)
            {
                // remove (Loading...)
                m_viewer.SetItemText(m_hItem, Common::wstr(node->GetLabel(0) + "\t(error)").c_str());
            }
        }

        void ReturnErrorInForeground ()         
        {
            m_viewer.ShowWaitCursor(false);
        }
    };


void CTreeViewer::ExpandItem (HTREEITEM hItem)
{
    if (!GetChildItem(hItem))
    {
        SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

        if (node)
        {
            std::wstring label1;
            bool expandFirstChild = false;
            {
                HTREEITEM hParent = GetParentItem(hItem);
                SafeTreeNodePtr parent(m_pNodePool, (hParent ? GetItemData(hParent) : 0));
                label1 = Common::wstr(node->GetLabel(parent.get())); // label might be modified on load
                if (!hParent && node->GetType() == "SYNONYM")
                    expandFirstChild = true;
            }

            if (node->IsComplete())
            {
                TV_ITEM item;
                item.hItem     = hItem;
                item.mask      = TVIF_CHILDREN | TVIF_TEXT;
                item.pszText   = (LPWSTR)label1.c_str();
                item.cChildren = node->IsComplete() ? node->GetChildrenCount() : 1;
                SetItem(&item);

                TV_INSERTSTRUCT  tvstruct;
                tvstruct.hParent      = hItem;
                tvstruct.hInsertAfter = TVI_LAST;
                tvstruct.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

                int children = node->GetChildrenCount();
                for (int i = 0; i < children; i++)
                {
                    TreeNode* child = node->GetChild(i);

                    std::wstring label2 = Common::wstr(child->GetLabel(node.get()));
                    tvstruct.item.pszText = (LPWSTR)label2.c_str();
                    tvstruct.item.lParam  = reinterpret_cast<LPARAM>(static_cast<TreeNode*>(child));
                    tvstruct.item.iImage  =
                    tvstruct.item.iSelectedImage = child->GetImage();
                    tvstruct.item.cChildren = child->IsComplete() ? child->GetChildrenCount() : 1;
                    InsertItem(&tvstruct);
                }

                if (expandFirstChild)
                    Expand(GetChildItem(hItem), TVE_EXPAND);

            }
            else
            {
                TV_ITEM item;
                item.hItem     = hItem;
                item.mask      = TVIF_CHILDREN | TVIF_TEXT;
                label1         +=  L"\t(loading...)";
                item.pszText   = (LPWSTR)label1.c_str();
                item.cChildren = node->IsComplete() ? node->GetChildrenCount() : 1;
                SetItem(&item);

                BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_LoadTreeNodeOnExpand(*this, hItem)));
            }
        } // if (node)
    } // if (!GetChildItem(hItem))
}

void CTreeViewer::SortChildren (ObjectTree::Order order)
{
	if (GetSelectedCount() == 1)
    {
        if (HTREEITEM hItem = GetFirstSelectedItem())
        {
            SafeTreeNodePtr parent(m_pNodePool, GetItemData(hItem));

            if (parent)
            {
                if (parent->CanSortChildren())
                {
                    parent->SortChildren(order);

                    // deleting all children and expanding the item again is the most reliable approch
                    // the drowback is loosing all children state - assume it is ok for now
                    while (HTREEITEM hChild = GetNextItem(hItem, TVGN_CHILD))
                        DeleteItem(hChild);

                    ExpandItem(hItem);

                    return; // the only successful exit
                }
            }
        }
    }
    MessageBeep((UINT)-1);
}

void CTreeViewer::BuildContexMenu (CMenu& menu, HTREEITEM hItem)
{
    if (!hItem) return;

    UINT selectedCount = GetSelectedCount();
    SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));
    Object* object = dynamic_cast<Object*>(node.get());
    bool canLoadDDL = (selectedCount == 1) && (object && object->IsTrueObject()) ? true : false;


    menu.AppendMenu(node ? MF_STRING : MF_STRING|MF_GRAYED, ID_EDIT_COPY, L"&Copy");
    menu.AppendMenu(canLoadDDL ? MF_STRING : MF_STRING|MF_GRAYED, ID_SQL_LOAD, L"&Load DDL");
    menu.AppendMenu(canLoadDDL ? MF_STRING : MF_STRING|MF_GRAYED, ID_SQL_LOAD_WITH_DBMS_METADATA, L"Load DDL with Dbms_&Metadata");
    menu.AppendMenu(MF_SEPARATOR);

    Table* table    = object ? dynamic_cast<Table*>(object) : 0;
    View*  view     = object ? dynamic_cast<View*>(object) : 0;

    if (Column*  column = node ? dynamic_cast<Column*>(node.get()) : 0)
    {
        table = dynamic_cast<Table*>(column->GetParent());
        view  = dynamic_cast<View*>(column->GetParent());
    }

    if (Partition* partition = object ? dynamic_cast<Partition*>(object) : 0)
    {
        if (Partition* parent = dynamic_cast<Partition*>(partition->GetParent()))
            partition = parent;

        if (Partitions* partitions = dynamic_cast<Partitions*>(partition->GetParent()))
            if (Partitioning* partitioning = dynamic_cast<Partitioning*>(partitions->GetParent()))
                table = dynamic_cast<Table*>(partitioning->GetParent());
    }

    if (table || view)
    {
        menu.AppendMenu(MF_STRING, ID_SQL_QUICK_QUERY, L"&Query");
        menu.AppendMenu(MF_STRING, ID_SQL_QUICK_COUNT, L"&Count query");
        menu.AppendMenu(MF_SEPARATOR);
    }

    if (node && node->CanSortChildren())
    {
        CMenu srtMenu;
        srtMenu.CreateMenu();

        srtMenu.AppendMenu(MF_STRING,  ID_OV_SORT_NATURAL,      L"&Natural");
        srtMenu.AppendMenu(MF_STRING,  ID_OV_SORT_ALPHABETICAL, L"&Alphabetical");

        switch(node->GetSortOrder())
        {
        case Natural:
            srtMenu.CheckMenuRadioItem(ID_OV_SORT_NATURAL, ID_OV_SORT_ALPHABETICAL, ID_OV_SORT_NATURAL, MF_BYCOMMAND);
            break;
        case Alphabetical:
            srtMenu.CheckMenuRadioItem(ID_OV_SORT_NATURAL, ID_OV_SORT_ALPHABETICAL, ID_OV_SORT_ALPHABETICAL, MF_BYCOMMAND);
            break;
        }
        menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(srtMenu.m_hMenu), L"&Sort order");
        srtMenu.Detach();
    }
    else
    {
        menu.AppendMenu(MF_STRING|MF_GRAYED, ID_OV_SORT_NATURAL, L"&Sort order");
    }

    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu((selectedCount == 1) && (node && node->IsRefreshable()) ? MF_STRING : MF_STRING|MF_GRAYED, ID_OV_REFRESH, L"&Refresh");
    menu.AppendMenu(MF_STRING, ID_OV_PURGE_CACHE, L"&Purge Cache");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_OV_SETTING, L"&Settings...");

    Common::GUICommandDictionary::AddAccelDescriptionToMenu(menu.m_hMenu);
}

void CTreeViewer::ShowWaitCursor (bool show)
{
    if (show) 
        ++m_requestCounter;
    else
        --m_requestCounter;
    
    CPoint pt;
    if (GetCursorPos(&pt))
    {
        CRect rc;
        GetClientRect(rc);
        ClientToScreen(rc);

        if (rc.PtInRect(pt))
            SendMessage(WM_SETCURSOR, (WPARAM)m_hWnd, MAKELONG(HTCLIENT, 0));
    }
}


LRESULT CTreeViewer::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CTreeCtrlEx::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

BOOL CTreeViewer::PreTranslateMessage (MSG* pMsg)
{
	if (m_accelTable
    && TranslateAccelerator(m_hWnd, m_accelTable, pMsg))
        return TRUE;

    return CTreeCtrlEx::PreTranslateMessage(pMsg);
}

struct CTreeViewerDataSource : COleDataSource
    {
        CTreeViewer* m_pTree;

        CTreeViewerDataSource (CTreeViewer* pTree)
        {
            m_pTree = pTree;
            DelayRenderData(CF_TEXT);
            DelayRenderData(CF_PRIVATEFIRST);
        }

        virtual BOOL OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
        {
            CWaitCursor wait;

            string text;

            switch (lpFormatEtc->cfFormat)
            {
            case CF_TEXT:
                text = m_pTree->GetSelectedItemsAsText(
                        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0,
                        (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
                    );
                break;
            case CF_PRIVATEFIRST:
                text = "{code}";
                break;
            }

            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, text.size() + 1);

            if (!text.empty())
            {
                if (hGlobal)
                {
                    char* buff = (char*)GlobalLock(hGlobal);
                    if (buff)
                    {
                        memcpy(buff, text.c_str(), text.size() + 1);
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

/////////////////////////////////////////////////////////////////////////////
// CTreeViewer message handlers

void CTreeViewer::OnBeginDrag (NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    CTreeViewerDataSource(this).DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_LINK);

    *pResult = 0;
}

void CTreeViewer::OnLButtonDblClk (UINT nFlags, CPoint point)
{
    try { EXCEPTION_FRAME;

        if (!GetSQLToolsSettings().GetObjectViewerCopyPasteOnDoubleClick()) 
        {
            CTreeCtrlEx::OnLButtonDblClk(nFlags, point);
            return;
        }

        UINT nHitFlags = 0;
        if (HTREEITEM hItem = HitTest(point, &nHitFlags))
            if ((nHitFlags & TVHT_ONITEM) || (!(nHitFlags & TVHT_ONITEMBUTTON) && (GetStyle() & TVS_FULLROWSELECT)))
                if (CObjectViewerWnd* parent = dynamic_cast<CObjectViewerWnd*>(GetParent()))
                {
                    SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

                    if (node)
                    {
                        OnEditCopy();

                        if (CPLSWorksheetDoc* document = CPLSWorksheetDoc::GetActiveDoc())
                        {
                            COEditorView* editor = document->GetEditorView();
                            editor->SendMessage(WM_COMMAND, ID_EDIT_PASTE);
                            editor->SetFocus();
                        }
                    }
                }
    }
    _DEFAULT_HANDLER_
}

void CTreeViewer::OnTvnItemexpanding (NMHDR *pNMHDR, LRESULT *pResult)
{
    try { EXCEPTION_FRAME;

        NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

        if (pNMTreeView->action == TVE_EXPAND)
            ExpandItem(pNMTreeView->itemNew.hItem);
    }
    _DEFAULT_HANDLER_

   *pResult = 0;
}

void CTreeViewer::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
    SetFocus();

    UINT uFlags;
    HTREEITEM hItem = HitTest(point, &uFlags);

    if (hItem && !((uFlags & TVHT_ONITEM) || (GetStyle() & TVS_FULLROWSELECT)))
        hItem = 0;

	if (hItem 
    && !(GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED))
    {
		ClearSelection();
		SelectItem(hItem);
    }

    CMenu menu;
    menu.CreatePopupMenu();

    BuildContexMenu(menu, hItem);

    ClientToScreen(&point);
    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

    //CTreeCtrlEx::OnRButtonDown(nFlags, point);
}

void CTreeViewer::OnNMCustomdraw (NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = CDRF_DODEFAULT;
    
    if (m_classicTreeLook) return;

    try { EXCEPTION_FRAME;
        
        LPNMTVCUSTOMDRAW pNMTVCD = reinterpret_cast<LPNMTVCUSTOMDRAW>(pNMHDR);

        switch (pNMTVCD->nmcd.dwDrawStage)
        {
        case CDDS_PREPAINT:
            *pResult = CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;
            break;
        case CDDS_ITEMPREPAINT:
            {
                SafeTreeNodePtr node(m_pNodePool, pNMTVCD->nmcd.lItemlParam);

                if (node)
                {
                    HTREEITEM hItem = (HTREEITEM)(pNMTVCD->nmcd.dwItemSpec);
                    CString text = GetItemText(hItem);
                    if (text.Find('\t') != -1)
                    {
                        pNMTVCD->clrText = pNMTVCD->clrTextBk;
                        *pResult = CDRF_NOTIFYPOSTPAINT;
                    }
                }
            }
            break;
        case CDDS_ITEMPOSTPAINT:
            {
                SafeTreeNodePtr node(m_pNodePool, pNMTVCD->nmcd.lItemlParam);

                if (node)
                {
                    bool hasFocus = (::GetFocus() == m_hWnd) ? true : false;
                    bool isSeleted = pNMTVCD->nmcd.uItemState & CDIS_SELECTED ? true: false;
                    HTREEITEM hItem = (HTREEITEM)(pNMTVCD->nmcd.dwItemSpec);
                        
                    if (isSeleted)
                        ::SetBkColor(pNMTVCD->nmcd.hdc, ::GetSysColor(hasFocus ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
                    else
                        ::SetBkColor(pNMTVCD->nmcd.hdc, ::GetSysColor(COLOR_WINDOW));

                    ::SetTextColor(pNMTVCD->nmcd.hdc, ::GetSysColor(COLOR_GRAYTEXT));

                    CString text = GetItemText(hItem);
                    int tabPos = text.Find('\t');

                    int datatypeOffset;
                    INT nTabStopPositions[3];
                        
                    TEXTMETRIC tm = { 0 };

                    if (GetTextMetrics(pNMTVCD->nmcd.hdc, &tm))
                    {
                        int length = 0;
                        
                        if (Object* parent = dynamic_cast<Object*>(node->GetParent()))
                            length = parent->GetChildFirstTabPos();

                        if (!length) 
                            length = tabPos != -1 ? tabPos : text.GetLength();

                        datatypeOffset = tm.tmAveCharWidth * (((length/3)*3) + 1);
                        nTabStopPositions[0] = tm.tmAveCharWidth * 3;
                        nTabStopPositions[1] = tm.tmAveCharWidth * 6;
                        nTabStopPositions[2] = tm.tmAveCharWidth * 9;
                    }
                    else
                    {
                        datatypeOffset = 120;
                        nTabStopPositions[0] = 20;
                        nTabStopPositions[1] = 40;
                        nTabStopPositions[2] = 60;
                    }

                    RECT rc;
                    GetItemRect(hItem, &rc, TRUE);

                    ::TabbedTextOut(pNMTVCD->nmcd.hdc, rc.left + 2, rc.top + 1, text,  text.GetLength(),
                        sizeof(nTabStopPositions)/sizeof(nTabStopPositions[0]), nTabStopPositions, rc.left + datatypeOffset);

                    if (tabPos != -1) 
                    {
                        LPWSTR buff = text.GetBuffer();
                        buff[tabPos] = 0;
                        text.ReleaseBuffer();
                    }

                    if (isSeleted)
                        ::SetTextColor(pNMTVCD->nmcd.hdc, ::GetSysColor(hasFocus ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
                    else
                        ::SetTextColor(pNMTVCD->nmcd.hdc, ::GetSysColor(COLOR_WINDOWTEXT));

                    ::TabbedTextOut(pNMTVCD->nmcd.hdc, rc.left + 2, rc.top + 1, text,  text.GetLength(),
                        sizeof(nTabStopPositions)/sizeof(nTabStopPositions[0]), nTabStopPositions, rc.left + datatypeOffset);
                }
            }
            break;
        }
    }
    _COMMON_DEFAULT_HANDLER_
}

BOOL CTreeViewer::OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT && m_requestCounter > 0)
    {
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        return TRUE;
    }

    return CTreeCtrlEx::OnSetCursor(pWnd, nHitTest, message);
}

void CTreeViewer::OnEditCopy ()
{
    if (OpenClipboard())
    {
        if (EmptyClipboard())
        {
            string text = GetSelectedItemsAsText(
                        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0,
                        (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
                );
            Common::AppSetClipboardText(text.c_str(), text.length(), CF_TEXT);
            Common::AppSetClipboardText("{code}", sizeof("{code}"), CF_PRIVATEFIRST);
        }
        CloseClipboard();
    }
}

void CTreeViewer::OnSettings()
{
    SQLToolsSettings settings = GetSQLToolsSettings();

    CObjectViewerPage page(settings);
    page.m_psp.pszTitle = L"Object Viewer / List";
    page.m_psp.dwFlags |= PSP_USETITLE;

    static UINT gStarPage = 0;
    Common::CPropertySheetMem sheet(L"Settings", gStarPage);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;
    sheet.AddPage(&page);

    int retVal = sheet.DoModal();

    if (retVal == IDOK)
    {
        GetSQLToolsSettingsForUpdate() = settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
    }
}

void CTreeViewer::OnRefresh ()
{
	if (GetSelectedCount() == 1)
    {
        if (HTREEITEM hItem = GetFirstSelectedItem())
        {
            SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

            if (node)
            {
                if (node->IsRefreshable())
                {
                    // TODO: check if the object is displayed as another node
                    Expand(hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
                    node->Clear(); // TODO: add garbage collector, call it OnOk
                    ExpandItem(hItem);
                    return; // the only successful exit
                }
            }
        }
    }

    MessageBeep((UINT)-1);
}

#include "ObjectViewerWnd.h"

void CTreeViewer::OnPurgeCache ()
{
    DeleteAllItems();

    if (m_pNodePool.get()) 
        m_pNodePool->DestroyAll();

    if (CObjectViewerWnd* parent = dynamic_cast<CObjectViewerWnd*>(GetParent()))
        parent->OnInput_SelChange();
}

void CTreeViewer::OnSortNatural ()
{
    SortChildren(ObjectTree::Natural);
}

void CTreeViewer::OnSortAlphabetical ()
{
    SortChildren(ObjectTree::Alphabetical);
}

void CTreeViewer::OnSqlLoad ()
{
    // do that only for "true" db objects
    // and columns and package procedures
    // reuse alredy open DDL

	if (GetSelectedCount() == 1)
    {
        if (HTREEITEM hItem = GetFirstSelectedItem())
        {
            SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

            if (node)
            {
                if (Object* object = dynamic_cast<Object*>(node.get()))
                {
                    if (object->IsTrueObject())
                    {
                        CPLSWorksheetDoc::LoadDDL(object->GetSchema(), object->GetName(), object->GetType());
                        return; // the only successful exit
                    }
                }
            }
        }
    }

    MessageBeep((UINT)-1);
}

void CTreeViewer::OnSqlLoadWithDbmsMetadata ()
{
    // do that only for "true" db objects
    // and columns and package procedures
    // reuse alredy open DDL

	if (GetSelectedCount() == 1)
    {
        if (HTREEITEM hItem = GetFirstSelectedItem())
        {
            SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

            if (node)
            {
                if (Object* object = dynamic_cast<Object*>(node.get()))
                {
                    if (object->IsTrueObject())
                    {
                        CPLSWorksheetDoc::LoadDDLWithDbmsMetadata(object->GetSchema(), object->GetName(), object->GetType());
                        return; // the only successful exit
                    }
                }
            }
        }
    }

    MessageBeep((UINT)-1);
}


void CTreeViewer::OnSqlQuickQuery()
{
    set<Object*>    parents; // table or view
    vector<Column*> columns;
    set<Partition*> partitions;

    for (HTREEITEM hItem = GetFirstSelectedItem(); hItem != NULL; hItem = GetNextSelectedItem(hItem))
    {
        SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

        if (node)
        {
            if (Column* column = dynamic_cast<Column*>(node.get()))
            {
                columns.push_back(column);
                parents.insert(dynamic_cast<Object*>(column->GetParent()));
            }
            else if (Partition* partition = dynamic_cast<Partition*>(node.get()))
            {
                partitions.insert(partition);

                if (Partition* parent = dynamic_cast<Partition*>(partition->GetParent()))
                    partition = parent;

                if (Partitions* partitions2 = dynamic_cast<Partitions*>(partition->GetParent()))
                    if (Partitioning* partitioning = dynamic_cast<Partitioning*>(partitions2->GetParent()))
                        parents.insert(dynamic_cast<Table*>(partitioning->GetParent()));
            }
            else
            {
                Object* object = dynamic_cast<Table*>(node.get());
                if (!object) object = dynamic_cast<View*>(node.get());
                parents.insert(object);
            }
        }
    }

    if (parents.size() == 1 && partitions.size() <= 1)
    {
        Object* parent = *parents.begin();
        if (parent
        && (dynamic_cast<Table*>(parent) || dynamic_cast<View*>(parent))
        )
        {
            string sql = !GetSQLToolsSettings().GetQueryInLowerCase() ? "SELECT " : "select ";
                
            if (columns.empty())
            {
                sql += '*';
            }
            else
            {
                for (vector<Column*>::const_iterator it = columns.begin(); it != columns.end(); ++it)
                {
                    sql += SQLUtilities::GetSafeDatabaseObjectName((*it)->GetName());
                    sql += ", ";
                }
                sql.resize(sql.size() - 2);
            }

            string user, schema;
            ConnectionTasks::GetCurrentUserAndSchema(user, schema, 1000); //wait for 1 sec, if not ready then ignore

            sql += !GetSQLToolsSettings().GetQueryInLowerCase() ? " FROM " : " from ";
            string from;
            if (schema != parent->GetSchema())
            {
                from += SQLUtilities::GetSafeDatabaseObjectName(parent->GetSchema());
                from += '.';
            }
            from += SQLUtilities::GetSafeDatabaseObjectName(parent->GetName());
            
            if (partitions.size() == 1)
            {
                Partition* partition = *partitions.begin();
                from += dynamic_cast<Partition*>(partition->GetParent()) ? " SUBPARTITION (" : " PARTITION (";
                from += SQLUtilities::GetSafeDatabaseObjectName(partition->GetName());
                from += ")";
            }
            
            sql += from;
            sql += ';';

            CPLSWorksheetDoc::DoSqlQuery(
                sql, 
                parent->GetSchema() + '.' + parent->GetName() + ".sql",
                ("Quick query from " + from),
                GetSQLToolsSettings().GetObjectViewerQueryInActiveWindow()
            );
            return; // success
        }
    }

    MessageBeep((UINT)-1);
}

void CTreeViewer::OnSqlQuickCount()
{
    set<Object*>    parents; // table or view
    vector<Column*> columns;
    set<Partition*> partitions;

    for (HTREEITEM hItem = GetFirstSelectedItem(); hItem != NULL; hItem = GetNextSelectedItem(hItem))
    {
        SafeTreeNodePtr node(m_pNodePool, GetItemData(hItem));

        if (node)
        {
            if (Column* column = dynamic_cast<Column*>(node.get()))
            {
                columns.push_back(column);
                parents.insert(dynamic_cast<Object*>(column->GetParent()));
            }
            else if (Partition* partition = dynamic_cast<Partition*>(node.get()))
            {
                partitions.insert(partition);

                if (Partition* parent = dynamic_cast<Partition*>(partition->GetParent()))
                    partition = parent;

                if (Partitions* partitions2 = dynamic_cast<Partitions*>(partition->GetParent()))
                    if (Partitioning* partitioning = dynamic_cast<Partitioning*>(partitions2->GetParent()))
                        parents.insert(dynamic_cast<Table*>(partitioning->GetParent()));
            }
            else
            {
                Object* object = dynamic_cast<Table*>(node.get());
                if (!object) object = dynamic_cast<View*>(node.get());
                parents.insert(object);
            }
        }
    }

    if (parents.size() == 1)
    {
        Object* parent = *parents.begin();
        if (parent
        && (dynamic_cast<Table*>(parent) || dynamic_cast<View*>(parent))
        )
        {
            string sql = !GetSQLToolsSettings().GetQueryInLowerCase() ? "SELECT " : "select ";
                
            string list;
            for (vector<Column*>::const_iterator it = columns.begin(); it != columns.end(); ++it)
            {
                list += SQLUtilities::GetSafeDatabaseObjectName((*it)->GetName());
                list += ", ";
            }
            sql += list; 
            sql += !GetSQLToolsSettings().GetQueryInLowerCase() ? "Count(*)" : "count(*)";

            string user, schema;
            ConnectionTasks::GetCurrentUserAndSchema(user, schema, 1000); //wait for 1 sec, if not ready then ignore

            sql += !GetSQLToolsSettings().GetQueryInLowerCase() ? " FROM " : " from ";
            string from;
            if (schema != parent->GetSchema())
            {
                from += SQLUtilities::GetSafeDatabaseObjectName(parent->GetSchema());
                from += '.';
            }
            from += SQLUtilities::GetSafeDatabaseObjectName(parent->GetName());
            
            if (partitions.size() == 1)
            {
                Partition* partition = *partitions.begin();
                from += dynamic_cast<Partition*>(partition->GetParent()) ? " SUBPARTITION (" : " PARTITION (";
                from += SQLUtilities::GetSafeDatabaseObjectName(partition->GetName());
                from += ")";
            }
            
            sql += from;

            if (list.size() > 2) list.resize(list.size() - 2);
            if (!list.empty())
            {
                sql += !GetSQLToolsSettings().GetQueryInLowerCase() ? " GROUP BY " : " group by ";
                sql += list;
                char buff[64];
                _snprintf(buff, sizeof(buff)-1, 
                    !GetSQLToolsSettings().GetQueryInLowerCase() ? " ORDER BY %d DESC" : " order by %d desc", columns.size()+1);
                sql += buff;
            }

            sql += ';';

            CPLSWorksheetDoc::DoSqlQuery(
                sql, 
                parent->GetSchema() + '.' + parent->GetName() + ".sql",
                ("Quick query from " + from),
                GetSQLToolsSettings().GetObjectViewerQueryInActiveWindow()
            );
            return; // success
        }
    }

    MessageBeep((UINT)-1);
}
