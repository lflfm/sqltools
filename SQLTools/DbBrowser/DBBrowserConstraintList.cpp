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
#include "DbBrowser\DBBrowserConstraintList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "SQLUtilities.h" // for SQLUtilities::GetSafeDatabaseObjectName

using std::map;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserConstraintList::DBBrowserConstraintList(ConstraintListAdapter& adapter, const char* constraint_type)
: DbBrowserList(adapter, true),
m_constraint_type(constraint_type)
{
}

DBBrowserConstraintList::~DBBrowserConstraintList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserConstraintList, DbBrowserList)
    ON_COMMAND(ID_DS_ENABLE,  OnEnable)
    ON_COMMAND(ID_DS_DISABLE, OnDisable)
END_MESSAGE_MAP()

    const int cn_owner              = 0;
    const int cn_table_name         = 1;   
    const int cn_constraint_name    = 2;   
    const int cn_status             = 3;
    const int cn_deferrable         = 4;
    const int cn_deferred           = 5;  
    const int cn_search_condition   = 6;
    const int cn_r_owner            = 7;
    const int cn_r_constraint_name  = 8;
    const int cn_delete_rule        = 9;

    static const char* csz_sttm =
        "SELECT <RULE>"
        " owner, table_name, constraint_name, status,"
        " <DEFERRABLE>,"
        " <DEFERRED>,"
        " search_condition,"
        " decode(owner, r_owner, null, r_owner) r_owner, r_constraint_name,"
        " decode(delete_rule,'NO ACTION', null, delete_rule) delete_rule"
        " FROM sys.all_constraints"
        " WHERE owner = :owner"
        " AND constraint_type = :type"
        " <NOT_RECYCLED>";

	struct BackgroundTask_RefreshConstraintList 
        : BackgroundTask_Refresh_Templ<DBBrowserConstraintList, ConstraintCollection>
    {
        string m_constraint_type;

        BackgroundTask_RefreshConstraintList (DBBrowserConstraintList& list) 
            : BackgroundTask_Refresh_Templ(list, ("Refreshing ConstraintList " + list.GetConstraintType()).c_str())
        {
            m_constraint_type = list.GetConstraintType();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 50, 196);

                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();

                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst.AddPair("<DEFERRABLE>", (servVer > OCI8::esvServer73X) ? "decode(deferrable,'DEFERRABLE','Y')" : "NULL");
                subst.AddPair("<DEFERRED>", (servVer > OCI8::esvServer73X) ? "decode(deferred,'DEFERRED','Y')" : "NULL");
                subst.AddPair("<NOT_RECYCLED>", (servVer >= OCI8::esvServer10X) ? "AND (owner, table_name) NOT IN (SELECT user, object_name FROM user_recyclebin)" : "");
                subst << csz_sttm;

                cur.Prepare(subst.GetResult());

                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type",  m_constraint_type.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    ConstraintEntry code;

                    code.owner            = cur.ToString(cn_owner            );
                    code.constraint_name  = cur.ToString(cn_constraint_name  );
                    code.table_name       = cur.ToString(cn_table_name       );
                    code.deferrable       = cur.ToString(cn_deferrable       );
                    code.deferred         = cur.ToString(cn_deferred         );
                    code.status           = cur.ToString(cn_status           );
                    code.search_condition = cur.ToString(cn_search_condition );
                    code.r_owner          = cur.ToString(cn_r_owner          );
                    code.r_constraint_name= cur.ToString(cn_r_constraint_name);
                    code.delete_rule      = cur.ToString(cn_delete_rule      );

                    if (!code.search_condition.empty()
                    && *code.search_condition.begin() == '\"')
                    {
                        unsigned int second_quote_pos = code.search_condition.find('\"', 1);
                        if (second_quote_pos != std::string::npos
                        && code.search_condition.substr(second_quote_pos) == "\" IS NOT NULL")
                            continue;
                    }
                    Common::trim_symmetric(code.search_condition);

                    m_data.push_back(code);
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
        cn_con_const_name = 0, 
        cn_con_column_name, 
        cn_con_column_position
    };

    static const char* csz_index_col_sttm =
        "SELECT constraint_name, column_name, position FROM sys.all_cons_columns"
        " WHERE owner = :owner"
          " ORDER BY constraint_name, position";

    struct BackgroundTask_RefreshConstraintColumnList : Task
    {
        DBBrowserConstraintList& m_list;
        map<string, string> m_conColLists;
        string m_schema;

        BackgroundTask_RefreshConstraintColumnList(DBBrowserConstraintList& list)
            : Task("Refresh ConstraintList.columns", list.GetParent()),
            m_list(list)
        {
            m_schema = m_list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_index_col_sttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    string const_name = cur.ToString(cn_con_const_name);
                    string column_name = SQLUtilities::GetSafeDatabaseObjectName(cur.ToString(cn_con_column_name));

                    map<string, string>::iterator it = m_conColLists.find(const_name);
                    if (it == m_conColLists.end())
                        m_conColLists.insert(make_pair(const_name, column_name));
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

            ConstraintCollection::iterator it = m_list.m_data.begin();
            for (; it != m_list.m_data.end(); ++it)
            {
                map<string, string>::iterator it2 = m_conColLists.find(it->constraint_name);
                if (it2 != m_conColLists.end())
                    it->column_list = it2->second;
            }

            m_list.Invalidate();
            //m_list.Common::CManagedListCtrl::Refresh(true);
        }
    };

