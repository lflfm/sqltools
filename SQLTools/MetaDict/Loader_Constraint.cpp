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
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
#include "config.h"
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
    using namespace std;
    using namespace Common;

    const int cn_cons_owner             = 0;
    const int cn_cons_table_name        = 1;
    const int cn_cons_constraint_type   = 2;
    const int cn_cons_constraint_name   = 3;
    const int cn_cons_r_owner           = 4;
    const int cn_cons_r_constraint_name = 5;
    const int cn_cons_delete_rule       = 6;
    const int cn_cons_status            = 7;
    const int cn_cons_search_condition  = 8;
    const int cn_cons_generated         = 9;
    const int cn_cons_deferrable        = 10; 
    const int cn_cons_deferred          = 11; 
    const int cn_cons_validate          = 12; 
    const int cn_cons_relay             = 13; 
    const int cn_cons_view_related      = 14;

    const int cn_col_owner              = 0;
    const int cn_col_constraint_name    = 1;
    const int cn_col_column_name        = 2;
    const int cn_col_position           = 3;

    LPSCSTR csz_tab_constraints =
      "SELECT <RULE> " // table constraints
        "owner,"
        "table_name,"
        "constraint_type,"
        "constraint_name,"
        "r_owner,"
        "r_constraint_name,"
        "delete_rule,"
        "status,"
        "search_condition,"
        "<GENERATED>,"
        "<DEFERRABLE>,"
        "<DEFERRED>,"
        "<VALIDATED>," 
        "<RELY>,"
        "<VIEW_RELATED> "
      "FROM sys.all_constraints "
        "WHERE owner = :owner AND <NAME> <EQUAL> :name "

      "UNION ALL "
      "SELECT <RULE> " // primary keys for references
        "v2.owner,"
        "v2.table_name,"
        "v2.constraint_type,"
        "v2.constraint_name,"
        "v2.r_owner,"
        "v2.r_constraint_name,"
        "v2.delete_rule,"
        "v2.status,"
        "v2.search_condition,"
        "<V2_GENERATED>,"
        "<V2_DEFERRABLE>,"
        "<V2_DEFERRED>,"
        "<V2_VALIDATED>," 
        "<V2_RELY>,"
        "<V2_VIEW_RELATED> "
      "FROM sys.all_constraints v1, sys.all_constraints v2 "
        "WHERE v2.owner = v1.r_owner "
          "AND v2.constraint_name = v1.r_constraint_name "
          "AND v1.owner = :owner AND v1.<NAME> <EQUAL> :name";

    LPSCSTR csz_tab_ref_constraints =
      "SELECT <RULE> " // referenced foreign keys
        "v2.owner,"
        "v2.table_name,"
        "v2.constraint_type,"
        "v2.constraint_name,"
        "v2.r_owner,"
        "v2.r_constraint_name,"
        "v2.delete_rule,"
        "v2.status,"
        "v2.search_condition,"
        "<V2_GENERATED>,"
        "<V2_DEFERRABLE>,"
        "<V2_DEFERRED>,"
        "<V2_VALIDATED>," 
        "<V2_RELY>,"
        "<V2_VIEW_RELATED> "
      "FROM sys.all_constraints v1, sys.all_constraints v2 "
        "WHERE v2.r_owner = v1.owner "
          "AND v2.r_constraint_name = v1.constraint_name "
          "AND v1.owner = :owner AND v1.table_name <EQUAL> :name";

    LPSCSTR csz_const_columns =
      "SELECT <RULE> " // table constraint columns, 
        "v1.owner,"
        "v1.constraint_name,"
        "v1.column_name,"
        "v1.position "
        // join with all_constraints because a column may exist without a constraint!
      "FROM sys.all_cons_columns v1, sys.all_constraints v2 "
        "WHERE v1.owner = :owner AND v1.<NAME> <EQUAL> :name "
        "AND v1.owner = v2.owner AND v1.constraint_name = v2.constraint_name "

      "UNION ALL "
      "SELECT <RULE> " // columns of primary keys for references
        "v3.owner,"
        "v3.constraint_name,"
        "v3.column_name,"
        "v3.position "
      "FROM sys.all_constraints v1, sys.all_constraints v2, sys.all_cons_columns v3 "
        "WHERE v2.owner = v1.r_owner "
          "AND v2.constraint_name = v1.r_constraint_name "
          "AND v3.owner = v2.owner "
          "AND v3.table_name = v2.table_name "
          "AND v3.constraint_name = v2.constraint_name "
          "AND v1.owner = :owner AND v1.<NAME> <EQUAL> :name";

    LPSCSTR csz_ref_const_columns =  // columns of referenced foreign keys
		"SELECT <RULE> "
          "v3.owner,"
          "v3.constraint_name,"
          "v3.column_name,"
          "v3.position "
        "FROM sys.all_cons_columns v3,"
          "( "
            "SELECT "
              "v2.owner,"
              "v2.table_name,"
              "v2.constraint_type,"
              "v2.constraint_name,"
              "v2.r_owner,"
              "v2.r_constraint_name,"
              "v2.delete_rule,"
              "v2.status,"
              "v2.search_condition "
            "FROM sys.all_constraints v1, sys.all_constraints v2 "
              "WHERE v2.r_owner = v1.owner "
                "AND v2.r_constraint_name = v1.constraint_name "
                "AND v1.owner = :owner AND v1.<NAME> <EQUAL> :name "
          ") v0 "
        "WHERE v3.owner = v0.owner "
          "AND v3.table_name = v0.table_name "
          "AND v3.constraint_name = v0.constraint_name";

    static void main_loop (
            OciConnect& con, Dictionary& dict, const char* con_sttm, const char* con_col_sttm, 
            const char* owner, const char* name, bool byTable, bool useLike, OCI8::EServerVersion servVer
        );

    static void prepare_cursor (
            OciCursor& cur, const char* sttm, const char* owner, 
            const char* name, bool byTable, bool useLike, OCI8::EServerVersion servVer
        );

    static void load_constraint_body (OciCursor& cur, Dictionary& dict);

    static void load_constraint_column (OciCursor& cur, Dictionary& dict);


