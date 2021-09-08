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

#include "stdafx.h"
#include "ExtractDDLSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    CMN_IMPL_PROPERTY_BINDER(ExtractDDLSettings);

	CMN_IMPL_PROPERTY(ExtractDDLSettings, OptimalViewOrder,       true);
    CMN_IMPL_PROPERTY(ExtractDDLSettings, GroupByDDL,             true);

	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractCode,            true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractTables,          true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractTriggers,        true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractViews,           true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractSequences,       true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractSynonyms,        true);
	CMN_IMPL_PROPERTY(ExtractDDLSettings, ExtractGrantsByGrantee, true);

    CMN_IMPL_PROPERTY(ExtractDDLSettings, UseDbAlias,             false);
    CMN_IMPL_PROPERTY(ExtractDDLSettings, UseDbAliasAs,           0);

    CMN_IMPL_PROPERTY(ExtractDDLSettings, TableFolder             ,L"Table");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TriggerFolder           ,L"Trigger");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ViewFolder              ,L"View");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, SequenceFolder          ,L"Sequence");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeFolder              ,L"Type");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeBodyFolder          ,L"TypeBody");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, FunctionFolder          ,L"Function");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ProcedureFolder         ,L"Procedure");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageFolder           ,L"Package");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageBodyFolder       ,L"PackageBody");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, GrantFolder             ,L"Grant");
                                                              
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TableExt                ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TriggerExt              ,L"sql"); 
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ViewExt                 ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, SequenceExt             ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeExt                 ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeBodyExt             ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, FunctionExt             ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ProcedureExt            ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageExt              ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageBodyExt          ,L"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, GrantExt                ,L"sql");


ExtractDDLSettings::ExtractDDLSettings ()
{
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
};
