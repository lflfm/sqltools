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
#include "ObjectTree_Builder.h"
#include "SQLTools.h"
#include "COMMON\StrHelpers.h"
#include <OCI8/BCursor.h>
#include "MetaDict/MetaObjects.h" // for OraMetaDict::TabColumn
#include "MetaDict\Loader.h"      // for Loader::IsYes
#include "SQLUtilities.h"

using namespace std;
using namespace Common;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ADD_FOLDER_TO_CHILDREN(T) { T* folder = m_treeNodePool.Create<T, Object>(this); \
    folder->SetSchemaNameType(GetSchema(), GetName(), GetType()); \
    m_children.push_back(folder); }
#define ADD_FOLDER_TO_CHILDREN2(T, folder) { folder = m_treeNodePool.Create<T, Object>(this); \
    folder->SetSchemaNameType(GetSchema(), GetName(), GetType()); \
    m_children.push_back(folder); }

namespace ObjectTree {

    static
    void prepare_cursor (OciCursor& cur, const char* sttm, Object& obj, bool skip_bind = false)
    {
        OCI8::EServerVersion ver = cur.GetConnect().GetVersion();
        Common::Substitutor subst;
        subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
        subst.AddPair("<OBJECT_TYPE_EQ_TABLE>",  (ver ==  OCI8::esvServer81X) ?  "Trim(object_type) = 'TABLE'"  : "object_type = 'TABLE'");
        subst.AddPair("<SUBPART_HIGH_VALUE_DUMMY>"  , 
            (ver ==  OCI8::esvServer81X) ? 
            ",(SELECT data_default high_value FROM all_tab_columns WHERE owner = 'SYS' AND table_name = 'DUAL' AND column_name = 'DUMMY')" 
            : ""
          );
        subst.AddPair("<INTERVAL>", (ver >= OCI8::esvServer11X) ? "interval" : "NULL");
        subst << sttm;
        cur.Prepare(subst.GetResult());
        if (!skip_bind)
        {
            cur.Bind(":schema", obj.GetSchema().c_str());
            cur.Bind(":name", obj.GetName().c_str());
        }
    }


int Object::operator == (const ObjectDescriptor& desc) const
{
    return desc.name == GetName() && desc.owner == GetSchema() && desc.type == GetType();
}

    const int cn_ref_owner   = 0;
    const int cn_ref_name    = 1;
    const int cn_ref_db_link = 2;

    const char* csz_syn_all_sttm_1 =
        "SELECT <RULE> table_owner, table_name, db_link"
            " FROM sys.all_synonyms"
            " WHERE synonym_name = :name"
                " AND owner = :schema";

    const char* csz_syn_user_sttm_1 =
        "SELECT <RULE> table_owner, table_name, db_link"
            " FROM sys.user_synonyms"
            " WHERE synonym_name = :name"
                " AND SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA') = :schema";

    //const int cn_ref_owner   = 0;
    //const int cn_ref_name    = 1;
    const int cn_ref_type    = 2;

    const char* csz_syn_sttm_2 =
        "SELECT <RULE> owner, object_name, object_type"
            " FROM sys.all_objects "
            " WHERE object_name = :name"
                " AND owner = :schema"
            " ORDER BY object_type";

void Synonym::Load (OciConnect& connect)
{
    {
        string user, schema;
        connect.GetCurrentUserAndSchema(user, schema);

        OciCursor cur(connect);
        prepare_cursor(cur, ((GetSchema() == user && GetSchema() == schema)) ? csz_syn_user_sttm_1 : csz_syn_all_sttm_1, *this);
        cur.Execute();

        if (cur.Fetch())
        {
            SetRefOwner (cur.ToString(cn_ref_owner));
            SetRefName  (cur.ToString(cn_ref_name));
            SetRefDBLink(cur.ToString(cn_ref_db_link));
        }
    }

    // TODO: create dummy remote object
    if (GetRefDBLink().empty())
    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_syn_sttm_2, *this, true);
        cur.Bind(":schema", GetRefOwner().c_str());
        cur.Bind(":name", GetRefName().c_str());

        cur.Execute();
        if (cur.Fetch())
            SetRefType(cur.ToString(cn_ref_type));

        ObjectDescriptor desc;
        desc.owner = GetRefOwner();
        desc.name  = GetRefName();
        desc.type  = GetRefType();

        TreeNode* refObj = m_treeNodePool.CreateObject(desc);
        m_children.push_back(refObj);
    }
    ADD_FOLDER_TO_CHILDREN(DependentObjects);

    SetComplete();
}

    const int cn_owner   = 0;
    const int cn_name    = 1;
    const int cn_type    = 2;

    const char* csz_dep_objs_sttm =
        "SELECT <RULE> owner, name, type FROM sys.all_dependencies"
          " WHERE referenced_owner = :schema"
            " AND referenced_name = :name"
            " AND referenced_type = :type"
            " AND referenced_link_name IS NULL"
        " ORDER BY type, owner, name";

void DependentObjects::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_dep_objs_sttm, *this);
    cur.Bind(":type", GetType().c_str());

    cur.Execute();
    while (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_owner);
        desc.name  = cur.ToString(cn_name);
        desc.type  = cur.ToString(cn_type);

        if (TreeNode* obj = m_treeNodePool.CreateObject(desc))
            m_children.push_back(obj);
    }

    SetComplete();
}

    const int cn_dep_on_owner   = 0;
    const int cn_dep_on_name    = 1;
    const int cn_dep_on_type    = 2;

    const char* csz_dep_on_sttm =
        "SELECT <RULE> referenced_owner, referenced_name, referenced_type FROM sys.all_dependencies"
          " WHERE owner = :schema"
            " AND name = :name"
            " AND type = :type"
            " AND referenced_type <> 'NON-EXISTENT'"
        " ORDER BY referenced_type, referenced_owner, referenced_name";

void DependentOn::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_dep_on_sttm, *this);
    cur.Bind(":type", GetType().c_str());

    cur.Execute();
    while (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_dep_on_owner);
        desc.name  = cur.ToString(cn_dep_on_name );
        desc.type  = cur.ToString(cn_dep_on_type );

        if (TreeNode* obj = m_treeNodePool.CreateObject(desc))
            m_children.push_back(obj);
    }

    SetComplete();
}

    const int cn_tab_priv_grantee   = 0;
    const int cn_tab_priv_privilege = 1;

    const char* csz_tab_priv_sttm =
        "select <RULE> grantee, privilege from sys.all_tab_privs "
        "where table_name = :name and table_schema = :schema "
          "order by 1, 2";

