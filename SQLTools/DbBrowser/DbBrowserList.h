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

#pragma once

// 24.09.2007 some improvements taken from sqltools++ by Randolf Geist

#include "COMMON\ManagedListCtrl.h"

    enum IDB_DB_OBJ_IMAGE_INDEX
    {
        IDII_FUNCTION       = 0 ,
        IDII_PROCEDURE      = 1 ,
        IDII_PACKAGE        = 2 ,
        IDII_PACKAGE_BODY   = 3 ,
        IDII_TRIGGER        = 4 ,
        IDII_VIEW           = 5 ,
        IDII_TABLE          = 6 ,
        IDII_SEQUENCE       = 7 ,
        IDII_SYNONYM        = 8 ,
        IDII_GRANTEE        = 9 ,
        IDII_CLUSTER        = 10,
        IDII_DBLINK         = 11,
        IDII_SNAPSHOT       = 12,
        IDII_SNAPSHOT_LOG   = 13,
        IDII_TYPE           = 14,
        IDII_TYPE_BODY      = 15,
        IDII_INDEX          = 16,
        IDII_CHK_CONSTRAINT = 17,
        IDII_PK_CONSTRAINT  = 18,
        IDII_UK_CONSTRAINT  = 19,
        IDII_FK_CONSTRAINT  = 20,
        IDII_RECYCLEBIN     = 21,
        IDII_INVALID_OBJECT = 22,
        IDII_JAVA           = 23,
    };

class DbBrowserList : public Common::CManagedListCtrl
{
    friend struct BackgroundTask_ListSqlBatch;
    bool m_dirty, m_useStateImage, m_busy;
    std::string m_schema;
	static HACCEL m_accelTable;

    static CImageList m_images, m_stateImages;

protected:
    virtual unsigned int GetTotalEntryCount () const = 0;
    void GetSelEntries (std::vector<unsigned int>&) const;
    bool IsSelectionEmpty () const;
    // Parameter "sttm" might constains placeholders <SCHEMA>, <NAME> , <TYPE>
    void DoSqlForSel (const char* taskName, const char* sttm, bool refresh = true);
    virtual const char* refineDoSqlForSel (const char* sttm, unsigned int /*dataInx*/) { return sttm; }

    void doLoad (bool bAsOne);

public:
    DbBrowserList (Common::ListCtrlDataProvider&, bool useStateImage = false);
	virtual ~DbBrowserList();

    virtual const char* GetTitle () const       = 0;
    virtual int         GetImageIndex () const  = 0;
    virtual const char* GetMonoType () const  = 0;
    virtual const char* GetObjectType (unsigned int /*dataInx*/) const { return GetMonoType(); };
    virtual const char* GetObjectName (unsigned int dataInx) const  = 0;
    // for constraints only
    virtual const char* GetTableName (unsigned int /*dataInx*/) const  { return ""; } 

    void SetSchema (const char* schema)         { m_schema = schema; SetDirty(); }
    const char* GetSchema () const              { return m_schema.c_str(); }
    bool IsDirty () const                       { return m_dirty; }
    void SetDirty (bool dirty = true)           { m_dirty = dirty; }
    bool IsBusy () const                        { return m_busy; }
    void SetBusy (bool busy = true); 

    virtual void Refresh      (bool force = false)       = 0;
    virtual void RefreshEntry (int entry)                = 0;
    virtual void ClearData ()                            = 0;
    
    virtual void ApplyQuickFilter (bool /*valid*/, bool /*invalid*/) {};
    virtual bool IsQuickFilterSupported () const                     { return false; }

    virtual void MarkAsDeleted (int entry) = 0;
    virtual void MakeDropSql (int entry, std::string& sql) const;
    virtual void DoDrop (int entry);

    virtual UINT GetDefaultContextCommand (bool bAltPressed) const { return bAltPressed ? ID_SQL_DESCRIBE : ID_SQL_LOAD; }
	virtual void BuildContexMenu (CMenu* pMenu);
	virtual void ExtendContexMenu (CMenu* pMenu) = 0;

    void GetListSelectionAsText (CString& text);
    void DoShowInObjectView(const string& schema, const string& name, const string& type, const std::vector<std::string>& drilldown = std::vector<std::string>());


    using CListCtrl::OnInitMenuPopup;

protected:
	DECLARE_MESSAGE_MAP()
public:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnShowAsList();
    afx_msg void OnShowAsReport();
    afx_msg void OnSelectAll();
    afx_msg void OnRefresh();
    afx_msg void OnSettings();
    afx_msg void OnLoad();
    afx_msg void OnLoadWithDbmsMetadata();
    afx_msg void OnLoadAsOne();
    afx_msg void OnDrop();
    afx_msg void OnFilter();
    afx_msg void OnCopy();
    virtual afx_msg void OnShowInObjectView();
    afx_msg void OnTabTitles();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};


    template<class List, class ListData>
	struct BackgroundTask_Refresh_Templ : Task
    {
        ListData m_data;
        List&    m_list;
        string   m_schema, m_monoType;

        BackgroundTask_Refresh_Templ(List& list, const char* taskName) 
            : Task(taskName, list.GetParent()),
            m_list(list)
        {
            // access to m_list only in the main thread!
            m_schema   = m_list.GetSchema();
            m_monoType = m_list.GetMonoType();
            m_list.EnableWindow(FALSE);
        }

        //void DoInBackground (OciConnect& connect);

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            std::swap(m_list.m_data, m_data);

            m_list.Common::CManagedListCtrl::Refresh(true);
            m_list.SetDirty(false);
            m_list.EnableWindow(TRUE);
        }
    };

