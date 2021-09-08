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
#include "SQLTools.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/GUICommandDictionary.h"
#include <COMMON/ConfigFilesLocator.h>
#include "OpenEditor/OEDocument.h"
#include "MainFrm.h"
#include "CommandLineParser.h"
#include <sstream>
#include "ConnectionTasks.h"

/*
    30.03.2003 improvement, command line support, try sqltools /h to get help
    24.09.2007 some improvements taken from sqltools++ by Randolf Geist
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Common;

    const int SQLTOOLS_COPYDATA_ID = 2;

 /*
 TODO Map the following commands

ID_GRID_COPY_AS_TEXT            
ID_GRID_COPY_AS_PLSQL_VALUES    
ID_GRID_COPY_AS_PLSQL_EXPRESSION
ID_GRID_COPY_AS_PLSQL_IN_LIST   
ID_GRID_COPY_AS_PLSQL_INSERTS   

ID_GRID_POPUP                   
ID_GRIDPOPUP_TEXT               
ID_GRIDPOPUP_HTML               
ID_GRIDPOPUP_WORDWRAP           
ID_GRIDPOPUP_CLOSE              
*/

void CSQLToolsApp::InitGUICommand ()
{
#if (ID_APP_DBLKEYACCEL_FIRST != 0xDF00 || ID_APP_DBLKEYACCEL_LAST != 0xDFFF)
#pragma error("Check ID_APP_DBLKEYACCEL_FIRST or ID_APP_DBLKEYACCEL_LAST definition")
#endif

    if (!GUICommandDictionary::m_firstDblKeyAccelCommandId)
    {
    GUICommandDictionary::m_firstDblKeyAccelCommandId = ID_APP_DBLKEYACCEL_FIRST;

//File
    GUICommandDictionary::InsertCommand("File.New",                        ID_FILE_NEW);
    GUICommandDictionary::InsertCommand("File.NewExtended",                ID_FILE_NEW_EXT);
    GUICommandDictionary::InsertCommand("File.Open",                       ID_FILE_OPEN);
    GUICommandDictionary::InsertCommand("File.Reload",                     ID_FILE_RELOAD);
    GUICommandDictionary::InsertCommand("File.Close",                      ID_FILE_CLOSE);
    GUICommandDictionary::InsertCommand("File.CloseAll",                   ID_FILE_CLOSE_ALL);
    GUICommandDictionary::InsertCommand("File.Save",                       ID_FILE_SAVE);
    GUICommandDictionary::InsertCommand("File.SaveAs",                     ID_FILE_SAVE_AS);
    GUICommandDictionary::InsertCommand("File.SaveAll",                    ID_FILE_SAVE_ALL);
    GUICommandDictionary::InsertCommand("File.ShowFileLocation",           ID_FILE_SYNC_LOCATION);
    GUICommandDictionary::InsertCommand("File.CopyFileLocation",           ID_FILE_COPY_LOCATION);
    GUICommandDictionary::InsertCommand("File.PageSetup",                  ID_EDIT_PRINT_PAGE_SETUP);
    GUICommandDictionary::InsertCommand("File.PrintSetup",                 ID_FILE_PRINT_SETUP);
    GUICommandDictionary::InsertCommand("File.PrintPreview",               ID_FILE_PRINT_PREVIEW);
    GUICommandDictionary::InsertCommand("File.Print",                      ID_FILE_PRINT);
    GUICommandDictionary::InsertCommand("File.Exit",                       ID_APP_EXIT);
//Edit
    GUICommandDictionary::InsertCommand("Edit.Undo",                       ID_EDIT_UNDO);
    GUICommandDictionary::InsertCommand("Edit.Undo",                       ID_EDIT_UNDO);
    GUICommandDictionary::InsertCommand("Edit.Redo",                       ID_EDIT_REDO);
    GUICommandDictionary::InsertCommand("Edit.Cut",                        ID_EDIT_CUT);
    GUICommandDictionary::InsertCommand("Edit.Cut",                        ID_EDIT_CUT);
    GUICommandDictionary::InsertCommand("Edit.Copy",                       ID_EDIT_COPY);
    GUICommandDictionary::InsertCommand("Edit.Copy",                       ID_EDIT_COPY);
    GUICommandDictionary::InsertCommand("Edit.Paste",                      ID_EDIT_PASTE);
    GUICommandDictionary::InsertCommand("Edit.Paste",                      ID_EDIT_PASTE);
    GUICommandDictionary::InsertCommand("Edit.CutAppend",                  ID_EDIT_CUT_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CutAppend",                  ID_EDIT_CUT_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CutBookmarkedLines",         ID_EDIT_CUT_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.CopyAppend",                 ID_EDIT_COPY_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CopyAppend",                 ID_EDIT_COPY_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CopyBookmarkedLines",        ID_EDIT_COPY_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.Delete",                     ID_EDIT_DELETE);
    GUICommandDictionary::InsertCommand("Edit.DeletetoEndOfWord",          ID_EDIT_DELETE_WORD_TO_RIGHT);
    GUICommandDictionary::InsertCommand("Edit.DeleteWordBack",             ID_EDIT_DELETE_WORD_TO_LEFT);
    GUICommandDictionary::InsertCommand("Edit.DeleteLine",                 ID_EDIT_DELETE_LINE);
    GUICommandDictionary::InsertCommand("Edit.DeleteBookmarkedLines",      ID_EDIT_DELETE_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.SelectWord",                 ID_EDIT_SELECT_WORD);
    GUICommandDictionary::InsertCommand("Edit.SelectLine",                 ID_EDIT_SELECT_LINE);
    GUICommandDictionary::InsertCommand("Edit.SelectAll",                  ID_EDIT_SELECT_ALL);
    GUICommandDictionary::InsertCommand("Edit.ScrollUp",                   ID_EDIT_SCROLL_UP);
    GUICommandDictionary::InsertCommand("Edit.ScrollToCenter",             ID_EDIT_SCROLL_CENTER);
    GUICommandDictionary::InsertCommand("Edit.ScrollToCenter",             ID_EDIT_SCROLL_CENTER);
    GUICommandDictionary::InsertCommand("Edit.ScrollDown",                 ID_EDIT_SCROLL_DOWN);
    GUICommandDictionary::InsertCommand("Edit.DupLineOrSelection",         ID_EDIT_DUP_LINE_OR_SELECTION);
//Search
    GUICommandDictionary::InsertCommand("Edit.Find",                       ID_EDIT_FIND);
    GUICommandDictionary::InsertCommand("Edit.Replace",                    ID_EDIT_REPLACE);
    GUICommandDictionary::InsertCommand("Edit.FindNext",                   ID_EDIT_FIND_NEXT);
    GUICommandDictionary::InsertCommand("Edit.FindPrevious",               ID_EDIT_FIND_PREVIOUS);
    GUICommandDictionary::InsertCommand("Edit.FindSelectedNext",           ID_EDIT_FIND_SELECTED_NEXT    );
    GUICommandDictionary::InsertCommand("Edit.FindSelectedPrevious",       ID_EDIT_FIND_SELECTED_PREVIOUS);
    GUICommandDictionary::InsertCommand("Edit.GoTo",                       ID_EDIT_GOTO);
    GUICommandDictionary::InsertCommand("Edit.JumpNext",                   ID_EDIT_NEXT);
    GUICommandDictionary::InsertCommand("Edit.JumpPrevious",               ID_EDIT_PREVIOUS);
    GUICommandDictionary::InsertCommand("Edit.ToggleBookmark",             ID_EDIT_BKM_TOGGLE);
    GUICommandDictionary::InsertCommand("Edit.NextBookmark",               ID_EDIT_BKM_NEXT);
    GUICommandDictionary::InsertCommand("Edit.PreviousBookmark",           ID_EDIT_BKM_PREV);
    GUICommandDictionary::InsertCommand("Edit.RemoveallBookmarks",         ID_EDIT_BKM_REMOVE_ALL);
//SetRandomBookmark
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.0",        ID_EDIT_BKM_SET_0);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.1",        ID_EDIT_BKM_SET_1);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.2",        ID_EDIT_BKM_SET_2);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.3",        ID_EDIT_BKM_SET_3);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.4",        ID_EDIT_BKM_SET_4);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.5",        ID_EDIT_BKM_SET_5);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.6",        ID_EDIT_BKM_SET_6);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.7",        ID_EDIT_BKM_SET_7);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.8",        ID_EDIT_BKM_SET_8);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.9",        ID_EDIT_BKM_SET_9);
//GetRandomBookmark
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.0",        ID_EDIT_BKM_GET_0);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.1",        ID_EDIT_BKM_GET_1);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.2",        ID_EDIT_BKM_GET_2);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.3",        ID_EDIT_BKM_GET_3);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.4",        ID_EDIT_BKM_GET_4);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.5",        ID_EDIT_BKM_GET_5);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.6",        ID_EDIT_BKM_GET_6);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.7",        ID_EDIT_BKM_GET_7);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.8",        ID_EDIT_BKM_GET_8);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.9",        ID_EDIT_BKM_GET_9);
    GUICommandDictionary::InsertCommand("Edit.FindMatch",                  ID_EDIT_FIND_MATCH);
    GUICommandDictionary::InsertCommand("Edit.FindMatchSelect",            ID_EDIT_FIND_MATCH_N_SELECT);
    GUICommandDictionary::InsertCommand("Edit.FindInFile",                 ID_FILE_FIND_IN_FILE);
