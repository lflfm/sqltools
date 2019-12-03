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

// 25.12.2004 (ak) R1091240 - Find Object/Object Viewer improvements
// 09.01.2005 (ak) replaced CComboBoxEx with CComboBox because of unpredictable CComboBoxEx

#pragma once

#include "TreeViewer.h"
#include <arg_shared.h>
#include "CppTooltip/PPTooltip.h"
#include <OpenEditor/OESettings.h>

namespace ObjectTree
{
    enum Order;
    class TreeNodePool;
    struct ObjectDescriptor;
    typedef std::vector<ObjectDescriptor>   ObjectDescriptorArray;
};

    typedef arg::counted_ptr<ObjectTree::ObjectDescriptorArray> ObjectDescriptorArrayPtr;
    typedef arg::counted_ptr<ObjectTree::TreeNodePool> TreeNodePoolPtr;

class CObjectViewerWnd : public CDialog, protected OpenEditor::SettingsSubscriber
{
    friend class CTreeViewer;
    friend struct BackgroundTask_FindObjectsForViewer;

    std::string m_input;
    CComboBox   m_inputBox;
    CTreeViewer m_treeViewer;
	CPPToolTip  m_tooltip;
    //CImageList  m_Images;
    HACCEL      m_accelTable;
    bool        m_selChanged;
    bool        m_infoTip;

    ObjectDescriptorArrayPtr m_pInputBoxContent;

// Dialog Data
	enum { IDD = IDD_OBJ_INFO_TREE };

public:
	CObjectViewerWnd();
	virtual ~CObjectViewerWnd();

    BOOL Create (CWnd* pParentWnd);

    void ShowObject (const std::string&);
    void ShowObject (const ObjectTree::ObjectDescriptor&, const std::vector<std::string>& = std::vector<std::string>());

    CTreeCtrl& GetTreeCtrl ()               { return m_treeViewer; }
    void LoadAndSetImageList (UINT nResId)  { m_treeViewer.LoadAndSetImageList(nResId); }

protected:
    void setQuestionIcon ();
    void DoDataExchange (CDataExchange*);
    virtual BOOL OnInitDialog ();
    void OnOK ();
    void OnCancel ();

public:
    void EvOnConnect ();
    void EvOnDisconnect ();

    // SettingsSubscriber
    virtual void OnSettingsChanged ();

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnInput_SelChange ();
    afx_msg void OnInput_CloseUp ();
	afx_msg void OnNotifyDisplayTooltip (NMHDR * pNMHDR, LRESULT * result);

    virtual BOOL PreTranslateMessage (MSG* pMsg);
    virtual LRESULT WindowProc (UINT message, WPARAM wParam, LPARAM lParam);

    static void CALLBACK OnFindObject (void*, const string& input, const string& error, std::vector<ObjectTree::ObjectDescriptor>&);
};
