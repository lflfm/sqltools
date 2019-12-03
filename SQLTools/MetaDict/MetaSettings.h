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

#ifndef __MetaSettings_h__
#define __MetaSettings_h__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <Common\Properties.h>

namespace OraMetaDict 
{
    using std::string;
    using std::set;
    using std::map;
    using std::list;
    using std::vector;

    class WriteSettings 
    {
    public:
        CMN_DECL_PROPERTY_BINDER(WriteSettings);
        // Common settings
        CMN_DECL_PUBLIC_PROPERTY(bool,   Comments);               // table, view & column comments
        CMN_DECL_PUBLIC_PROPERTY(bool,   Grants);                 // grants
        CMN_DECL_PUBLIC_PROPERTY(bool,   LowerNames);             // safe lowercase object names
        CMN_DECL_PUBLIC_PROPERTY(bool,   ShemaName);              // schema as object names prefix
        CMN_DECL_PUBLIC_PROPERTY(bool,   SQLPlusCompatibility);   // view do not have no white space lines, truncate end-line white space 
        CMN_DECL_PUBLIC_PROPERTY(bool,   GeneratePrompts);        // table, view & column comments
        // Table Specific
        CMN_DECL_PUBLIC_PROPERTY(bool,   CommentsAfterColumn);    // column comments as end line remark
        CMN_DECL_PUBLIC_PROPERTY(int ,   CommentsPos);            // end line comments position
        CMN_DECL_PUBLIC_PROPERTY(bool,   Constraints);            // constraints (table writing)
        CMN_DECL_PUBLIC_PROPERTY(bool,   Indexes);                // indexes (table writing)
        CMN_DECL_PUBLIC_PROPERTY(bool,   NoStorageForConstraint); // no storage for primary key & unique key
        CMN_DECL_PUBLIC_PROPERTY(int ,   StorageClause);          // table & index storage clause
        CMN_DECL_PUBLIC_PROPERTY(bool,   AlwaysPutTablespace);    // always put tablespace in storage clause (independent of previous flag)
        CMN_DECL_PUBLIC_PROPERTY(bool,   TableDefinition);        // short table definition (table writing)
        CMN_DECL_PUBLIC_PROPERTY(bool,   Triggers);               // table triggers (table writing)
        // Othes
        CMN_DECL_PUBLIC_PROPERTY(bool,   SequnceWithStart);       // sequence with START clause
        CMN_DECL_PUBLIC_PROPERTY(bool,   ViewWithTriggers);       // view triggers (view writing)
        CMN_DECL_PUBLIC_PROPERTY(bool,   ViewWithForce);          // use FORCE for view statement
        // Hidden
        CMN_DECL_PUBLIC_PROPERTY(string, EndOfShortStatement);    // what put after comments and grants 
        CMN_DECL_PUBLIC_PROPERTY(bool,   StorageSubstitutedClause); // generate Sql*Plus substitution instead of table/index storage
        
        CMN_DECL_PUBLIC_PROPERTY(bool,   AlwaysWriteColumnLengthSematics);

    public:
        WriteSettings ();

        bool IsStorageEnabled () const { return m_StorageClause ?  true : false; }
        bool IsStorageAlways ()  const { return m_StorageClause == 2; }
    };

} // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__MetaSettings_h__