void Loader::LoadConstraints (const char* owner, const char* name, bool useLike)
{
    loadConstraints(owner, name, false/*byTable*/, useLike);
}

void Loader::loadConstraints (const char* owner, const char* name, bool byTable, bool useLike)
{
    main_loop(m_connect, m_dictionary, csz_tab_constraints, 
        csz_const_columns, owner, name, byTable, useLike, m_connect.GetVersion());
}

void Loader::LoadRefFkConstraints (const char* owner, const char* name, bool useLike)
{
    main_loop(m_connect, m_dictionary, csz_tab_ref_constraints, 
        csz_ref_const_columns, owner, name, true/*byTable*/, useLike, m_connect.GetVersion());
}


    static
    void main_loop (
        OciConnect& con, Dictionary& dict, const char* sttm, const char* col_sttm, 
        const char* owner, const char* name, bool byTable, bool useLike, OCI8::EServerVersion servVer)
    {
        {
#if TEST_LOADING_OVERFLOW
            OciCursor cur(con, 300, 32);
#else
            OciCursor cur(con, 300, 128);
#endif
			// remove RULE hint from queries (above 9i databases)
            prepare_cursor(cur, sttm, owner, name, byTable, useLike, servVer);

            cur.Execute();
            while (cur.Fetch())
                load_constraint_body(cur, dict);
        }
        {
#if TEST_LOADING_OVERFLOW
            OciCursor cur(con, 50, 32);
#else
            OciCursor cur(con, 50, 128);
#endif
            prepare_cursor(cur, col_sttm, owner, name, byTable, useLike, servVer);

            cur.Execute();
            while (cur.Fetch())
                load_constraint_column(cur, dict);
        }
    }

    static
    void prepare_cursor (
        OciCursor& cur, const char* sttm, const char* owner, 
        const char* name, bool byTable, bool useLike, OCI8::EServerVersion servVer)
    {
        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst.AddPair("<NAME>", byTable  ? "table_name" : "constraint_name");
        subst.AddPair("<RULE>",         (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");

        subst.AddPair("<GENERATED>",    (servVer > OCI8::esvServer73X) ? "generated"     : "NULL");  
        subst.AddPair("<DEFERRABLE>",   (servVer > OCI8::esvServer73X) ? "deferrable"    : "NULL"); 
        subst.AddPair("<DEFERRED>",     (servVer > OCI8::esvServer73X) ? "deferred"      : "NULL"); 

        subst.AddPair("<VALIDATED>",    (servVer >= OCI8::esvServer81X) ? "validated"    : "NULL"); 
        subst.AddPair("<RELY>",         (servVer >= OCI8::esvServer81X) ? "rely"         : "NULL"); 
        subst.AddPair("<VIEW_RELATED>", (servVer >= OCI8::esvServer9X ) ? "view_related" : "NULL"); 


        subst.AddPair("<V2_GENERATED>", (servVer > OCI8::esvServer73X) ? "v2.generated"  : "NULL"); 
        subst.AddPair("<V2_DEFERRABLE>",(servVer > OCI8::esvServer73X) ? "v2.deferrable" : "NULL");
        subst.AddPair("<V2_DEFERRED>",  (servVer > OCI8::esvServer73X) ? "v2.deferred"   : "NULL");
        
        subst.AddPair("<V2_VALIDATED>",    (servVer >= OCI8::esvServer81X) ? "v2.validated"    : "NULL"); 
        subst.AddPair("<V2_RELY>",         (servVer >= OCI8::esvServer81X) ? "v2.rely"         : "NULL"); 
        subst.AddPair("<V2_VIEW_RELATED>", (servVer >= OCI8::esvServer9X ) ? "v2.view_related" : "NULL"); 

        subst << sttm;

        cur.Prepare(subst.GetResult());
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
    }

    static
    void load_constraint_body (OciCursor& cur, Dictionary& dict)
    {
        try {
            Constraint& constraint = dict.CreateConstraint(cur.ToString(cn_cons_owner).c_str(),
                                                           cur.ToString(cn_cons_constraint_name).c_str());

            cur.GetString(cn_cons_owner            , constraint.m_strOwner);
            cur.GetString(cn_cons_table_name       , constraint.m_strTableName);
            cur.GetString(cn_cons_constraint_type  , constraint.m_strType);
            cur.GetString(cn_cons_constraint_name  , constraint.m_strName);
            cur.GetString(cn_cons_r_owner          , constraint.m_strROwner);
            cur.GetString(cn_cons_r_constraint_name, constraint.m_strRConstraintName);
            cur.GetString(cn_cons_delete_rule      , constraint.m_strDeleteRule);
            cur.GetString(cn_cons_status           , constraint.m_strStatus);

            constraint.m_bGenerated  = cur.ToString(cn_cons_generated   ) == "GENERATED NAME";
            constraint.m_bDeferrable = cur.ToString(cn_cons_deferrable  ) == "DEFERRABLE";
            constraint.m_bDeferred   = cur.ToString(cn_cons_deferred    ) == "DEFERRED";
            constraint.m_bRely       = cur.ToString(cn_cons_relay       ) == "RELY";
            constraint.m_bNoValidate = cur.ToString(cn_cons_validate    ) == "NOT VALIDATED";
            constraint.m_bOnView     = cur.ToString(cn_cons_view_related) == "DEPEND ON VIEW";

            if (cur.IsGood(cn_cons_search_condition))
                cur.GetString(cn_cons_search_condition, constraint.m_strSearchCondition);
            else 
            {
                OciCursor ovrfCur(cur.GetConnect(),
                    "SELECT search_condition FROM sys.all_constraints WHERE owner = :owner AND constraint_name = :name ",
                    1,
                    4 * 1024
                    );
                
                ovrfCur.Bind(":owner", constraint.m_strOwner.c_str());
                ovrfCur.Bind(":name",  constraint.m_strName.c_str());

                ovrfCur.Execute();
                ovrfCur.Fetch();

                _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Constraint search condition loading error:\nsize exceed 4K!");
                
                ovrfCur.GetString(0, constraint.m_strSearchCondition);
            }

            trim_symmetric(constraint.m_strSearchCondition);

            try {
                Table& table = dict.LookupTable(constraint.m_strOwner.c_str(), constraint.m_strTableName.c_str());

                table.m_setConstraints.insert(constraint.m_strName);
            }
            catch (const XNotFound&) {
                try {
                    View& view = dict.LookupView(constraint.m_strOwner.c_str(), constraint.m_strTableName.c_str());

                    if (constraint.m_strType == "O" || constraint.m_strType == "V") 
                        view.m_strViewConstraint = constraint.m_strName;
                    else
                        view.m_setConstraints.insert(constraint.m_strName);
                }
                catch (const XNotFound&) {}
            }
        }
        catch (const XAlreadyExists& x) 
        {
            TRACE1("%s\n", x.what());
            _ASSERTE(cur.ToString(cn_cons_constraint_type)[0] == 'P' || cur.ToString(cn_cons_constraint_type)[0] == 'U');
        }
    }

    static
    void load_constraint_column (OciCursor& cur, Dictionary& dict)
    {
        Constraint& constraint = dict.LookupConstraint(cur.ToString(cn_col_owner).c_str(),
                                                       cur.ToString(cn_col_constraint_name).c_str());
        
        constraint.m_Columns[cur.ToInt(cn_col_position, 1) - 1] = cur.ToString(cn_col_column_name);
    }

}// END namespace OraMetaDict
