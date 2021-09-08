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
#include "DbBrowser\DBBrowserIndexList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "SQLUtilities.h" // for SQLUtilities::GetSafeDatabaseObjectName
#include <ActivePrimeExecutionNote.h>

using namespace OraMetaDict;
using namespace ServerBackgroundThread;


DBBrowserIndexList::DBBrowserIndexList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserIndexList::~DBBrowserIndexList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserIndexList, DbBrowserList)
END_MESSAGE_MAP()

    enum
    {
        cn_table_owner     = 0,
        cn_owner           ,
        cn_index_name      ,  
        cn_table_name      ,
        cn_index_type      ,
        cn_uniqueness      , 
        cn_tablespace_name ,
        cn_partitioned     , 
        cn_created         ,    
        cn_last_ddl_time   ,    
        cn_last_analyzed   ,
    };


    static const char* csz_index_sttm =
        "SELECT <RULE> "
            "t1.table_owner,"
            "t1.owner,"
            "t1.index_name,"
            "t1.table_name,"
            "<INDEX_TYPE>,"
            "t1.uniqueness,"  
            "t1.tablespace_name," 
            "t1.partitioned,"  
            "t2.created,"     
            "t2.last_ddl_time,"     
            "t1.last_analyzed"
        " FROM sys.all_indexes t1, sys.all_objects t2"
        " WHERE t1.owner = :owner"
        " AND t2.owner(+) = :owner AND t2.object_type(+) = 'INDEX'"
        " AND t1.owner = t2.owner(+) AND t1.index_name = t2.object_name(+)"
        " <IOT_FILTER>";

	struct BackgroundTask_RefreshIndexList 
        : BackgroundTask_Refresh_Templ<DBBrowserIndexList, IndexCollection>
    {
        BackgroundTask_RefreshIndexList(DBBrowserIndexList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh IndexList")
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
                subst.AddPair("<INDEX_TYPE>", (servVer > OCI8::esvServer73X) ? "t1.index_type" : "NULL");
                subst.AddPair("<IOT_FILTER>", (servVer > OCI8::esvServer73X) ? "AND t1.index_type != \'IOT - TOP\'" : "");
                subst << csz_index_sttm;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    IndexEntry index;

                    index.table_owner     = cur.ToString(cn_table_owner    );
                    index.owner           = cur.ToString(cn_owner          );
                    index.index_name      = cur.ToString(cn_index_name     );
                    index.table_name      = cur.ToString(cn_table_name     );
                    index.index_type      = cur.ToString(cn_index_type     );
                    index.uniqueness      = cur.ToString(cn_uniqueness     );
                    index.tablespace_name = cur.ToString(cn_tablespace_name);
                    index.partitioned     = cur.ToString(cn_partitioned    );

                    cur.GetTime(cn_last_analyzed, index.last_analyzed);
                    cur.GetTime(cn_created,       index.created);
                    cur.GetTime(cn_last_ddl_time, index.last_ddl_time);

                    m_data.push_back(index);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

    enum
    {
        cn_col_index_name = 0, 
        cn_col_column_name, 
        cn_col_column_position
    };

    static const char* csz_index_col_sttm =
        "SELECT index_name, column_name, column_position FROM sys.all_ind_columns"
        " WHERE index_owner = :owner"
          " ORDER BY index_name, column_position";

	struct BackgroundTask_RefreshIndexColumnList : Task
    {
        DBBrowserIndexList& m_list;
        map<string, string> m_indColLists;
        string m_schema;

        BackgroundTask_RefreshIndexColumnList(DBBrowserIndexList& list)
            : Task("Refresh IndexList.columns", list.GetParent()),
            m_list(list)
        {
            m_schema = m_list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_index_col_sttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    string index_name = cur.ToString(cn_col_index_name);
                    string column_name = SQLUtilities::GetSafeDatabaseObjectName(cur.ToString(cn_col_column_name));

                    map<string, string>::iterator it = m_indColLists.find(index_name);
                    if (it == m_indColLists.end())
                        m_indColLists.insert(make_pair(index_name, column_name));
                    else
                        it->second += ", " + column_name;
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            IndexCollection::iterator it = m_list.m_data.begin();
            for (; it != m_list.m_data.end(); ++it)
            {
                map<string, string>::iterator it2 = m_indColLists.find(it->index_name);
                if (it2 != m_indColLists.end())
                    it->column_list = it2->second;
            }

            m_list.Invalidate();
            //m_list.Common::CManagedListCtrl::Refresh(true);
        }
    };

void DBBrowserIndexList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshIndexList(*this)));
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshIndexColumnList(*this)));
        SetDirty(false);
    }
}

void DBBrowserIndexList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserIndexList::ExtendContexMenu (CMenu* pMenu)
{
    pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}


void DBBrowserIndexList::OnShowInObjectView ()
{
    std::vector<unsigned int> data;
    GetSelEntries(data);

    if (!data.empty())
    {
        vector<string> drilldown;
        drilldown.push_back(m_data.at(data.front()).index_name);
        drilldown.push_back("Indexes");

        DoShowInObjectView(m_data.at(data.front()).table_owner, m_data.at(data.front()).table_name, "TABLE", drilldown);
    }
}

