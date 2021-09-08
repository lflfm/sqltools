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

#pragma once
#ifndef __ExtractDDLSettings_H__
#define __ExtractDDLSettings_H__

#include <string>
#include "MetaDict/MetaSettings.h"

    using std::string;

class ExtractDDLSettings : public OraMetaDict::WriteSettings
{
public:
    CMN_DECL_PROPERTY_BINDER(ExtractDDLSettings);
    ExtractDDLSettings ();

	CMN_DECL_PUBLIC_PROPERTY(bool,   OptimalViewOrder);
    CMN_DECL_PUBLIC_PROPERTY(bool,   GroupByDDL);

	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractCode);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractTables);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractTriggers);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractViews);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractSequences);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractSynonyms);
	CMN_DECL_PUBLIC_PROPERTY(bool,   ExtractGrantsByGrantee);

    CMN_DECL_PUBLIC_PROPERTY(bool,   UseDbAlias);
    CMN_DECL_PUBLIC_PROPERTY(int,    UseDbAliasAs);

    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TableFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TriggerFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, ViewFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, SequenceFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TypeFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TypeBodyFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, FunctionFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, ProcedureFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, PackageFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, PackageBodyFolder);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, GrantFolder);

    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TableExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TriggerExt); 
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, ViewExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, SequenceExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TypeExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, TypeBodyExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, FunctionExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, ProcedureExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, PackageExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, PackageBodyExt);
    CMN_DECL_PUBLIC_PROPERTY(std::wstring, GrantExt);

public:
    std::wstring	
        m_strSchema,
	    m_strFolder,
        m_strDbAlias,
        m_strTableTablespace,
        m_strIndexTablespace;
};

#endif//__ExtractDDLSettings_H__
