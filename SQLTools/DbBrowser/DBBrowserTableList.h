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


    struct TableEntry
    {
        string owner               ; //HIDDEN
        string table_name          ;
        string table_type          ; 
        string part_key            ;  
        string compression         ;  
        string tablespace_name     ;
        tm     created             ; //"created"       "Created"
        tm     last_ddl_time       ; //"last_ddl_time" "Modified"
        tm     last_analyzed       ;
        __int64 num_rows           ;
        __int64 blocks             ;

        TableEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<TableEntry> TableCollection;

    class TableListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const TableCollection& m_entries;

        enum
        {
            e_table_name            = 0, 
            e_table_type            , 
            e_tablespace_name       , 
            e_compression           ,
            e_created               ,
            e_last_ddl_time         ,
            e_num_rows              ,
            e_blocks                ,
            e_last_analyzed         , 
            e_count
        };

    public:
        TableListAdapter (const TableCollection& entries) : m_entries(entries) {}

        const TableEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return e_count; }

        virtual Type getColType (int col) const {
            switch (col) {
            case e_table_name           : return String; //table_name       
            case e_table_type           : return String; //table_type       
            case e_compression          : return String; //compression         
            case e_tablespace_name      : return Number; //tablespace_name  
            case e_created              : return Date;   //created     
            case e_last_ddl_time        : return Date;   //last_ddl_time
            case e_last_analyzed        : return Date;   //last_analyzed    
            case e_num_rows             : return Number;
            case e_blocks               : return Number;
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case e_table_name           : return L"Name"           ; //table_name       
            case e_table_type           : return L"Type"           ; //table_type       
            case e_compression          : return L"Compression"    ; //compression         
            case e_tablespace_name      : return L"Tablespace"     ; //tablespace_name  
            case e_created              : return L"Created"        ; //created
            case e_last_ddl_time        : return L"Modified"       ; //last_ddl_time
            case e_last_analyzed        : return L"Last Analyzed"  ; //last_analyzed    
            case e_num_rows             : return L"Rows"           ;
            case e_blocks               : return L"Blocks"         ;
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case e_table_name          : return getStr(data(row).table_name          );
            case e_table_type          : return getStr(data(row).table_type          );   
            case e_compression         : return getStr(data(row).compression         );
            case e_tablespace_name     : return getStr(data(row).tablespace_name     );
            case e_created             : return getStr(data(row).created             );
            case e_last_ddl_time       : return getStr(data(row).last_ddl_time       );
            case e_last_analyzed       : return getStr(data(row).last_analyzed       );  
            case e_num_rows            : return getStr(data(row).num_rows            );
            case e_blocks              : return getStr(data(row).blocks              );
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case e_table_name          : return comp(data(row1).table_name          , data(row2).table_name          );
            case e_table_type          : return comp(data(row1).table_type          , data(row2).table_type          );
            case e_compression         : return comp(data(row1).compression         , data(row2).compression         );
            case e_tablespace_name     : return comp(data(row1).tablespace_name     , data(row2).tablespace_name     );
            case e_created             : return comp(data(row1).created             , data(row2).created             );
            case e_last_ddl_time       : return comp(data(row1).last_ddl_time       , data(row2).last_ddl_time       );
            case e_last_analyzed       : return comp(data(row1).last_analyzed       , data(row2).last_analyzed       );
            case e_num_rows            : return comp(data(row1).num_rows            , data(row2).num_rows            );
            case e_blocks              : return comp(data(row1).blocks              , data(row2).blocks              );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_TABLE; }
    };


class DBBrowserTableList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserTableList, TableCollection>;
    friend struct BackgroundTask_Truncate;

    TableCollection  m_data;
    TableListAdapter m_dataAdapter;

public:
	DBBrowserTableList ();
	virtual ~DBBrowserTableList();

    virtual const char* GetTitle () const       { return "Tables"; };
    virtual int         GetImageIndex () const  { return IDII_TABLE; }
    virtual const char* GetMonoType () const    { return "TABLE"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).table_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }
    virtual void MakeDropSql (int entry, std::string& sql) const;

	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()

    afx_msg void OnQuery ();
//    afx_msg void OnDelete ();
    afx_msg void OnTruncate ();
    afx_msg void OnTransformTable ();
};
