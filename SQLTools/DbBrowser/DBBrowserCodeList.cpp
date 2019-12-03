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

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserCodeList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "ServerBackgroundThread\TaskQueue.h"

using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserCodeList::DBBrowserCodeList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserCodeList::~DBBrowserCodeList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserCodeList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
END_MESSAGE_MAP()

    const int cn_owner         = 0;
    const int cn_object_name   = 1;   
    const int cn_status        = 2;
    const int cn_created       = 3;
    const int cn_last_ddl_time = 4;  

    static const char* csz_sttm =
        "SELECT * FROM ("
        "  SELECT"
        "    owner,"        
        "    object_name,"
        "    status,"
        "    created,"      
        "    last_ddl_time,"
        "    Row_Number() OVER (PARTITION BY owner, object_name, object_type ORDER BY last_ddl_time DESC) rn"
        "  FROM sys.all_objects"
        "    WHERE owner = :owner"
        "      AND object_type = :type"
        ") WHERE rn = 1";

	struct BackgroundTask_RefreshCodeList
        : BackgroundTask_Refresh_Templ<DBBrowserCodeList, CodeCollection>
    {
        BackgroundTask_RefreshCodeList(DBBrowserCodeList& list) 
            : BackgroundTask_Refresh_Templ(list, "Refresh CodeList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_sttm);
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type",  m_monoType.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    CodeEntry code;

                    code.owner       = cur.ToString(cn_owner      );
                    code.object_name = cur.ToString(cn_object_name);
                    code.status      = cur.ToString(cn_status     );

                    cur.GetTime(cn_created,       code.created);
                    cur.GetTime(cn_last_ddl_time, code.last_ddl_time);

                    m_data.push_back(code);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserCodeList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshCodeList(*this)));
        SetDirty(false);
    }
}

	struct BackgroundTask_RefreshCodeListEntry : Task
    {
        int m_entry;
        CodeEntry m_dataEntry;
        DBBrowserCodeList& m_list;
        string m_schema, m_monoType;

        BackgroundTask_RefreshCodeListEntry (DBBrowserCodeList& list, int entry) 
            : Task("Refresh CodeList", list.GetParent()),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry)),
            m_list(list)
        {
            SetSilent(true);
            m_schema   = list.GetSchema();
            m_monoType = list.GetMonoType();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 1, 196);
                cur.Prepare((string(csz_sttm) + " and object_name = :name").c_str());
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type", m_monoType.c_str());
                cur.Bind(":name", m_dataEntry.object_name.c_str());

                cur.Execute();
                if (cur.Fetch())
                {
                    m_dataEntry.owner       = cur.ToString(cn_owner      );
                    m_dataEntry.object_name = cur.ToString(cn_object_name);
                    m_dataEntry.status      = cur.ToString(cn_status     );
                    cur.GetTime(cn_created,       m_dataEntry.created);
                    cur.GetTime(cn_last_ddl_time, m_dataEntry.last_ddl_time);
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

void DBBrowserCodeList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshCodeListEntry(*this, entry)));
}

void DBBrowserCodeList::ApplyQuickFilter (bool valid, bool invalid) 
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

void DBBrowserCodeList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE, "&Compile");                      
    pMenu->AppendMenu(MF_SEPARATOR);
}

void DBBrowserCodeList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER <TYPE> \"<SCHEMA>\".\"<NAME>\" COMPILE");
}

void DBBrowserPackageBodyList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER PACKAGE \"<SCHEMA>\".\"<NAME>\" COMPILE BODY");
}

void DBBrowserJavaList::ExtendContexMenu (CMenu* pMenu)
{
    pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}