//Format
    GUICommandDictionary::InsertCommand("Edit.Sort",                       ID_EDIT_SORT);
    GUICommandDictionary::InsertCommand("Edit.IndentSelection",            ID_EDIT_INDENT);
    GUICommandDictionary::InsertCommand("Edit.UndentSelection",            ID_EDIT_UNDENT);
    GUICommandDictionary::InsertCommand("Edit.UntabifySelection",          ID_EDIT_UNTABIFY);
    GUICommandDictionary::InsertCommand("Edit.TabifySelectionAll",         ID_EDIT_TABIFY);
    GUICommandDictionary::InsertCommand("Edit.TabifySelectionLeading",     ID_EDIT_TABIFY_LEADING);
    GUICommandDictionary::InsertCommand("Edit.NormalizeKeyword",           ID_EDIT_NORMALIZE_TEXT);
    GUICommandDictionary::InsertCommand("Edit.Lowercase",                  ID_EDIT_LOWER);
    GUICommandDictionary::InsertCommand("Edit.Uppercase",                  ID_EDIT_UPPER);
    GUICommandDictionary::InsertCommand("Edit.Capitalize",                 ID_EDIT_CAPITALIZE);
    GUICommandDictionary::InsertCommand("Edit.InvertCase",                 ID_EDIT_INVERT_CASE);
    GUICommandDictionary::InsertCommand("Edit.CommentLine",                ID_EDIT_COMMENT);
    GUICommandDictionary::InsertCommand("Edit.UncommentLine",              ID_EDIT_UNCOMMENT);
    GUICommandDictionary::InsertCommand("Edit.ExpandTemplate",             ID_EDIT_EXPAND_TEMPLATE);
    GUICommandDictionary::InsertCommand("Edit.CreateTemplate",             ID_EDIT_CREATE_TEMPLATE);
    GUICommandDictionary::InsertCommand("Edit.Autocomplete",               ID_EDIT_AUTOCOMPLETE);
    GUICommandDictionary::InsertCommand("Edit.DatetimeStamp",              ID_EDIT_DATETIME_STAMP);
    GUICommandDictionary::InsertCommand("Edit.OpenFileUnderCursor",        ID_EDIT_OPENFILEUNDERCURSOR);
