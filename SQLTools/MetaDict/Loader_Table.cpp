/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2007, 2017 Aleksey Kochetov

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

/*
    29.05.2002 bug fix, using number intead of integer 
    10.02.2003 bug fix, wrong declaration length of character type column for fixed charset
    14.04.2003 bug fix, Oracle9i and UTF8 data truncation
    09.02.2004 bug fix, DDL reengineering: Oracle Server 8.0.5 support (tables)
    17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
    08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
    25.02.2007 improvement, partition support
    26.09.2007 bug fix, compatibility with 8.0.X
    2011.11.06 bug fix, DDL reengineering returns wrong column size for CHAR types in AL32UTF8 database
    2011.11.06 bug fix, DDL reengineering returns zero column size for RAW type
    2017-12-31 bug fix, Oracle client version was used to control RULE hint
    2017-12-31 improvement, DDL reverse engineering handles 12c indentity columns
*/

#include "stdafx.h"
#include "config.h"
//#pragma warning ( disable : 4786 )
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OCI8/BCursor.h"

namespace OraMetaDict 
{
    using namespace std;
    using namespace Common;

    const int cn_rec_type = 0;

    const int cn_tbl_owner           = 1;
    const int cn_tbl_table_name      = 2;
    const int cn_tbl_tablespace      = 3;
    const int cn_tbl_cluster         = 4;
    const int cn_tbl_initial_extent  = 5;
    const int cn_tbl_next_extent     = 6;
    const int cn_tbl_min_extents     = 7;
    const int cn_tbl_max_extents     = 8;
    const int cn_tbl_pct_increase    = 9;
    const int cn_tbl_freelists       = 10;
    const int cn_tbl_freelist_groups = 11;
    const int cn_tbl_pct_free        = 12;
    const int cn_tbl_pct_used        = 13;
    const int cn_tbl_ini_trans       = 14;
    const int cn_tbl_max_trans       = 15;
    const int cn_tbl_cache           = 16;
    const int cn_tbl_temporary       = 17;
    const int cn_tbl_duration        = 18;
    const int cn_tbl_iot_type        = 19;
    const int cn_tbl_logging         = 20;
    const int cn_tbl_partitioned     = 21;
    const int cn_tbl_buffer_pool     = 22;
    const int cn_tbl_row_movement    = 23;
    const int cn_tbl_compression     = 24;
    const int cn_tbl_degree          = 25;
    const int cn_tbl_instances       = 26;
    const int cn_tbl_compress_for    = 27;


    const int cn_cmt_owner           = 1;
    const int cn_cmt_table_name      = 2;
    const int cn_cmt_comments        = 3;

    LPSCSTR csz_tab_sttm =
        "SELECT <RULE> 1,"
            "owner,"
            "table_name,"
            "tablespace_name,"
            "cluster_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "freelists,"
            "freelist_groups,"
            "pct_free,"
            "pct_used,"
            "ini_trans,"
            "max_trans,"
            "cache,"
            "<TEMPORARY>,"
            "<DURATION>,"
            "<IOT_TYPE>,"
            "<LOGGING>,"
            "<PARTITIONED>,"
            "<BUFFER_POOL>,"
            "<ROW_MOVEMENT>,"
            "<TAB_COMPRESSION>,"
            "<DEGREE>,"
            "<INSTANCES>,"
            "<TAB_COMPRESS_FOR>,"
            "<OBJECT_ID_TYPE>,"
            "<TABLE_TYPE_OWNER>," 
            "<TABLE_TYPE> "
        "FROM sys.<ALL_TABLES> "
        "WHERE owner = :owner AND table_name <EQUAL> :name "
            "<AND_IOT_NAME_IS_NULL>"
        "UNION ALL "
        "SELECT 2,"
            "owner,"
            "table_name,"
            "comments,"
            "NULL,"
            "To_Number(NULL),"
            "To_Number(NULL),"
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
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL,"
            "NULL "
        "FROM sys.all_tab_comments "
        "WHERE owner = :owner AND table_name <EQUAL> :name "
            "AND table_type = 'TABLE' "
            "<AND_NOT_RECYCLEBIN>"
        "ORDER BY 1";

    const int cn_col_owner          = 0;
    const int cn_col_table_name     = 1;
    const int cn_col_column_id      = 2;
    const int cn_col_column_name    = 3;
    const int cn_col_data_type      = 4;
    const int cn_col_data_precision = 5;
    const int cn_col_data_scale     = 6;
    const int cn_col_data_length    = 7;
    const int cn_char_length        = 8;
    const int cn_col_nullable       = 9;
    const int cn_col_data_default   = 10;
    const int cn_col_comments       = 11;
    const int cn_col_char_used      = 12;
    const int cn_col_virtual_column = 13;
    const int cn_col_identity_column= 14;

    LPSCSTR csz_col_sttm =
        "SELECT <RULE> "
            "v1.owner,"
            "v1.table_name,"
            "v1.column_id,"
            "v1.column_name,"
            "v1.data_type,"
            "v1.data_precision,"
            "v1.data_scale,"
            "v1.data_length,"
            "<V1_CHAR_LENGTH>,"
            "v1.nullable,"
            "v1.data_default,"
            "v2.comments,"
            "<V1_CHAR_USED>,"
            "<VIRTUAL_COLUMN>, "
            "<IDENTITY_COLUMN> "
        "FROM <ALL_TAB_COL_VIEW> v1, sys.all_col_comments v2, sys.all_tables v3 "
        "WHERE v1.owner = :owner AND v1.table_name <EQUAL> :name "
            "AND v1.owner = v2.owner AND v1.table_name = v2.table_name "
            "AND v1.owner = v3.owner AND v1.table_name = v3.table_name "
            "AND v1.column_name = v2.column_name";

