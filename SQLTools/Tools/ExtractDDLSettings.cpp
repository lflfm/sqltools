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

    CMN_IMPL_PROPERTY(ExtractDDLSettings, TableFolder             ,"Table");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TriggerFolder           ,"Trigger");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ViewFolder              ,"View");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, SequenceFolder          ,"Sequence");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeFolder              ,"Type");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeBodyFolder          ,"TypeBody");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, FunctionFolder          ,"Function");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ProcedureFolder         ,"Procedure");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageFolder           ,"Package");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageBodyFolder       ,"PackageBody");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, GrantFolder             ,"Grant");
                                                              
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TableExt                ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TriggerExt              ,"sql"); 
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ViewExt                 ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, SequenceExt             ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeExt                 ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, TypeBodyExt             ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, FunctionExt             ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, ProcedureExt            ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageExt              ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, PackageBodyExt          ,"sql");
    CMN_IMPL_PROPERTY(ExtractDDLSettings, GrantExt                ,"sql");


ExtractDDLSettings::ExtractDDLSettings ()
{
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
};
