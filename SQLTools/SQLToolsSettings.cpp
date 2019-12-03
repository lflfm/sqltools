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

// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 07.02.2005 (Ken Clubok) R1105003: Bind variables

#include "stdafx.h"
#include "SQLToolsSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    CMN_IMPL_PROPERTY_BINDER(SQLToolsSettings);

    CMN_IMPL_PROPERTY(SQLToolsSettings, DateFormat              ,"dd.mm.yy"); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, Autocommit              ,false); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, CommitOnDisconnect      ,2 /*confirm*/); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectCache             ,true); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, OutputEnable            ,true); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, OutputSize              ,0); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, SessionStatistics       ,false); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, ScanForSubstitution     ,false); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, PlanTable               ,"PLAN_TABLE"); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, CancelQueryDelay        ,1); // obsolete
    CMN_IMPL_PROPERTY(SQLToolsSettings, TopmostCancelQuery      ,true); // obsolete
    CMN_IMPL_PROPERTY(SQLToolsSettings, CancelConfirmation      ,true); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, TextExplainPlan         ,false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, KeepAlivePeriod         ,0); // in mins
    
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridMaxColLength        ,16);    
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridMultilineCount      ,3);     
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNullRepresentation  ,"");    
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridAllowLessThanHeader ,false);  
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridAllowRememColWidth  ,true);  
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridColumnFitType       ,0);     

    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNlsNumberFormat     ,"");      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNlsDateFormat       ,"");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNlsTimeFormat       ,"");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNlsTimestampFormat  ,"");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridNlsTimestampTzFormat,"");

    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpFormat           ,0);     
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpWithHeader       ,0);     
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpCommaChar        ,",");   
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpQuoteChar        ,"\"");  
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpQuoteEscapeChar  ,"\"");  
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpPrefixChar       ,"=");   
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpColumnNameAsAttr ,false); 
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpFilenameTemplate ,"&script_&yyyy-&mm-&dd_&n");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpXmlRootElement   , "data");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpXmlRecordElement , "record");
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridExpXmlFieldElementCase, 1);

    CMN_IMPL_PROPERTY(SQLToolsSettings, GridPrefetchLimit       ,100);      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridMaxLobLength        ,8 * 1024);      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridBlobHexRowLength    ,32);      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridSkipLobs            ,false);      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridTimestampFormat     ,"dd.mm.yy hh24:mi ss:ff3");      
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridHeadersInLowerCase  ,true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, GridWraparound          ,true);

    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryEnabled          ,true);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryAction           ,Paste);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryKeepSelection    ,true);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryMaxCout          ,100);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryMaxSize          ,64);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryValidOnly        ,true);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryDistinct         ,false);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryInherit          ,false);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryPersitent        ,false);   
    CMN_IMPL_PROPERTY(SQLToolsSettings, HistoryGlobal           ,false);   

    CMN_IMPL_PROPERTY(SQLToolsSettings, GroupByDDL              ,true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, PreloadDictionary       ,false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, PreloadStartPercent     ,7);
    CMN_IMPL_PROPERTY(SQLToolsSettings, BodyWithSpec            ,false); // package body with specification
    CMN_IMPL_PROPERTY(SQLToolsSettings, SpecWithBody            ,false); // specification with package body

    CMN_IMPL_PROPERTY(SQLToolsSettings, MainWindowFixedPane                ,1);
    CMN_IMPL_PROPERTY(SQLToolsSettings, MainWindowFixedPaneDefaultHeight   ,150);

    CMN_IMPL_PROPERTY(SQLToolsSettings, SQLPlusExecutable,  "$(SQLPLUS)");
    CMN_IMPL_PROPERTY(SQLToolsSettings, SQLPlusCommandLine, "$(USER)/$(PASSWORD)@$(CONNECT_STRING) $(SYS_PRIVS) @$(FILENAME)");

    CMN_IMPL_PROPERTY(SQLToolsSettings, ExecutionStyle,     1); // new sqltools style
    CMN_IMPL_PROPERTY(SQLToolsSettings, ExecutionStyleAlt,  2); // old toad style (blank line separated)
    CMN_IMPL_PROPERTY(SQLToolsSettings, ExecutionHaltOnError, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, HighlightExecutedStatement, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, AutoscrollExecutedStatement, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, AutoscrollToExecutedStatement, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, BufferedExecutionOutput, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, IgnoreCommmentsAfterSemicolon, true);

    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerSortTabColAlphabetically, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerSortPkgProcAlphabetically, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerUseCache, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerMaxLineLength, 60);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerCopyProcedureWithPackageName, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerShowComments, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerClassicTreeLook, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerInfotips, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerCopyPasteOnDoubleClick, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerQueryInActiveWindow, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectViewerRetainFocusInEditor, false);

    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectsListSynonymWithoutObjectInvalid, true);
    CMN_IMPL_PROPERTY(SQLToolsSettings, ObjectsListQueryInActiveWindow, false);
    CMN_IMPL_PROPERTY(SQLToolsSettings, QueryInLowerCase, false);


SQLToolsSettings::SQLToolsSettings ()
{
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);

    m_vasets.push_back(VisualAttributesSet());
    m_vasets.at(0).SetName("Data Grid Window");
    m_vasets.at(0).Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_vasets.at(0).Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

    m_vasets.push_back(VisualAttributesSet());
    m_vasets.at(1).SetName("Statistics Window");
    m_vasets.at(1).Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_vasets.at(1).Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

    m_vasets.push_back(VisualAttributesSet());
    m_vasets.at(2).SetName("Output Window");
    m_vasets.at(2).Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_vasets.at(2).Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));
    
    m_vasets.push_back(VisualAttributesSet());
    m_vasets.at(3).SetName("History Window");
    m_vasets.at(3).Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_vasets.at(3).Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

    m_vasets.push_back(VisualAttributesSet());
	m_vasets.at(4).SetName("Bind Window");
    m_vasets.at(4).Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_vasets.at(4).Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));
}

const VisualAttributesSet& SQLToolsSettings::GetVASet (const string& name) const
{
    Vasets::const_iterator it = m_vasets.begin();
    for (; it != m_vasets.end(); ++it)
        if (it->GetName() == name)
            return *it;

    _CHECK_AND_THROW_(0, (string("VisualAttributesSet \"") + name + "\" not found!").c_str());
}

std::vector<VisualAttributesSet*> SQLToolsSettings::GetVASets ()
{
    std::vector<VisualAttributesSet*> vasets;

    Vasets::iterator it = m_vasets.begin();
    for (; it != m_vasets.end(); ++it)
        vasets.push_back(&(*it));

    return vasets;
}
