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

#ifndef __TREEVIEWER_H__
#define __TREEVIEWER_H__
#pragma once

#include "TreeCtrlEx\TreeCtrlEx.h"

namespace ObjectTree
{
    enum Order;
    struct TreeNode;
    class TreeNodePool;
    struct ObjectDescriptor;

    typedef std::vector<ObjectDescriptor>   ObjectDescriptorArray;
    typedef arg::counted_ptr<ObjectDescriptorArray> ObjectDescriptorArrayPtr;
    typedef arg::counted_ptr<TreeNodePool> TreeNodePoolPtr;
};

class CTreeViewer : public CTreeCtrlEx
{
    friend struct BackgroundTask_LoadTreeNodeOnExpand;
    friend struct BackgroundTask_LoadTreeNodeOnCopy;
    friend struct BackgroundTask_Drilldown;

    HACCEL          m_accelTable;
    CImageList      m_Images;
    ObjectTree::TreeNodePoolPtr m_pNodePool;
    CEvent          m_bkgrSyncEvent;

    bool            m_classicTreeLook;
    int             m_requestCounter;
public:

    CTreeViewer();
    ~CTreeViewer();

    void SetClassicTreeLook (bool);
    void LoadAndSetImageList (UINT nResId);

    void OnInit ();
    string GetTooltipText (HTREEITEM);
    void ShowObject (const ObjectTree::ObjectDescriptor& objDesc, const std::vector<std::string>& drilldown = std::vector<std::string>());
    const string GetSelectedItemsAsText (bool shift = false, bool ctrl = false);
    void EvOnConnect ();
    void EvOnDisconnect ();

protected:
    void ExpandItem (HTREEITEM hItem);
    void SortChildren (ObjectTree::Order);
    void BuildContexMenu (CMenu& menu, HTREEITEM hItem);
public:    
    void ShowWaitCursor (bool show);
protected:
    virtual LRESULT WindowProc (UINT message, WPARAM wParam, LPARAM lParam);
    virtual BOOL PreTranslateMessage (MSG* pMsg);

    DECLARE_MESSAGE_MAP()

    afx_msg void OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDblClk (UINT nFlags, CPoint point);
    afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg BOOL OnSetCursor (CWnd* pWnd, UINT nHitTest, UINT message);

    afx_msg void OnEditCopy ();
    afx_msg void OnSettings ();
    afx_msg void OnRefresh ();
    afx_msg void OnPurgeCache ();
    afx_msg void OnSortNatural ();
    afx_msg void OnSortAlphabetical ();
    afx_msg void OnSqlLoad ();
    afx_msg void OnSqlLoadWithDbmsMetadata ();
    afx_msg void OnSqlQuickQuery();
    afx_msg void OnSqlQuickCount();
};

//{{AFX_INSERT_LOCATION}}
#endif//__TREEVIEWER_H__