void GrantedPriveleges::Load (OciConnect& connect)
{
    map<string, string> privileges;

    OciCursor cur(connect);
    prepare_cursor(cur, csz_tab_priv_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        string grantee   = cur.ToString(cn_tab_priv_grantee  );
        string privilege = cur.ToString(cn_tab_priv_privilege);
        
        map<string, string>::iterator it = privileges.find(grantee);
        if (it == privileges.end())
            privileges.insert(make_pair(grantee, privilege));
        else
            it->second += ", " + privilege;
    }

    map<string, string>::const_iterator it = privileges.begin();
    for (; it != privileges.end(); ++it)
    {
        if (GrantedPrivelege* privilege = m_treeNodePool.Create<GrantedPrivelege, GrantedPriveleges>(this))
        {
            privilege->SetName(it->first + " can " + it->second);
            privilege->SetComplete();
            m_children.push_back(privilege);
        }
    }

    SetComplete();
}

    const int cn_tab_ref_owner   = 0;
    const int cn_tab_ref_name    = 1;

    const char* csz_tab_ref_sttm =
        "select <RULE> distinct owner, table_name from sys.all_constraints "
          "where (owner, constraint_name) "
            "in ("
              "select r_owner, r_constraint_name from sys.all_constraints "
              "where owner = :schema and table_name = :name and constraint_type in ('R')"
            ")"
          "order by 1, 2";

void References::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_tab_ref_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_tab_ref_owner);
        desc.name  = cur.ToString(cn_tab_ref_name);
        desc.type  = "TABLE";

        if (TreeNode* obj = m_treeNodePool.CreateObject(desc))
            m_children.push_back(obj);
    }

    SetComplete();
}

    const int cn_tab_ref_by_owner   = 0;
    const int cn_tab_ref_by_name    = 1;

    const char* csz_tab_ref_by_sttm =
        "select <RULE> distinct owner, table_name from sys.all_constraints "
          "where (r_owner,r_constraint_name) "
            "in ("
              "select owner, constraint_name from sys.all_constraints "
                "where owner = :schema and table_name = :name and constraint_type in (\'P\',\'U\')"
             ")"
          "order by 1, 2";

void ReferencedBy::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_tab_ref_by_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_tab_ref_by_owner);
        desc.name  = cur.ToString(cn_tab_ref_by_name );
        desc.type  = "TABLE";

        if (TreeNode* obj = m_treeNodePool.CreateObject(desc))
            m_children.push_back(obj);
    }

    SetComplete();
}

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

    const char* csz_col_sttm =
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
            "<VIRTUAL_COLUMN> "
        "FROM <ALL_TAB_COL_VIEW> v1, sys.all_col_comments v2 "
        "WHERE v1.owner = :schema AND v1.table_name = :name "
            "AND v1.owner = v2.owner AND v1.table_name = v2.table_name "
            "AND v1.column_name = v2.column_name "
        "ORDER BY v1.column_id";

void TableBase::Load (OciConnect& connect)
{
    {
        OciCursor cur(connect);
        prepare_cursor(cur, 
            "SELECT <RULE> comments FROM sys.all_tab_comments "
            "WHERE owner = :schema AND table_name = :name", *this);

        cur.Execute();
        if (cur.Fetch())
        {
            string comments = cur.ToString(0);
            trim_symmetric(comments);
            SetComments(comments);
        }
    }
    {
        OCI8::EServerVersion ver = connect.GetVersion();
        char defCharLengthSemantics = connect.GetNlsLengthSemantics() == "CHAR" ? 'C' : 'B';

        Common::Substitutor subst;
        subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");

        if (ver >= OCI8::esvServer9X)
            subst.AddPair("<V1_CHAR_LENGTH>", "v1.char_length");
        else if (ver > OCI8::esvServer73X)
            subst.AddPair("<V1_CHAR_LENGTH>", "v1.char_col_decl_length");
        else
            subst.AddPair("<V1_CHAR_LENGTH>", "NULL");

        subst.AddPair("<V1_CHAR_USED>",     (ver >= OCI8::esvServer9X) ?  "v1.char_used" : "NULL");
        subst.AddPair("<ALL_TAB_COL_VIEW>", (ver >= OCI8::esvServer11X) ? "(SELECT * FROM sys.all_tab_cols WHERE hidden_column = 'NO')" : "sys.all_tab_columns");
        subst.AddPair("<VIRTUAL_COLUMN>"  , (ver >= OCI8::esvServer11X) ? "virtual_column"   : "NULL");

        subst << csz_col_sttm;

        OciCursor cur(connect, subst.GetResult());

        cur.Bind(":schema", GetSchema().c_str());
        cur.Bind(":name", GetName().c_str());

        int childFirstTabPos = 0;
        cur.Execute();
        while (cur.Fetch())
        {
            OraMetaDict::TabColumn metaColumn;

            cur.GetString(cn_col_column_name, metaColumn.m_strColumnName);
            cur.GetString(cn_col_data_type,   metaColumn.m_strDataType);
            metaColumn.m_nDataPrecision = cur.IsNull(cn_col_data_precision) ? -1 : cur.ToInt(cn_col_data_precision);
            metaColumn.m_nDataScale     = cur.IsNull(cn_col_data_scale)     ? -1 : cur.ToInt(cn_col_data_scale);
            metaColumn.m_nDataLength = (!cur.IsNull(cn_char_length) && cur.ToInt(cn_char_length) != 0)
                ? cur.ToInt(cn_char_length) : cur.ToInt(cn_col_data_length);
            metaColumn.m_defCharLengthSemantics = (defCharLengthSemantics == 'C') ? OraMetaDict::USE_CHAR : OraMetaDict::USE_BYTE;
            metaColumn.m_charLengthSemantics = (cur.ToString(cn_col_char_used) == "C") ? OraMetaDict::USE_CHAR : OraMetaDict::USE_BYTE;
            metaColumn.m_bNullable = OraMetaDict::Loader::IsYes(cur.ToString(cn_col_nullable));
            metaColumn.m_bVirtual  = (metaColumn.m_strDataType != "XMLTYPE") ? OraMetaDict::Loader::IsYes(cur.ToString(cn_col_virtual_column)) : false;
            // the following two columns might be truncated but that is not really important for our purpose here
            cur.GetString(cn_col_comments, metaColumn.m_strComments);
            cur.GetString(cn_col_data_default, metaColumn.m_strDataDefault);
            trim_symmetric(metaColumn.m_strDataDefault);

            Column* column = m_treeNodePool.Create<Column, TableBase>(this);
            column->SetName(metaColumn.m_strColumnName);
            column->SetPosition(cur.ToInt(cn_col_column_id));

            string type;
            metaColumn.GetSpecString(type, OraMetaDict::WriteSettings());
            column->SetType(type);
            column->SetNullable(metaColumn.m_bNullable);
            column->SetVirtual(metaColumn.m_bVirtual);
            column->SetDefault(metaColumn.m_strDataDefault);
            column->SetComments(metaColumn.m_strComments);
            column->SetComplete();
            m_children.push_back(column);
            childFirstTabPos = std::max(childFirstTabPos, (int)column->GetName().length());
        }

        SetChildFirstTabPos(childFirstTabPos);
    }

    m_order = Natural;
    if (GetSQLToolsSettings().GetObjectViewerSortTabColAlphabetically())
        SortChildren(Alphabetical);
}

