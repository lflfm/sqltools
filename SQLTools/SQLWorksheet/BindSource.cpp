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


#include "stdafx.h"
#include "BindSource.h"
#include "SQLTools.h"
#include "OCI8/ACursor.h"
#include "ScriptAnalyser.h"
#include "Common\StrHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;
    using namespace OCI8;

/** @brief Default constructor for BindGridSource.
 */
BindGridSource::BindGridSource ()
{
}

/** @brief Empty destructor for BindGridSource.
 */
BindGridSource::~BindGridSource ()
{
}

/** @brief Refreshes string table from bind map.
 *  This should be called when bind variables are added to the map, or the values changed by SQL.
 */
void BindGridSource::Refresh(const vector<string>& data)
{
	GridStringSource::Clear();
    vector<string>::const_iterator it;
	
    for (it = data.begin(); it != data.end(); ++it)
	{
		SetEmpty(false);

		int row = GridStringSource::Append();
		
        SetString(row, 0, *it);

        if (++it == data.end()) break;

		SetString(row, 1, *it);

        if (++it == data.end()) break;

		SetString(row, 2, *it);
	}
}

/** @brief Adds a bind variable to the grid.
 *  @arg name: Name of new bind variable
 *  @arg type: Datatype of new variable
 *  @arg size(optional): Size of new variable - defaults to 1.
 */
int BindVarHolder::Append(OciConnect& connect, const BindMapKey& name, EBindVariableTypes type, ub2 size)
{
    // TODO: consider a new properties page for BindVariables
	int gridMaxLobLength = GetSQLToolsSettings().GetGridMaxLobLength();

	counted_ptr<Variable> fld;
	switch (type)
	{
    case EBVT_NUMBER:
        // We cannot use NumStrVar because NumStrVar is actually a string
        // this might be critical for calling a right overload procedure
		fld = counted_ptr<Variable>(new NumberVar(connect, "TM9"));
        break;
    case EBVT_VARCHAR2:
		fld = counted_ptr<Variable>(new StringVar(size));
        break;
    case EBVT_DATE:
		fld = counted_ptr<Variable>(new DateVar(connect, GetSQLToolsSettings().GetGridNlsDateFormat()));
        break;
    case EBVT_CLOB:
		fld = counted_ptr<Variable>(new CLobVar(connect, gridMaxLobLength));
        break;
    case EBVT_REFCURSOR:
		fld = counted_ptr<Variable>(new RefCursorVariable(connect));
        break;
    }

    _CHECK_AND_THROW_(fld.get(), "Unsupported bind variable type!" );

	m_boundFields[name] = fld;

	return(m_boundFields.size()-1);
}

/** @brief Performs database bind calls.
 *  @arg cursor: Statement to bind - must already be prepared.
 *  @arg bindVars: list of bind variables names - we expect that all of them are declared.
 */
void BindVarHolder::DoBinds(OCI8::AutoCursor& cursor, const vector<string>& bindVars)
{
    vector<string>::const_iterator bindIt = bindVars.begin();

	for (; bindIt != bindVars.end(); ++bindIt)
	{
	    string name;
		Common::to_upper_str(bindIt->c_str(), name);
		BindMap :: iterator it = m_boundFields.find(name);

		if (it != m_boundFields.end())
			cursor.Bind(it->second.get(), name);
	}
}

arg::counted_ptr<OCI8::Variable> BindVarHolder::GetBindVariable (const std::string name)
{
	BindMap :: iterator it = m_boundFields.find(name);

	if (it != m_boundFields.end())
		return it->second;

    return 0;
}

void BindVarHolder::GetTextData (vector<string>& _data) const
{
    vector<string> data;

	BindMap :: const_iterator it;
	for (it = m_boundFields.begin(); it != m_boundFields.end(); it++)
	{
        data.push_back(it->first); //name
        string type;
        GetTypeString(*it->second, type);
        data.push_back(type);
        string value;
		it->second->GetString(value);
        data.push_back(value);
	}

    _data.swap(data);
}

// TODO#3: add NVARCHAR and NCLOB
void BindVarHolder::GetTypeString (const Variable& var, string& str) const
{
    const type_info& ti = typeid(var);

    if (ti == typeid(NumberVar))
    {
		str.assign("NUMBER");
    }
	else if (ti == typeid(StringVar))
	{
	    char s_size[18];
	    itoa(((StringVar&)var).GetStringBufSize(), s_size, 10);
		str.assign("VARCHAR2(");
		str.append(s_size);
		str.append(")");
    }
    else if (ti == typeid(DateVar))
    {
		str.assign("DATE");
    }
    else if (ti == typeid(CLobVar))
    {
		str.assign("CLOB");
    }
    else if (ti == typeid(RefCursorVariable))
    {
		str.assign("REFCURSOR");
    }
    else
    {
        ASSERT(0);
		str.assign("Unknown");
    }
}