//Text/Format
    GUICommandDictionary::InsertCommand("Edit.JoinLines",                  ID_EDIT_FORMAT_JOIN_LINES);
    GUICommandDictionary::InsertCommand("Edit.SplitLine",                  ID_EDIT_FORMAT_SPLIT_LINES);
    GUICommandDictionary::InsertCommand("Edit.RemoveBlankLines",           ID_EDIT_FORMAT_REMOVE_BLANK);
    GUICommandDictionary::InsertCommand("Edit.RemoveExcessiveBlankLines",  ID_EDIT_FORMAT_REMOVE_EXCESSIVE_BLANK);
    GUICommandDictionary::InsertCommand("Edit.TrimLeadingSpaces",          ID_EDIT_FORMAT_TRIM_LEADING_SPACES);
    GUICommandDictionary::InsertCommand("Edit.TrimExcessiveSpaces",        ID_EDIT_FORMAT_TRIM_EXCESSIVE_SPACES);
    GUICommandDictionary::InsertCommand("Edit.TrimTrailingSpaces",         ID_EDIT_FORMAT_TRIM_TRAILING_SPACES);

    GUICommandDictionary::InsertCommand("Edit.ColumnInsert",               ID_EDIT_COLUMN_INSERT_FILL);     
    GUICommandDictionary::InsertCommand("Edit.ColumnInsertNumber",         ID_EDIT_COLUMN_INSERT_NUMBER);   
    GUICommandDictionary::InsertCommand("Edit.ColumnLeftJustify",          ID_EDIT_COLUMN_LEFT_JUSTIFY);    
    GUICommandDictionary::InsertCommand("Edit.ColumnCenterJustify",        ID_EDIT_COLUMN_CENTER_JUSTIFY);  
    GUICommandDictionary::InsertCommand("Edit.ColumnRightJustify",         ID_EDIT_COLUMN_RIGT_JUSTIFY);    

    GUICommandDictionary::InsertCommand("Edit.FileFormat_Winows",          ID_EDIT_FILE_FORMAT_WINOWS);
    GUICommandDictionary::InsertCommand("Edit.FileFormat_Unix"  ,          ID_EDIT_FILE_FORMAT_UNIX  );
    GUICommandDictionary::InsertCommand("Edit.FileFormat_Mac"   ,          ID_EDIT_FILE_FORMAT_MAC   );