void Table::Load (OciConnect& connect)
{
    TableBase::Load(connect);

    m_Partitioned = false;
    m_Subpartitioned = false;
    string partitioning_type, subpartitioning_type;

    vector<Property> properties;
    {
        OciCursor cur(connect);
        prepare_cursor(cur, 
            "SELECT <RULE> partitioned, temporary, duration FROM sys.all_tables "
            "WHERE owner = :schema AND table_name = :name", *this);

        cur.Execute();
        if (cur.Fetch())
        {
            if (OraMetaDict::Loader::IsYes(cur.ToString(0)))
            {
                SetInfo("[partitioned]");
                m_Partitioned = true;
            }
            else if (OraMetaDict::Loader::IsYes(cur.ToString(1)))
            {
                SetInfo("[temporary]");
                properties.push_back(Property("Temporary", "YES"));
                properties.push_back(Property("Duration", cur.ToString(2)));
            }
        }
    }
    if (m_Partitioned)
    {
        OciCursor cur(connect);
        prepare_cursor(cur, 
            "SELECT <RULE> partitioning_type, subpartitioning_type, <INTERVAL> FROM sys.all_part_tables "
            "WHERE owner = :schema AND table_name = :name", *this);

        cur.Execute();
        if (cur.Fetch())
        {
            partitioning_type = cur.ToString(0);
            properties.push_back(Property("Partitioning_type", partitioning_type));
            if (cur.ToString(1) != "NONE")
            {
                subpartitioning_type = cur.ToString(1);
                properties.push_back(Property("Subpartitioning_type", subpartitioning_type));
                m_Subpartitioned = true;
            }
            if (!cur.IsNull(2))
            properties.push_back(Property("Interval", cur.ToString(2)));
        }
    }
    SetProperties(properties);

    if (m_Partitioned)
    {
        Partitioning* partitioning = 0;
        ADD_FOLDER_TO_CHILDREN2(Partitioning, partitioning);
        partitioning->SetPartitioningType(partitioning_type);
        partitioning->SetSubpartitioningType(subpartitioning_type);
    }

    ADD_FOLDER_TO_CHILDREN(Constraints);
    ADD_FOLDER_TO_CHILDREN(Indexes);
    ADD_FOLDER_TO_CHILDREN(Triggers);
    ADD_FOLDER_TO_CHILDREN(References);
    ADD_FOLDER_TO_CHILDREN(ReferencedBy);
    ADD_FOLDER_TO_CHILDREN(DependentObjects);
    ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

    SetComplete();
}

void View::Load (OciConnect& connect)
{
    TableBase::Load(connect);

    //ADD_FOLDER_TO_CHILDREN(Constraints);
    ADD_FOLDER_TO_CHILDREN(Triggers);
    ADD_FOLDER_TO_CHILDREN(DependentOn);
    ADD_FOLDER_TO_CHILDREN(DependentObjects);
    ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

    SetComplete();
}

    const int cn_const_name				= 0;
    const int cn_const_type				= 1;
    const int cn_const_search_condition = 2;
    const int cn_const_status           = 3;
    const int cn_const_generated        = 4;
    const int cn_const_ref_schema       = 5;
    const int cn_const_ref_constraint_name = 6;

    const char* csz_constr_sttm =
        "SELECT <RULE> constraint_name, constraint_type,"
          " search_condition, status, generated,"
          " r_owner, r_constraint_name"
          " FROM sys.all_constraints"
            " WHERE owner = :schema AND table_name = :name"
          " ORDER BY decode(constraint_type, 'P', 1, 'U', 2, 'R', 3, 'C', 4, 5), constraint_name";

    const int cn_const_constraint_name	= 0;
    const int cn_const_column_name      = 1;

    const char* csz_constr_columns_sttm =
        "SELECT <RULE> constraint_name, column_name"
          " FROM sys.all_cons_columns"
            " WHERE owner = :schema AND table_name = :name"
          " ORDER BY position";

    struct ConstraintDesc
    {
        string constraint_name, constraint_type,  
               ref_schema, ref_constraint_name,
               search_condition, status, generated;
        vector<string> column_list;
    };

