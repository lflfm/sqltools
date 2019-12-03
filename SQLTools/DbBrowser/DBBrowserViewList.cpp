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
#include "DbBrowser\DBBrowserViewList.h"
#include "OCI8/BCursor.h"
#include "COMMON\AppGlobal.h"
#include "COMMON\StrHelpers.h"
#include "SQLWorksheet\SqlDocCreater.h"
#include "SQLWorksheet\SQLWorksheetDoc.h"
#include "SQLWorksheet\2PaneSplitter.h"
#include "SQLUtilities.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "ConnectionTasks.h"

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserViewList::DBBrowserViewList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserViewList::~DBBrowserViewList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserViewList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
    ON_COMMAND(ID_SQL_QUICK_QUERY, OnQuery)
END_MESSAGE_MAP()

    const int cn_owner         = 0;
    const int cn_view_name     = 1;
    const int cn_text_length   = 2;   
    const int cn_created       = 3;
    const int cn_last_ddl_time = 4;
    const int cn_status        = 5;  

    static const char* csz_sttm =
        "SELECT <RULE> "
            "v.owner,"        
            "v.view_name,"
            "v.text_length,"
            "o.created,"    
            "o.last_ddl_time,"    
            "o.status"
    " FROM sys.all_objects o, sys.all_views v"
    " WHERE o.owner = :owner"
    " AND v.owner = :owner"
    " AND o.owner = v.owner"
    " AND object_name = view_name"
    " AND object_type = 'VIEW'";

    struct BackgroundTask_RefreshViewList 
        : BackgroundTask_Refresh_Templ<DBBrowserViewList, ViewCollection>
    {
        BackgroundTask_RefreshViewList(DBBrowserViewList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh ViewList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst << csz_sttm;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    ViewEntry view;

                    view.owner         = cur.ToString(cn_owner        );
                    view.view_name     = cur.ToString(cn_view_name    );
                    view.text_length   = cur.ToInt   (cn_text_length  );
                    view.status        = cur.ToString(cn_status       );

                    cur.GetTime(cn_created      , view.created        );
                    cur.GetTime(cn_last_ddl_time, view.last_ddl_time  );

                    m_data.push_back(view);
                }

            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserViewList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshViewList(*this)));
        SetDirty(false);
    }
}

	struct BackgroundTask_RefreshViewListEntry : Task
    {
        int m_entry;
        ViewEntry m_dataEntry;
        DBBrowserViewList& m_list;
        string m_schema;

        BackgroundTask_RefreshViewListEntry (DBBrowserViewList& list, int entry)
            : Task("Refresh ViewEntry", list.GetParent()),
            m_list(list),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry))
        {
            SetSilent(true);
            m_schema = list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst << csz_sttm;

                OciCursor cur(connect, 1, 196);
                cur.Prepare((subst.GetResultStr() + " and view_name = :name").c_str());
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":name", m_dataEntry.view_name.c_str());

                cur.Execute();
                if (cur.Fetch())
                {
                    m_dataEntry.owner         = cur.ToString(cn_owner        );
                    m_dataEntry.view_name     = cur.ToString(cn_view_name    );
                    m_dataEntry.text_length   = cur.ToInt   (cn_text_length  );
                    m_dataEntry.status        = cur.ToString(cn_status       );

                    cur.GetTime(cn_created      , m_dataEntry.created        );
                    cur.GetTime(cn_last_ddl_time, m_dataEntry.last_ddl_time  );
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            if (!m_list.m_data.empty()) // if refresh immediatly after compile
            {
                m_list.m_data.at(m_entry) = m_dataEntry;
                m_list.OnUpdateEntry(m_entry);
            }
        }
    };

void DBBrowserViewList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshViewListEntry(*this, entry)));
}

void DBBrowserViewList::ApplyQuickFilter (bool valid, bool invalid) 
{
    ListCtrlManager::FilterCollection filter;
    GetFilter(filter);

    for (int i = 0, n = m_dataAdapter.getColCount(); i < n; ++i)
        if (!stricmp(m_dataAdapter.getColHeader(i), "status"))
        {
            const char* val = 0;

            if (valid && invalid)
                val = "";
            else if (valid)
                val = "VALID";
            else if  (invalid)
                val = "INVALID";
            else
                val = "NA";

            if (val != filter.at(i).value
            || ListCtrlManager::EXACT_MATCH != filter.at(i).operation)
            {
                filter.at(i).value = val;
                filter.at(i).operation = ListCtrlManager::EXACT_MATCH;
                SetFilter(filter);
                break;
            }
        }
}

void DBBrowserViewList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE, "&Compile");                      
    pMenu->AppendMenu(MF_STRING|grayed, ID_SQL_QUICK_QUERY, "&Query");                      
    pMenu->AppendMenu(MF_SEPARATOR);
}

void DBBrowserViewList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER VIEW \"<SCHEMA>\".\"<NAME>\" COMPILE");
}

// copy of DBBrowserTableList::OnQuery
void DBBrowserViewList::OnQuery ()
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

