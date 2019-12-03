/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2014 Aleksey Kochetov

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

#pragma once

#include <string>
#include <COMMON/NVector.h>
#include "OpenGrid/GridSourceBase.h"

using std::string;

struct plan_table_record
{
    string   statement_id     ; 
    __int64  plan_id          ; 
    string   timestamp        ; 
    string   remarks          ; 
    string   operation        ; 
    string   options          ; 
    string   object_node      ; 
    string   object_owner     ; 
    string   object_name      ; 
    string   object_alias     ; 
    __int64  object_instance  ; 
    string   object_type      ; 
    string   optimizer        ; 
    __int64  search_columns   ; 
    __int64  id               ; 
    __int64  parent_id        ; 
    __int64  depth            ; 
    __int64  position         ; 
    __int64  cost             ; 
    __int64  cardinality      ; 
    __int64  bytes            ; 
    string   other_tag        ; 
    string   partition_start  ; 
    string   partition_stop   ; 
    __int64  partition_id     ; 
    string   other            ; 
    string   other_xml        ; 
    string   distribution     ; 
    __int64  cpu_cost         ; 
    __int64  io_cost          ; 
    __int64  temp_space       ; 
    string   access_predicates; 
    string   filter_predicates; 
    string   projection       ; 
    __int64  time             ; 
    string   qblock_name      ; 

    bool display_children;
    bool display_visible ;
    bool display_expanded;

    plan_table_record ()
    {
        plan_id         = 0; 
        object_instance = 0; 
        search_columns  = 0; 
        id              = 0; 
        parent_id       = 0; 
        depth           = 0; 
        position        = 0; 
        cost            = 0; 
        cardinality     = 0; 
        bytes           = 0; 
        partition_id    = 0; 
        cpu_cost        = 0; 
        io_cost         = 0; 
        temp_space      = 0; 
        time            = 0; 

        display_children = false;
        display_visible  = true;
        display_expanded = true;
    }
};

class ExplainPlanSource : public OG2::GridSourceBase
{
    static CImageList m_imageList;
    Common::nvector<plan_table_record> m_table;
    mutable int m_lastRow, m_lastIndex;

public:
    ExplainPlanSource  ();
    ~ExplainPlanSource ();

    int RowToInx (int row) const;
    void CalcRowNumbering ();

    const plan_table_record& GetRow (int row) const;
    void GetProperties (int row, std::vector<std::pair<string, string> >& properties) const;

    void Append (const plan_table_record& rec);
    void Toggle (int row);

    // implementation of the following methods MUST call notification methods
    virtual void Clear ();           
    // the follow methods operate with rows only
    virtual int  Append     ()      { return 0; };
    virtual int  Insert     (int)   { return 0; };
    virtual void Delete     (int)   {};
    virtual void DeleteAll  ()      {};

    // First fix column image support
    virtual int GetImageIndex  (int row) const;
    virtual void SetImageIndex (int row, int image);

    virtual int GetCount (OG2::eDirection dir) const;
    virtual void GetCellStr (std::string&, int row, int col) const;
    virtual int  ExportText (std::ostream&, int row = -1, int nrows = -1, int col = -1, int ncols = -1, int format = -1) const;
};