void Constraints::Load (OciConnect& connect)
{
    vector<ConstraintDesc> constraints;

    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_constr_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            ConstraintDesc constraint;
            constraint.constraint_name	   = cur.ToString(cn_const_name);
            constraint.constraint_type	   = cur.ToString(cn_const_type);
            constraint.search_condition    = cur.ToString(cn_const_search_condition);
            trim_symmetric(constraint.search_condition);
            constraint.status              = cur.ToString(cn_const_status);
            constraint.generated           = cur.ToString(cn_const_generated);
            constraint.ref_schema          = cur.ToString(cn_const_ref_schema);         
            constraint.ref_constraint_name = cur.ToString(cn_const_ref_constraint_name);

            constraints.push_back(constraint);
        }
    }

    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_constr_columns_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            string constraint_name	= cur.ToString(cn_const_constraint_name);
            string column_name      = cur.ToString(cn_const_column_name    );

            vector<ConstraintDesc>::iterator it = constraints.begin();
            for (; it != constraints.end(); ++it)
                if (it->constraint_name == constraint_name)
                    it->column_list.push_back(column_name);
        }
    }

    vector<ConstraintDesc>::const_iterator it = constraints.begin();
    for (; it != constraints.end(); ++it)
    {
        bool skip = false;

        Constraint* constraint = (it->constraint_type != "R") 
                ? m_treeNodePool.Create<Constraint, Constraints>(this)
                : m_treeNodePool.Create<FkConstraint, Constraints>(this);

        constraint->SetSchemaNameType(GetSchema(), it->constraint_name, it->constraint_type);
        constraint->SetColumns(it->column_list);

        if (it->constraint_type == "C")
        {
            if (it->column_list.size() == 1
                && it->generated != "USER NAME"
                && 
                (
                    it->search_condition == "\"" + it->column_list.front() + "\" IS NOT NULL"
                    || it->search_condition == it->column_list.front() + " IS NOT NULL"
                )
            )
            {
                skip = true;
            }
            else
            {
                constraint->SetName(it->constraint_name);
                constraint->SetInfo("[" + it->search_condition + "]");
            }
        }
        else
        {
            constraint->SetName(it->constraint_name);

            string list;
            vector<string>::const_iterator list_it = it->column_list.begin();
            for (; list_it != it->column_list.end(); ++list_it)
            {
                if (list_it != it->column_list.begin())
                    list += ", ";
                list += *list_it;

                // TODO: FK should be handled differently
                //re-introducing columns as children nodes
                if (typeid(*constraint) != typeid(FkConstraint))
                {
                    Column* column = m_treeNodePool.Create<Column, Constraint>(constraint);
                    column->SetName(*list_it);
                    column->SetComplete();
                    constraint->m_children.push_back(column);
                }
            }

            constraint->SetInfo("(" + list + ")");
        }

        if (it->constraint_type == "R")
        {
            if (FkConstraint* fkConstraint = dynamic_cast<FkConstraint*>(constraint))
            {
                fkConstraint->SetRefSchema(it->ref_schema);
                fkConstraint->SetRefConstraintName(it->ref_constraint_name);
            }
        }
        else
            constraint->SetComplete();


        if (!skip)
            m_children.push_back(constraint);
    }

    SetComplete();
}

    const int cn_ref_constr_owner		= 0;
    const int cn_ref_constr_table_name = 1;

    const char* csz_ref_constr_sttm =
        "SELECT <RULE> owner, table_name "
          " FROM sys.all_constraints"
            " WHERE owner = :schema AND constraint_name = :name";

void FkConstraint::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_ref_constr_sttm, *this, true);
    cur.Bind(":schema", GetRefSchema().c_str());
    cur.Bind(":name", GetRefConstraintName().c_str());

    cur.Execute();
    if (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_ref_constr_owner);
        desc.name  = cur.ToString(cn_ref_constr_table_name);
        desc.type  = "TABLE";

        TreeNode* refTable = m_treeNodePool.CreateObject(desc);
        m_children.push_back(refTable);
    }

    SetComplete();
}

    const int cn_tab_inx_owner      = 0;
    const int cn_tab_inx_index_name = 1;
    const int cn_tab_inx_index_type = 2;
    const int cn_tab_inx_uniqueness = 3;
    const int cn_tab_inx_status     = 4;

    const char* csz_tab_idx_sttm =
        "SELECT <RULE> owner, index_name, index_type, uniqueness, status "
            "FROM sys.all_indexes "
                "WHERE table_owner = :schema AND table_name = :name "
            "ORDER BY owner, index_name";

    const int cn_inx_col_index_owner	 = 0;
    const int cn_inx_col_index_name      = 1;
    const int cn_inx_col_column_name	 = 2;
    const int cn_inx_col_column_position = 3;

    const char* csz_inx_col_sttm =
        "SELECT <RULE> index_owner, index_name, column_name, column_position "
           "FROM sys.all_ind_columns "
             "WHERE table_owner = :schema AND table_name = :name "
           "ORDER BY index_owner, index_name, column_position";

    const int cn_inx_exp_index_owner	   = 0;
    const int cn_inx_exp_index_name        = 1;
    const int cn_inx_exp_column_expression = 2;
    const int cn_inx_exp_column_position   = 3;

    const char* csz_inx_exp_sttm =
        "SELECT index_owner, index_name, column_expression, column_position "
          "FROM sys.all_ind_expressions "
          "WHERE table_owner = :schema AND table_name = :name "
          "ORDER BY index_owner, index_name, column_position";

    struct IndexDesc
    {
        string owner, index_name, index_type, uniqueness, status;
        vector<string> column_list;
    };

void Indexes::Load (OciConnect& connect)
{
    vector<IndexDesc> indexes;

    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_tab_idx_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            IndexDesc index;
            index.owner        = cur.ToString(cn_tab_inx_owner     );
            index.index_name   = cur.ToString(cn_tab_inx_index_name);
            index.index_type   = cur.ToString(cn_tab_inx_index_type);
            index.uniqueness   = cur.ToString(cn_tab_inx_uniqueness);
            index.status       = cur.ToString(cn_tab_inx_status    );

            indexes.push_back(index);
        }
    }

    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_inx_col_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            string index_owner	    = cur.ToString(cn_inx_col_index_owner	 );
            string index_name       = cur.ToString(cn_inx_col_index_name     );
            string column_name	    = cur.ToString(cn_inx_col_column_name	 );
            //int column_position     = cur.ToInt   (cn_inx_col_column_position);

            vector<IndexDesc>::iterator it = indexes.begin();
            for (; it != indexes.end(); ++it)
                if (it->owner == index_owner)
                if (it->index_name == index_name)
                {
                    it->column_list.push_back(column_name);
                }
        }
    }

    if (connect.GetVersion() > OCI8::esvServer80X)
    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_inx_exp_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            string index_owner	     = cur.ToString(cn_inx_exp_index_owner	    );
            string index_name        = cur.ToString(cn_inx_exp_index_name       );
            string column_expression = cur.ToString(cn_inx_exp_column_expression);
            int    column_position   = cur.ToInt   (cn_inx_exp_column_position  ) - 1;

            vector<IndexDesc>::iterator it = indexes.begin();
            for (; it != indexes.end(); ++it)
                if (it->owner == index_owner)
                if (it->index_name == index_name)
                {
                    if ((string::size_type)column_position < it->column_list.size())
                        it->column_list.at(column_position) = column_expression;
                    else // should not happen but
                        it->column_list.push_back(column_expression);
                }
        }
    }

    vector<IndexDesc>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
    {
        Index* index = m_treeNodePool.Create<Index, Indexes>(this);
        index->SetSchemaNameType(GetSchema(), it->index_name, it->index_type);
        
        string info, subtype = it->index_type;

        if (!it->uniqueness.empty() && it->uniqueness.at(0) == 'U')
        {
            info += !info.empty() ? '/' : '[';
            info += "unique";

            subtype += " / UNIQUE";
        }
        if (it->index_type.find("FUNCTION-BASED") != std::string::npos)
        {
            info += !info.empty() ? '/' : '[';
            info += "function";
        }
        if (it->index_type.find("BITMAP") != std::string::npos)
        {
            info += !info.empty() ? '/' : '[';
            info += "bitmap";
        }
        if (!info.empty()) info += "]   ";

        info += '(';

        vector<string>::const_iterator col_it = it->column_list.begin();
        for (; col_it != it->column_list.end(); ++col_it)
        {
            if (col_it != it->column_list.begin())
                info += ", ";
            info += *col_it;

            //re-introducing columns as children nodes
            Column* column = m_treeNodePool.Create<Column, Index>(index);
            column->SetName(*col_it);
            column->SetComplete();
            index->m_children.push_back(column);
        }
        info += ')';

        index->SetType("INDEX");
        index->SetSubtype(subtype);
        index->SetInfo(info);
        index->SetComplete();
        index->SetColumns(it->column_list); 

        m_children.push_back(index);
    }

    SetComplete();
}

