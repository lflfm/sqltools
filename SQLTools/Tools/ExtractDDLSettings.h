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
#include <OpenEditor/OEStreams.h>

    using std::string;
    using OpenEditor::Stream;
    using OpenEditor::InStream;
    using OpenEditor::OutStream;

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

    CMN_DECL_PUBLIC_PROPERTY(string, TableFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, TriggerFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, ViewFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, SequenceFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, TypeFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, TypeBodyFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, FunctionFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, ProcedureFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, PackageFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, PackageBodyFolder);
    CMN_DECL_PUBLIC_PROPERTY(string, GrantFolder);

    CMN_DECL_PUBLIC_PROPERTY(string, TableExt);
    CMN_DECL_PUBLIC_PROPERTY(string, TriggerExt); 
    CMN_DECL_PUBLIC_PROPERTY(string, ViewExt);
    CMN_DECL_PUBLIC_PROPERTY(string, SequenceExt);
    CMN_DECL_PUBLIC_PROPERTY(string, TypeExt);
    CMN_DECL_PUBLIC_PROPERTY(string, TypeBodyExt);
    CMN_DECL_PUBLIC_PROPERTY(string, FunctionExt);
    CMN_DECL_PUBLIC_PROPERTY(string, ProcedureExt);
    CMN_DECL_PUBLIC_PROPERTY(string, PackageExt);
    CMN_DECL_PUBLIC_PROPERTY(string, PackageBodyExt);
    CMN_DECL_PUBLIC_PROPERTY(string, GrantExt);

public:
    std::string	
        m_strSchema,
	    m_strFolder,
        m_strDbAlias,
        m_strTableTablespace,
        m_strIndexTablespace;
};


class ExtractDDLSettingsReader
{
public:
    ExtractDDLSettingsReader (InStream& in) : m_in(in) {}
    virtual ~ExtractDDLSettingsReader () {}

    void operator >> (ExtractDDLSettings&);

protected:
    InStream& m_in;
    int       m_version;
};


class ExtractDDLSettingsWriter
{
public:
    ExtractDDLSettingsWriter (OutStream& out) : m_out(out) {}
    virtual ~ExtractDDLSettingsWriter () {}

    void operator << (const ExtractDDLSettings&);

protected:
    OutStream& m_out;
};

#endif//__ExtractDDLSettings_H__