    const int cn_clu_tab_col_name = 0;
    const int cn_clu_column_id    = 1;

    LPSCSTR csz_clu_col_sttm =
        "SELECT <RULE> "
            //"<OWNER>,"                                // if (dba) then "c.owner" else "USER"
            //"c.cluster_name,"
            //"c.table_name," 
            "c.tab_column_name," 
            //"c.clu_column_name," 
            "t.column_id "
        "FROM sys.<SRC>_tab_columns t, sys.<SRC>_clu_columns c "// if (dba) then "dba" else "user"
        "WHERE t.table_name = c.cluster_name "
            "AND t.column_name = c.clu_column_name "
            "AND c.cluster_name = :clu_name "
            "AND c.table_name = :tab_name " 
            "<OWNER_EXPRS>";                            // if (dba) then "AND t.owner = c.owner "
                                                        //               "AND c.owner = :owner"
    static void prepare_cursor    (OciCursor&, const char* sttm, const char* owner, const char* name, bool useLike);
    static void load_table_body   (OciCursor&, Dictionary&, bool useDba);
    static void load_table_column (OciCursor&, Dictionary&, bool& indentityEncountered);
    static void load_clu_columns  (OciConnect&, Table&, bool useDba);
    void load_part_info (OciConnect&, Table&);

void Loader::LoadTables (const char* owner, const char* name, bool useLike)
{
    {
#if TEST_LOADING_OVERFLOW
    OciCursor cur(m_connect, 50, 32);
#else
    OciCursor cur(m_connect, 50, 196);
#endif
    
    prepare_cursor(cur, csz_tab_sttm, owner, name, useLike);

    bool useDba = m_strCurSchema.compare(owner) ? true : false;
    
    cur.Execute();
    while (cur.Fetch())
        load_table_body(cur, m_dictionary, useDba);
    }

    bool indentityEncountered = false;
    {
#if TEST_LOADING_OVERFLOW
    OciCursor cur(m_connect, 50, 32);
#else
    OciCursor cur(m_connect, 50, 196);
#endif
    
    prepare_cursor(cur, csz_col_sttm, owner, name, useLike);

    cur.Execute();
    while (cur.Fetch())
        load_table_column(cur, m_dictionary, indentityEncountered);
    }

    loadIndexes(owner, name, true/*byTable*/, false/*isCluster*/, useLike);
    loadConstraints(owner, name, true/*byTable*/, useLike);
    if (indentityEncountered)
    {
        loadSequences(owner, name, true/*byTable*/, useLike);
    }
}

    static
    bool is_tab_compressin_supported (OciConnect& con)
    {
        OCI8::EServerVersion servVer = con.GetVersion();
        
        if (servVer < OCI8::esvServer9X)
            return false;
        else if (servVer > OCI8::esvServer9X)
            return true;
        else
        {
            const char* cookie = "sys.all_tables.compression";
            const char* yes = "1", * no = "0";

            string val;
            if (!con.GetSessionCookie(cookie, val))
            {
                try // some 9i releases might not have this column
                {
                    OciStatement cursor(con);
                    cursor.Prepare("SELECT compression FROM sys.all_tables where 1=2");
                    cursor.Execute();
                    con.SetSessionCookie(cookie, yes);
                    return true;
                }
                catch (OciException&)
                {
                    con.SetSessionCookie(cookie, no);
                    return false;
                }
            }
            else
            {
                return val == yes;
            }
        }
    }

