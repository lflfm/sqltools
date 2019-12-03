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

#ifndef __SQLToolsSettings_h__
#define __SQLToolsSettings_h__

#include <string>
#include <vector>
#include <MetaDict/MetaSettings.h>
#include <OpenEditor/OESettings.h>

    using std::string;
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;
    using OpenEditor::SettingsSubscriber;

class SQLToolsSettings : public OpenEditor::BaseSettings, public OraMetaDict::WriteSettings
{
    friend class CPropScriptPage;
    friend class CPropServerPage;
    friend class CPropGridPage1;
    friend class CPropGridPage2;
    friend class CPropGridOutputPage1;
    friend class CPropGridOutputPage2;
    friend class CPropHistoryPage;
    friend class CDBSCommonPage;
    friend class CDBSTablePage;
    friend class CObjectViewerPage;

public:
    enum HistoryAction {
        Copy = 0,
        Paste = 1,
        ReplaceAll = 2
    };

    SQLToolsSettings ();

    CMN_DECL_PROPERTY_BINDER(SQLToolsSettings);
    OES_DECL_STREAMABLE_PROPERTY(string, DateFormat              );
    OES_DECL_STREAMABLE_PROPERTY(bool,   Autocommit              );
    OES_DECL_STREAMABLE_PROPERTY(int,    CommitOnDisconnect      ); // 0-rollback,1-commit,2-confirm
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectCache             );
    OES_DECL_STREAMABLE_PROPERTY(bool,   OutputEnable            );
    OES_DECL_STREAMABLE_PROPERTY(int,    OutputSize              );
    OES_DECL_STREAMABLE_PROPERTY(bool,   SessionStatistics       );
    OES_DECL_STREAMABLE_PROPERTY(bool,   ScanForSubstitution     );
    OES_DECL_STREAMABLE_PROPERTY(string, PlanTable               );
    OES_DECL_STREAMABLE_PROPERTY(int,    CancelQueryDelay        ); // obsolete
    OES_DECL_STREAMABLE_PROPERTY(bool,   TopmostCancelQuery      ); // obsolete
    OES_DECL_STREAMABLE_PROPERTY(bool,   CancelConfirmation      ); 
    OES_DECL_STREAMABLE_PROPERTY(bool,   TextExplainPlan         );
    OES_DECL_STREAMABLE_PROPERTY(int,    KeepAlivePeriod         ); // in mins
    
    OES_DECL_STREAMABLE_PROPERTY(int,    GridMaxColLength        );
    OES_DECL_STREAMABLE_PROPERTY(int,    GridMultilineCount      );
    OES_DECL_STREAMABLE_PROPERTY(string, GridNullRepresentation  );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridAllowLessThanHeader );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridAllowRememColWidth  );
    OES_DECL_STREAMABLE_PROPERTY(int,    GridColumnFitType       );

    OES_DECL_STREAMABLE_PROPERTY(string, GridNlsNumberFormat     );
    OES_DECL_STREAMABLE_PROPERTY(string, GridNlsDateFormat       );
    OES_DECL_STREAMABLE_PROPERTY(string, GridNlsTimeFormat       );
    OES_DECL_STREAMABLE_PROPERTY(string, GridNlsTimestampFormat  );
    OES_DECL_STREAMABLE_PROPERTY(string, GridNlsTimestampTzFormat);

    OES_DECL_STREAMABLE_PROPERTY(int,    GridExpFormat           );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridExpWithHeader       );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpCommaChar        );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpQuoteChar        );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpQuoteEscapeChar  );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpPrefixChar  );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridExpColumnNameAsAttr );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpFilenameTemplate );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpXmlRootElement   );
    OES_DECL_STREAMABLE_PROPERTY(string, GridExpXmlRecordElement );
    OES_DECL_STREAMABLE_PROPERTY(int,    GridExpXmlFieldElementCase);

    OES_DECL_STREAMABLE_PROPERTY(int,    GridPrefetchLimit       );
    OES_DECL_STREAMABLE_PROPERTY(int,    GridMaxLobLength        );
    OES_DECL_STREAMABLE_PROPERTY(int,    GridBlobHexRowLength    );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridSkipLobs            );
    OES_DECL_STREAMABLE_PROPERTY(string, GridTimestampFormat     );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridHeadersInLowerCase  );
    OES_DECL_STREAMABLE_PROPERTY(bool,   GridWraparound          );

    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryEnabled          );
    OES_DECL_STREAMABLE_PROPERTY(int,    HistoryAction           );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryKeepSelection    );
    OES_DECL_STREAMABLE_PROPERTY(int,    HistoryMaxCout          );
    OES_DECL_STREAMABLE_PROPERTY(int,    HistoryMaxSize          );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryValidOnly        );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryDistinct         );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryInherit          );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryPersitent        );
    OES_DECL_STREAMABLE_PROPERTY(bool,   HistoryGlobal           );

    // DDL settings
    OES_DECL_STREAMABLE_PROPERTY(bool,   GroupByDDL              );
    OES_DECL_STREAMABLE_PROPERTY(bool,   PreloadDictionary       );
    OES_DECL_STREAMABLE_PROPERTY(int,    PreloadStartPercent     );
    OES_DECL_STREAMABLE_PROPERTY(bool,   BodyWithSpec            ); // package body with specification
    OES_DECL_STREAMABLE_PROPERTY(bool,   SpecWithBody            ); // specification with package body

    OES_DECL_STREAMABLE_PROPERTY(int,    MainWindowFixedPane             );
    OES_DECL_STREAMABLE_PROPERTY(int,    MainWindowFixedPaneDefaultHeight);

    OES_DECL_STREAMABLE_PROPERTY(string, SQLPlusExecutable);
    OES_DECL_STREAMABLE_PROPERTY(string, SQLPlusCommandLine);

    OES_DECL_STREAMABLE_PROPERTY(int,    ExecutionStyle);
    OES_DECL_STREAMABLE_PROPERTY(int,    ExecutionStyleAlt);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ExecutionHaltOnError);
    OES_DECL_STREAMABLE_PROPERTY(bool,   HighlightExecutedStatement);
    OES_DECL_STREAMABLE_PROPERTY(bool,   AutoscrollExecutedStatement);
    OES_DECL_STREAMABLE_PROPERTY(bool,   AutoscrollToExecutedStatement);
    OES_DECL_STREAMABLE_PROPERTY(bool,   BufferedExecutionOutput);
    OES_DECL_STREAMABLE_PROPERTY(bool,   IgnoreCommmentsAfterSemicolon);

    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerSortTabColAlphabetically);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerSortPkgProcAlphabetically);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerUseCache);
    OES_DECL_STREAMABLE_PROPERTY(int,    ObjectViewerMaxLineLength);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerCopyProcedureWithPackageName);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerShowComments);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerClassicTreeLook);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerInfotips);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerCopyPasteOnDoubleClick);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerQueryInActiveWindow);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectViewerRetainFocusInEditor);

    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectsListSynonymWithoutObjectInvalid);
    OES_DECL_STREAMABLE_PROPERTY(bool,   ObjectsListQueryInActiveWindow);

    OES_DECL_STREAMABLE_PROPERTY(bool,   QueryInLowerCase);


    typedef std::vector<VisualAttributesSet> Vasets;
    Vasets m_vasets;

    const VisualAttributesSet& GetVASet (const string&) const;
    std::vector<VisualAttributesSet*> GetVASets ();
};

#endif//__SQLToolsSettings_h__