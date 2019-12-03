/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2007 Aleksey Kochetov

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

// 17.09.2002 bug fix: cluster reengineering fails
// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 03.08.2003 improvement, domain index support
// 09.02.2004 bug fix, DDL reengineering: Oracle Server 8.0.5 support (indexes)
// 21.03.2004 bug fix, DDL reengineering: domain index bug (introduced in 141b46)
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
// 25.02.2007 improvement, partition support
// 26.09.2007 bug fix, compatibility with 8.0.X
// 18.12.2016 DDL reverse engineering incrorrectly handles function-base indexes with quoted columns
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    //
    //  Attention: in cluster case user_indexes or dba_indexes must be used
    //  if schema is equal user, then we can use user_indexes
    //  else we must use dba_indexes (user must have select grant)
    //

    LPSCSTR csz_tab_index_sttm =
		"SELECT <RULE> "
            "1,"
            "owner,"
            "index_name,"
            "<INDEX_TYPE>,"    //NORMAL for 7.3
            "table_owner,"
            "table_name,"
            "table_type,"
            "uniqueness,"
            "tablespace_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "freelists,"
            "freelist_groups,"
            "pct_free,"
            "ini_trans,"
            "max_trans,"
            "<TEMPORARY>,"     //N for 7.3
            "<GENERATED>,"     //N for 7.3
            "<LOGGING>,"       //YES for 7.3
            "<COMPRESSION>,"   //NULL
            "<PREFIX_LENGTH>," //To_Number(NULL)
            "<ITYP_OWNER>,"    //NULL
            "<ITYP_NAME>,"     //NULL
            "<PARAMETERS>,"    //NULL
            "<PARTITIONED>,"
            "<BUFFER_POOL>,"
            "<DEGREE>,"
            "<INSTANCES> "
        "FROM sys.all_indexes "
        "WHERE <OWNER> = :owner "
            "AND <NAME> <EQUAL> :name "
        "UNION ALL "
        "SELECT "
            "2,"
            "v2.index_owner,"
            "v2.index_name,"
            "NULL,"
            "v2.table_owner,"
            "v2.table_name,"
            "v2.column_name,"
            "NULL,"
            "NULL,"
            "v2.column_position,"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "To_Number(NULL),"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "To_Number(NULL),"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL "
        "FROM sys.all_indexes v1, sys.all_ind_columns v2 "
        "WHERE v2.index_owner = v1.owner "
            "AND v2.index_name = v1.index_name "
            "AND v1.<OWNER> = :owner "
            "AND v1.<NAME> <EQUAL> :name";

    LPSCSTR csz_ind_exp_sttm =
        "SELECT column_position, column_expression "
            "FROM sys.all_ind_expressions "
            "WHERE index_owner = :owner "
                "AND index_name = :name";

    LPSCSTR csz_clu_index_sttm =
		"SELECT <RULE> "
            "1,"
            "<INDEX_OWNER>,"   //if (dba) then "INDEX_OWNER" else "USER"
            "index_name,"
            "<INDEX_TYPE>,"    //NORMAL for 7.3
            "table_owner,"
            "table_name,"
            "table_type,"
            "uniqueness,"
            "tablespace_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "freelists,"
            "freelist_groups,"
            "pct_free,"
            "ini_trans,"
            "max_trans, "
            "<TEMPORARY>,"     //N for 7.3
            "<GENERATED>,"     //N for 7.3
            "<LOGGING>,"       //YES for 7.3
            "<COMPRESSION>,"   //NULL
            "<PREFIX_LENGTH>," //To_Number(NULL)
            "<ITYP_OWNER>,"    //NULL
            "<ITYP_NAME>,"     //NULL
            "<PARAMETERS>,"    //NULL
            "<PARTITIONED>,"
            "<BUFFER_POOL>,"
            "<DEGREE>,"
            "<INSTANCES> "
        "FROM <SRC_INDEXES> "           // if (dba) then "dba_indexes" else "user_indexes"
        "WHERE table_owner = :owner "
            "AND table_name <EQUAL> :name "
            "AND EXISTS "
                "("
                    "SELECT NULL FROM <SRC_CLUSTERS> "// if (dba) then "dba_clusters" else "user_clusters"
                    "WHERE <CLUSTER_OWNER> = :owner "
                        "AND cluster_name <EQUAL> :name "
                ")";

    const int cn_inx_rec_type           = 0;

    const int cn_inx_owner              = 1;
    const int cn_inx_index_name         = 2;
    const int cn_inx_index_type         = 3;
    const int cn_inx_table_owner        = 4;
    const int cn_inx_table_name         = 5;
    const int cn_inx_table_type         = 6;
    const int cn_inx_uniqueness         = 7;
    const int cn_inx_tablespace_name    = 8;
    const int cn_inx_initial_extent     = 9;
    const int cn_inx_next_extent        = 10;
    const int cn_inx_min_extents        = 11;
    const int cn_inx_max_extents        = 12;
    const int cn_inx_pct_increase       = 13;
    const int cn_inx_freelists          = 14;
    const int cn_inx_freelist_groups    = 15;
    const int cn_inx_pct_free           = 16;
    const int cn_inx_ini_trans          = 17;
    const int cn_inx_max_trans          = 18;
    const int cn_inx_temporary          = 19;
    const int cn_inx_generated          = 20;
    const int cn_inx_logging            = 21;
    const int cn_inx_compression        = 22;
    const int cn_inx_prefix_length      = 23;
    const int cn_inx_domain_owner       = 24; // 03.08.2003 improvement, domain index support
    const int cn_inx_domain             = 25;
    const int cn_inx_domain_params      = 26;
    const int cn_inx_partitioned        = 27;
    const int cn_inx_buffer_pool        = 28;
    const int cn_inx_degree             = 29;
    const int cn_inx_instances          = 30;

    const int cn_inx_column_name         = 6;
    const int cn_inx_column_position     = 9;

    const int cn_inx_exp_column_position     = 0;
    const int cn_inx_exp_column_expressoin   = 1;

    static void load_iot_overflow (OciConnect&, Index&, Table&);
    static void load_part_info  (OciConnect&, Index&);
    void load_part_info (OciConnect& con, Table& table);

