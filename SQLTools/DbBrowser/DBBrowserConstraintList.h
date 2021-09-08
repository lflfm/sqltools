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

#include "DbBrowser\DbBrowserList.h"
#include "COMMON\ListCtrlDataProvider.h"


    struct ConstraintEntry
    {
        // HIDDEN
        string owner            ; // "owner" 
        // ALL
        string constraint_name  ; // "Constraint"
        string table_name       ; // "Table"
        string deferrable       ; // "Deferrable"
        string deferred         ; // "Deferred"
        string status           ; // "Status"
        // CHECK
        string search_condition ; // "Condition" 
        // FK
        string r_owner          ; // "Ref Owner", 
        string r_constraint_name; // "Ref Constraint",
        string delete_rule      ; // "Delete Rule",
        string column_list      ;

        ConstraintEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<ConstraintEntry> ConstraintCollection;

    class ConstraintListAdapter : public Common::ListCtrlDataProvider, protected Common::ListCtrlDataProviderHelper
    {
        const ConstraintCollection& m_entries;
        int m_imageIndex;

    public:
        ConstraintListAdapter (const ConstraintCollection& entries) : m_entries(entries), m_imageIndex(-1) {}

        void SetImageIndex (int imageIndex) { m_imageIndex = imageIndex; }

        const ConstraintEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 5; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // "Constraint"
            case 1: return String; // "Table"
            case 2: return String; // "Columns"
            case 3: return String; // "Deferrable"
            case 4: return String; // "Deferred"
            case 5: return String; // "Status"
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Constraint" ;
            case 1: return L"Table"      ;
            case 2: return L"Columns"    ;
            case 3: return L"Deferrable" ;
            case 4: return L"Deferred"   ;
            case 5: return L"Status"     ;
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).constraint_name);
            case 1: return getStr(data(row).table_name     );
            case 2: return getStr(data(row).column_list    );
            case 3: return getStr(data(row).deferrable     );
            case 4: return getStr(data(row).deferred       );   
            case 5: return getStr(data(row).status         );   
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).constraint_name, data(row2).constraint_name);
            case 1: return comp(data(row1).table_name     , data(row2).table_name     );
            case 2: return comp(data(row1).column_list    , data(row2).column_list    );
            case 3: return comp(data(row1).deferrable     , data(row2).deferrable     );
            case 4: return comp(data(row1).deferred       , data(row2).deferred       );
            case 5: return comp(data(row1).status         , data(row2).status         );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return m_imageIndex; }
        virtual int getStateImageIndex (int row) const { return !stricmp(data(row).status.c_str(), "ENABLED") ? -1 : 2; }
    };

    class ChkConstraintListAdapter : public ConstraintListAdapter
    {
    public:
        ChkConstraintListAdapter (const ConstraintCollection& entries) : ConstraintListAdapter(entries) {}

        virtual int getColCount () const { return 6; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // "Constraint"
            case 1: return String; // "Table"
            case 2: return String; // "Condition" 
            case 3: return String; // "Deferrable"
            case 4: return String; // "Deferred"
            case 5: return String; // "Status"
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Constraint" ;
            case 1: return L"Table"      ;
            case 2: return L"Condition"  ;
            case 3: return L"Deferrable" ;
            case 4: return L"Deferred"   ;
            case 5: return L"Status"     ;
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).constraint_name );
            case 1: return getStr(data(row).table_name      );
            case 2: return getStr(data(row).search_condition);
            case 3: return getStr(data(row).deferrable      );
            case 4: return getStr(data(row).deferred        );   
            case 5: return getStr(data(row).status          );   
            }
            return L"Unknown";
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).constraint_name , data(row2).constraint_name );
            case 1: return comp(data(row1).table_name      , data(row2).table_name      );
            case 2: return comp(data(row1).search_condition, data(row2).search_condition);
            case 3: return comp(data(row1).deferrable      , data(row2).deferrable      );
            case 4: return comp(data(row1).deferred        , data(row2).deferred        );
            case 5: return comp(data(row1).status          , data(row2).status          );
            }
            return 0;
        }
    };

    class FkConstraintListAdapter : public ConstraintListAdapter
    {
    public:
        FkConstraintListAdapter (const ConstraintCollection& entries) : ConstraintListAdapter(entries) {}

        virtual int getColCount () const { return 8; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // "Constraint"
            case 1: return String; // "Table"
            case 2: return String; // "Columns"
            case 3: return String; // "Ref Owner",  
            case 4: return String; // "Ref Constraint", 
            case 5: return String; // "Delete Rule", 
            case 6: return String; // "Deferrable"
            case 7: return String; // "Deferred"
            case 8: return String; // "Status"
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Constraint"    ;
            case 1: return L"Table"         ;
            case 2: return L"Columns"       ;
            case 3: return L"Ref Owner"     ; 
            case 4: return L"Ref Constraint";
            case 5: return L"Delete Rule"   ;
            case 6: return L"Deferrable"    ;
            case 7: return L"Deferred"      ;
            case 8: return L"Status"        ;
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).constraint_name  );
            case 1: return getStr(data(row).table_name       );
            case 2: return getStr(data(row).column_list      );
            case 3: return getStr(data(row).r_owner          );
            case 4: return getStr(data(row).r_constraint_name);
            case 5: return getStr(data(row).delete_rule      );
            case 6: return getStr(data(row).deferrable       );
            case 7: return getStr(data(row).deferred         );   
            case 8: return getStr(data(row).status           );   
            }
            return L"Unknown";
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).constraint_name  , data(row2).constraint_name  );
            case 1: return comp(data(row1).table_name       , data(row2).table_name       );
            case 2: return comp(data(row1).column_list      , data(row2).column_list      );
            case 3: return comp(data(row1).r_owner          , data(row2).r_owner          );
            case 4: return comp(data(row1).r_constraint_name, data(row2).r_constraint_name);
            case 5: return comp(data(row1).delete_rule      , data(row2).delete_rule      );    
            case 6: return comp(data(row1).deferrable       , data(row2).deferrable       );
            case 7: return comp(data(row1).deferred         , data(row2).deferred         );
            case 8: return comp(data(row1).status           , data(row2).status           );
            }
            return 0;
        }
    };

