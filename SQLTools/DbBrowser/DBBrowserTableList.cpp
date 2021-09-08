/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015, 2018 Aleksey Kochetov

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

// 2018-01-01 improvement, RULE hint is used for databases prior to 10g only

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserTableList.h"
#include "DbBrowser\DeleteTableDataDlg.h"
#include "DbBrowser\TableTruncateOptionsDlg.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "MetaDict\TextOutput.h"
#include "SQLWorksheet\SqlDocCreater.h"
#include "SQLWorksheet\SQLWorksheetDoc.h"
#include "SQLWorksheet\2PaneSplitter.h"
#include "SQLUtilities.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "Tools\TableTransformer.h"
#include "ConnectionTasks.h"
#include <ActivePrimeExecutionNote.h>

using namespace OraMetaDict;
using namespace ServerBackgroundThread;


DBBrowserTableList::DBBrowserTableList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserTableList::~DBBrowserTableList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserTableList, DbBrowserList)
    ON_COMMAND(ID_SQL_QUICK_QUERY, OnQuery)
//    ON_COMMAND(ID_DS_DELETE, OnDelete)
    ON_COMMAND(ID_DS_TRUNCATE, OnTruncate)
    ON_COMMAND(ID_DS_TRANSFORM_TABLE, OnTransformTable)
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////////////////////////

    const int cn_owner                =  0;
    const int cn_table_name           =  1;
    const int cn_compression          =  2;
    const int cn_tablespace_name      =  3;
    const int cn_last_analyzed        =  4;
    const int cn_created              =  5;
    const int cn_last_ddl_time        =  6;
    const int cn_partitioning_type    =  7;
    const int cn_subpartitioning_type =  8;
    const int cn_def_tablespace_name  =  9; 
    const int cn_def_compression      = 10;
    const int cn_duration             = 11;
    const int cn_compress_for         = 12;
    const int cn_def_compress_for     = 13;
    const int cn_temporary            = 14;
    const int cn_iot_type             = 15;
    const int cn_partitioned          = 16;
    const int cn_num_rows             = 17;
    const int cn_blocks               = 18;

    static const char* csz_table_sttm =
        "SELECT <RULE> "
            "t1.owner,"
            "t1.table_name,"
            "<COMPRESSION> compression,"
            "t1.tablespace_name,"
            "t1.last_analyzed,"
            "t2.created,"
            "t2.last_ddl_time,"
            "t3.partitioning_type," 
            "t3.subpartitioning_type," 
            "t3.def_tablespace_name," 
            "<DEF_COMPRESSION> def_compression,"
            "t1.duration,"
            "<COMPRESS_FOR> compress_for,"
            "<DEF_COMPRESS_FOR> def_compress_for,"
            "t1.temporary,"
            "t1.iot_type,"
            "t1.partitioned,"
            "t1.num_rows,"
            "t1.blocks"   
        " FROM sys.all_tables t1, sys.all_objects t2, sys.all_part_tables t3"
        " WHERE t1.owner = :owner"
        " AND t2.owner = :owner AND t2.object_type = 'TABLE'"
        " AND t1.owner = t2.owner AND t1.table_name = t2.object_name"
        " AND t3.owner(+) = :owner"
        " AND t1.owner = t3.owner(+) AND t1.table_name = t3.table_name(+)"
        " <IOT_FILTER>"; // iot_name is null because of overflow tables

    //SELECT * FROM all_part_key_columns;
    //SELECT owner, table_name, partitioning_type, subpartitioning_type, partition_count, def_tablespace_name, def_compression FROM all_part_tables;

    struct BackgroundTask_RefreshTableList 
        : BackgroundTask_Refresh_Templ<DBBrowserTableList, TableCollection>
    {
        BackgroundTask_RefreshTableList(DBBrowserTableList& list) 
            : BackgroundTask_Refresh_Templ(list, "Refresh TableList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();

                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst.AddPair("<COMPRESSION>",      (servVer >= OCI8::esvServer10X) ? "t1.compression" : "NULL");
                subst.AddPair("<COMPRESS_FOR>",     (servVer >= OCI8::esvServer11X) ? "t1.compress_for" : "NULL");
                subst.AddPair("<DEF_COMPRESS_FOR>", (servVer >= OCI8::esvServer11X) ? "t3.def_compress_for" : "NULL");
                subst.AddPair("<DEF_COMPRESSION>",  (servVer >= OCI8::esvServer9X ) ? "t3.def_compression" : "NULL");
                subst.AddPair("<IOT_FILTER>",       (servVer >= OCI8::esvServer81X) ? "AND t1.iot_name IS NULL" : "");
            
                subst << csz_table_sttm;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    TableEntry table;

                    table.owner            = cur.ToString(cn_owner          );
                    table.table_name       = cur.ToString(cn_table_name     );

                    if (cur.ToString(cn_temporary) == "Y")
                    {
                        table.table_type = "TEMP";
                        string duration = cur.ToString(cn_duration);

                        if (!strnicmp(duration.c_str(), "SYS$", 4))
                        {
                            table.table_type += " / ";
                            table.table_type += duration.substr(4);
                        }
                    }
                    else if (cur.ToString(cn_partitioned) == "YES")
                    {
                        table.table_type = "PART";
                        string partitioning_type    = cur.ToString(cn_partitioning_type   );
                        string subpartitioning_type = cur.ToString(cn_subpartitioning_type);

                        if (!cur.IsNull(cn_iot_type))
                        {
                            table.table_type += " by ";
                            table.table_type += cur.ToString(cn_iot_type);
                        }

                        table.table_type += " - ";
                        table.table_type += partitioning_type;

                        if (subpartitioning_type != "NONE")
                        {
                            table.table_type += " / ";
                            table.table_type += subpartitioning_type;
                        }
                    }
                    else
                    {
                        if (!cur.IsNull(cn_iot_type))
                            table.table_type = cur.ToString(cn_iot_type);
                        else
                            table.table_type = ".";
                    }

                    table.compression      = !cur.IsNull(cn_compression    ) ? cur.ToString(cn_compression    ) : cur.ToString(cn_def_compression    );
                    table.tablespace_name  = !cur.IsNull(cn_tablespace_name) ? cur.ToString(cn_tablespace_name) : cur.ToString(cn_def_tablespace_name);
                    
                    cur.GetTime(cn_last_analyzed, table.last_analyzed);
                    cur.GetTime(cn_created,       table.created);
                    cur.GetTime(cn_last_ddl_time, table.last_ddl_time);

                    if (table.compression == "ENABLED")
                    {
                        if (!cur.IsNull(cn_def_compress_for))
                            table.compression = cur.ToString(cn_def_compress_for);
                        else if (!cur.IsNull(cn_compress_for))
                            table.compression = cur.ToString(cn_compress_for);
                        else
                            table.compression = "BASIC";
                    }
                    else if (table.compression == "DISABLED")
                        table.compression = ".";
                    else if (table.compression == "NONE")
                        table.compression = ".";
                    else if (table.compression.empty())
                        table.compression = ".";

                    table.num_rows = cur.ToInt64(cn_num_rows);
                    table.blocks   = cur.ToInt64(cn_blocks  );

                    m_data.push_back(table);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

///////////////////////////////////////////////////////////////////////////////////////////////////

void DBBrowserTableList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshTableList(*this)));
        SetDirty(false);
    }
}

