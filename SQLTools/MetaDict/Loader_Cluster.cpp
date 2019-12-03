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

// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OCI8/BCursor.h"


// 17.09.02 bug fix: cluster reengineering fails

namespace OraMetaDict 
{
    using namespace std;
    using namespace Common;

    const int cn_owner           = 0;
    const int cn_cluster_name    = 1;
    const int cn_tablespace_name = 2;
    const int cn_pct_free        = 3;
    const int cn_pct_used        = 4;
    const int cn_key_size        = 5;
    const int cn_ini_trans       = 6;
    const int cn_max_trans       = 7;
    const int cn_initial_extent  = 8;
    const int cn_next_extent     = 9;
    const int cn_min_extents     = 10;
    const int cn_max_extents     = 11;
    const int cn_pct_increase    = 12;
    const int cn_freelists       = 13;
    const int cn_freelist_groups = 14;
    const int cn_cluster_type    = 15;
    const int cn_function        = 16;
    const int cn_hashkeys        = 17;
    const int cn_cache           = 18;
    const int cn_buffer_pool     = 19;
    const int cn_degree          = 20;
    const int cn_instances       = 21;

    LPSCSTR csz_clu_sttm =
		"SELECT <RULE> "
	        "owner,"
	        "cluster_name,"
	        "tablespace_name,"
	        "pct_free,"
            "Nvl(pct_used, 40),"
	        "key_size,"
	        "ini_trans,"
	        "max_trans,"
	        "initial_extent,"
	        "next_extent,"
	        "min_extents,"
	        "max_extents,"
	        "pct_increase,"
            "Nvl(freelists, 1),"
            "Nvl(freelist_groups, 1),"
	        "cluster_type,"
	        "function,"
	        "hashkeys,"
	        "cache,"
            "<BUFFER_POOL>,"
            "<DEGREE>,"
            "<INSTANCES> "
        "FROM sys.all_clusters "
            "WHERE owner = :owner AND cluster_name <EQUAL> :name "
        /*"ORDER BY 1,2"*/;


    const int cn_hash_expression = 2;

    LPSCSTR csz_hash_exp_sttm =
		"SELECT <RULE> "
            "owner,"
            "cluster_name,"
            "hash_expression "
        "FROM sys.all_cluster_hash_expressions "
            "WHERE owner = :owner AND cluster_name <EQUAL> :name "
        /*"ORDER BY 1,2"*/;


    const int cn_column_id      = 2;
    const int cn_column_name    = 3;
    const int cn_data_type      = 4;
    const int cn_data_length    = 5;
    const int cn_data_precision = 6;
    const int cn_data_scale     = 7;
    const int cn_nullable       = 8;

    LPSCSTR csz_clu_col_sttm =
		"SELECT <RULE> "
	        "owner,"
	        "table_name,"
	        "column_id,"
	        "column_name,"
	        "data_type,"
	        "data_length,"
	        "data_precision,"
	        "data_scale,"
	        "nullable "
        "FROM sys.all_tab_columns v1 "
	        "WHERE (owner, table_name) IN "
		        "(SELECT owner, cluster_name FROM sys.all_clusters) "
            "AND owner = :owner AND table_name <EQUAL> :name "
        /*"ORDER BY 1,2,3"*/;


    static Cluster& load_cluster     (OciCursor& cur, Dictionary& dict);
    static void     load_clu_columns (OciCursor& cur, Dictionary& dict);
    static void     prepare_cursor   (OciCursor& cur, const char* sttm, const char* owner, 
                                      const char* name, bool useLike);

void Loader::LoadClusters (const char* owner, const char* name, bool useLike)
{
    int clu_count = 0;
    {
        OciCursor cur(m_connect);
		prepare_cursor(cur, csz_clu_sttm, owner, name, useLike);
        cur.Execute();
        while (cur.Fetch()) 
        {
            Cluster& clu = load_cluster(cur, m_dictionary);
            if (!clu.m_bIsHash)
                loadIndexes(owner, clu.m_strName.c_str(), true/*byTable*/, true/*isCluster*/, false/*useLike*/);
            clu_count++;
        }
    }
    
    if (clu_count)
    {
        OciCursor cur(m_connect);
        prepare_cursor(cur, csz_clu_col_sttm, owner, name, useLike);
        cur.Execute();
        while (cur.Fetch())
            load_clu_columns(cur, m_dictionary);
    }
}