    static
    void prepare_cursor (OciCursor& cur, const char* sttm, const char* owner, const char* name, bool useLike)
    {
        OCI8::EServerVersion servVer = cur.GetConnect().GetVersion();

        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst.AddPair("<RULE>",      (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
        subst.AddPair("<DURATION>",  (servVer > OCI8::esvServer80X) ? "duration" : "NULL");

        subst.AddPair("<TEMPORARY>", (servVer > OCI8::esvServer73X) ? "temporary" : "NULL");
        subst.AddPair("<IOT_TYPE>",  (servVer > OCI8::esvServer73X) ? "iot_type"  : "NULL");
        subst.AddPair("<LOGGING>",   (servVer > OCI8::esvServer73X) ? "logging"   : "'YES'");

        subst.AddPair("<PARTITIONED>", (servVer > OCI8::esvServer73X) ? "partitioned" : "'NO'");
        subst.AddPair("<BUFFER_POOL>", (servVer > OCI8::esvServer73X) ? "buffer_pool" : "'DEFAULT'");
        
        subst.AddPair("<COMPOSITE>",  (servVer > OCI8::esvServer80X) ? "composite" : "NULL");
        subst.AddPair("<BLOCK_SIZE>", (servVer > OCI8::esvServer81X) ? "block_size" : "NULL");

        subst.AddPair("<INTERVAL>",             (servVer >= OCI8::esvServer11X) ? "interval" : "NULL");
        subst.AddPair("<INTERVAL_FILTER_112>" , (servVer >= OCI8::esvServer112) ? "AND Nvl(interval, 'NO') <> 'YES'"  : "");
        
        subst.AddPair("<SUBPARTITIONING_TYPE>"     , (servVer > OCI8::esvServer80X)  ? "subpartitioning_type" :      "NULL");
        subst.AddPair("<DEF_SUBPARTITION_COUNT>"   , (servVer > OCI8::esvServer80X)  ? "def_subpartition_count" :    "NULL");
        subst.AddPair("<SUBPARTITIONING_KEY_COUNT>", (servVer > OCI8::esvServer80X)  ? "subpartitioning_key_count" : "NULL");
        subst.AddPair("<DEF_COMPRESSION>"          , (servVer > OCI8::esvServer81X)  ? "def_compression" :           "NULL");
        subst.AddPair("<DEF_COMPRESS_FOR>"         , (servVer >= OCI8::esvServer11X) ? "def_compress_for" :          "NULL");

        subst.AddPair("<ROW_MOVEMENT>"              , (servVer >= OCI8::esvServer9X)  ? "row_movement" : "'DISABLED'");
        subst.AddPair("<PART_COMPRESSION>"          , (servVer >= OCI8::esvServer9X)  ? "compression"  : "'DISABLED'");
        subst.AddPair("<PART_COMPRESS_FOR>"         , (servVer >= OCI8::esvServer11X) ? "compress_for" : "NULL");
        
        subst.AddPair("<SUBPART_HIGH_VALUE_DUMMY>"  , 
            (servVer ==  OCI8::esvServer81X) ? 
            ",(SELECT data_default high_value FROM all_tab_columns WHERE owner = 'SYS' AND table_name = 'DUAL' AND column_name = 'DUMMY')" 
            : ""
          );
        subst.AddPair("<OBJECT_TYPE_EQ_TABLE>",  (servVer ==  OCI8::esvServer81X) ?  "Trim(object_type) = 'TABLE'"  : "object_type = 'TABLE'");

        // // some 9i releases might not have this column
        subst.AddPair("<TAB_COMPRESSION>",  is_tab_compressin_supported(cur.GetConnect()) ? "compression" : "NULL"); 
        subst.AddPair("<TAB_COMPRESS_FOR>", (servVer >=  OCI8::esvServer11X) ? "compress_for" : "NULL"); 

        subst.AddPair("<AND_IOT_NAME_IS_NULL>", (servVer > OCI8::esvServer73X) ?  "AND iot_name IS NULL " : "");

        if (servVer >= OCI8::esvServer9X)
            subst.AddPair("<V1_CHAR_LENGTH>", "v1.char_length");
        else if (servVer > OCI8::esvServer73X)
            subst.AddPair("<V1_CHAR_LENGTH>", "v1.char_col_decl_length");
        else
            subst.AddPair("<V1_CHAR_LENGTH>", "NULL");

        subst.AddPair("<V1_CHAR_USED>", (servVer >= OCI8::esvServer9X) ?  "v1.char_used" : "NULL");
        // B#1407225, Skip objects from recyclebin showed in 'SYS.ALL_TAB_COMMENTS' (unfortunately we have only table_name!)
        // If we want to do it 100% properly, we should MINUS with 'SELECT object_name FROM recyclebin WHERE type='TABLE'', 
        // but I think that current solution is good enough.
        subst.AddPair("<AND_NOT_RECYCLEBIN>", (servVer >= OCI8::esvServer10X) ? "AND table_name NOT LIKE 'BIN$%' " : "");
        // remove RULE hint from queries (above 9i databases)

        subst.AddPair("<DEGREE>"   , (servVer > OCI8::esvServer73X) ? "degree"    : "NULL");
        subst.AddPair("<INSTANCES>", (servVer > OCI8::esvServer73X) ? "instances" : "NULL");

        subst.AddPair("<OBJECT_ID_TYPE>"  , (servVer > OCI8::esvServer80X) ? "object_id_type"   : "NULL");
        subst.AddPair("<TABLE_TYPE_OWNER>", (servVer > OCI8::esvServer80X) ? "table_type_owner" : "NULL");
        subst.AddPair("<TABLE_TYPE>"      , (servVer > OCI8::esvServer80X) ? "table_type"       : "NULL");
        subst.AddPair("<ALL_TABLES>"      , (servVer > OCI8::esvServer80X) ? "all_all_tables"   : "all_tables");

        subst.AddPair("<ALL_TAB_COL_VIEW>", (servVer >= OCI8::esvServer11X) ? "(SELECT * FROM sys.all_tab_cols WHERE hidden_column = 'NO')" : "sys.all_tab_columns");
        subst.AddPair("<VIRTUAL_COLUMN>"  , (servVer >= OCI8::esvServer11X) ? "virtual_column"   : "NULL");
        subst.AddPair("<IDENTITY_COLUMN>" , (servVer >= OCI8::esvServer12X) ? "identity_column"  : "NULL");

        subst << sttm;

        cur.Prepare(subst.GetResult());
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
    }

    static
    void load_table_body (OciCursor& cur, Dictionary& dict, bool useDba)
    {
        if (cur.ToInt(cn_rec_type) == 1)
        {
            Table& table = dict.CreateTable(cur.ToString(cn_tbl_owner).c_str(), cur.ToString(cn_tbl_table_name).c_str());
            
            cur.GetString(cn_tbl_owner, table.m_strOwner);
            cur.GetString(cn_tbl_table_name, table.m_strName);

            if (Loader::IsYes(cur.ToString(cn_tbl_temporary)))
            {
                table.m_TableType = ettTemporary;

                if (cur.ToString(cn_tbl_duration) == "SYS$TRANSACTION")
                    table.m_TemporaryTableDuration = ettdTransaction;
                else if (cur.ToString(cn_tbl_duration) == "SYS$SESSION")
                    table.m_TemporaryTableDuration = ettdSession;
                else
                    _CHECK_AND_THROW_(0, "Table loading error:\nunknown temporary table duration!");
            }
            else if (cur.IsNull(cn_tbl_iot_type))
            {
                table.m_TableType = ettNormal;

                Loader::Read(cur, cn_tbl_tablespace     , table.m_strTablespaceName);
                Loader::Read(cur, cn_tbl_initial_extent , table.m_nInitialExtent   );
                Loader::Read(cur, cn_tbl_next_extent    , table.m_nNextExtent      );
                Loader::Read(cur, cn_tbl_min_extents    , table.m_nMinExtents      );
                Loader::Read(cur, cn_tbl_max_extents    , table.m_nMaxExtents      );
                Loader::Read(cur, cn_tbl_pct_increase   , table.m_nPctIncrease     );
                Loader::Read(cur, cn_tbl_freelists      , table.m_nFreeLists       );
                Loader::Read(cur, cn_tbl_freelist_groups, table.m_nFreeListGroups  );
                Loader::Read(cur, cn_tbl_pct_free       , table.m_nPctFree         );
                Loader::Read(cur, cn_tbl_pct_used       , table.m_nPctUsed         );
                Loader::Read(cur, cn_tbl_ini_trans      , table.m_nIniTrans        );
                Loader::Read(cur, cn_tbl_max_trans      , table.m_nMaxTrans        );
                Loader::Read(cur, cn_tbl_buffer_pool    , table.m_strBufferPool    );
                
                Loader::ReadYes(cur, cn_tbl_logging     , table.m_bLogging         ); // table.m_bLogging = Loader::IsYes(cur.ToString(cn_tbl_logging, "YES"));
                Loader::Read(cur, cn_tbl_compression    , table.m_bCompression, "ENABLED");
                Loader::Read(cur, cn_tbl_row_movement   , table.m_bRowMovement, "ENABLED");
                Loader::Read(cur, cn_tbl_degree         , table.m_nDegree          );
                Loader::Read(cur, cn_tbl_instances      , table.m_nInstances       );

                if (!cur.IsNull(cn_tbl_compress_for))
                {
                    string buff;
                    cur.GetString(cn_tbl_compress_for, buff);
                    if (buff == "BASIC")
                        table.m_strCompressFor.Set(buff);
                    else if (buff == "OLTP")
                        table.m_strCompressFor.Set("FOR OLTP");
                    else 
                        table.m_strCompressFor.Set(buff + " /*NOT TESTED*/");
                }

                cur.GetString(cn_tbl_cluster, table.m_strClusterName);
                if (!table.m_strClusterName.empty())
                    load_clu_columns(cur.GetConnect(), table, useDba);
            }
            else if (cur.ToString(cn_tbl_iot_type) == "IOT")
            {
                table.m_TableType = ettIOT;
            }
            else
                _CHECK_AND_THROW_(0, "Table loading error:\nunknown table type!");
            
            // TODO: why do we have cn_tbl_logging twice?
            Loader::ReadYes(cur, cn_tbl_cache     , table.m_bCache           );
            Loader::ReadYes(cur, cn_tbl_logging   , table.m_bLogging         );

            if (Loader::IsYes(cur.ToString(cn_tbl_partitioned)))
                load_part_info(cur.GetConnect(), table);
        }
        else
        {
            Table& table = dict.LookupTable(cur.ToString(cn_cmt_owner).c_str(), cur.ToString(cn_cmt_table_name).c_str());

            if (cur.IsGood(cn_cmt_comments))
                cur.GetString(cn_cmt_comments, table.m_strComments);
            else 
            {
                OciCursor ovrfCur(cur.GetConnect(), "SELECT comments FROM sys.all_tab_comments WHERE owner = :owner AND table_name = :name", 1, 4 * 1024);
                
                ovrfCur.Bind(":owner", table.m_strOwner.c_str());
                ovrfCur.Bind(":name",  table.m_strName.c_str());

                ovrfCur.Execute();
                ovrfCur.Fetch();

                _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Table comments loading error:\nsize exceed 4K!");
                
                ovrfCur.GetString(0, table.m_strComments);
            }
        }
    }

    static
    void load_table_column (OciCursor& cur, Dictionary& dict, bool& indentityEncountered)
    {
        CharLengthSemantics defCharLengthSemantics = cur.GetConnect().GetNlsLengthSemantics() == "CHAR" ? USE_CHAR : USE_BYTE;

        counted_ptr<TabColumn> p_column(new TabColumn());

        cur.GetString(cn_col_column_name, p_column->m_strColumnName);
        cur.GetString(cn_col_data_type, p_column->m_strDataType);

        // bug fix, using number intead of integer 
        p_column->m_nDataPrecision = cur.IsNull(cn_col_data_precision) ? -1 : cur.ToInt(cn_col_data_precision);
        p_column->m_nDataScale     = cur.IsNull(cn_col_data_scale) ? -1 : cur.ToInt(cn_col_data_scale);

        // bug fix, wrong declaration length of character type column for fixed charset
        if (!cur.IsNull(cn_char_length) && cur.ToInt(cn_char_length) != 0)
            p_column->m_nDataLength = cur.ToInt(cn_char_length);
        else
            p_column->m_nDataLength = cur.ToInt(cn_col_data_length);

        p_column->m_defCharLengthSemantics = defCharLengthSemantics;
        if (cur.ToString(cn_col_char_used) == "C")
            p_column->m_charLengthSemantics = USE_CHAR;

        p_column->m_bNullable = Loader::IsYes(cur.ToString(cn_col_nullable));
        p_column->m_bVirtual  = (p_column->m_strDataType != "XMLTYPE") ? Loader::IsYes(cur.ToString(cn_col_virtual_column)) : false;
        p_column->m_bIdentity = Loader::IsYes(cur.ToString(cn_col_identity_column));

        if (p_column->m_bIdentity)
            indentityEncountered = true;

        if (cur.IsGood(cn_col_comments) && cur.IsGood(cn_col_data_default))
        {
            cur.GetString(cn_col_comments, p_column->m_strComments);
            cur.GetString(cn_col_data_default, p_column->m_strDataDefault);
        } 
        else 
        {
            OciCursor ovrfCur(
                cur.GetConnect(),
                "SELECT v2.comments, v1.data_default "
                    "FROM sys.all_tab_columns v1, sys.all_col_comments v2 "
                    "WHERE v1.owner = v2.owner "
                        "AND v1.table_name = v2.table_name "
                        "AND v1.column_name = v2.column_name "
                        "AND v1.owner = :owner "
                        "AND v1.table_name = :name "
                        "AND v1.column_name = :col_name",
                1,
                4 * 1024
                );

            ovrfCur.Bind(":owner",    cur.ToString(cn_col_owner).c_str());
            ovrfCur.Bind(":name",     cur.ToString(cn_col_table_name).c_str());
            ovrfCur.Bind(":col_name", p_column->m_strColumnName.c_str());

            ovrfCur.Execute();
            ovrfCur.Fetch();

            _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Column comments loading error:\nsize exceed 4K!");
            _CHECK_AND_THROW_(ovrfCur.IsGood(1), "Column default data value loading error:\nsize exceed 4K!");
            
            ovrfCur.GetString(0 ,p_column->m_strComments);
            ovrfCur.GetString(1, p_column->m_strDataDefault);
        }

        trim_symmetric(p_column->m_strDataDefault);

        Table& table = dict.LookupTable(cur.ToString(cn_col_owner).c_str(), cur.ToString(cn_col_table_name).c_str());
        table.m_Columns[cur.ToInt(cn_col_column_id) - 1] = p_column;
    }

    static
    void load_clu_columns (OciConnect& con, Table& table, bool useDba)
    {
        try
        {
            Common::Substitutor subst;

            subst.AddPair("<RULE>",        con.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
            subst.AddPair("<SRC>",         useDba  ? "dba" : "user");
            subst.AddPair("<OWNER_EXPRS>", useDba  ? "AND t.owner = c.owner AND c.owner = :owner" : "");

            subst << csz_clu_col_sttm;

            OciCursor cur(con, subst.GetResult(), 20);

            if (useDba) cur.Bind(":owner", table.m_strOwner.c_str());
            cur.Bind(":clu_name", table.m_strClusterName.c_str());
            cur.Bind(":tab_name", table.m_strName.c_str());

            cur.Execute();
            while (cur.Fetch())
                table.m_clusterColumns[cur.ToInt(cn_clu_column_id) - 1] = cur.ToString(cn_clu_tab_col_name);
        }
        catch (const OciException& x)
        {
            if (useDba && x == 942)
            {
                _CHECK_AND_THROW_(0, "DBA_TAB_COLUMNS or DBA_CLU_COLUMNS is not accessble! "
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

    const int cn_part_col_name = 0;
    const int cn_part_col_type = 1;

    LPSCSTR csz_part_col_sttm =
        "SELECT column_name, 'P' key_type, column_position " 
        "FROM sys.all_part_key_columns " 
            "WHERE <OBJECT_TYPE_EQ_TABLE>"
            "AND owner = :owner "
            "AND name = :name ";

    LPSCSTR csz_part_col_order_by_sttm =
        "ORDER BY column_position";

    LPSCSTR csz_subpart_col_sttm =
        "UNION ALL SELECT column_name, 'S' key_type, column_position " 
        "FROM sys.all_subpart_key_columns " 
            "WHERE <OBJECT_TYPE_EQ_TABLE>"
            "AND owner = :owner "
            "AND name = :name "
        "ORDER BY key_type, column_position";

    const int cn_part_info_type                = 0;
    const int cn_part_info_subpart_type        = 1;
    const int cn_part_info_def_tablespace_name = 5;
    const int cn_part_info_def_pct_free        = 6;
    const int cn_part_info_def_pct_used        = 7;
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
    const int cn_part_info_def_compression     = 18;
    const int cn_part_info_def_buffer_pool     = 19;
    const int cn_part_info_block_size          = 20;
    const int cn_part_info_interval            = 21;
    const int cn_part_info_def_compress_for    = 22;

    LPSCSTR csz_part_info_sttm =
        "SELECT "
            "v1.partitioning_type, "
            "<SUBPARTITIONING_TYPE>," 
            "<DEF_SUBPARTITION_COUNT>," 
            "v1.partitioning_key_count," 
            "<SUBPARTITIONING_KEY_COUNT>,"
            "v1.def_tablespace_name,"
            "v1.def_pct_free,"
            "v1.def_pct_used,"
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
            "<DEF_COMPRESSION>,"
            "v1.def_buffer_pool,"
            "<BLOCK_SIZE>,"
            "<INTERVAL>,"
            "<DEF_COMPRESS_FOR> "
        "FROM sys.all_part_tables v1, sys.user_tablespaces v2 "             
            "WHERE v1.owner = :owner "
            "AND v1.table_name = :name "
            "AND v1.def_tablespace_name = v2.tablespace_name(+)";

    const int cn_part_partition_name    = 0;
    const int cn_part_partition_pos     = 1;
    const int cn_part_subpartition_name = 2;
    const int cn_part_subpartition_pos  = 3;
    const int cn_part_high_value        = 4;
    const int cn_part_composite         = 5;
    const int cn_part_tablespace_name   = 6;
    const int cn_part_pct_free          = 7;
    const int cn_part_pct_used          = 8;
    const int cn_part_ini_trans         = 9;
    const int cn_part_max_trans         = 10;
    const int cn_part_initial_extent    = 11;
    const int cn_part_next_extent       = 12;
    const int cn_part_min_extent        = 13;
    const int cn_part_max_extent        = 14;
    const int cn_part_pct_increase      = 15;
    const int cn_part_freelists         = 16;
    const int cn_part_freelist_groups   = 17;
    const int cn_part_logging           = 18;
    const int cn_part_compression       = 19;
    const int cn_part_compress_for      = 20;
    const int cn_part_buffer_pool       = 21;
    const int cn_part_record_type       = 22;
    const int cn_part_block_size        = 23;
                                        
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
            "<PART_COMPRESSION>," 
            "<PART_COMPRESS_FOR>," 
            "buffer_pool,"
            "'P' record_type,"
            "<BLOCK_SIZE> "
        "FROM sys.all_tab_partitions, "
            "(SELECT tablespace_name tbs_tablespace_name, <BLOCK_SIZE> FROM sys.user_tablespaces) "   
            "WHERE table_owner = :owner "
            "AND table_name = :name "
            "AND tablespace_name = tbs_tablespace_name(+) "
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
            "NULL," 
            "tablespace_name," 
            "To_Number(NULL) pct_free," 
            "To_Number(NULL) pct_used," 
            "To_Number(NULL) ini_trans," 
            "To_Number(NULL) max_trans," 
            "To_Number(NULL) initial_extent," 
            "To_Number(NULL) next_extent," 
            "To_Number(NULL) min_extent," 
            "To_Number(NULL) max_extent," 
            "To_Number(NULL) pct_increase," 
            "To_Number(NULL) freelists," 
            "To_Number(NULL) freelist_groups," 
            "NULL logging," 
            "NULL,"//"<PART_COMPRESSION>," 
            "NULL,"//"<PART_COMPRESS_FOR>," 
            "NULL buffer_pool,"
            "'S' record_type,"
            "NULL block_size "
        "FROM sys.all_tab_subpartitions <SUBPART_HIGH_VALUE_DUMMY> "   
            "WHERE table_owner = :owner "
            "AND table_name = :name "
            "<INTERVAL_FILTER_112> ";

    LPSCSTR csz_subpart_templ_sttm =
        "UNION ALL SELECT <RULE>"
            "NULL partition_name,"
            "NULL partition_position," 
            "subpartition_name,"
            "subpartition_position,"
            "high_bound high_value," 
            "NULL," 
            "tablespace_name," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL," 
            "NULL,"
            "'T' record_type,"
            "NULL block_size "
        "FROM sys.all_subpartition_templates "   
            "WHERE user_name = :owner "
            "AND table_name = :name ";

    LPSCSTR csz_part_with_sub_order_by_sttm =
        "ORDER BY record_type, partition_position, subpartition_position";

    // the procedure is used also for partitioned iot overflow  
    void load_part_info  (OciConnect& con, Table& table)
    {
        table.m_PartitioningType = NONE;
        table.m_subpartitioningType = NONE;

        {
            OciCursor cur(con);
            
            prepare_cursor(cur, csz_part_info_sttm, table.m_strOwner.c_str(), table.m_strName.c_str(), false);
            
            cur.Execute();
            if (cur.Fetch())
            {
                string buff;

                cur.GetString(cn_part_info_type, buff);
                if (buff == "RANGE")     table.m_PartitioningType = RANGE;
                else if (buff == "HASH") table.m_PartitioningType = HASH;
                else if (buff == "LIST") table.m_PartitioningType = LIST;
                else table.m_PartitioningType = NONE;

                cur.GetString(cn_part_info_subpart_type, buff);
                if (buff == "RANGE")     table.m_subpartitioningType = RANGE;
                else if (buff == "HASH") table.m_subpartitioningType = HASH;
                else if (buff == "LIST") table.m_subpartitioningType = LIST;
                else table.m_subpartitioningType = NONE;

                Loader::Read(cur, cn_part_info_def_tablespace_name, table.m_strTablespaceName);
                if (table.m_strTablespaceName.GetValue() == "DEFAULT")
                    table.m_strTablespaceName.IsDefault();

                Loader::Read(cur, cn_part_info_def_pct_free,  table.m_nPctFree);
                Loader::Read(cur, cn_part_info_def_pct_used,  table.m_nPctUsed);
                Loader::Read(cur, cn_part_info_def_ini_trans, table.m_nIniTrans);
                Loader::Read(cur, cn_part_info_def_max_trans, table.m_nMaxTrans);

                if (!cur.IsNull(cn_part_info_def_initial_extent) && !cur.IsNull(cn_part_info_block_size))
                {   
                    cur.GetString(cn_part_info_def_initial_extent, buff);
                    if (buff != "DEFAULT")
                        table.m_nInitialExtent.Set(atoi(buff.c_str()) * cur.ToInt(cn_part_info_block_size));
                }

                if (!cur.IsNull(cn_part_info_def_next_extent) && !cur.IsNull(cn_part_info_block_size))
                {
                    cur.GetString(cn_part_info_def_next_extent, buff);
                    if (buff != "DEFAULT")
                        table.m_nNextExtent.Set(atoi(buff.c_str()) * cur.ToInt(cn_part_info_block_size));
                }

                if (!cur.IsNull(cn_part_info_def_min_extents))
                {
                    cur.GetString(cn_part_info_def_min_extents, buff);
                    if (buff != "DEFAULT")
                        table.m_nMinExtents.Set(atoi(buff.c_str()));
                }

                if (!cur.IsNull(cn_part_info_def_max_extents))
                {
                    cur.GetString(cn_part_info_def_max_extents, buff);
                    if (buff != "DEFAULT")
                        table.m_nMaxExtents.Set(atoi(buff.c_str()));
                }

                if (!cur.IsNull(cn_part_info_def_pct_increase))
                {
                    cur.GetString(cn_part_info_def_pct_increase, buff);
                    if (buff != "DEFAULT")
                        table.m_nPctIncrease.Set(atoi(buff.c_str()));
                }
                
                if (!cur.IsNull(cn_part_info_def_freelists))
                {
                    cur.GetString(cn_part_info_def_freelists, buff);
                    if (buff != "DEFAULT")
                        if (int val = atoi(buff.c_str()))
                            table.m_nFreeLists.Set(val);
                }

                if (!cur.IsNull(cn_part_info_def_freelist_groups))
                {
                    cur.GetString(cn_part_info_def_freelist_groups, buff);
                    if (buff != "DEFAULT")
                        if (int val = atoi(buff.c_str()))
                            table.m_nFreeListGroups.Set(val);
                }

                // 'NONE', 'YES', 'NO', 'UNKNOWN'
                if (!cur.IsNull(cn_part_info_def_logging))
                {
                    cur.GetString(cn_part_info_def_logging, buff);
                    if (buff != "NONE" && buff != "UNKNOWN")
                        table.m_bLogging.Set(Loader::IsYes(buff));
                }
                
                // 'NONE', 'YES', 'NO', 'UNKNOWN'
                // 'NONE', 'ENABLED', 'DISABLED', 'UNKNOWN' for 10g and 11g
                if (!cur.IsNull(cn_part_info_def_compression))
                {
                    cur.GetString(cn_part_info_def_compression, buff);
                    if (Loader::IsYes(buff) || buff == "ENABLED")
                        table.m_bCompression.Set(true);
                }

                if (!cur.IsNull(cn_part_info_def_compress_for))
                {
                    string buff;
                    cur.GetString(cn_part_info_def_compress_for, buff);
                    if (buff == "BASIC")
                        table.m_strCompressFor.Set(buff);
                    else if (buff == "OLTP")
                        table.m_strCompressFor.Set("FOR OLTP");
                    else 
                        table.m_strCompressFor.Set(buff + " /*NOT TESTED*/");
                }

                //  DEFAULT, KEEP, RECYCLE
                if (!cur.IsNull(cn_part_info_def_buffer_pool))
                {
                    cur.GetString(cn_part_info_def_buffer_pool, buff);
                    if (buff != "DEFAULT")
                        table.m_strBufferPool.Set(buff);
                }

                if (!cur.IsNull(cn_part_info_interval))
                    cur.GetString(cn_part_info_interval, table.m_interval);
            }
        }

        {
            OciCursor cur(con);

            string sttm(csz_part_col_sttm);
            sttm += (table.m_subpartitioningType == NONE) ? csz_part_col_order_by_sttm : csz_subpart_col_sttm;
            prepare_cursor(cur, sttm.c_str(), table.m_strOwner.c_str(), table.m_strName.c_str(), false);
            cur.Execute();

            while (cur.Fetch())
            {
                if (cur.ToString(cn_part_col_type) == "P")
                    table.m_PartKeyColumns.push_back(cur.ToString(cn_part_col_name));
                else
                    table.m_subpartKeyColumns.push_back(cur.ToString(cn_part_col_name));
            }
        }

        {
            OciCursor cur(con);
            string sttm = csz_part_sttm;
      
            if (table.m_subpartitioningType != NONE)
            {
                sttm += csz_subpart_sttm;
                if (con.GetVersion() > OCI8::esvServer81X) 
                    sttm += csz_subpart_templ_sttm;
                sttm += csz_part_with_sub_order_by_sttm;
            }
            else
                sttm += csz_part_order_by_sttm;

            prepare_cursor(cur, sttm.c_str(), table.m_strOwner.c_str(), table.m_strName.c_str(), false);
            
            Table::PartitionPtr lastPartitionPtr;
            cur.Execute();
            while (cur.Fetch())
            {
                Table::PartitionPtr partitionPtr(new Partition);
                
                string partition_name, subpartition_name;
                cur.GetString(cn_part_partition_name,    partition_name);
                cur.GetString(cn_part_subpartition_name, subpartition_name);
                cur.GetString(cn_part_high_value,      partitionPtr->m_strHighValue);
                
                Loader::Read(cur, cn_part_tablespace_name, partitionPtr->m_strTablespaceName);

                Loader::Read(cur, cn_part_initial_extent  , partitionPtr->m_nInitialExtent );
                Loader::Read(cur, cn_part_next_extent     , partitionPtr->m_nNextExtent    );
                Loader::Read(cur, cn_part_min_extent      , partitionPtr->m_nMinExtents    );
                Loader::Read(cur, cn_part_max_extent      , partitionPtr->m_nMaxExtents    );
                Loader::Read(cur, cn_part_pct_increase    , partitionPtr->m_nPctIncrease   );
                Loader::Read(cur, cn_part_freelists       , partitionPtr->m_nFreeLists     );
                Loader::Read(cur, cn_part_freelist_groups , partitionPtr->m_nFreeListGroups);
                Loader::Read(cur, cn_part_pct_free        , partitionPtr->m_nPctFree       );
                Loader::Read(cur, cn_part_pct_used        , partitionPtr->m_nPctUsed       );
                Loader::Read(cur, cn_part_ini_trans       , partitionPtr->m_nIniTrans      );
                Loader::Read(cur, cn_part_max_trans       , partitionPtr->m_nMaxTrans      );
                Loader::Read(cur, cn_part_buffer_pool     , partitionPtr->m_strBufferPool  );
                
                string logging = cur.ToString(cn_part_logging);
                if (!logging.empty() && logging != "NONE")
                    partitionPtr->m_bLogging.Set(Loader::IsYes(logging));

                Loader::Read(cur, cn_part_compression     , partitionPtr->m_bCompression, "ENABLED");

                if (!cur.IsNull(cn_part_compress_for))
                {
                    string buff;
                    cur.GetString(cn_part_compress_for, buff);
                    if (buff == "BASIC")
                        partitionPtr->m_strCompressFor.Set(buff);
                    else if (buff == "OLTP")
                        partitionPtr->m_strCompressFor.Set("FOR OLTP");
                    else 
                        partitionPtr->m_strCompressFor.Set(buff + " /*NOT TESTED*/");
                }


                string record_type = cur.ToString(cn_part_record_type);
                if (record_type == "P")
                {
                    partitionPtr->m_strName = partition_name;

                    if (table.m_subpartitioningType != NONE)
                    {
                        if (!cur.IsNull(cn_part_block_size))
                        {
                            int block_size = cur.ToInt(cn_part_block_size);

                            if (!partitionPtr->m_nInitialExtent.IsNull())
                                partitionPtr->m_nInitialExtent.Set(partitionPtr->m_nInitialExtent.GetValue() * block_size);
                            if (!partitionPtr->m_nNextExtent.IsNull())
                                partitionPtr->m_nNextExtent.Set(partitionPtr->m_nNextExtent.GetValue() * block_size);
                        }
                        else
                        {
                            partitionPtr->m_nInitialExtent.SetNull();
                            partitionPtr->m_nNextExtent.SetNull();
                        }
                    }

                    table.m_Partitions.push_back(partitionPtr);
                }
                else if (record_type == "T")
                {
                    partitionPtr->m_strName = subpartition_name;
                    table.m_subpartTemplates.push_back(partitionPtr);
                }
                else if (record_type == "S")
                {
                    partitionPtr->m_strName = subpartition_name;
                    
                    if (!lastPartitionPtr.get() || lastPartitionPtr->m_strName != partition_name)
                    {
                        lastPartitionPtr.reset(0);

                        Table::PartitionContainer::const_iterator it = table.m_Partitions.begin();
                        for (; it != table.m_Partitions.end(); ++it)
                            if ((*it)->m_strName == partition_name)
                            {
                                lastPartitionPtr = *it;
                                break;
                            }
                    }
                    
                    if (lastPartitionPtr.get())
                    {
                        lastPartitionPtr->m_subpartitions.push_back(partitionPtr);

                        //if (lastPartitionPtr->m_nInitialExtent.IsNull())
                        //    lastPartitionPtr->m_nInitialExtent.SetIfNotNull(partitionPtr->m_nInitialExtent);
                        //if (lastPartitionPtr->m_nNextExtent.IsNull())
                        //    lastPartitionPtr->m_nNextExtent.SetIfNotNull(partitionPtr->m_nNextExtent);
                    }
                    
                    _CHECK_AND_THROW_(lastPartitionPtr.get() || !table.m_interval.empty(), "Table subpartition loading error:\nParent partition not found!");
                }
            }
        }
    }

}// END namespace OraMetaDict