void Index::Load (OciConnect& /*connect*/)
{
    SetComplete();
}

    const int cn_tab_trg_owner        = 0;
    const int cn_tab_trg_trigger_name = 1;

    const char* csz_tab_trg_sttm =
        "SELECT <RULE> owner, trigger_name FROM sys.all_triggers "
          "WHERE table_owner = :schema AND table_name = :name";

void Triggers::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_tab_trg_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        ObjectDescriptor desc;
        desc.owner = cur.ToString(cn_tab_trg_owner);
        desc.name  = cur.ToString(cn_tab_trg_trigger_name);
        desc.type  = "TRIGGER";

        if (TreeNode* obj = m_treeNodePool.CreateObject(desc))
            m_children.push_back(obj);
    }

    SetComplete();
}

void Trigger::Load (OciConnect& /*connect*/)
{
    ADD_FOLDER_TO_CHILDREN(DependentOn);

    SetComplete();
}

    enum
    {
    cn_min_value = 0,
    cn_max_value,   
    cn_increment_by, 
    cn_cycle_flag, 
    cn_order_flag, 
    cn_cache_size, 
    cn_last_number
    };

    const char* csz_seq_sttm =
      "SELECT min_value, max_value, increment_by, cycle_flag, order_flag, cache_size, last_number "
        "FROM all_sequences "
          "WHERE sequence_owner = :schema AND sequence_name = :name";

void Sequence::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_seq_sttm, *this);

    cur.Execute();
    if (cur.Fetch())
    {
        string info = "[last# = " + cur.ToString(cn_last_number) + "]";
        SetInfo(info);

        vector<Property> properties;
        properties.push_back(Property("Last#", cur.ToString(cn_last_number)));
        properties.push_back(Property("Cache", cur.ToString(cn_cache_size)));
        SetProperties(properties);
    }

    ADD_FOLDER_TO_CHILDREN(DependentObjects);
    ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

    SetComplete();
}

    const int cn_proc_arg_position      = 0;
    const int cn_proc_arg_argument_name = 1;
    const int cn_proc_arg_in_out        = 2;
    const int cn_proc_arg_type_name     = 3;
    const int cn_proc_arg_type_subname  = 4;
    const int cn_proc_arg_data_type     = 5;
    const int cn_proc_arg_default_value = 6;

    const char* csz_proc_arg_sttm =
        "SELECT <RULE> position, argument_name, in_out, type_name, type_subname, data_type, default_value "
          "FROM sys.all_arguments "
            "WHERE owner = :schema "
              "AND object_name = :name "
              "AND package_name IS NULL "
              "AND data_level = 0 "
          "ORDER BY position";

