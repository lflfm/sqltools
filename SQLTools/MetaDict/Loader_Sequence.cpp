/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

// 2017-12-31 bug fix, Oracle client version was used to control RULE hint
// 2017-12-31 improvement, DDL reverse engineering handles 12c indentity columns


#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "COMMON\StrHelpers.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    const int cn_sequence_owner = 0;
    const int cn_sequence_name  = 1;
    const int cn_min_value      = 2;
    const int cn_max_value      = 3;
    const int cn_increment_by   = 4;
    const int cn_cycle_flag     = 5;
    const int cn_order_flag     = 6;
    const int cn_cache_size     = 7;
    const int cn_last_number    = 8;
    // 12c indentity related
    const int cn_generated      = 9;
    const int cn_table_name     = 10;
    const int cn_column_name    = 11;
    const int cn_generation_type= 12;

    LPSCSTR csz_seq_sttm =
        "SELECT <RULE> "
            "sequence_owner,"
            "sequence_name,"
            "min_value,"
            "max_value,"
            "increment_by,"
            "cycle_flag,"
            "order_flag,"
            "cache_size,"
            "last_number, "
            "'N'"
        "FROM sys.all_sequences "
        "WHERE sequence_owner = :owner "
            "AND sequence_name <EQUAL> :name";

    LPSCSTR csz_seq_sttm_12c =
        "SELECT "
          "seq.sequence_owner,"
          "seq.sequence_name,"
          "seq.min_value,"
          "seq.max_value,"
          "seq.increment_by,"
          "seq.cycle_flag,"
          "seq.order_flag,"
          "seq.cache_size,"
          "seq.last_number,"
          "obj.generated "
        "FROM sys.all_sequences seq, sys.all_objects obj "
          "WHERE seq.sequence_owner = :owner "
            "AND obj.owner = :owner "
            "AND seq.sequence_name <EQUAL> :name "
            "AND obj.object_name <EQUAL> :name "
            "AND seq.sequence_owner = obj.owner "
            "AND seq.sequence_name = obj.object_name";

    LPSCSTR csz_seq_by_tab_sttm_12c =
        "SELECT "
          "seq.sequence_owner,"
          "seq.sequence_name,"
          "seq.min_value,"
          "seq.max_value,"
          "seq.increment_by,"
          "seq.cycle_flag,"
          "seq.order_flag,"
          "seq.cache_size,"
          "seq.last_number,"
          "obj.generated,"
          "idn.table_name,"
          "idn.column_name," 
          "idn.generation_type "
        "FROM sys.all_tab_identity_cols idn, sys.all_sequences seq, sys.all_objects obj "
          "WHERE idn.owner = :owner "
            "AND seq.sequence_owner = :owner "
            "AND obj.owner = :owner "
            "AND idn.owner = seq.sequence_owner "
            "AND seq.sequence_owner = obj.owner "
            "AND idn.table_name <EQUAL> :name "
            "AND idn.sequence_name = seq.sequence_name "
            "AND seq.sequence_name = obj.object_name ";

void Loader::LoadSequences (const char* owner, const char* name, bool useLike)
{
    loadSequences(owner, name, false/*byTable*/, useLike);
}

void Loader::loadSequences (const char* owner, const char* name, bool byTable, bool useLike)
{
    if (byTable && m_connect.GetVersion() < OCI8::esvServer12X)
        return; // INDENTITY has been inroduced with 12c

    Common::Substitutor subst;
    subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
    subst.AddPair("<RULE>", m_connect.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
    subst << (m_connect.GetVersion() < OCI8::esvServer12X 
        ? csz_seq_sttm
        : (byTable ? csz_seq_by_tab_sttm_12c : csz_seq_sttm_12c)
        );

    OciCursor cur(m_connect, subst.GetResult());

    cur.Bind(":owner", owner);
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        Sequence& seq = m_dictionary.CreateSequence(cur.ToString(cn_sequence_owner).c_str(), 
                                                    cur.ToString(cn_sequence_name).c_str());

        cur.GetString(cn_sequence_owner, seq.m_strOwner);
        cur.GetString(cn_sequence_name,  seq.m_strName);
        cur.GetString(cn_min_value,      seq.m_strMinValue);
        cur.GetString(cn_max_value,      seq.m_strMaxValue);
        cur.GetString(cn_increment_by,   seq.m_strIncrementBy);

        seq.m_bCycleFlag = IsYes(cur.ToString(cn_cycle_flag));
        seq.m_bOrderFlag = IsYes(cur.ToString(cn_order_flag));
        seq.m_bGenerated = IsYes(cur.ToString(cn_generated));

        cur.GetString(cn_cache_size,  seq.m_strCacheSize);
        cur.GetString(cn_last_number, seq.m_strLastNumber);

        if (byTable)
        {
            string table_name, column_name;
            cur.GetString(cn_table_name, table_name);
            cur.GetString(cn_column_name, column_name);

            Table& table = m_dictionary.LookupTable(owner, table_name.c_str());
            Table::ColumnContainer::iterator it = table.m_Columns.begin();
            for (; it != table.m_Columns.end(); ++it)
                if (it->second->m_strColumnName == column_name)
                {
                    it->second->m_strSequenceName = seq.m_strName;
                    cur.GetString(cn_generation_type, it->second->m_strIndentityGenerationType);
                }
        }

    }
}

}// END namespace OraMetaDict