void DBBrowserTableList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserTableList::MakeDropSql (int entry, std::string& sql) const
{
    Common::Substitutor subst;
    subst.AddPair("<SCHEMA>", GetSchema());
    subst.AddPair("<NAME>", GetObjectName(entry));
    subst.AddPair("<TYPE>", GetObjectType(entry));
    subst <<  ("DROP <TYPE> \"<SCHEMA>\".\"<NAME>\" CASCADE CONSTRAINTS");
    sql = subst.GetResult();
}

void DBBrowserTableList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->InsertMenu(1, MF_BYPOSITION|MF_STRING|grayed, ID_SQL_QUICK_QUERY, L"&Query");
    pMenu->InsertMenu(1, MF_BYPOSITION|MF_SEPARATOR);
//    pMenu->AppendMenu(MF_STRING|grayed, ID_SQL_QUICK_QUERY, "&Query");                      
//    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_DELETE,   "Dele&te");                     
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_TRUNCATE, L"Tr&uncate");                   
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_TRANSFORM_TABLE, L"Create Transformation &Script");                   
    pMenu->AppendMenu(MF_SEPARATOR);
}


void DBBrowserTableList::OnQuery ()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (; it != data.end(); ++it)
        {
            string user, schema;
            ConnectionTasks::GetCurrentUserAndSchema(user, schema, 1000); //wait for 1 sec, if not ready then ignore

            string sql = "SELECT * FROM ";
            if (GetSQLToolsSettings().GetQueryInLowerCase())
                Common::to_lower_str(sql.c_str(), sql);

            if (schema != GetSchema())
            {
                sql += SQLUtilities::GetSafeDatabaseObjectName(GetSchema());
                sql += '.';
            }
            sql += SQLUtilities::GetSafeDatabaseObjectName(GetObjectName(*it));
            sql += ';';

            string title = GetSchema();
            title += '.';
            title += GetObjectName(*it);
            title += ".sql";

            CPLSWorksheetDoc::DoSqlQuery(sql, title, sql, GetSQLToolsSettings().GetObjectsListQueryInActiveWindow());
        }
    }
    catch (CUserException*)
    {
        MessageBeep((UINT)-1);
    }
    _DEFAULT_HANDLER_
}