void Procedure::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_proc_arg_sttm, *this);

    string return_type;
    string arg_list;
    vector<Argument> arguments;
    int childFirstTabPos = 0;

    cur.Execute();
    while (cur.Fetch())
    {
        int    position      = cur.ToInt   (cn_proc_arg_position     );
        string argument_name = cur.ToString(cn_proc_arg_argument_name);
        string in_out        = cur.ToString(cn_proc_arg_in_out       );
        string type_name     = cur.ToString(cn_proc_arg_type_name    );
        string type_subname  = cur.ToString(cn_proc_arg_type_subname );
        string data_type     = cur.ToString(cn_proc_arg_data_type    );
        // not impleneted in 10g
        //string default_value = cur.ToString(cn_proc_arg_default_value);

        string arg_type;
        
        if (!type_name.empty())
        {
            arg_type = type_name;
            if (!type_subname.empty())
                arg_type += '.' + type_subname;
        }
        else
        {
            if (data_type == "PL/SQL BOOLEAN")
                arg_type = "BOOLEAN";
            else
                arg_type = data_type;
        }

        if (!position)
        {
            return_type = arg_type;
        }
        else
        {
            //re-introducing parameters as children nodes
            ProcedureParameter* param = 0;

            if (in_out == "IN")
                param = m_treeNodePool.Create<ProcedureParameterIn, Procedure>(this);
            else if (in_out == "OUT")
                param = m_treeNodePool.Create<ProcedureParameterOut, Procedure>(this);
            else
                param = m_treeNodePool.Create<ProcedureParameterInOut, Procedure>(this);

            param->SetName(argument_name);
            param->SetType(in_out + ' ' + arg_type);
//            param->SetPosition(position);
            param->SetComplete();
            m_children.push_back(param);

            childFirstTabPos = std::max(childFirstTabPos, (int)param->GetName().length());

            if (!arg_list.empty())
                arg_list += ", ";

            arg_list += SQLUtilities::GetSafeDatabaseObjectName(argument_name);
            arguments.push_back(Argument(argument_name, arg_type, in_out, false));

            if (in_out != "IN")
            {
                //to_lower_str(in_out.c_str(), in_out);
                arg_list += ' ' + in_out + ' ';
            }

            arg_list += ' ';
            //to_lower_str(arg_type.c_str(), arg_type);
            arg_list += arg_type;
        }
    }
    SetChildFirstTabPos(childFirstTabPos);

    if (Function* This = dynamic_cast<Function*>(this))
    {
        This->SetReturn(return_type);
        to_lower_str(return_type.c_str(), return_type);
        SetInfo(" (" + arg_list + ") RETURN " + return_type);
    }
    else
    {
        SetInfo(" (" + arg_list + ")");
    }

    ADD_FOLDER_TO_CHILDREN(DependentOn);
    ADD_FOLDER_TO_CHILDREN(DependentObjects);
    ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

    SetComplete();
}

    static
    bool has_subprogram_id (OciConnect& con)
    {
        const char* cookie = "sys.all_arguments.subprogram_id";
        const char* yes = "1", * no = "0";

        string val;
        if (!con.GetSessionCookie(cookie, val))
        {
            try // some 10g releases might have this column
            {
                OciStatement cursor(con);
                cursor.Prepare("SELECT subprogram_id FROM sys.all_arguments where 1=2");
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

    const int cn_pack_proc_arg_object_name      = 0;
    const int cn_pack_proc_arg_subprogram_id    = 1;
    const int cn_pack_proc_arg_argument_name    = 2;
    const int cn_pack_proc_arg_position         = 3;
    const int cn_pack_proc_arg_type_name        = 4;
    const int cn_pack_proc_arg_type_subname     = 5;
    const int cn_pack_proc_arg_data_type        = 6;
    const int cn_pack_proc_arg_overload         = 7;
    const int cn_pack_proc_arg_in_out           = 8;
    const int cn_pack_proc_arg_default_value    = 9;

    const char* csz_pack_proc_arg_sttm =
      "SELECT "
        "object_name,"
        "<SUBPROGRAM_ID> subprogram_id,"
        "argument_name,"
        "position,"
        "type_name,"      // dont forger TYPE_OWNER
        "type_subname,"
        "data_type,"
        "overload,"
        "in_out,"
        "default_value "
      "FROM sys.all_arguments "
        "WHERE owner = :schema "
          "AND package_name = :name "
          "AND data_level = 0"
        "ORDER BY subprogram_id, position";

void Package::Load (OciConnect& connect)
{
    OCI8::EServerVersion ver = connect.GetVersion();
    Common::Substitutor subst;
    subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
    subst.AddPair("<SUBPROGRAM_ID>", has_subprogram_id(connect) ? "subprogram_id" : "object_name||'.'||overload");
    subst << csz_pack_proc_arg_sttm;

    OciCursor cur(connect, subst.GetResult());
    cur.Bind(":schema", GetSchema().c_str());
    cur.Bind(":name", GetName().c_str());

    int subprogram_position = 0;
    string curr_subprogram_id;
    string subprogram_name;
    string return_type;
    string arg_list;
    vector<Argument> arguments;

    cur.Execute();
    while (cur.Fetch())
    {
        string subprogram_id = cur.ToString(cn_pack_proc_arg_subprogram_id);
        int    position      = cur.ToInt   (cn_pack_proc_arg_position     );
        string object_name   = cur.ToString(cn_pack_proc_arg_object_name  );
        string argument_name = cur.ToString(cn_pack_proc_arg_argument_name);
        string in_out        = cur.ToString(cn_pack_proc_arg_in_out       );
        string type_name     = cur.ToString(cn_pack_proc_arg_type_name    );
        string type_subname  = cur.ToString(cn_pack_proc_arg_type_subname );
        string data_type     = cur.ToString(cn_pack_proc_arg_data_type    );
        // not impleneted in 10g
        //string default_value = cur.ToString(cn_proc_arg_default_value);

        if (!curr_subprogram_id.empty()
        && curr_subprogram_id != subprogram_id)
        {
            addProcedure(subprogram_position++, subprogram_name,  arg_list, return_type, arguments);
            return_type.clear();
            arg_list.clear();
            arguments.clear();
        }

        curr_subprogram_id = subprogram_id;
        subprogram_name = object_name;

        string arg_type;
        
        if (!type_name.empty())
        {
            arg_type = type_name;
            if (!type_subname.empty())
                arg_type += '.' + type_subname;
        }
        else
        {
            if (data_type == "PL/SQL BOOLEAN")
                arg_type = "BOOLEAN";
            else
                arg_type = data_type;
        }

        if (!position)
        {
            return_type = arg_type;
        }
        else
        {
            if (!argument_name.empty()) // a procedure w/o arguments still has a dummy entry
            {
                if (!arg_list.empty())
                    arg_list += ", ";

                arg_list += SQLUtilities::GetSafeDatabaseObjectName(argument_name);
                arguments.push_back(Argument(argument_name, arg_type, in_out, false));

                if (in_out != "IN")
                {
                    //to_lower_str(in_out.c_str(), in_out);
                    arg_list += ' ' + in_out;
                }

                arg_list += ' ';
                //to_lower_str(arg_type.c_str(), arg_type);
                arg_list += arg_type;
            }
        }
    }

    if (!curr_subprogram_id.empty())
    {
        addProcedure(subprogram_position, subprogram_name,  arg_list, return_type, arguments);
        return_type.clear();
        arg_list.clear();
        arguments.clear();
    }

    {
        int childFirstTabPos = 0;

        for (std::vector<TreeNode*>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            if (PackageProcedure* column = dynamic_cast<PackageProcedure*>(*it))
                childFirstTabPos = max(childFirstTabPos, (int)column->GetName().length());

        SetChildFirstTabPos(childFirstTabPos);
    }

    ADD_FOLDER_TO_CHILDREN(DependentOn);
    ADD_FOLDER_TO_CHILDREN(DependentObjects);
    ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

    SetComplete();

    m_order = Natural;
    if (GetSQLToolsSettings().GetObjectViewerSortPkgProcAlphabetically())
        SortChildren(Alphabetical);
}

void Package::addProcedure (int position, const string& name,  const string& arg_list, const string& return_type, const vector<Argument>& arguments)
{
    PackageProcedure* procedure = 0;
    if (return_type.empty())
    {
        procedure = m_treeNodePool.Create<PackageProcedure, Package>(this);
        procedure->SetArgumentList("(" + arg_list + ")");
    }
    else
    {
        PackageFunction* function = m_treeNodePool.Create<PackageFunction, Package>(this);
        function->SetReturn(return_type);
        procedure = function;
        procedure->SetArgumentList("(" + arg_list + ") RETURN " + return_type);
    }
    if (procedure)
    {
        procedure->SetName(name);
        procedure->SetPosition(position);
        procedure->SetComplete();
        m_children.push_back(procedure);

        //re-introducing parameters as children nodes
        int childFirstTabPos = 0;
        vector<Argument>::const_iterator it = arguments.begin();
        for (; it != arguments.end(); ++it)
        {
            ProcedureParameter* param = 0;

            if (it->m_in_out == "IN")
                param = m_treeNodePool.Create<ProcedureParameterIn, PackageProcedure>(procedure);
            else if (it->m_in_out == "OUT")
                param = m_treeNodePool.Create<ProcedureParameterOut, PackageProcedure>(procedure);
            else
                param = m_treeNodePool.Create<ProcedureParameterInOut, PackageProcedure>(procedure);

            param->SetName(it->m_name);
            param->SetType(it->m_in_out + ' ' + it->m_type);
//            param->SetPosition(position);
            param->SetComplete();
            procedure->m_children.push_back(param);
            
            childFirstTabPos = std::max(childFirstTabPos, (int)param->GetName().length());
        }
        procedure->SetChildFirstTabPos(childFirstTabPos);
    }
}

void PackageBody::Load (OciConnect& /*connect*/)
{
    ADD_FOLDER_TO_CHILDREN(DependentOn);

    SetComplete();
}

    enum 
    {
        cn_type_attr_name = 0,
        cn_type_attr_type_mod, 
        cn_type_attr_type_owner,  // currently it is not used 
        cn_type_attr_type_name, 
        cn_type_attr_length, 
        cn_type_attr_precision, 
        cn_type_attr_scale, 
        cn_type_attr_attr_no
    };

    const char* cn_type_attr_sttm =
      "SELECT attr_name, attr_type_mod, attr_type_owner, attr_type_name, length, precision, scale, attr_no "
        "FROM ALL_TYPE_ATTRS "
          "WHERE owner = :schema "
            "AND type_name = :name "
        "ORDER BY attr_no";

	enum
	{
		cn_coll_type_coll_type = 0, 
		cn_coll_type_upper_bound, 
		cn_coll_type_elem_type_mod, 
		cn_coll_type_elem_type_owner, 
		cn_coll_type_elem_type_name, 
		cn_coll_type_length, 
		cn_coll_type_precision, 
		cn_coll_type_scale
	};

    const char* cn_coll_type_sttm =
		"select coll_type, upper_bound, elem_type_mod, elem_type_owner, elem_type_name, length, precision, scale "
		  "from all_coll_types "
	        "where owner = :schema and type_name = :name";

void Type::Load (OciConnect& connect)
{
    string typecode;
    {
        OciCursor cur(connect);
        prepare_cursor(cur, "select typecode from all_types where owner = :schema and type_name = :name", *this);
        
        cur.Execute();
        if (cur.Fetch())
            cur.GetString(0, typecode);
    }

    if (typecode == "OBJECT")
    {
        SetInfo("[object]");

        OciCursor cur(connect);
        prepare_cursor(cur, cn_type_attr_sttm, *this);

        cur.Execute();
        while (cur.Fetch())
        {
            OraMetaDict::TabColumn metaColumn;

            cur.GetString(cn_type_attr_name, metaColumn.m_strColumnName);
            cur.GetString(cn_type_attr_type_name, metaColumn.m_strDataType);
            metaColumn.m_nDataPrecision = cur.IsNull(cn_type_attr_precision) ? -1 : cur.ToInt(cn_type_attr_precision);
            metaColumn.m_nDataScale = cur.IsNull(cn_type_attr_scale) ? -1 : cur.ToInt(cn_type_attr_scale);
            metaColumn.m_nDataLength = !cur.IsNull(cn_type_attr_length) ? cur.ToInt(cn_type_attr_length) : -1;
            metaColumn.m_defCharLengthSemantics = OraMetaDict::USE_BYTE; //???
            metaColumn.m_charLengthSemantics = OraMetaDict::USE_BYTE;    //???
            metaColumn.m_bNullable = true;

            Column* column = m_treeNodePool.Create<Column, Type>(this);
            column->SetName(metaColumn.m_strColumnName);
            // TODO: move this logic in GetLabel
            string type;
            metaColumn.GetSpecString(type, OraMetaDict::WriteSettings());
            to_lower_str(type.c_str(), type);
            type = '[' + type + ']';
            column->SetType(type);
            column->SetComplete();
            m_children.push_back(column);
        }

        Package::Load(connect);

        {
            int childFirstTabPos = 0;

            for (std::vector<TreeNode*>::iterator it = m_children.begin(); it != m_children.end(); ++it)
                if (PackageProcedure* column1 = dynamic_cast<PackageProcedure*>(*it))
                    childFirstTabPos = max(childFirstTabPos, (int)column1->GetName().length());
                else if (Column* column2 = dynamic_cast<Column*>(*it)) 
                    childFirstTabPos = max(childFirstTabPos, (int)column2->GetName().length());

            SetChildFirstTabPos(childFirstTabPos);
        }
    }
    else
    {
        OciCursor cur(connect);
        prepare_cursor(cur, cn_coll_type_sttm, *this);
        
        cur.Execute();
        if (cur.Fetch())
		{
            OraMetaDict::TabColumn metaColumn;

            //cur.GetString(cn_type_attr_name, metaColumn.m_strColumnName);
            cur.GetString(cn_coll_type_elem_type_name, metaColumn.m_strDataType);
            metaColumn.m_nDataPrecision = cur.IsNull(cn_coll_type_precision) ? -1 : cur.ToInt(cn_coll_type_precision);
            metaColumn.m_nDataScale = cur.IsNull(cn_coll_type_scale) ? -1 : cur.ToInt(cn_coll_type_scale);
            metaColumn.m_nDataLength = !cur.IsNull(cn_coll_type_length) ? cur.ToInt(cn_coll_type_length) : -1;
            metaColumn.m_defCharLengthSemantics = OraMetaDict::USE_BYTE; //???
            metaColumn.m_charLengthSemantics = OraMetaDict::USE_BYTE;    //???
            metaColumn.m_bNullable = true;

            // TODO: move this logic in GetLabel
            string type;
            metaColumn.GetSpecString(type, OraMetaDict::WriteSettings());
			string coll_type = cur.ToString(cn_coll_type_coll_type);
			if (coll_type == "VARYING ARRAY") coll_type = "VARRAY";
			if (!cur.IsNull(cn_coll_type_upper_bound))
				coll_type += '(' + cur.ToString(cn_coll_type_upper_bound) + ')';
            type = "[" + coll_type + " of " + type + ']';
			to_lower_str(type.c_str(), type);
            SetInfo(type);
		}

        ADD_FOLDER_TO_CHILDREN(DependentOn);
        ADD_FOLDER_TO_CHILDREN(DependentObjects);
        ADD_FOLDER_TO_CHILDREN(GrantedPriveleges);

        SetComplete();
    }
}

void Partitioning::Load (OciConnect& connect)
{
    ADD_FOLDER_TO_CHILDREN(PartitionedBy);
    (*m_children.rbegin())->Load(connect);

    if (!m_SubpartitioningType.empty())
    {
        ADD_FOLDER_TO_CHILDREN(SubpartitionedBy);
        (*m_children.rbegin())->Load(connect);

        ADD_FOLDER_TO_CHILDREN(SubpartitionTemplate);
    }

    ADD_FOLDER_TO_CHILDREN(Partitions);

    SetComplete();
}

    const char* csz_part_key_sttm =
        "SELECT column_name " 
          "FROM sys.all_part_key_columns " 
            "WHERE <OBJECT_TYPE_EQ_TABLE>"
            "AND owner = :schema "
            "AND name = :name "
            "ORDER BY column_position";

void PartitionedBy::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_part_key_sttm, *this);

    string info;
    info += '(';
    cur.Execute();
    while (cur.Fetch())
    {
        Column* column = m_treeNodePool.Create<Column, PartitionedBy>(this);
        column->SetName(cur.ToString(0));
        column->SetComplete();
        m_children.push_back(column);
        if (info.size() > 1) info += ", ";
        info += column->GetName();
    }
    info += ')';

    if (Partitioning* parent = dynamic_cast<Partitioning*>(GetParent()))
        info = parent->GetPartitioningType() + " " + info;

    SetInfo(info);

    SetComplete();
}

    const char* csz_subpart_key_sttm =
        "SELECT column_name " 
          "FROM sys.all_subpart_key_columns " 
            "WHERE <OBJECT_TYPE_EQ_TABLE>"
            "AND owner = :schema "
            "AND name = :name "
            "ORDER BY column_position";

void SubpartitionedBy::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_subpart_key_sttm, *this);

    string info;
    info += '(';
    cur.Execute();
    while (cur.Fetch())
    {
        Column* column = m_treeNodePool.Create<Column, SubpartitionedBy>(this);
        column->SetName(cur.ToString(0));
        column->SetComplete();
        m_children.push_back(column);
        if (info.size() > 1) info += ", ";
        info += column->GetName();
    }
    info += ')';

    if (Partitioning* parent = dynamic_cast<Partitioning*>(GetParent()))
        info = parent->GetSubpartitioningType() + " " + info;

    SetInfo(info);

    SetComplete();
}

    LPSCSTR csz_subpart_templ_sttm =
        "SELECT <RULE>"
            "subpartition_name,"
            "high_bound " 
        "FROM sys.all_subpartition_templates "   
            "WHERE user_name = :schema "
            "AND table_name = :name "
        "ORDER BY subpartition_position";

