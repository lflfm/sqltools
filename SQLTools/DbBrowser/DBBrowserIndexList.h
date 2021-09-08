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

    struct IndexEntry
    {
        string table_owner;     //HIDDEN
        string owner;           //HIDDEN
        string index_name     ; //"Index"
        string table_name     ; //"Table"
        string index_type     ; //"Type"
        string uniqueness     ; //"Unique"
        string tablespace_name; //"Tablespace"
        string partitioned    ; //
        tm     created        ; //
        tm     last_ddl_time  ; //
        tm     last_analyzed  ; //
        string column_list    ;

        IndexEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<IndexEntry> IndexCollection;

    class IndexListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        enum
        {
            e_index_name      = 0,//"Index"          partitioned
            e_table_name      ,//"Table"             created      
            e_index_type      ,//"Type"              last_ddl_time
            e_uniqueness      ,//"Unique"            last_analyzed
            e_column_list     ,
            e_tablespace_name ,//"Tablespace"
            e_partitioned     ,//"Initial"
            e_created         ,//"Next"
            e_last_ddl_time   ,//"Increase"
            e_last_analyzed   ,//"Max Ext"
            e_count
        };

        const IndexCollection& m_entries;

    public:
        IndexListAdapter (const IndexCollection& entries) : m_entries(entries) {}

        const IndexEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return e_count; }

        virtual Type getColType (int col) const {
            switch (col) {
            case e_index_name      : return String; //string index_name;     
            case e_table_name      : return String; //string table_name;     
            case e_index_type      : return String; //string index_type;     
            case e_uniqueness      : return String; //string uniqueness;     
            case e_tablespace_name : return String; //string tablespace_name;
            case e_partitioned     : return String; //
            case e_created         : return Date;   //
            case e_last_ddl_time   : return Date;   //
            case e_last_analyzed   : return Date;   //
            case e_column_list     : return String;    
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case e_index_name      : return L"Index"; 
            case e_table_name      : return L"Table"; 
            case e_index_type      : return L"Type"; 
            case e_uniqueness      : return L"Unique"; 
            case e_tablespace_name : return L"Tablespace"; 
            case e_partitioned     : return L"Partitioned"; 
            case e_created         : return L"Created"; 
            case e_last_ddl_time   : return L"Modified"; 
            case e_last_analyzed   : return L"Last analyzed"; 
            case e_column_list     : return L"Columns";    
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case e_index_name      : return getStr(data(row).index_name     );
            case e_table_name      : return getStr(data(row).table_name     );   
            case e_index_type      : return getStr(data(row).index_type     );
            case e_uniqueness      : return getStr(data(row).uniqueness     );
            case e_tablespace_name : return getStr(data(row).tablespace_name);  
            case e_partitioned     : return getStr(data(row).partitioned    ); 
            case e_created         : return getStr(data(row).created        );  
            case e_last_ddl_time   : return getStr(data(row).last_ddl_time  );     
            case e_last_analyzed   : return getStr(data(row).last_analyzed  );     
            case e_column_list     : return getStr(data(row).column_list    );     
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case e_index_name      : return comp(data(row1).index_name     , data(row2).index_name     );
            case e_table_name      : return comp(data(row1).table_name     , data(row2).table_name     );
            case e_index_type      : return comp(data(row1).index_type     , data(row2).index_type     );
            case e_uniqueness      : return comp(data(row1).uniqueness     , data(row2).uniqueness     );
            case e_tablespace_name : return comp(data(row1).tablespace_name, data(row2).tablespace_name);
            case e_partitioned     : return comp(data(row1).partitioned    , data(row2).partitioned    );
            case e_created         : return comp(data(row1).created        , data(row2).created        );
            case e_last_ddl_time   : return comp(data(row1).last_ddl_time  , data(row2).last_ddl_time  );
            case e_last_analyzed   : return comp(data(row1).last_analyzed  , data(row2).last_analyzed  );
            case e_column_list     : return comp(data(row1).column_list    , data(row2).column_list    ); 
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_INDEX; }
    };


class DBBrowserIndexList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserIndexList, IndexCollection>;
    friend struct BackgroundTask_RefreshIndexColumnList;
    IndexCollection m_data;
    IndexListAdapter m_dataAdapter;

public:
	DBBrowserIndexList ();
	virtual ~DBBrowserIndexList();

    virtual const char* GetTitle () const       { return "Indexes"; };
    virtual int         GetImageIndex () const  { return IDII_INDEX; }
    virtual const char* GetMonoType () const    { return "INDEX"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).index_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);
    virtual void OnShowInObjectView ();

protected:
	DECLARE_MESSAGE_MAP()
};