//void DBBrowserTableList::OnDelete ()
//{       
//    CWaitCursor wait;
//
//    try { EXCEPTION_FRAME;
//
//        CDeleteTableDataDlg dlg;
//        int retVal = dlg.DoModal();
 //       SetFocus();
//        if (retVal == IDOK)
//        {
//            if (!dlg.m_commit)
//                DoSqlForSel("DELETE FROM \"<SCHEMA>\".\"<NAME>\"");
//            else
//                DoSqlForSel("BEGIN DELETE FROM \"<SCHEMA>\".\"<NAME>\"; COMMIT; END;");
//            
//            //TODO: implement "commit after N rows"
//
//        }
//    }
//    _DEFAULT_HANDLER_
//}

    struct ConstaintInfo { string owner, table, constraint; bool modified; };

static 
void load_constraint_info (OciConnect& connect, const char* schema, const char* name, vector<ConstaintInfo>& fks)
{
    OciCursor cursor(connect, 
        "SELECT owner, table_name, constraint_name FROM sys.all_constraints"
        " WHERE (r_owner, r_constraint_name) IN"
            "("
            " SELECT owner, constraint_name FROM sys.all_constraints"
                " WHERE owner = :owner"
                " AND table_name = :name"
                " AND constraint_type IN ('P', 'U')"
                " AND status = 'ENABLED'"
            ")"
            " AND constraint_type = 'R'"
            " AND status = 'ENABLED'"
        );

    cursor.Bind("owner", schema);
    cursor.Bind("name", name);

    cursor.Execute();
    while (cursor.Fetch())
    {
        ConstaintInfo fk;
        fk.modified = false;
        cursor.GetString(0, fk.owner);
        cursor.GetString(1, fk.table);
        cursor.GetString(2, fk.constraint);
        fks.push_back(fk);
    }
}

static 
void enable_fk_constraints (OciConnect& connect, vector<ConstaintInfo>& fks, bool enable)
{
    vector<ConstaintInfo>::iterator it = fks.begin();
    for (; it != fks.end(); ++it)
    {
        if (it->modified == enable)
        {
            try { EXCEPTION_FRAME;

                Common::Substitutor subst;
                subst.AddPair("<SCHEMA>", it->owner);
                subst.AddPair("<NAME>", it->constraint);
                subst.AddPair("<TABLE_NAME>", it->table);
                subst.AddPair("<ACTION>", enable ? "ENABLE" : "DISABLE");
                subst <<  ("ALTER TABLE \"<SCHEMA>\".\"<TABLE_NAME>\" <ACTION> CONSTRAINT \"<NAME>\"");
                Global::SetStatusText(subst.GetResult(), TRUE);
                connect.ExecuteStatement(subst.GetResult());
                Global::SetStatusText("", TRUE);
                it->modified = !enable;
            } 
            _DEFAULT_HANDLER_
        }
    }
}

