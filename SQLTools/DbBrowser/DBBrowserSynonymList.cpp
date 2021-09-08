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
#include "DbBrowser\DBBrowserSynonymList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserSynonymList::DBBrowserSynonymList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserSynonymList::~DBBrowserSynonymList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserSynonymList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
END_MESSAGE_MAP()

    const int cn_owner        = 0;
    const int cn_synonym_name = 1;
    const int cn_table_owner  = 2;   
    const int cn_table_name   = 3;
    const int cn_db_link      = 4;
    const int cn_status       = 5;

    static const char* csz_sttm =
        "SELECT <RULE> "
          "s.owner,"
          "s.synonym_name,"
          "s.table_owner,"
          "s.table_name,"
          "s.db_link,"
          "'VALID' status "
        "FROM sys.all_synonyms s "
        "WHERE owner = :owner "
          "AND EXISTS ("
            "SELECT 1 FROM sys.all_objects o "
              "WHERE s.table_name  = o.object_name "
                "AND s.table_owner = o.owner "
          ")"
        "UNION ALL"
        "("
          "SELECT "
            "s.owner,"
            "s.synonym_name,"
            "s.table_owner,"
            "s.table_name,"
            "s.db_link,"
            "'INVALID' status "
          "FROM ("
            "SELECT * FROM sys.all_synonyms WHERE owner = :owner "
            "MINUS "
            "SELECT * FROM sys.all_synonyms s "
              "WHERE owner = :owner "
                "AND EXISTS ("
                  "SELECT 1 FROM sys.all_objects o "
                    "WHERE s.table_name  = o.object_name "
                      "AND s.table_owner = o.owner "
                ")"
          ") s"
        ")";

    static const char* csz_sttm_10g =
        "WITH valid AS"
        "("
          "SELECT "
            "s.owner,"
            "s.synonym_name,"
            "s.table_owner,"
            "s.table_name,"
            "s.db_link,"
            "'VALID' status "
          "FROM sys.all_synonyms s "
          "WHERE owner = :owner "
            "AND ("
              "db_link IS NOT NULL "
              "OR EXISTS "
              "("
                "SELECT 1 FROM sys.all_objects o "
                  "WHERE s.table_name  = o.object_name "
                    "AND s.table_owner = o.owner "
              ")"
            ")"
        ")"
        "SELECT * FROM valid "
        "UNION ALL "
        "SELECT "
          "s.owner,"
          "s.synonym_name,"
          "s.table_owner,"
          "s.table_name,"
          "s.db_link,"
          "'INVALID' status "
        "FROM sys.all_synonyms s "
        "WHERE owner = :owner "
          "AND (s.owner, s.synonym_name)"
            "NOT IN (SELECT owner, synonym_name FROM valid)"
    ;

    static const char* csz_user_fast_sttm =
        "SELECT <RULE> "
          ":owner,"
          "s.synonym_name,"
          "s.table_owner,"
          "s.table_name,"
          "s.db_link,"
          "o.status "
        "FROM sys.user_synonyms s, sys.user_objects o "
        "WHERE :owner= :owner "
          "AND o.object_type = 'SYNONYM' "
          "AND s.synonym_name = o.object_name "
        ;

    static const char* csz_fast_sttm =
        "SELECT <RULE> "
          "s.owner,"
          "s.synonym_name,"
          "s.table_owner,"
          "s.table_name,"
          "s.db_link,"
          "'NA' status "
        "FROM sys.all_synonyms s "
        "WHERE s.owner = :owner "
        ;

	struct BackgroundTask_RefreshSynonymList : 
        BackgroundTask_Refresh_Templ<DBBrowserSynonymList, SynonymCollection>
    {
        BackgroundTask_RefreshSynonymList(DBBrowserSynonymList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh SynonymList")
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

                if (GetSQLToolsSettings().GetObjectsListSynonymWithoutObjectInvalid())
                {
                    subst << ((servVer < OCI8::esvServer10X) ? csz_sttm : csz_sttm_10g);
                }
                else
                {
                    if (servVer < OCI8::esvServer81X)
                        subst << csz_fast_sttm;
                    else
                    {
                        string user, schema;
                        connect.GetCurrentUserAndSchema (user, schema);

                        if (m_schema == user && m_schema == schema)
                            subst << csz_user_fast_sttm;
                        else
                            subst << csz_fast_sttm;
                    }
                }

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    SynonymEntry synonym;

                    synonym.owner        = cur.ToString(cn_owner       );
                    synonym.synonym_name = cur.ToString(cn_synonym_name);
                    synonym.table_owner  = cur.ToString(cn_table_owner );
                    synonym.table_name   = cur.ToString(cn_table_name  );
                    synonym.db_link      = cur.ToString(cn_db_link     );
                    synonym.status       = cur.ToString(cn_status      );

                    m_data.push_back(synonym);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserSynonymList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshSynonymList(*this)));
        SetDirty(false);
    }
}

	struct BackgroundTask_RefreshSynonymListEntry : Task
    {
        int m_entry;
        SynonymEntry m_dataEntry;
        DBBrowserSynonymList& m_list;
        string m_schema;

        BackgroundTask_RefreshSynonymListEntry (DBBrowserSynonymList& list, int entry)
            : Task("Refresh SynonymEntry", list.GetParent()),
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
                ActivePrimeExecutionOnOff onOff;

                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");

                if (GetSQLToolsSettings().GetObjectsListSynonymWithoutObjectInvalid())
                {
                    subst << ((servVer < OCI8::esvServer10X) ? csz_sttm : csz_sttm_10g);
                }
                else
                {
                    if (servVer < OCI8::esvServer81X)
                        subst << csz_fast_sttm;
                    else
                    {
                        string user, schema;
                        connect.GetCurrentUserAndSchema (user, schema);

                        if (m_schema == user && m_schema == schema)
                            subst << csz_user_fast_sttm;
                        else
                            subst << csz_fast_sttm;
                    }
                }
                subst << " and synonym_name = :name";

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":name", m_dataEntry.synonym_name.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    m_dataEntry.owner        = cur.ToString(cn_owner       );
                    m_dataEntry.synonym_name = cur.ToString(cn_synonym_name);
                    m_dataEntry.table_owner  = cur.ToString(cn_table_owner );
                    m_dataEntry.table_name   = cur.ToString(cn_table_name  );
                    m_dataEntry.db_link      = cur.ToString(cn_db_link     );
                    m_dataEntry.status       = cur.ToString(cn_status      );
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

void DBBrowserSynonymList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshSynonymListEntry(*this, entry)));
}

void DBBrowserSynonymList::ApplyQuickFilter (bool valid, bool invalid) 
{
    ListCtrlManager::FilterCollection filter;
    GetFilter(filter);

    for (int i = 0, n = m_dataAdapter.getColCount(); i < n; ++i)
        if (!wcsicmp(m_dataAdapter.getColHeader(i), L"status"))
        {
            const wchar_t* val = 0;

            if (valid && invalid)
                val = L"";
            else if (valid)
                val = L"VALID";
            else if  (invalid)
                val = L"INVALID";
            else
                val = L"NA";

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

void DBBrowserSynonymList::ExtendContexMenu (CMenu* pMenu)
{
    if (theApp.GetServerVersion() >= OCI8::esvServer9X)
    {
        UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

        pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE, L"&Compile");                      
        pMenu->AppendMenu(MF_SEPARATOR);
    }
}

void DBBrowserSynonymList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER <TYPE> \"<SCHEMA>\".\"<NAME>\" COMPILE");
}