class DBBrowserConstraintList : public DbBrowserList
{
protected:
    friend struct BackgroundTask_Refresh_Templ<DBBrowserConstraintList, ConstraintCollection>;
    friend struct BackgroundTask_RefreshConstraintColumnList;
    friend struct BackgroundTask_RefreshConstraintListEntry;
    ConstraintCollection m_data;
    const string m_constraint_type;

public:
	DBBrowserConstraintList (ConstraintListAdapter& adapter, const char* constraint_type);
	virtual ~DBBrowserConstraintList();

    virtual unsigned int GetTotalEntryCount () const               { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const     { return m_data.at(inx).constraint_name.c_str(); };
    virtual const char* GetTableName (unsigned int inx) const      { return m_data.at(inx).table_name.c_str(); };
    virtual const char* GetMonoType () const                       { return "CONSTRAINT"; }
                                            
    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData    ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }
    virtual void MakeDropSql (int entry, std::string& sql) const;

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

    const string& GetConstraintType () const { return  m_constraint_type; }
protected:
	DECLARE_MESSAGE_MAP()
    void OnEnable();
    void OnDisable();
    virtual void OnShowInObjectView ();
};


class DBBrowserChkConstraintList : public DBBrowserConstraintList
{
    ChkConstraintListAdapter m_dataAdapter;
public:
    DBBrowserChkConstraintList () 
        : DBBrowserConstraintList(m_dataAdapter, "C"),
        m_dataAdapter(m_data)
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "CHKs"; };
    virtual int         GetImageIndex () const  { return IDII_CHK_CONSTRAINT; }
};

class DBBrowserPkConstraintList : public DBBrowserConstraintList
{
    ConstraintListAdapter m_dataAdapter;
public:
    DBBrowserPkConstraintList () 
        : DBBrowserConstraintList(m_dataAdapter, "P"),
        m_dataAdapter(m_data)
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "PKs"; };
    virtual int         GetImageIndex () const  { return IDII_PK_CONSTRAINT; }
};

class DBBrowserUkConstraintList : public DBBrowserConstraintList
{
    ConstraintListAdapter m_dataAdapter;
public:
    DBBrowserUkConstraintList () 
        : DBBrowserConstraintList(m_dataAdapter, "U"),
        m_dataAdapter(m_data)
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "UKs"; };
    virtual int         GetImageIndex () const  { return IDII_UK_CONSTRAINT; }
};

class DBBrowserFkConstraintList : public DBBrowserConstraintList
{
    FkConstraintListAdapter m_dataAdapter;
public:
    DBBrowserFkConstraintList () 
        : DBBrowserConstraintList(m_dataAdapter, "R"),
        m_dataAdapter(m_data)
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "FKs"; };
    virtual int         GetImageIndex () const  { return IDII_FK_CONSTRAINT; }
};