//Text
    GUICommandDictionary::InsertCommand("Edit.StreamSelection",            ID_EDIT_STREAM_SEL);
    GUICommandDictionary::InsertCommand("Edit.ColumnSelection",            ID_EDIT_COLUMN_SEL);
    GUICommandDictionary::InsertCommand("Edit.ToggleSelectionMode",        ID_EDIT_TOGGLES_SEL);
    GUICommandDictionary::InsertCommand("Edit.VisibleSpaces",              ID_EDIT_VIEW_WHITE_SPACE);
    GUICommandDictionary::InsertCommand("Edit.LineNumbers",                ID_EDIT_VIEW_LINE_NUMBERS);
    GUICommandDictionary::InsertCommand("Edit.SyntaxGutter",               ID_EDIT_VIEW_SYNTAX_GUTTER);
    GUICommandDictionary::InsertCommand("Edit.Ruler",                      ID_EDIT_VIEW_RULER);
    GUICommandDictionary::InsertCommand("Edit.FileSettings",               ID_EDIT_FILE_SETTINGS);
    GUICommandDictionary::InsertCommand("Edit.PermanentSettings",          ID_APP_SETTINGS); //ID_EDIT_PERMANENT_SETTINGS
    GUICommandDictionary::InsertCommand("Edit.ColumnMarkers",              ID_EDIT_VIEW_COLUMN_MARKERS);
    GUICommandDictionary::InsertCommand("Edit.IndentGuide",                ID_EDIT_VIEW_INDENT_GUIDE);
//View
    GUICommandDictionary::InsertCommand("View.Toolbar",                    ID_VIEW_TOOLBAR);
    GUICommandDictionary::InsertCommand("View.StatusBar",                  ID_VIEW_STATUS_BAR);
    GUICommandDictionary::InsertCommand("View.FilePanel",                  ID_VIEW_FILE_PANEL);
    GUICommandDictionary::InsertCommand("View.History",                    ID_VIEW_HISTORY); // FRM
    GUICommandDictionary::InsertCommand("View.NextPane",                   ID_CUSTOM_NEXT_PANE); // there is a big difference
    GUICommandDictionary::InsertCommand("View.PrevPane",                   ID_CUSTOM_PREV_PANE); // between ID_NEXT_PANE and ID_CUSTOM_NEXT_PANE
//Window
    GUICommandDictionary::InsertCommand("Window.NewWindow",                ID_WINDOW_NEW);
    GUICommandDictionary::InsertCommand("Window.Cascade",                  ID_WINDOW_CASCADE);
    GUICommandDictionary::InsertCommand("Window.TileHorizontally",         ID_WINDOW_TILE_HORZ);
    GUICommandDictionary::InsertCommand("Window.TileVertically",           ID_WINDOW_TILE_VERT);
    GUICommandDictionary::InsertCommand("Window.ArrangeIcons",             ID_WINDOW_ARRANGE);
    GUICommandDictionary::InsertCommand("Window.LastWindow",               ID_WINDOW_LAST);
//Help
    GUICommandDictionary::InsertCommand("Help.AboutOpenEditor",            ID_APP_ABOUT);