void SubpartitionTemplate::Load (OciConnect& connect)
{
    OciCursor cur(connect);
    prepare_cursor(cur, csz_subpart_templ_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        Partition* partition = m_treeNodePool.Create<Partition, SubpartitionTemplate>(this);
        partition->SetName(cur.ToString(0));
        partition->SetType("SUBPARTITION");
        string high_value = cur.ToString(1);
        if (!high_value.empty())
            partition->SetInfo("[" + high_value + "]");
        partition->SetComplete();
        m_children.push_back(partition);
    }

    SetComplete();
}

    LPSCSTR csz_part_sttm =
        "SELECT <RULE>"
            "partition_name,"
            "high_value " 
        "FROM sys.all_tab_partitions "
            "WHERE table_owner = :schema "
            "AND table_name = :name "
        "ORDER BY partition_position";

void Partitions::Load (OciConnect& connect)
{
    bool subpartitioned = false;

    if (Partitioning* parent = dynamic_cast<Partitioning*>(GetParent()))
        subpartitioned = !parent->GetSubpartitioningType().empty();

    OciCursor cur(connect);
    prepare_cursor(cur, csz_part_sttm, *this);

    cur.Execute();
    while (cur.Fetch())
    {
        Partition* partition = m_treeNodePool.Create<Partition, Partitions>(this);
        partition->SetName(cur.ToString(0));
        partition->SetType("PARTITION");
        string high_value = cur.ToString(1);
        if (!high_value.empty())
            partition->SetInfo("[" + high_value + "]");

        if (!subpartitioned)
            partition->SetComplete();

        m_children.push_back(partition);
    }

    SetComplete();
}

    LPSCSTR csz_subpart_sttm =
        "SELECT <RULE>"
            "subpartition_name,"
            "high_value " 
        "FROM sys.all_tab_subpartitions <SUBPART_HIGH_VALUE_DUMMY> "   
            "WHERE table_owner = :schema "
            "AND table_name = :name "
            "AND partition_name = :partition_name "
        "ORDER BY subpartition_position";

void Partition::Load (OciConnect& connect)
{
    if (Partitions* parent = dynamic_cast<Partitions*>(GetParent()))
    {
        OciCursor cur(connect);
        prepare_cursor(cur, csz_subpart_sttm, *parent);
        cur.Bind(":partition_name", GetName().c_str());

        cur.Execute();
        while (cur.Fetch())
        {
            Partition* partition = m_treeNodePool.Create<Partition, Partition>(this);
            partition->SetName(cur.ToString(0));
            partition->SetType("SUBPARTITION");

            string high_value = cur.ToString(1);
            if (!high_value.empty())
                partition->SetInfo("[" + high_value + "]");

            partition->SetComplete();

            m_children.push_back(partition);
        }
    }

    SetComplete();
}

};//namespace ObjectTree {