    static
    void prepare_cursor (OciCursor& cur, const char* sttm, const char* owner, 
                         const char* name, bool useLike)
    {
        OCI8::EServerVersion servVer = cur.GetConnect().GetVersion();

        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst.AddPair("<RULE>",        (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
        subst.AddPair("<BUFFER_POOL>", (servVer > OCI8::esvServer73X) ? "buffer_pool" : "'DEFAULT'");
        subst.AddPair("<DEGREE>"   ,   (servVer > OCI8::esvServer73X) ? "degree"    : "NULL");
        subst.AddPair("<INSTANCES>",   (servVer > OCI8::esvServer73X) ? "instances" : "NULL");
        subst << sttm;
        cur.Prepare(subst.GetResult());
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
    }

    static 
    void load_hash_expression (OciConnect& con, Cluster& cluster)
    {
        OciCursor cur(con, 1, 64 * 1024);
        prepare_cursor(cur, csz_hash_exp_sttm, cluster.m_strOwner.c_str(), cluster.m_strName.c_str(), false);
        
        cur.Execute();
        if (cur.Fetch())
        {
            if (cur.IsGood(cn_hash_expression))
            {
                cur.GetString(cn_hash_expression, cluster.m_strHashExpression);
                trim_symmetric(cluster.m_strHashExpression);
            }
            else
                _CHECK_AND_THROW_(0 ,"Cluster hash expression loading error:\nsize exceed 64K!");
        }
        else
            _CHECK_AND_THROW_(0 ,"Cluster loading error:\nhash expression not found!");
    }

    static 
    Cluster& load_cluster (OciCursor& cur, Dictionary& dict)
    {
        Cluster& cluster = dict.CreateCluster(cur.ToString(cn_owner).c_str(),
                                              cur.ToString(cn_cluster_name).c_str());

        cur.GetString(cn_owner, cluster.m_strOwner);
        cur.GetString(cn_cluster_name, cluster.m_strName);

        // ! Need check { cluster_type IN ('HASH','INDEX') }
        cluster.m_bIsHash = cur.ToString(cn_cluster_type) == "HASH";

        cur.GetString(cn_key_size, cluster.m_strSize);
        cur.GetString(cn_hashkeys, cluster.m_strHashKeys);

        Loader::Read(cur, cn_tablespace_name, cluster.m_strTablespaceName);
        Loader::Read(cur, cn_initial_extent , cluster.m_nInitialExtent );
        Loader::Read(cur, cn_next_extent    , cluster.m_nNextExtent    );
        Loader::Read(cur, cn_min_extents    , cluster.m_nMinExtents    );
        Loader::Read(cur, cn_max_extents    , cluster.m_nMaxExtents    );
        Loader::Read(cur, cn_pct_increase   , cluster.m_nPctIncrease   );
        Loader::Read(cur, cn_freelists      , cluster.m_nFreeLists     );
        Loader::Read(cur, cn_freelist_groups, cluster.m_nFreeListGroups);
        Loader::Read(cur, cn_pct_free       , cluster.m_nPctFree       );
        Loader::Read(cur, cn_pct_used       , cluster.m_nPctUsed       );
        Loader::Read(cur, cn_ini_trans      , cluster.m_nIniTrans      );
        Loader::Read(cur, cn_max_trans      , cluster.m_nMaxTrans      );
        Loader::Read(cur, cn_buffer_pool    , cluster.m_strBufferPool  );
        Loader::Read(cur, cn_degree         , cluster.m_nDegree        );
        Loader::Read(cur, cn_instances      , cluster.m_nInstances     );

        //TODO: verify
        //if (cluster.m_nDegree <= 0) cluster.m_nDegree = 1;
        //if (cluster.m_nInstances <= 0) cluster.m_nInstances = 1;

        // ! Need check { cache IN ('Y','N') }
        cluster.m_bCache = Loader::IsYes(cur.ToString(cn_cache));

        if (cluster.m_bIsHash)
        {
            std::string function;
            // { function IN ('DEFAULT2','HASH EXPRESSION') }
            cur.GetString(cn_function, function);

            if (function == "DEFAULT2")             // no action
                ;
            else if (function == "HASH EXPRESSION") // ok, loading
                load_hash_expression(cur.GetConnect(), cluster);
            else                                    // unknow now
                _CHECK_AND_THROW_(0 ,"Cluster loading error:\nunsupporded hash function type!");
        }

        return cluster;
    }

    static 
    void load_clu_columns (OciCursor& cur, Dictionary& dict)
    {
        Cluster& cluster = dict.LookupCluster(cur.ToString(cn_owner).c_str(),
                                              cur.ToString(cn_cluster_name).c_str());

        counted_ptr<Column> p_column(new Column());

        cur.GetString(cn_column_name, p_column->m_strColumnName);
        cur.GetString(cn_data_type, p_column->m_strDataType);

        // bug fix: cluster reengineering fails 
        p_column->m_nDataPrecision = cur.ToInt(cn_data_precision, -1);
        p_column->m_nDataScale     = cur.ToInt(cn_data_precision, -1);

        p_column->m_nDataLength    = cur.ToInt(cn_data_length);

        // ! Need check { nullable IN ('Y','N') }
        p_column->m_bNullable = Loader::IsYes(cur.ToString(cn_nullable));

        cluster.m_Columns[cur.ToInt(cn_column_id)] = p_column;
     }

}// END namespace OraMetaDict