void Loader::LoadIndexes (const char* owner, const char* table, bool useLike)
{
    loadIndexes(owner, table, false/*byTable*/, false/*isCluster*/, useLike);
}

void Loader::loadIndexes (const char* owner, const char* table, bool byTable, bool isCluster, bool useLike)
{
    bool useDba = isCluster && m_strCurSchema.compare(owner);
    try
    {
        OCI8::EServerVersion ver = m_connect.GetVersion();
        Common::Substitutor subst;

        subst.AddPair("<OWNER>", byTable  ? "table_owner" : "owner");
        subst.AddPair("<NAME>",  byTable  ? "table_name"  : "index_name");
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst.AddPair("<RULE>", m_connect.GetVersion() < OCI8::esvServer10X? "/*+RULE*/" : "");

        subst.AddPair("<COMPRESSION>"  , (ver > OCI8::esvServer80X) ? "compression"   : "NULL"           );
        subst.AddPair("<PREFIX_LENGTH>", (ver > OCI8::esvServer80X) ? "prefix_length" : "To_Number(NULL)");
        subst.AddPair("<ITYP_OWNER>"   , (ver > OCI8::esvServer80X) ? "ityp_owner"    : "NULL"           );
        subst.AddPair("<ITYP_NAME>"    , (ver > OCI8::esvServer80X) ? "ityp_name"     : "NULL"           );
        subst.AddPair("<PARAMETERS>"   , (ver > OCI8::esvServer80X) ? "parameters"    : "NULL"           );

        subst.AddPair("<INDEX_TYPE>", (ver > OCI8::esvServer73X) ? "index_type" : "'NORMAL'");
        subst.AddPair("<TEMPORARY>" , (ver > OCI8::esvServer73X) ? "temporary"  : "'N'"     );
        subst.AddPair("<GENERATED>" , (ver > OCI8::esvServer73X) ? "generated"  : "'N'"     );
        subst.AddPair("<LOGGING>"   , (ver > OCI8::esvServer73X) ? "logging"    : "'YES'"   );

        subst.AddPair("<PARTITIONED>", (ver > OCI8::esvServer73X) ? "partitioned"   : "'NO'");
        subst.AddPair("<BUFFER_POOL>", (ver > OCI8::esvServer73X) ? "buffer_pool"   : "'DEFAULT'");

        subst.AddPair("<DEGREE>"   , (ver > OCI8::esvServer73X) ? "degree"    : "NULL");
        subst.AddPair("<INSTANCES>", (ver > OCI8::esvServer73X) ? "instances" : "NULL");

        if (isCluster)
        {
            subst.AddPair("<INDEX_OWNER>",  !useDba  ? "user" : "owner");
            subst.AddPair("<CLUSTER_OWNER>",!useDba  ? "user" : "owner");
            subst.AddPair("<SRC_INDEXES>",  !useDba  ? "user_indexes" : "dba_indexes");
            subst.AddPair("<SRC_CLUSTERS>", !useDba  ? "user_clusters" : "dba_clusters");
        }
    
        subst << (!isCluster ? csz_tab_index_sttm : csz_clu_index_sttm);

        OciCursor cur(m_connect, subst.GetResult(), 50);
        
        cur.Bind(":owner", owner);
        cur.Bind(":name",  table);

        cur.Execute();
        while (cur.Fetch())
        {
            if (cur.ToInt(cn_inx_rec_type) == 1)  // load index data
            {
                Index& index = m_dictionary.CreateIndex(cur.ToString(cn_inx_owner).c_str(),
                                                        cur.ToString(cn_inx_index_name).c_str());

                cur.GetString(cn_inx_owner,      index.m_strOwner);
                cur.GetString(cn_inx_index_name, index.m_strName);
                              
                cur.GetString(cn_inx_table_owner,index.m_strTableOwner);
                cur.GetString(cn_inx_table_name, index.m_strTableName);

                string type;
                cur.GetString(cn_inx_index_type, type);

                if (type == "NORMAL")
                    index.m_Type = eitNormal;
                else if (type == "NORMAL/REV")
                {
                    index.m_Type = eitNormal;
                    index.m_bReverse = true;
                }
                else if (type == "BITMAP")
                    index.m_Type = eitBitmap;
                else if (type == "CLUSTER")
                    index.m_Type = eitCluster;
                else if (type == "FUNCTION-BASED NORMAL")
                    index.m_Type = eitFunctionBased;
                else if (type == "FUNCTION-BASED BITMAP")
                    index.m_Type = eitFunctionBasedBitmap;
                else if (type == "IOT - TOP")
                    index.m_Type = eitIOT_TOP;
                else if (type == "DOMAIN") // 03.08.2003 improvement, domain index support
                    index.m_Type = eitDomain;
                else
                    _CHECK_AND_THROW_(0 ,"Index loading error:\nunsupporded index type!");

                index.m_bUniqueness = (cur.ToString(cn_inx_uniqueness) == "UNIQUE") ? true : false;
                index.m_bTemporary  = IsYes(cur.ToString(cn_inx_temporary));

                if (index.m_Type == eitFunctionBased 
                || index.m_Type == eitFunctionBasedBitmap)
                {
                    OciCursor cur_exp(m_connect, csz_ind_exp_sttm, 1, 4 * 1024);
                    
                    cur_exp.Bind(":owner", index.m_strOwner.c_str());
                    cur_exp.Bind(":name",  index.m_strName.c_str());

                    cur_exp.Execute();
                    while (cur_exp.Fetch())
                    {
                        _CHECK_AND_THROW_(cur_exp.IsGood(cn_inx_exp_column_expressoin), "Index loading error:\nexpression is too long!");
                        int pos = cur_exp.ToInt(cn_inx_exp_column_position) - 1;
                        index.m_Columns[pos] = cur_exp.ToString(cn_inx_exp_column_expressoin);
                        index.m_isExpression[pos] = true;
                    }
                }

                index.m_bGenerated = Loader::IsYes(cur.ToString(cn_inx_generated));
                Loader::ReadYes(cur, cn_inx_logging, index.m_bLogging);
                
                index.m_bIndexCompression = cur.ToString(cn_inx_compression) == "ENABLED";
                
                if (index.m_bIndexCompression)
                    index.m_nCompressionPrefixLength = cur.ToInt(cn_inx_prefix_length);

                Loader::Read(cur, cn_inx_tablespace_name, index.m_strTablespaceName);
                Loader::Read(cur, cn_inx_initial_extent , index.m_nInitialExtent  );
                Loader::Read(cur, cn_inx_next_extent    , index.m_nNextExtent     );
                Loader::Read(cur, cn_inx_min_extents    , index.m_nMinExtents     );
                Loader::Read(cur, cn_inx_max_extents    , index.m_nMaxExtents     );
                Loader::Read(cur, cn_inx_pct_increase   , index.m_nPctIncrease    );
                Loader::Read(cur, cn_inx_freelists      , index.m_nFreeLists      );
                Loader::Read(cur, cn_inx_freelist_groups, index.m_nFreeListGroups );
                Loader::Read(cur, cn_inx_pct_free       , index.m_nPctFree        );
                Loader::Read(cur, cn_inx_ini_trans      , index.m_nIniTrans       );
                Loader::Read(cur, cn_inx_max_trans      , index.m_nMaxTrans       );
                Loader::Read(cur, cn_inx_degree         , index.m_nDegree         );
                Loader::Read(cur, cn_inx_instances      , index.m_nInstances      );
                index.m_nPctUsed.SetNull();

                //DOTO: verify this
                //if (index.m_nDegree <= 0) index.m_nDegree = 1;
                //if (index.m_nInstances <= 0) index.m_nInstances = 1;

                string strKey;
                strKey = index.m_strOwner + '.' + index.m_strName;

                if (index.m_Type == eitDomain) // 03.08.2003 improvement, domain index support
                {
                    index.m_strDomainOwner  = cur.ToString(cn_inx_domain_owner);
                    index.m_strDomain       = cur.ToString(cn_inx_domain);
                    index.m_strDomainParams = cur.ToString(cn_inx_domain_params);
                }

                Loader::Read(cur, cn_inx_buffer_pool, index.m_strBufferPool);

                if (Loader::IsYes(cur.ToString(cn_inx_partitioned)))
                    load_part_info(cur.GetConnect(), index);

                try
                {
                    if (index.m_Type == eitCluster)
                    {
                        Cluster& cluster = m_dictionary.LookupCluster(index.m_strTableOwner.c_str(), 
                                                                      index.m_strTableName.c_str());
                        cluster.m_setIndexes.insert(strKey);
                    }
                    else
                    {
                        Table& table = m_dictionary.LookupTable(index.m_strTableOwner.c_str(), 
                                                                index.m_strTableName.c_str());
                        table.m_setIndexes.insert(strKey);

                        if (index.m_Type == eitIOT_TOP)
                            load_iot_overflow(cur.GetConnect(), index, table);
                    }
                }
                catch (const XNotFound&) 
                {
                    if (byTable) throw;
                }
            }
            else // load index columns
            {
                Index& index = m_dictionary.LookupIndex(cur.ToString(cn_inx_owner).c_str(), 
                                                        cur.ToString(cn_inx_index_name).c_str());
            
                int pos = cur.ToInt(cn_inx_column_position);
            
                if (index.m_Columns.find(pos - 1) == index.m_Columns.end())
                    index.m_Columns[pos - 1] = cur.ToString(cn_inx_column_name);
                else if (index.m_Type != eitFunctionBased 
                && index.m_Type != eitFunctionBasedBitmap)
                    _CHECK_AND_THROW_(0, "Index reengineering algorithm failure!");
            }
        }
    }
    catch (const OciException& x)
    {
        if (useDba && x == 942)
        {
            _CHECK_AND_THROW_(0, "DBA_INDEXES or DBA_CLUSTERS is not accessble! "
                               "Can't capture claster table definition. "
                               "Reconnect as schema owner and repeat");
        }
        else
            throw;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Partitioning
////////////////////////////////////////////////////////////////////////////////

    static
    void prepare_cursor (OciCursor& cur, const char* sttm, const char* owner, const char* name, bool useLike, bool byTable)
    {
        Common::Substitutor subst;

        subst.AddPair("<OWNER>", byTable  ? "table_owner" : "owner");
        subst.AddPair("<NAME>",  byTable  ? "table_name"  : "index_name");
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");

        OCI8::EServerVersion ver = cur.GetConnect().GetVersion();
        
        subst.AddPair("<RULE>"                      , (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
        subst.AddPair("<COMPOSITE>"                 , (ver > OCI8::esvServer80X) ? "composite" : "NULL");
        subst.AddPair("<COMPRESSION>"               , (ver > OCI8::esvServer80X) ? "compression" : "NULL");
        subst.AddPair("<SUBPARTITIONING_TYPE>"      , (ver > OCI8::esvServer80X) ? "subpartitioning_type" : "NULL");
        subst.AddPair("<DEF_SUBPARTITION_COUNT>"    , (ver > OCI8::esvServer80X) ? "def_subpartition_count" : "NULL");
        subst.AddPair("<SUBPARTITIONING_KEY_COUNT>" , (ver > OCI8::esvServer80X) ? "subpartitioning_key_count" : "NULL");
        subst.AddPair("<BLOCK_SIZE>"                , (ver > OCI8::esvServer81X) ? "block_size" : "NULL");
        subst.AddPair("<INTERVAL>"                  , (ver >= OCI8::esvServer11X) ? "interval" : "NULL");
        subst.AddPair("<INTERVAL_FILTER_112>"       , (ver >= OCI8::esvServer112) ? "AND Nvl(interval, 'NO') <> 'YES'"  : "");
    
        subst.AddPair("<SUBPART_HIGH_VALUE_DUMMY>"  , 
            (ver ==  OCI8::esvServer81X) ? 
            ",(SELECT data_default high_value FROM all_tab_columns WHERE owner = 'SYS' AND table_name = 'DUAL' AND column_name = 'DUMMY')" 
            : ""
          );

        subst << sttm;

        cur.Prepare(subst.GetResult());
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
    }


    const int cn_iot_owrflw_table_name      = 0;
    const int cn_iot_owrflw_pct_threshold   = 1;
    const int cn_iot_owrflw_include_column  = 2;
    const int cn_iot_owrflw_tablespace_name = 3;
    const int cn_iot_owrflw_ini_trans       = 4;
    const int cn_iot_owrflw_max_trans       = 5;
    const int cn_iot_owrflw_initial_extent  = 6;
    const int cn_iot_owrflw_next_extent     = 7;
    const int cn_iot_owrflw_min_extents     = 8;
    const int cn_iot_owrflw_max_extents     = 9;
    const int cn_iot_owrflw_pct_increase    = 10;
    const int cn_iot_owrflw_freelists       = 11;
    const int cn_iot_owrflw_freelist_groups = 12;
    const int cn_iot_owrflw_pct_free        = 13;
    const int cn_iot_owrflw_pct_used        = 14;
    const int cn_iot_owrflw_logging         = 15;
    const int cn_iot_owrflw_buffer_pool     = 16;

    LPSCSTR csz_iot_owrflw_sttm =
		"SELECT <RULE> "
          "v2.table_name,"
          "v1.pct_threshold,"
          "v1.include_column,"
          "v2.tablespace_name,"
          "v2.ini_trans,"
          "v2.max_trans,"
          "v2.initial_extent,"
          "v2.next_extent," 
          "v2.min_extents," 
          "v2.max_extents,"
          "v2.pct_increase,"
          "v2.freelists,"
          "v2.freelist_groups,"
          "v2.pct_free," 
          "v2.pct_used," 
          "v2.logging," 
          "v2.buffer_pool "
          // compress is not supported for IOT overflow
        "FROM sys.all_indexes v1, sys.all_tables v2 "
          "WHERE v1.index_type = \'IOT - TOP\' " 
            "AND v1.table_name = v2.iot_name "
            "AND v1.table_name = :name "
            "AND v1.table_owner = v2.owner "
            "AND v1.table_owner = :owner";

    static 
    void load_iot_overflow (OciConnect& con, Index& index, Table& table)
    {
        OciCursor cur(con, 1);
        prepare_cursor(cur, csz_iot_owrflw_sttm, index.m_strOwner.c_str(), index.m_strTableName.c_str(), false, false);
        
        cur.Execute();
        if (cur.Fetch())
        {
            Loader::Read(cur, cn_iot_owrflw_tablespace_name, index.m_IOTOverflow_Storage.m_strTablespaceName);
            Loader::Read(cur, cn_iot_owrflw_initial_extent , index.m_IOTOverflow_Storage.m_nInitialExtent );
            Loader::Read(cur, cn_iot_owrflw_next_extent    , index.m_IOTOverflow_Storage.m_nNextExtent    );
            Loader::Read(cur, cn_iot_owrflw_min_extents    , index.m_IOTOverflow_Storage.m_nMinExtents    );
            Loader::Read(cur, cn_iot_owrflw_max_extents    , index.m_IOTOverflow_Storage.m_nMaxExtents    );
            Loader::Read(cur, cn_iot_owrflw_pct_increase   , index.m_IOTOverflow_Storage.m_nPctIncrease   );
            Loader::Read(cur, cn_iot_owrflw_freelists      , index.m_IOTOverflow_Storage.m_nFreeLists     );
            Loader::Read(cur, cn_iot_owrflw_freelist_groups, index.m_IOTOverflow_Storage.m_nFreeListGroups);
            Loader::Read(cur, cn_iot_owrflw_pct_free       , index.m_IOTOverflow_Storage.m_nPctFree       );
            Loader::Read(cur, cn_iot_owrflw_pct_used       , index.m_IOTOverflow_Storage.m_nPctUsed       );
            Loader::Read(cur, cn_iot_owrflw_ini_trans      , index.m_IOTOverflow_Storage.m_nIniTrans      );
            Loader::Read(cur, cn_iot_owrflw_max_trans      , index.m_IOTOverflow_Storage.m_nMaxTrans      );
            Loader::Read(cur, cn_iot_owrflw_buffer_pool    , index.m_IOTOverflow_Storage.m_strBufferPool  );
            Loader::ReadYes(cur, cn_iot_owrflw_logging     , index.m_IOTOverflow_Storage.m_bLogging       );

            index.m_IOTOverflow_PCTThreshold  = cur.ToInt(cn_iot_owrflw_pct_threshold);
            table.m_IOTOverflow_includeColumn = cur.ToInt(cn_iot_owrflw_include_column);

            Table overflowDummy(table.GetDictionary());
            overflowDummy.m_strName = cur.ToString(cn_iot_owrflw_table_name);
            overflowDummy.m_strOwner = table.m_strOwner;
            
            load_part_info(con, overflowDummy);
            index.m_IOTOverflowPartitionDefaultStrorage = overflowDummy; // TODO: verify
        }
    }

    const int cn_part_col_name = 0;

    LPSCSTR csz_part_col_sttm =
        "SELECT column_name " 
        "FROM sys.all_part_key_columns " 
            "WHERE object_type = 'INDEX'"
            "AND owner = :owner "
            "AND name = :name "
        "ORDER BY column_position";

    const int cn_part_info_type                = 0;
    const int cn_part_info_subpart_type        = 0;
    const int cn_part_info_locality            = 5;
    const int cn_part_info_def_tablespace_name = 6;
    const int cn_part_info_def_pct_free        = 7;
    const int cn_part_info_def_ini_trans       = 8;
    const int cn_part_info_def_max_trans       = 9;
    const int cn_part_info_def_initial_extent  = 10;
    const int cn_part_info_def_next_extent     = 11;
    const int cn_part_info_def_min_extents     = 12;
    const int cn_part_info_def_max_extents     = 13;
    const int cn_part_info_def_pct_increase    = 14;
    const int cn_part_info_def_freelists       = 15;
    const int cn_part_info_def_freelist_groups = 16;
    const int cn_part_info_def_logging         = 17;
    const int cn_part_info_def_buffer_pool     = 18;
    const int cn_part_info_block_size          = 19;
    const int cn_part_info_interval            = 20;

    LPSCSTR csz_part_info_sttm =
        "SELECT "
            "v1.partitioning_type, "
            "<SUBPARTITIONING_TYPE>," 
            "<DEF_SUBPARTITION_COUNT>," 
            "v1.partitioning_key_count," 
            "<SUBPARTITIONING_KEY_COUNT>,"
            "v1.locality,"
            "Nvl(v1.def_tablespace_name, v3.default_tablespace),"
            "v1.def_pct_free,"
            "v1.def_ini_trans,"
            "v1.def_max_trans,"
            "v1.def_initial_extent,"
            "v1.def_next_extent,"
            "v1.def_min_extents,"
            "v1.def_max_extents,"
            "v1.def_pct_increase,"
            "v1.def_freelists,"
            "v1.def_freelist_groups,"
            "v1.def_logging,"
            "v1.def_buffer_pool,"
            "Nvl(v2.block_size, v4.block_size),"
            "<INTERVAL> "
        "FROM sys.all_part_indexes v1,"
        "(SELECT tablespace_name, <BLOCK_SIZE> block_size FROM sys.user_tablespaces) v2, "
        "sys.user_users v3,"
        "(SELECT tablespace_name, <BLOCK_SIZE> block_size FROM sys.user_tablespaces) v4 "             
            "WHERE v1.owner = :owner "
            "AND v1.<NAME> = :name "
            "AND v1.def_tablespace_name = v2.tablespace_name(+) "
            "AND v1.owner = v3.username(+) "
            "AND v3.default_tablespace = v4.tablespace_name(+) "
            ;

    const int cn_part_partition_name     = 0;
    const int cn_part_partition_pos      = 1;
    const int cn_part_subpartition_name  = 2;
    const int cn_part_subpartition_pos   = 3;
    const int cn_part_high_value         = 4;
    const int cn_part_composite          = 5;
    const int cn_part_tablespace_name    = 6;
    const int cn_part_pct_free           = 7;
    const int cn_part_ini_trans          = 8;
    const int cn_part_max_trans          = 9;
    const int cn_part_initial_extent     = 10;
    const int cn_part_next_extent        = 11;
    const int cn_part_min_extent         = 12;
    const int cn_part_max_extent         = 13;
    const int cn_part_pct_increase       = 14;
    const int cn_part_freelists          = 15;
    const int cn_part_freelist_groups    = 16;
    const int cn_part_logging            = 17;
    const int cn_part_compression        = 18;
    const int cn_part_buffer_pool        = 19;
    const int cn_part_record_type        = 20;

    LPSCSTR csz_part_sttm =
        "SELECT <RULE>"
            "partition_name,"
            "partition_position,"
            "To_Char(NULL) subpartition_name,"
            "To_Number(NULL) subpartition_position,"
            "high_value," 
            "<COMPOSITE>," 
            "tablespace_name," 
            "pct_free," 
            "ini_trans," 
            "max_trans," 
            "initial_extent," 
            "next_extent," 
            "min_extent," 
            "max_extent," 
            "pct_increase," 
            "freelists," 
            "freelist_groups," 
            "logging," 
            "<COMPRESSION>," 
            "buffer_pool,"
            "'P' record_type "
        "FROM sys.all_ind_partitions "         
            "WHERE index_owner = :owner "
            "AND index_name = :name "
            "<INTERVAL_FILTER_112> ";

    LPSCSTR csz_part_order_by_sttm =
        "ORDER BY partition_position";

    LPSCSTR csz_subpart_sttm =
        "UNION ALL SELECT <RULE>"
            "partition_name,"
            "To_Number(NULL) partition_position,"
            "subpartition_name,"
            "subpartition_position,"
            "high_value," 
            "NULL composite," 
            "tablespace_name," 
            "To_Number(NULL) pct_free," 
            "To_Number(NULL) ini_trans," 
            "To_Number(NULL) max_trans," 
            "To_Number(NULL) initial_extent," 
            "To_Number(NULL) next_extent," 
            "To_Number(NULL) min_extent," 
            "To_Number(NULL) max_extent," 
            "To_Number(NULL) pct_increase," 
            "To_Number(NULL) freelists," 
            "To_Number(NULL) freelist_groups," 
            "logging," 
            "NULL compression," 
            "NULL buffer_pool,"
            "'S' record_type "
        "FROM sys.all_ind_subpartitions <SUBPART_HIGH_VALUE_DUMMY>"         
            "WHERE index_owner = :owner "
            "AND index_name = :name "
            //"<INTERVAL_FILTER_112> "
        "ORDER BY record_type, partition_position, subpartition_position";


    const int cn_overflow_part_partition_name  = 0;
    const int cn_overflow_part_tablespace_name = 1;
    const int cn_overflow_part_pct_free        = 2;
    const int cn_overflow_part_pct_used        = 3;
    const int cn_overflow_part_ini_trans       = 4;
    const int cn_overflow_part_max_trans       = 5;
    const int cn_overflow_part_initial_extent  = 6;
    const int cn_overflow_part_next_extent     = 7;
    const int cn_overflow_part_min_extent      = 8;
    const int cn_overflow_part_max_extent      = 9;
    const int cn_overflow_part_pct_increase    = 10;
    const int cn_overflow_part_freelists       = 11;
    const int cn_overflow_part_freelist_groups = 12;
    const int cn_overflow_part_logging         = 13;
    const int cn_overflow_part_compression     = 14;
    const int cn_overflow_part_buffer_pool     = 15;
          
    LPSCSTR csz_overflow_part_sttm =
        "SELECT <RULE>"
          "partition_name," 
          "tablespace_name,"
          "pct_free,"
          "pct_used,"       
          "ini_trans," 
          "max_trans," 
          "initial_extent," 
          "next_extent," 
          "min_extent," 
          "max_extent," 
          "pct_increase," 
          "freelists," 
          "freelist_groups," 
          "logging," 
          "<COMPRESSION>," 
          "buffer_pool "
        "FROM sys.all_tab_partitions "
          "WHERE (table_owner, table_name) = ( "
              "SELECT tab.owner, tab.table_name "
                "FROM sys.all_tables tab, sys.all_indexes ind "
                  "WHERE ind.owner = :owner "
                    "AND ind.index_name = :name "
                    "AND ind.owner = tab.owner "
                    "AND ind.table_name = tab.iot_name "
              ") "
          "ORDER BY partition_position";

    static 
    void load_part_info  (OciConnect& con, Index& index)
    {
        index.m_PartitioningType = NONE;
        index.m_subpartitioningType = NONE;

        {
            OciCursor cur(con);
            
            prepare_cursor(cur, csz_part_info_sttm, index.m_strOwner.c_str(), index.m_strName.c_str(), false, false);
            
            cur.Execute();
            if (cur.Fetch())
            {
                string buff;
                
                cur.GetString(cn_part_info_type, buff);
                if (buff == "RANGE")     index.m_PartitioningType = RANGE;
                else if (buff == "HASH") index.m_PartitioningType = HASH;
                else if (buff == "LIST") index.m_PartitioningType = LIST;
                else index.m_PartitioningType = NONE;

                cur.GetString(cn_part_info_subpart_type, buff);
                if (buff == "RANGE")     index.m_subpartitioningType = RANGE;
                else if (buff == "HASH") index.m_subpartitioningType = HASH;
                else if (buff == "LIST") index.m_subpartitioningType = LIST;
                else index.m_subpartitioningType = NONE;

                index.m_bLocal = cur.ToString(cn_part_info_locality) == "LOCAL";
            
                Loader::Read(cur, cn_part_info_def_tablespace_name, index.m_strTablespaceName);
                if (index.m_strTablespaceName.GetValue() == "DEFAULT")
                    index.m_strTablespaceName.IsDefault();

                Loader::Read(cur, cn_part_info_def_pct_free,  index.m_nPctFree);
                Loader::Read(cur, cn_part_info_def_ini_trans, index.m_nIniTrans);
                Loader::Read(cur, cn_part_info_def_max_trans, index.m_nMaxTrans);

                if (!cur.IsNull(cn_part_info_def_initial_extent) && !cur.IsNull(cn_part_info_block_size))
                {   
                    cur.GetString(cn_part_info_def_initial_extent, buff);
                    if (buff != "DEFAULT")
                        index.m_nInitialExtent.Set(atoi(buff.c_str()) * cur.ToInt(cn_part_info_block_size));
                }

                if (!cur.IsNull(cn_part_info_def_next_extent) && !cur.IsNull(cn_part_info_block_size))
                {
                    cur.GetString(cn_part_info_def_next_extent, buff);
                    if (buff != "DEFAULT")
                        index.m_nNextExtent.Set(atoi(buff.c_str()) * cur.ToInt(cn_part_info_block_size));
                }

                if (!cur.IsNull(cn_part_info_def_min_extents))
                {
                    cur.GetString(cn_part_info_def_min_extents, buff);
                    if (buff != "DEFAULT")
                        index.m_nMinExtents.Set(atoi(buff.c_str()));
                }

                if (!cur.IsNull(cn_part_info_def_max_extents))
                {
                    cur.GetString(cn_part_info_def_max_extents, buff);
                    if (buff != "DEFAULT")
                        index.m_nMaxExtents.Set(atoi(buff.c_str()));
                }

                if (!cur.IsNull(cn_part_info_def_pct_increase))
                {
                    cur.GetString(cn_part_info_def_pct_increase, buff);
                    if (buff != "DEFAULT")
                        index.m_nPctIncrease.Set(atoi(buff.c_str()));
                }
                
                if (!cur.IsNull(cn_part_info_def_freelists))
                {
                    cur.GetString(cn_part_info_def_freelists, buff);
                    if (buff != "DEFAULT")
                        if (int val = atoi(buff.c_str()))
                            index.m_nFreeLists.Set(val);
                }

                if (!cur.IsNull(cn_part_info_def_freelist_groups))
                {
                    cur.GetString(cn_part_info_def_freelist_groups, buff);
                    if (buff != "DEFAULT")
                        if (int val = atoi(buff.c_str()))
                            index.m_nFreeListGroups.Set(val);
                }

                // 'NONE', 'YES', 'NO', 'UNKNOWN'
                if (!cur.IsNull(cn_part_info_def_logging))
                {
                    cur.GetString(cn_part_info_def_logging, buff);
                    if (buff != "NONE" && buff != "UNKNOWN")
                        index.m_bLogging.Set(Loader::IsYes(buff));
                }
                
                //  DEFAULT, KEEP, RECYCLE
                if (!cur.IsNull(cn_part_info_def_buffer_pool))
                {
                    cur.GetString(cn_part_info_def_buffer_pool, buff);
                    if (buff != "DEFAULT")
                        index.m_strBufferPool.Set(cur.ToString(cn_part_info_def_buffer_pool));
                }

                cur.GetString(cn_part_info_interval, index.m_interval);
            }
        }

        {
            OciCursor cur(con);
            
            prepare_cursor(cur, csz_part_col_sttm, index.m_strOwner.c_str(), index.m_strName.c_str(), false, false);
            
            cur.Execute();
            while (cur.Fetch())
            {
                index.m_PartKeyColumns.push_back(cur.ToString(cn_part_col_name));
            }
        }

        {
            OciCursor cur(con);
            string sttm = csz_part_sttm;
            sttm += (index.m_subpartitioningType != NONE) ? csz_subpart_sttm : csz_part_order_by_sttm;
            prepare_cursor(cur, sttm.c_str(), index.m_strOwner.c_str(), index.m_strName.c_str(), false, false);
            
            Index::PartitionPtr lastPartitionPtr;
            cur.Execute();
            while (cur.Fetch())
            {
                string record_type = cur.ToString(cn_part_record_type);

                Index::PartitionPtr partitionPtr(new IndexPartition);

                string partition_name, subpartition_name;
                cur.GetString(cn_part_partition_name,    partition_name);
                cur.GetString(cn_part_subpartition_name, subpartition_name);

                cur.GetString(cn_part_high_value,     partitionPtr->m_strHighValue);

                Loader::Read(cur, cn_part_tablespace_name, partitionPtr->m_strTablespaceName);
                Loader::Read(cur, cn_part_initial_extent , partitionPtr->m_nInitialExtent );
                Loader::Read(cur, cn_part_next_extent    , partitionPtr->m_nNextExtent    );
                Loader::Read(cur, cn_part_min_extent     , partitionPtr->m_nMinExtents    );
                Loader::Read(cur, cn_part_max_extent     , partitionPtr->m_nMaxExtents    );
                Loader::Read(cur, cn_part_pct_increase   , partitionPtr->m_nPctIncrease   );
                Loader::Read(cur, cn_part_freelists      , partitionPtr->m_nFreeLists     );
                Loader::Read(cur, cn_part_freelist_groups, partitionPtr->m_nFreeListGroups);
                Loader::Read(cur, cn_part_pct_free       , partitionPtr->m_nPctFree       );
                Loader::Read(cur, cn_part_ini_trans      , partitionPtr->m_nIniTrans      );
                Loader::Read(cur, cn_part_max_trans      , partitionPtr->m_nMaxTrans      );
                Loader::Read(cur, cn_part_buffer_pool    , partitionPtr->m_strBufferPool  );

                string logging = cur.ToString(cn_part_logging);
                if (!logging.empty() && logging != "NONE")
                    partitionPtr->m_bLogging.Set(Loader::IsYes(logging));

                //Loader::Read   (cur, cn_part_compression, partitionPtr->m_bCompression, "ENABLED");
                partitionPtr->m_bCompression.SetNull(); // oracle does not allow to change compression on partition level
                
                if (record_type == "P")
                {
                    partitionPtr->m_strName = partition_name;
                    index.m_Partitions.push_back(partitionPtr);
                }
                else if (record_type == "S")
                {
                    partitionPtr->m_strName = subpartition_name;
                    
                    if (!lastPartitionPtr.get() || lastPartitionPtr->m_strName != partition_name)
                    {
                        lastPartitionPtr.reset(0);

                        Index::PartitionContainer::const_iterator it = index.m_Partitions.begin();
                        for (; it != index.m_Partitions.end(); ++it)
                            if ((*it)->m_strName == partition_name)
                            {
                                lastPartitionPtr = *it;
                                break;
                            }
                    }
                    
                    if (lastPartitionPtr.get())
                        lastPartitionPtr->m_subpartitions.push_back(partitionPtr);
                    
                    _CHECK_AND_THROW_(lastPartitionPtr.get() || !index.m_interval.empty(), "Index subpartition loading error:\nParent partition not found!");
                }

                //index.m_Partitions.push_back(partitionPtr);
            }
        }

        if (index.m_Type == eitIOT_TOP)
        {
            OciCursor cur(con);
            
            prepare_cursor(cur, csz_overflow_part_sttm, index.m_strOwner.c_str(), index.m_strName.c_str(), false, false);
            
            cur.Execute();
            while (cur.Fetch())
            {
                Index::IOTOverflowStoragePtr partitionPtr(new TableStorage);

                Loader::Read(cur, cn_overflow_part_tablespace_name, partitionPtr->m_strTablespaceName);
                Loader::Read(cur, cn_overflow_part_initial_extent , partitionPtr->m_nInitialExtent );
                Loader::Read(cur, cn_overflow_part_next_extent    , partitionPtr->m_nNextExtent    );
                Loader::Read(cur, cn_overflow_part_min_extent     , partitionPtr->m_nMinExtents    );
                Loader::Read(cur, cn_overflow_part_max_extent     , partitionPtr->m_nMaxExtents    );
                Loader::Read(cur, cn_overflow_part_pct_increase   , partitionPtr->m_nPctIncrease   );
                Loader::Read(cur, cn_overflow_part_freelists      , partitionPtr->m_nFreeLists     );
                Loader::Read(cur, cn_overflow_part_freelist_groups, partitionPtr->m_nFreeListGroups);
                Loader::Read(cur, cn_overflow_part_pct_free       , partitionPtr->m_nPctFree       );
                Loader::Read(cur, cn_overflow_part_pct_used       , partitionPtr->m_nPctUsed       );
                Loader::Read(cur, cn_overflow_part_ini_trans      , partitionPtr->m_nIniTrans      );
                Loader::Read(cur, cn_overflow_part_max_trans      , partitionPtr->m_nMaxTrans      );
                Loader::Read(cur, cn_overflow_part_buffer_pool    , partitionPtr->m_strBufferPool  );
                Loader::ReadYes(cur, cn_overflow_part_logging,     partitionPtr->m_bLogging);

                index.m_IOTOverflowPartitions.push_back(partitionPtr);
            }
        }
    }

}// END namespace OraMetaDict