static 
void check_fk_dependencies (OciConnect& connect, const std::set<std::string> tables, const vector<ConstaintInfo>& fks)
{
    bool ignore = false;
    vector<ConstaintInfo>::const_iterator it = fks.begin();
    for (; !ignore && it != fks.end(); ++it)
    {
        if (tables.find(it->table) == tables.end()) // if a table is not in the selected list to
        try { EXCEPTION_FRAME;

            Common::Substitutor subst;
            subst.AddPair("<SCHEMA>", it->owner);
            subst.AddPair("<NAME>", it->constraint);
            subst.AddPair("<TABLE_NAME>", it->table);
            subst <<  "SELECT 1 FROM \"<SCHEMA>\".\"<TABLE_NAME>\" WHERE ROWNUM < 2";
            OciCursor cursor(connect, subst.GetResult(), 1);
            cursor.Execute();
            if (cursor.Fetch())
            {
                subst.ResetResult();
                subst <<  "Table \"<SCHEMA>\".\"<TABLE_NAME>\" has one or more rows."
                    "\nProbably after truncation constraint \"<NAME>\" will be invalid."
                    "\nPress <Abort> to stop, <Retry> to continue this checking"
                    "\nor <Ignore> to skip it for the rest of tables.";
                
                switch (AfxMessageBox(Common::wstr(subst.GetResult()).c_str(), MB_ABORTRETRYIGNORE|MB_ICONWARNING))
                {
                case IDIGNORE: 
                    ignore = true; 
                    break;
                case IDABORT:    
                    AfxThrowUserException();
                }
            }
        } 
        _DEFAULT_HANDLER_
    }
}

    struct TableInfo { string owner, table; };

    struct BackgroundTask_Truncate : Task
    {
        typedef set<string> Tables;
        string m_schema;
        Tables m_tables;
        vector<ConstaintInfo> m_fks;
        bool m_disableFKs, m_checkDependencies, m_keepAllocated;

        BackgroundTask_Truncate (DBBrowserTableList& list, bool disableFKs, bool checkDependencies, bool keepAllocated) 
            : Task("Truncate table(s)", list.GetParent()),
            m_disableFKs(disableFKs), 
            m_checkDependencies(checkDependencies),
            m_keepAllocated(keepAllocated)
        {
            m_schema = list.GetSchema();

            std::vector<unsigned int> data;
            list.GetSelEntries(data);

            std::vector<unsigned int>::const_iterator it = data.begin();
            for (; it != data.end(); ++it)
                m_tables.insert(list.GetObjectName(*it));
        }

        void DoInBackground (OciConnect& connect)
        {
            ActivePrimeExecutionOnOff onOff;

            try {

                if (m_disableFKs || m_checkDependencies)
                {
                    Tables::const_iterator it = m_tables.begin();
                    for (; it != m_tables.end(); ++it)
                        load_constraint_info(connect, m_schema.c_str(), it->c_str(), m_fks);
                }

                if (m_checkDependencies)
                {
                    check_fk_dependencies(connect, m_tables, m_fks);
                }
                
                if (m_disableFKs)
                    enable_fk_constraints(connect, m_fks, false);

                Tables::const_iterator it = m_tables.begin();
                for (; it != m_tables.end(); ++it)
                {
                    Common::Substitutor subst;
                    subst.AddPair("<SCHEMA>", m_schema);
                    subst.AddPair("<NAME>", *it);
                    subst << (
                        m_keepAllocated 
                        ? "TRUNCATE TABLE \"<SCHEMA>\".\"<NAME>\" REUSE STORAGE" 
                        : "TRUNCATE TABLE \"<SCHEMA>\".\"<NAME>\" DROP STORAGE"
                        );

                    connect.ExecuteStatement(subst.GetResult());
                }
                
                if (m_disableFKs)
                    enable_fk_constraints(connect, m_fks, true);
            
            }
            catch (const OciException& x)
            {
                if (m_disableFKs)
                    enable_fk_constraints(connect, m_fks, true);

                SetError(x.what());
            }
            catch (CUserException*)
            {
                // ignore
            }
        }

        void ReturnInForeground ()
        {
        }
    };

void DBBrowserTableList::OnTruncate ()
{
    try { EXCEPTION_FRAME;

        CTableTruncateOptionsDlg dlg;
        int retVal = dlg.DoModal();
        SetFocus();

        if (retVal == IDOK)
            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_Truncate(*this, dlg.m_disableFKs, dlg.m_checkDependencies, dlg.m_keepAllocated)));
    }
    _DEFAULT_HANDLER_
}

void DBBrowserTableList::OnTransformTable ()
{
    try { EXCEPTION_FRAME;

        string schema = GetSchema();
        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (; it != data.end(); ++it)
            CTableTransformer::MakeScript(schema, GetObjectName(*it));
    }
    _DEFAULT_HANDLER_
}

//TODO: add command to show table indexes/columns/constraints/...