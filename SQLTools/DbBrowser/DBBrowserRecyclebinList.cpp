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
#include "DbBrowser\DBBrowserRecyclebinList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "COMMON\InputDlg.h"
#include "Dlg\ConfirmationDlg.h"
#include "ServerBackgroundThread\TaskQueue.h"

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserRecyclebinList::DBBrowserRecyclebinList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserRecyclebinList::~DBBrowserRecyclebinList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserRecyclebinList, DbBrowserList)
    ON_COMMAND(ID_EDIT_DELETE_WORD_TO_RIGHT, OnPurge)
    ON_COMMAND(ID_DS_PURGE_ALL, OnPurgeAll)
    ON_COMMAND(ID_DS_FLASHBACK, OnFlashback)
    ON_WM_CREATE()
END_MESSAGE_MAP()

    const int cn_object_name    = 0;
    const int cn_original_name  = 1;
    const int cn_operation      = 2;
    const int cn_type           = 3;
    const int cn_ts_name        = 4;
    const int cn_createtime     = 5;
    const int cn_droptime       = 6;
    const int cn_dropscn        = 7;
    const int cn_partition_name = 8; 
    const int cn_can_undrop     = 9;
    const int cn_can_purge      = 10;
    const int cn_related        = 11;
    const int cn_base_object    = 12;
    const int cn_purge_object   = 13;
    const int cn_space          = 14;

    static const char* csz_sttm =
        "SELECT"
        " object_name,"
        " original_name,"
        " operation,"
        " type,"
        " ts_name,"
        " createtime,"
        " droptime,"
        " dropscn,"
        " partition_name,"
        " can_undrop,"
        " can_purge,"
        " related,"
        " base_object,"
        " purge_object,"
        " space"
        " FROM user_recyclebin"
        " WHERE USER = :owner";

	struct BackgroundTask_RefreshRecyclebinList 
        : BackgroundTask_Refresh_Templ<DBBrowserRecyclebinList, RecyclebinCollection>
    {
        BackgroundTask_RefreshRecyclebinList (DBBrowserRecyclebinList& list)
            : BackgroundTask_Refresh_Templ<DBBrowserRecyclebinList, RecyclebinCollection>(list, "Refresh RecyclebinList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            if (!(connect.GetVersion() >= OCI8::esvServer10X)) return; 

            try
            {
                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_sttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    RecyclebinEntry recycled;

                    recycled.original_name  = cur.ToString(cn_original_name );
                    recycled.operation      = cur.ToString(cn_operation     );
                    recycled.type           = cur.ToString(cn_type          );
                    recycled.partition_name = cur.ToString(cn_partition_name);
                    recycled.ts_name        = cur.ToString(cn_ts_name       );
                    recycled.createtime     = cur.ToString(cn_createtime    );
                    recycled.droptime       = cur.ToString(cn_droptime      );
                    recycled.can_undrop     = cur.ToString(cn_can_undrop    );
                    recycled.can_purge      = cur.ToString(cn_can_purge     );
                    recycled.space          = cur.ToInt   (cn_space         );
                    // HIDDEN
                    recycled.object_name    = cur.ToString(cn_object_name   );
                    recycled.dropscn        = cur.ToString(cn_dropscn       );
                    recycled.related        = cur.ToString(cn_related       );
                    recycled.base_object    = cur.ToString(cn_base_object   );
                    recycled.purge_object   = cur.ToString(cn_purge_object  );

                    string buff = recycled.type;
                    Common::to_upper_str(recycled.type.c_str(), buff);
                    if (buff.find("TABLE") != string::npos || buff.find("LOB") != string::npos)
                        recycled.image_index = IDII_TABLE;
                    else if (buff.find("INDEX") != string::npos)
                        recycled.image_index = IDII_INDEX;
                    else 
                        recycled.image_index = IDII_RECYCLEBIN;

                    m_data.push_back(recycled);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserRecyclebinList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshRecyclebinList(*this)));

        SetDirty(false);
    }
}                                           

