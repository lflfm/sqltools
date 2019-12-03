/* 
    This code contributed by Ken Clubok

	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2005 Aleksey Kochetov

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

// 13.03.2005 (ak) R1105003: Bind variables

#ifndef __BINDSOURCE_H__
#define __BINDSOURCE_H__

/*
#include <memory>
#include <OCI8/ACursor.h>
*/
#include "OCI8/Datatypes.h"
#include "OpenGrid/GridSourceBase.h"
#include "SQLToolsSettings.h"
#include "SQLWorksheetDoc.h"
#include <arg_shared.h>
#include "BindParser.h"
#include "BindVariableTypes.h"

typedef string BindMapKey;
typedef std::map<BindMapKey, arg::counted_ptr<OCI8::Variable> > BindMap;

class OCI8::Statement;
class OCI8::Variable;

class BindVarHolder : boost::noncopyable
{
public:
	int  Append  (OciConnect&, const BindMapKey& name, EBindVariableTypes type, ub2 size=1);
	void DoBinds (OCI8::AutoCursor& cursor, const vector<string>& bindVars);

    arg::counted_ptr<OCI8::Variable> GetBindVariable (const std::string name); 
    void GetTextData (vector<string>& data) const; // in format 3 x N rows

private:
    void GetTypeString (const OCI8::Variable&, std::string&) const;
	BindMap m_boundFields; /**< Maps variable names to variable storage.*/
};

/** @brief Stores and processes data for bind grids.
 */
class BindGridSource : public OG2::GridStringSource
{
public:
    BindGridSource ();
    ~BindGridSource ();

	void Refresh (const vector<string>&);
};

#endif//__BINDSOURCE_H__