//Workspace
    GUICommandDictionary::InsertCommand("Workspace.Copy",                  ID_WORKSPACE_COPY);
    GUICommandDictionary::InsertCommand("Workspace.Paste",                 ID_WORKSPACE_PASTE);
    GUICommandDictionary::InsertCommand("Workspace.Save",                  ID_WORKSPACE_SAVE); 
    GUICommandDictionary::InsertCommand("Workspace.SaveAs",                ID_WORKSPACE_SAVE_AS);
    GUICommandDictionary::InsertCommand("Workspace.SaveQuick",             ID_WORKSPACE_SAVE_QUICK); 
    GUICommandDictionary::InsertCommand("Workspace.Open",                  ID_WORKSPACE_OPEN);
    GUICommandDictionary::InsertCommand("Workspace.OpenQuick",             ID_WORKSPACE_OPEN_QUICK); 
    GUICommandDictionary::InsertCommand("Workspace.OpenAutosaved",         ID_WORKSPACE_OPEN_AUTOSAVED);
    GUICommandDictionary::InsertCommand("Workspace.OpenClose",             ID_WORKSPACE_CLOSE);         


//SQLTools command set
//View
    GUICommandDictionary::InsertCommand("View.OpenFiles",                  ID_VIEW_OPEN_FILES);
    GUICommandDictionary::InsertCommand("View.ObjectTree",                 ID_SQL_OBJ_VIEWER);
    GUICommandDictionary::InsertCommand("View.Properties",                 ID_VIEW_PROPERTIES);
    GUICommandDictionary::InsertCommand("View.ExplainPlan",                ID_SQL_PLAN_VIEWER);
    GUICommandDictionary::InsertCommand("View.ObjectList",                 ID_SQL_DB_SOURCE);
    GUICommandDictionary::InsertCommand("View.FindInFiles",                ID_FILE_SHOW_GREP_OUTPUT);
//Session
    GUICommandDictionary::InsertCommand("Session.Connect",                 ID_SQL_CONNECT);
    GUICommandDictionary::InsertCommand("Session.Disconnect",              ID_SQL_DISCONNECT);
    GUICommandDictionary::InsertCommand("Session.Reconnect",               ID_SQL_RECONNECT);
    GUICommandDictionary::InsertCommand("Session.Cancel",                  ID_SQL_CANCEL);
    GUICommandDictionary::InsertCommand("Session.Commit",                  ID_SQL_COMMIT);
    GUICommandDictionary::InsertCommand("Session.Rollback",                ID_SQL_ROLLBACK);
    GUICommandDictionary::InsertCommand("Session.EnableDbmsOutput",        ID_SQL_DBMS_OUTPUT);
    GUICommandDictionary::InsertCommand("Session.EnableSessionStatistics", ID_SQL_SESSION_STATISTICS);
    GUICommandDictionary::InsertCommand("Session.RefreshObjectCache",      ID_SQL_REFRESH_OBJECT_CACHE);
    GUICommandDictionary::InsertCommand("Session.ToggleReadOnly",          ID_SQL_READ_ONLY);
//Script
    GUICommandDictionary::InsertCommand("Script.Execute",                  ID_SQL_EXECUTE);
    GUICommandDictionary::InsertCommand("Script.ExecuteFromCursor",        ID_SQL_EXECUTE_FROM_CURSOR);
    GUICommandDictionary::InsertCommand("Script.HaltOnError",              ID_SQL_HALT_ON_ERROR);
    GUICommandDictionary::InsertCommand("Script.ExecuteCurrent",           ID_SQL_EXECUTE_CURRENT);
    GUICommandDictionary::InsertCommand("Script.ExecuteCurrentAlt",        ID_SQL_EXECUTE_CURRENT_ALT);
    GUICommandDictionary::InsertCommand("Script.ExecuteCurrentAndStep",    ID_SQL_EXECUTE_CURRENT_AND_STEP);
    GUICommandDictionary::InsertCommand("Script.ExecuteInSQLPlus",         ID_SQL_EXECUTE_IN_SQLPLUS);
    GUICommandDictionary::InsertCommand("Script.NextError",                ID_SQL_NEXT_ERROR);
    GUICommandDictionary::InsertCommand("Script.PreviousError",            ID_SQL_PREV_ERROR);
    GUICommandDictionary::InsertCommand("Script.GetHistoryAndStepBack",    ID_SQL_HISTORY_GET_AND_STEPBACK);
    GUICommandDictionary::InsertCommand("Script.StepForwardAndGetHistory", ID_SQL_HISTORY_STEPFORWARD_AND_GET);
    GUICommandDictionary::InsertCommand("Script.FindObject",               ID_SQL_DESCRIBE);
    GUICommandDictionary::InsertCommand("Script.QuickQuery",               ID_SQL_QUICK_QUERY);
    GUICommandDictionary::InsertCommand("Script.QuickCount",               ID_SQL_QUICK_COUNT);
    GUICommandDictionary::InsertCommand("Script.LoadDDL",                  ID_SQL_LOAD);
    GUICommandDictionary::InsertCommand("Script.LoadDDLWithDbmsMetadata",  ID_SQL_LOAD_WITH_DBMS_METADATA);
    GUICommandDictionary::InsertCommand("Script.ExplainPlan",              ID_SQL_EXPLAIN_PLAN);
    GUICommandDictionary::InsertCommand("Script.SubstitutionVariables",    ID_SQL_SUBSTITUTION_VARIABLES);