void DBBrowserRecyclebinList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserRecyclebinList::ExtendContexMenu (CMenu* pMenu)
{
    // remove all convention command
    pMenu->DestroyMenu();
    pMenu->CreatePopupMenu();

    UINT flashbackFlags = MF_STRING|MF_GRAYED;
    UINT purgeFlags     = MF_STRING|MF_GRAYED;
    
    std::vector<unsigned int> selected;
    GetSelEntries(selected);

    std::vector<unsigned int>::const_iterator it = selected.begin();
    for (; it != selected.end(); ++it)
    {
        if (m_data.at(*it).can_undrop == "YES") flashbackFlags = MF_STRING; 
        if (m_data.at(*it).can_purge == "YES")  purgeFlags = MF_STRING; 
    }

    pMenu->AppendMenu(MF_STRING,        ID_DS_REFRESH,         "&Refresh");
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(flashbackFlags,   ID_DS_FLASHBACK,       "&Flushback");
    pMenu->AppendMenu(purgeFlags,       ID_EDIT_DELETE_WORD_TO_RIGHT, "&Purge");
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING,        ID_DS_PURGE_ALL,       "&Purge All");
    pMenu->AppendMenu(MF_SEPARATOR);
}

	struct BackgroundTask_ListPurge : Task
    {
        DBBrowserRecyclebinList& m_list;
        int m_entry;
        string m_statement;

        BackgroundTask_ListPurge (DBBrowserRecyclebinList& list, int entry)
            : Task("Purge Object", list.GetParent()),
            m_list(list),
            m_entry(entry)
        {
            Common::Substitutor subst;
            subst.AddPair("<SCHEMA>", list.GetSchema());
            subst.AddPair("<NAME>", list.GetObjectName(entry));
            subst.AddPair("<TYPE>", list.GetObjectType(entry));
            subst <<  ("PURGE <TYPE> \"<SCHEMA>\".\"<NAME>\"");
            m_statement = subst.GetResult();
            SetName(m_statement);
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                connect.ExecuteStatement(m_statement.c_str());
            }
            catch (const OciException& x)
            {
                ostringstream msg;
                msg << '\"' << m_statement << "\" failed with the following error:\n\n" << x.what();
                SetError(msg.str());
            }
        }

        void ReturnInForeground ()
        {
            // remove all entries with the same purge_object (for index)
            // remove all entries with base_object as the selected purge_object (for tables)
            string purge_object = m_list.m_data[m_entry].purge_object;
            RecyclebinCollection::const_iterator it = m_list.m_data.begin();
            for (int entry = 0; it != m_list.m_data.end(); ++entry, ++it)
                if (entry != m_entry 
                && (it->purge_object == purge_object || it->base_object == purge_object)
                )
                {
                    m_list.MarkAsDeleted(entry);
                    m_list.OnDeleteEntry(entry, false);
                }

            m_list.MarkAsDeleted(m_entry);
            m_list.OnDeleteEntry(m_entry);
        }

    };