void DBBrowserConstraintList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshConstraintList(*this)));
        if (m_constraint_type != "C")
            BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshConstraintColumnList(*this)));
        SetDirty(false);
    }
}


	struct BackgroundTask_RefreshConstraintListEntry : Task
    {
        int m_entry;
        ConstraintEntry m_dataEntry;
        DBBrowserConstraintList& m_list;
        string m_schema, m_constraint_type;

        BackgroundTask_RefreshConstraintListEntry(DBBrowserConstraintList& list, int entry) 
            : Task("Refreshing ConstraintListEntry", list.GetParent()),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry)),
            m_list(list)
        {
            SetSilent(true);
            m_schema   = list.GetSchema();
            m_constraint_type = list.m_constraint_type;
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 1, 196);

                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst.AddPair("<DEFERRABLE>", (servVer > OCI8::esvServer73X) ? "decode(deferrable,'DEFERRABLE','Y')" : "NULL");
                subst.AddPair("<DEFERRED>", (servVer > OCI8::esvServer73X) ? "decode(deferred,'DEFERRED','Y')" : "NULL");
                subst.AddPair("<NOT_RECYCLED>", (servVer >= OCI8::esvServer10X) ? "AND (owner, table_name) NOT IN (SELECT user, object_name FROM user_recyclebin)" : "");
                subst << csz_sttm;
                subst << " and constraint_name = :name";

                cur.Prepare(subst.GetResult());

                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type",  m_constraint_type.c_str());
                cur.Bind(":name",  m_dataEntry.constraint_name.c_str());

                cur.Execute();
                if (cur.Fetch())
                {
                    m_dataEntry.owner            = cur.ToString(cn_owner            );
                    m_dataEntry.constraint_name  = cur.ToString(cn_constraint_name  );
                    m_dataEntry.table_name       = cur.ToString(cn_table_name       );
                    m_dataEntry.deferrable       = cur.ToString(cn_deferrable       );
                    m_dataEntry.deferred         = cur.ToString(cn_deferred         );
                    m_dataEntry.status           = cur.ToString(cn_status           );
                    m_dataEntry.search_condition = cur.ToString(cn_search_condition );
                    m_dataEntry.r_owner          = cur.ToString(cn_r_owner          );
                    m_dataEntry.r_constraint_name= cur.ToString(cn_r_constraint_name);
                    m_dataEntry.delete_rule      = cur.ToString(cn_delete_rule      );
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            if (!m_list.m_data.empty()) // if refresh immediatly after enable/disable
            {
                m_list.m_data.at(m_entry) = m_dataEntry;
                m_list.OnUpdateEntry(m_entry);
            }
        }
    };

void DBBrowserConstraintList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshConstraintListEntry(*this, entry)));
}

void DBBrowserConstraintList::MakeDropSql (int entry, std::string& sql) const
{
    Common::Substitutor subst;
    subst.AddPair("<SCHEMA>", GetSchema());
    subst.AddPair("<NAME>", GetObjectName(entry));
    subst.AddPair("<TABLE_NAME>", GetTableName(entry));
    subst <<  ("ALTER TABLE \"<SCHEMA>\".\"<TABLE_NAME>\" DROP CONSTRAINT \"<NAME>\"");
    sql = subst.GetResult();
}

void DBBrowserConstraintList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_ENABLE,  "&Enable");                      
    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_DISABLE, "&Disable");                      
    pMenu->AppendMenu(MF_SEPARATOR);

    //pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}

void DBBrowserConstraintList::OnEnable ()
{
    DoSqlForSel("Enable selection", "ALTER TABLE \"<SCHEMA>\".\"<TABLE_NAME>\" ENABLE CONSTRAINT \"<NAME>\"");
}

void DBBrowserConstraintList::OnDisable ()
{
    DoSqlForSel("Disable selection", "ALTER TABLE \"<SCHEMA>\".\"<TABLE_NAME>\" DISABLE CONSTRAINT \"<NAME>\"");
}

void DBBrowserConstraintList::OnShowInObjectView ()
{
    std::vector<unsigned int> data;
    GetSelEntries(data);

    if (!data.empty())
    {
        vector<string> drilldown;
        drilldown.push_back(m_data.at(data.front()).constraint_name);
        drilldown.push_back("Constraints");

        DoShowInObjectView(m_data.at(data.front()).owner, m_data.at(data.front()).table_name, "TABLE", drilldown);
    }
}

