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

// 2018-01-01 improvement, added "Generated", "Created" and "Modified" to the "Sequence" tab of "Objects List"
// 2018-01-01 improvement, RULE hint is used for databases prior to 10g only

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserSequenceList.h"
#include "OCI8/BCursor.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "COMMON\StrHelpers.h"

using namespace OraMetaDict;
using namespace ServerBackgroundThread;


DBBrowserSequenceList::DBBrowserSequenceList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserSequenceList::~DBBrowserSequenceList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserSequenceList, DbBrowserList)
END_MESSAGE_MAP()

    const int cn_owner         = 0;
    const int cn_sequence_name = 1;
    const int cn_last_number   = 2;   
    const int cn_min_value     = 3;
    const int cn_max_value     = 4;
    const int cn_increment_by  = 5;  
    const int cn_cache_size    = 6;     
    const int cn_cycle_flag    = 7; 
    const int cn_order_flag    = 8;  
    const int cn_generated     = 9;
    const int cn_created       = 10;
    const int cn_last_ddl_time = 11;

    static const char* csz_table_sttm =
        "SELECT <RULE> "
            "seq.sequence_owner,"        
            "seq.sequence_name,"
            "seq.last_number,"
            "seq.min_value,"    
            "seq.max_value,"    
            "seq.increment_by,"
            "seq.cache_size, "
            "seq.cycle_flag,"    
            "seq.order_flag,"    
            "obj.generated,"
            "obj.created,"
            "obj.last_ddl_time "
        "FROM sys.all_sequences seq, sys.all_objects obj "
        "WHERE seq.sequence_owner = :owner "
        "AND obj.owner = :owner AND obj.object_type = 'SEQUENCE' "
        "AND seq.sequence_owner = obj.owner AND seq.sequence_name = obj.object_name"
        ;

	struct BackgroundTask_RefreshSequenceList 
        : BackgroundTask_Refresh_Templ<DBBrowserSequenceList, SequenceCollection>
    {
        BackgroundTask_RefreshSequenceList (DBBrowserSequenceList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh SequenceList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst << csz_table_sttm;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    SequenceEntry sequence;

                    sequence.owner         = cur.ToString(cn_owner        );
                    sequence.sequence_name = cur.ToString(cn_sequence_name);
                    sequence.last_number   = cur.ToString(cn_last_number  );
                    sequence.min_value     = cur.ToString(cn_min_value    );
                    sequence.max_value     = cur.ToString(cn_max_value    );
                    sequence.increment_by  = cur.ToString(cn_increment_by );
                    sequence.cache_size    = cur.ToString(cn_cache_size   );
                    sequence.cycle_flag    = cur.ToString(cn_cycle_flag   );
                    sequence.order_flag    = cur.ToString(cn_order_flag   );
                    sequence.generated     = cur.ToString(cn_generated    );
                    sequence.created       = cur.ToString(cn_created      );
                    sequence.last_ddl_time = cur.ToString(cn_last_ddl_time);

                    m_data.push_back(sequence);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserSequenceList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshSequenceList(*this)));
        SetDirty(false);
    }
}

void DBBrowserSequenceList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserSequenceList::ExtendContexMenu (CMenu* /*pMenu*/)
{
}