// TODO: remove all dependent objects when primaty object is purged
void DBBrowserRecyclebinList::OnPurge ()
{
    try {
        
        bool purgeAll = false;
        CConfirmationDlg dlg(this);

        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (; it != data.end(); ++it)
        {
            if (m_data.at(*it).can_purge != "YES")
                continue;

            bool purgeIt = false;

            if (purgeAll)
                purgeIt = true;
            else
            {
                dlg.m_strText.Format("Are you sure you want to purge \"%s\"?", GetObjectName(*it));
                int retVal = dlg.DoModal();
                SetFocus();

                switch (retVal) 
                {
                case IDCANCEL:
                    AfxThrowUserException();
                case ID_CNF_ALL:
                    purgeAll = true;
                case IDYES:
                    purgeIt = true;
                case IDNO:
                    break;
                }
            }

            if (purgeIt)
                BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListPurge(*this, *it)));
        }
    }
    catch (CUserException*)
    {
        MessageBeep((UINT)-1);
    }
}

	struct BackgroundTask_ListPurgeAll : Task
    {
        DbBrowserList& m_list;
        string m_schema;

        BackgroundTask_ListPurgeAll (DbBrowserList& list)
            : Task("PURGE RECYCLEBIN", list.GetParent()),
            m_list(list)
        {
            m_schema = m_list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                //TODO#TEST: re-test
                //TODO: check what should be used user or schema?
                string user, schema;
                connect.GetCurrentUserAndSchema(user, schema);

                if (m_schema == user)
                {
                    if (AfxMessageBox("Are you sure you want to PURGE RECYCLEBIN?", MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                    {
                        connect.ExecuteStatement("PURGE RECYCLEBIN");
                    }
                }
                else
                    AfxMessageBox("PURGE RECYCLEBIN is available only for the current user.", MB_OK|MB_ICONEXCLAMATION);
            }
            catch (const OciException& x)
            {
                ostringstream msg;
                msg << "\"PURGE RECYCLEBIN\" failed with the following error:\n\n" << x.what();
                SetError(msg.str());
            }
        }

        void ReturnInForeground ()
        {
            m_list.Refresh(true/*force*/);
        }
    };

void DBBrowserRecyclebinList::OnPurgeAll()
{
    BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListPurgeAll(*this)));
}

	struct BackgroundTask_ListFlashback : Task
    {
        DBBrowserRecyclebinList& m_list;
        int m_entry;
        string m_statement;

        BackgroundTask_ListFlashback (DBBrowserRecyclebinList& list, int entry, const string& to)
            : Task("Flashback Object", list.GetParent()),
            m_list(list),
            m_entry(entry)
        {
            Common::Substitutor subst;
            subst.AddPair("<SCHEMA>", list.GetSchema());
            subst.AddPair("<NAME>", list.GetObjectName(entry));
            subst.AddPair("<TYPE>", list.GetObjectType(entry));
            subst.AddPair("<TO_NAME>", to);
            subst <<  ("FLASHBACK <TYPE> \"<SCHEMA>\".\"<NAME>\" TO BEFORE DROP RENAME TO <TO_NAME>");
            m_statement = subst.GetResult();
            SetName(m_statement);
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                connect.ExecuteStatement(m_statement.c_str());
            }
            catch (const OciException& x)
            {
                ostringstream msg;
                msg << '\"' << m_statement << "\" failed with the following error:\n\n" << x.what();
                SetError(msg.str());
            }
        }

        void ReturnInForeground ()
        {
            // remove all entries with the same base_object
            string base_object = m_list.m_data[m_entry].base_object;
            RecyclebinCollection::const_iterator it = m_list.m_data.begin();
            for (int entry = 0; it != m_list.m_data.end(); ++entry, ++it)
                if (entry != m_entry 
                && it->base_object == base_object
                )
                {
                    m_list.MarkAsDeleted(entry);
                    m_list.OnDeleteEntry(entry, false);
                }

            m_list.MarkAsDeleted(m_entry);
            m_list.OnDeleteEntry(m_entry);
        }
    };

// TODO: remove all dependent objects when primaty object is flashed back
void DBBrowserRecyclebinList::OnFlashback ()
{
    try {
        
        CConfirmationDlg dlg(this);

        std::vector<unsigned int> data;
        GetSelEntries(data);

        std::vector<unsigned int>::const_iterator it = data.begin();
        for (; it != data.end(); ++it)
        {
            if (m_data.at(*it).can_undrop != "YES")
                continue;

            CInputDlg dlg;
            dlg.m_title  = "Flashback to Before Drop";
            dlg.m_prompt = "Flashback to Name:";
            dlg.m_value  = m_data.at(*it).original_name;

            int retVal = dlg.DoModal();
            SetFocus();

            if (retVal == IDOK)
                BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListFlashback(*this, *it, dlg.m_value)));
        }
    }
    catch (CUserException*)
    {
        MessageBeep((UINT)-1);
    }
}


int DBBrowserRecyclebinList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (DbBrowserList::OnCreate(lpCreateStruct) == -1)
        return -1;

    // hide secondary objects such as partitions
    ListCtrlManager::FilterCollection filter;
    GetFilter(filter);

    for (int i = 0, n = m_dataAdapter.getColCount(); i < n; ++i)
        if (!stricmp(m_dataAdapter.getColHeader(i), "can purge"))
        {
            filter.at(i).value = "YES";
            filter.at(i).operation = ListCtrlManager::EXACT_MATCH;
            SetFilter(filter);
            break;
        }

    return 0;
}

void DBBrowserRecyclebinList::OnShowInObjectView ()
{
    std::vector<unsigned int> data;
    GetSelEntries(data);

    if (!data.empty() && !strcmp(GetObjectType(data.front()), "TABLE"))
    {
        DbBrowserList::OnShowInObjectView();
    }
    else
    {
        MessageBeep(MB_ICONERROR);
		Global::SetStatusText("Unsuported object type!");
    }
}
