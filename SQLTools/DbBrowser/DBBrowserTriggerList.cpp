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
#include "DbBrowser\DBBrowserTriggerList.h"
#include "OCI8/BCursor.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "COMMON\StrHelpers.h"

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserTriggerList::DBBrowserTriggerList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserTriggerList::~DBBrowserTriggerList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserTriggerList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
    ON_COMMAND(ID_DS_ENABLE,  OnEnable)
    ON_COMMAND(ID_DS_DISABLE, OnDisable)
END_MESSAGE_MAP()

    const int cn_owner            = 0;
    const int cn_trigger_name     = 1;
    const int cn_enabled          = 2;
    const int cn_table_owner      = 3;
    const int cn_table_name       = 4;
    const int cn_trigger_type     = 5;
    const int cn_triggering_event = 6;
    const int cn_when_clause      = 7;
    const int cn_created          = 8;
    const int cn_last_ddl_time    = 9;
    const int cn_status           = 10;
    
    static const char* csz_sttm =
    "SELECT <RULE> "
    " t.owner,"
    " t.trigger_name,"
    " t.status enabled,"
    " t.table_owner,"
    " t.table_name,"
    " t.trigger_type,"
    " t.triggering_event,"
    " t.when_clause,"
    " o.created,"
    " o.last_ddl_time,"
    " o.status "
    " FROM sys.all_objects o, sys.all_triggers t"
    " WHERE o.owner(+) = :owner"
    " AND t.owner = :owner"
    " AND o.owner(+) = t.owner"
    " AND object_name(+) = trigger_name"
    " AND object_type(+) = 'TRIGGER'";

	struct BackgroundTask_RefreshTriggerList 
        : BackgroundTask_Refresh_Templ<DBBrowserTriggerList, TriggerCollection>
    {
        BackgroundTask_RefreshTriggerList (DBBrowserTriggerList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh TriggerList")
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
                    TriggerEntry trigger;

                    trigger.owner            = cur.ToString(cn_owner           );
                    trigger.trigger_name     = cur.ToString(cn_trigger_name    );
                    trigger.enabled          = cur.ToString(cn_enabled         );
                    trigger.table_owner      = cur.ToString(cn_table_owner     );
                    trigger.table_name       = cur.ToString(cn_table_name      );
                    trigger.trigger_type     = cur.ToString(cn_trigger_type    );
                    trigger.triggering_event = cur.ToString(cn_triggering_event);
                    trigger.when_clause      = cur.ToString(cn_when_clause     );
                    trigger.status           = cur.ToString(cn_status          );
                                     
                    cur.GetTime(cn_created      , trigger.created        );
                    cur.GetTime(cn_last_ddl_time, trigger.last_ddl_time  );

                    m_data.push_back(trigger);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };


void DBBrowserTriggerList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshTriggerList(*this)));
        SetDirty(false);
    }
}

	struct BackgroundTask_RefreshTriggerListEntry : Task
    {
        int m_entry;
        TriggerEntry m_dataEntry;
        DBBrowserTriggerList& m_list;
        string m_schema;

        BackgroundTask_RefreshTriggerListEntry (DBBrowserTriggerList& list, int entry)
            : Task("Refresh TriggerEntry", list.GetParent()),
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
                cur.Prepare((subst.GetResultStr() + " and trigger_name = :name").c_str());
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":name", m_dataEntry.trigger_name.c_str());

                cur.Execute();
                if (cur.Fetch())
                {
                    m_dataEntry.owner            = cur.ToString(cn_owner           );
                    m_dataEntry.trigger_name     = cur.ToString(cn_trigger_name    );
                    m_dataEntry.enabled          = cur.ToString(cn_enabled         );
                    m_dataEntry.table_owner      = cur.ToString(cn_table_owner     );
                    m_dataEntry.table_name       = cur.ToString(cn_table_name      );
                    m_dataEntry.trigger_type     = cur.ToString(cn_trigger_type    );
                    m_dataEntry.triggering_event = cur.ToString(cn_triggering_event);
                    m_dataEntry.when_clause      = cur.ToString(cn_when_clause     );
                    m_dataEntry.status           = cur.ToString(cn_status          );
                                     
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

void DBBrowserTriggerList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshTriggerListEntry(*this, entry)));
}

void DBBrowserTriggerList::ApplyQuickFilter (bool valid, bool invalid) 
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

void DBBrowserTriggerList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE, "&Compile");                      
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_ENABLE,  "&Enable");                      
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_DISABLE, "&Disable");                      
    pMenu->AppendMenu(MF_SEPARATOR);

    //pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}

void DBBrowserTriggerList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER TRIGGER \"<SCHEMA>\".\"<NAME>\" COMPILE");
}

void DBBrowserTriggerList::OnEnable ()
{
    DoSqlForSel("Enable selection", "ALTER TRIGGER \"<SCHEMA>\".\"<NAME>\" ENABLE");
}

void DBBrowserTriggerList::OnDisable ()
{
    DoSqlForSel("Disable selection", "ALTER TRIGGER \"<SCHEMA>\".\"<NAME>\" DISABLE");
}