//Grid
    GUICommandDictionary::InsertCommand("Grid.ShowPopupViewer",            ID_GRID_POPUP);
//Tools
    GUICommandDictionary::InsertCommand("Tools.ExtractSchemaDDL",          ID_SQL_EXTRACT_SCHEMA);
    GUICommandDictionary::InsertCommand("Tools.TableTransformationHelper", ID_SQL_TABLE_TRANSFORMER);
    GUICommandDictionary::InsertCommand("Tools.SessionDdlGridSettings",    ID_APP_SETTINGS);
//Window
    GUICommandDictionary::InsertCommand("Window.NextPane",                 ID_CUSTOM_NEXT_PANE);
    GUICommandDictionary::InsertCommand("Window.PreviousPane",             ID_CUSTOM_PREV_PANE);
    GUICommandDictionary::InsertCommand("Window.NextTab",                  ID_CUSTOM_NEXT_TAB);
    GUICommandDictionary::InsertCommand("Window.PreviousTab",              ID_CUSTOM_PREV_TAB);
    GUICommandDictionary::InsertCommand("Window.ActivateSplitter",         ID_WINDOW_SPLIT);
    GUICommandDictionary::InsertCommand("Window.SplitterDown",             ID_CUSTOM_SPLITTER_DOWN);
    GUICommandDictionary::InsertCommand("Window.SplitterUp",               ID_CUSTOM_SPLITTER_UP);
    GUICommandDictionary::InsertCommand("Window.PrevWindow",               ID_WINDOW_PREV);
    GUICommandDictionary::InsertCommand("Window.NextWindow",               ID_WINDOW_NEXT);
//Help
    GUICommandDictionary::InsertCommand("Help.Help",                       ID_HELP);
    GUICommandDictionary::InsertCommand("Help.SqlHelp",                    ID_SQL_HELP);
    GUICommandDictionary::InsertCommand("Help.ContextHelp",                ID_CONTEXT_HELP);

    GUICommandDictionary::InsertCommand("Test",                            ID_TEST);
    }

    // only a keymap "custom" is loading from a file and can be changed by a user, 
    // all others are loaded from the application resources 
    // so we don't need to upgrade those keymaps anymore
    // what is more complicated in the case of multi-user configuration (multiple private config folders)

    std::wstring path;
    std::auto_ptr<std::istream> is;
    string keymap = COEDocument::GetSettingsManager().GetGlobalSettings()->GetKeymapLayout();
    bool custom = !stricmp(keymap.c_str(), "custom");
    keymap += ".keymap";

    if (custom && ConfigFilesLocator::GetFilePath(wstr(keymap).c_str(), path))
        is.reset(new std::ifstream(path.c_str()));
    else
    {
        string text;
        
        if (AppLoadTextFromResources(Common::wstr(keymap).c_str(), L"TEXT", text))
        {
            is.reset(new std::istringstream(text));
            if (custom) 
                std::ofstream(path.c_str(), std::ios_base::out|std::ios_base::binary) << text;
        }
        else
            THROW_APP_EXCEPTION("Cannot load \"" +  keymap + "\" from the application resources.");
    }

    GUICommandDictionary::BuildAcceleratorTable(*is.get(), m_accelTable);
}

void CSQLToolsApp::UpdateAccelAndMenu ()
{
    {
        POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
        while (pos != NULL)
        {
            CDocTemplate* pTemplate = m_pDocManager->GetNextDocTemplate(pos);
            if (pTemplate && pTemplate->IsKindOf(RUNTIME_CLASS(CMultiDocTemplate)))
            {
                ((CMultiDocTemplate*)pTemplate)->m_hAccelTable = m_accelTable;
                GUICommandDictionary::AddAccelDescriptionToMenu(((CMultiDocTemplate*)pTemplate)->m_hMenuShared);
            }
        }

        if (m_pMainWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        {
            ((CFrameWnd*)m_pMainWnd)->m_hAccelTable = m_accelTable;
            GUICommandDictionary::AddAccelDescriptionToMenu(((CFrameWnd*)m_pMainWnd)->m_hMenuDefault);
        }

    }
    
    if (m_pMainWnd->IsKindOf(RUNTIME_CLASS(CMDIMainFrame)))
        ((CMDIMainFrame*)m_pMainWnd)->UpdateViewerAccelerationKeyLabels();

    // FRM
    if (m_pMainWnd->IsKindOf(RUNTIME_CLASS(CMDIMainFrame)))
        ((CMDIMainFrame*)m_pMainWnd)->UpdateViewerAccelerationKeyLabels();
}

    void CALLBACK EXPORT DblKeyAccelTimerProc (
        HWND hWnd,          // handle of CWnd that called SetTimer
        UINT /*nMsg*/,      // WM_TIMER
        UINT nIDEvent,      // timer identification
        DWORD /*dwTime*/    // system time
    )
    {
        KillTimer(hWnd, nIDEvent);

        if (theApp.IsWaitingForSecondAccelKey())
        {
            int dblKeyAccelInx = theApp.GetDblKeyAccelInx();
            theApp.CancelWaitingForSecondAccelKey();

            std::vector<std::pair<Command,string> > commands;
            GUICommandDictionary::GetDblKeyCommnds(dblKeyAccelInx, commands);

            CMenu menu;
            menu.CreatePopupMenu();

            std::vector<std::pair<Command,string> >::const_iterator it = commands.begin();

            for (; it != commands.end(); ++it)
                menu.AppendMenu(MF_STRING|MF_ENABLED, it->first, wstr(it->second).c_str());

            CRect rect;
            AfxGetMainWnd()->GetWindowRect(rect);

            //Common::GUICommandDictionary::AddAccelDescriptionToMenu(menu.m_hMenu);
            menu.TrackPopupMenu(TPM_CENTERALIGN|TPM_VCENTERALIGN|TPM_RIGHTBUTTON,
                rect.left + rect.Width()/2, rect.top + rect.Height()/2, AfxGetMainWnd());
        }
    }


void CSQLToolsApp::OnDblKeyAccel (UINT nID)
{
    string desc;
    m_dblKeyAccelInx = nID - ID_APP_DBLKEYACCEL_FIRST;
    if (GUICommandDictionary::GetDblKeyDescription(m_dblKeyAccelInx, desc))
    {
        desc +=  " was pressed. Waiting for second key of chord...";
        Global::SetStatusText(desc.c_str(), TRUE);
        SetTimer(0, 0, 1000, DblKeyAccelTimerProc);
    }
}


BOOL CSQLToolsApp::PreTranslateMessage (MSG* pMsg)
{
    if (m_dblKeyAccelInx != -1)
    {
        Common::Command commandId;
        Common::VKey vkey = static_cast<Common::VKey>(pMsg->wParam);

        if (pMsg->message == WM_KEYDOWN
        && GUICommandDictionary::GetDblKeyAccelCmdId(m_dblKeyAccelInx, vkey, commandId))
        {
            pMsg->message = WM_NULL;
            m_dblKeyAccelInx = -1;
            Global::SetStatusText("", TRUE);
            if (m_pMainWnd) m_pMainWnd->SendMessage(WM_COMMAND, commandId);
            return FALSE;
        }
        else if((pMsg->message == WM_KEYDOWN)
            || (pMsg->message == WM_COMMAND)
            || (pMsg->message == WM_SYSCOMMAND)
            || (pMsg->message > WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST))
        {
            string desc;
            GUICommandDictionary::GetDblKeyDescription(m_dblKeyAccelInx, desc, vkey);
            desc +=  "   is not an acceleration sequence.";
            Global::SetStatusText(desc.c_str(), TRUE);
            pMsg->message = WM_NULL;
            m_dblKeyAccelInx = -1;
            MessageBeep((UINT)-1);
            return FALSE;
        }
    }

    return CWinAppEx::PreTranslateMessage(pMsg);
}


BOOL CSQLToolsApp::AllowThisInstance ()
{
    if (!m_commandLineParser.GetReuseOption())
    {
        if (m_commandLineParser.GetStartOption())
            return TRUE;

        if (COEDocument::GetSettingsManager().GetGlobalSettings()->GetAllowMultipleInstances())
            return TRUE;
    }

    const wchar_t* cszMutex = L"Kochware.SQLTools";
    const wchar_t* cszError = L"Cannot connect to another program instance. ";

    m_hMutex = CreateMutex(NULL, FALSE, cszMutex);
    if (m_hMutex == NULL)
    {
        AfxMessageBox(CString(cszError) + "CreateMutex error.", MB_OK|MB_ICONHAND);
        return TRUE;
    }

    DWORD dwWaitResult = WaitForSingleObject(m_hMutex, 3000L);
    if (dwWaitResult == WAIT_TIMEOUT)
    {
        AfxMessageBox(CString(cszError) + " WaitForMutex timeout.", MB_OK|MB_ICONHAND);
        return TRUE;
    }

    HWND hAnother = FindWindow(CMDIMainFrame::m_cszClassName, NULL);

    if (hAnother)
    {
        CString buffer;
        m_commandLineParser.Pack(buffer);

        COPYDATASTRUCT cpdata;
        cpdata.dwData = SQLTOOLS_COPYDATA_ID;
        cpdata.cbData = buffer.GetLength() * sizeof(TCHAR); // FRM
        cpdata.lpData = (cpdata.cbData) ? (LPVOID)(LPCWSTR)buffer : NULL;

        DWORD dwResult = TRUE;
        if (SendMessageTimeout(
            hAnother,                       // handle to window
            WM_COPYDATA,                    // message
            0,                              // first message parameter
            (LPARAM)&cpdata,                // second message parameter
            SMTO_ABORTIFHUNG|SMTO_BLOCK,    // send options
            1500,                           // time-out duration
            &dwResult                       // return value for synchronous call
            )
        && dwResult == TRUE)
        {
            ::SetForegroundWindow(hAnother);
            if (::IsIconic(hAnother)) OpenIcon(hAnother);
            return FALSE;
        }
        else
            AfxMessageBox(CString(cszError) + L"Request ignored or timeout.", MB_OK|MB_ICONHAND);
    }

    return TRUE;
}

BOOL CSQLToolsApp::HandleAnotherInstanceRequest (COPYDATASTRUCT* pCopyDataStruct)
{
    try { EXCEPTION_FRAME;

        if (pCopyDataStruct->dwData == SQLTOOLS_COPYDATA_ID)
        {
            if (pCopyDataStruct->cbData)
            {
                m_commandLineParser.Unpack((LPCWSTR)pCopyDataStruct->lpData, pCopyDataStruct->cbData / sizeof(TCHAR)); // FRM
                PostThreadMessage(m_msgCommandLineProcess, 0, 0);
            }
            return TRUE;
        }
    }
    _DEFAULT_HANDLER_

    return FALSE;
}


afx_msg void CSQLToolsApp::OnCommandLineProcess (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;

        m_commandLineParser.Process();

        if (m_commandLineParser.GetConnectOption())
        {
            //TODO#TEST: test re-connect from command line (check for an open transaction)

            std::string user, password, alias, port, sid, service, mode;

            if (m_commandLineParser.GetConnectionInfo(user, password, alias, port, sid, mode, service))
            {
                if (port.empty())
                    ConnectionTasks::SubmitConnectTask(user, password, alias, 
                        ConnectionTasks::ToConnectionMode(mode), false, false, 3
                    );
                else
                    ConnectionTasks::SubmitConnectTask(user, password, alias, 
                        port, !service.empty() ? service : sid, !service.empty(), 
                        ConnectionTasks::ToConnectionMode(mode), false, false, 3
                    );
                //TODO#TEST: test if connect dialog shows if connection task above fails
            }
            else
                OnSqlConnect();
        }
    }
    _DEFAULT_HANDLER_
}
