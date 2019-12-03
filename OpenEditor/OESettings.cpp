/*
    Copyright (C) 2002,2005 Aleksey Kochetov

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

// 08.11.2003 bug fix, file extension is not recognized properly if it's shorter then 3 chars
// 17.01.2005 (ak) simplified settings class hierarchy
// 17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report
// 2014.05.09 bug fix, template changes are not available in already open files

#include "stdafx.h"
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OEHelpers.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

namespace OpenEditor
{
    using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// 	BaseSettings

void BaseSettings::AddSubscriber (SettingsSubscriber* psubscriber) const
{
    if (psubscriber)
        m_subscribers.insert(psubscriber);
}


void BaseSettings::RemoveSubscriber (SettingsSubscriber* psubscriber) const
{
    if (psubscriber)
        m_subscribers.erase(psubscriber);
}


void BaseSettings::NotifySettingsChanged ()
{
    std::set<SettingsSubscriber*>::iterator
        it(m_subscribers.begin()), end(m_subscribers.end());

    for (; it != end; ++it)
        (*it)->OnSettingsChanged();
}


/////////////////////////////////////////////////////////////////////////////
// 	GloalSettings
                                                                        
    CMN_IMPL_PROPERTY_BINDER(GlobalSettings);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintBlackAndWhite,               false);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintLineNumbers,                 true);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintHeader,                      "&F&b&b&D");
    CMN_IMPL_PROPERTY(GlobalSettings, PrintFooter,                      "&bPage &p of &P");
    CMN_IMPL_PROPERTY(GlobalSettings, PrintMarginMeasurement,           0);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintLeftMargin,                  20);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintRightMargin,                 10);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintTopMargin,                   10);
    CMN_IMPL_PROPERTY(GlobalSettings, PrintBottomMargin,                10);

    CMN_IMPL_PROPERTY(GlobalSettings, UndoLimit,                        1000);
    CMN_IMPL_PROPERTY(GlobalSettings, UndoMemLimit,                     1000); //kb
    CMN_IMPL_PROPERTY(GlobalSettings, UndoAfterSaving,                  true);
    CMN_IMPL_PROPERTY(GlobalSettings, TruncateSpaces,                   true);
    CMN_IMPL_PROPERTY(GlobalSettings, CursorBeyondEOL,                  false);
    CMN_IMPL_PROPERTY(GlobalSettings, CursorBeyondEOF,                  false);
    CMN_IMPL_PROPERTY(GlobalSettings, LineNumbers,                      true);
    CMN_IMPL_PROPERTY(GlobalSettings, VisibleSpaces,                    false);
    CMN_IMPL_PROPERTY(GlobalSettings, Ruler,                            true);
    CMN_IMPL_PROPERTY(GlobalSettings, TimestampFormat,                  "yyyy-mm-dd hh24:mi");
    CMN_IMPL_PROPERTY(GlobalSettings, EOFMark,                          true);
    CMN_IMPL_PROPERTY(GlobalSettings, UseSmartHome,                     true);
    CMN_IMPL_PROPERTY(GlobalSettings, UseSmartEnd,                      true);
    CMN_IMPL_PROPERTY(GlobalSettings, ColumnMarkers,                    true);
    CMN_IMPL_PROPERTY(GlobalSettings, IndentGuide,                      true);

    CMN_IMPL_PROPERTY(GlobalSettings, BlockKeepMarking,                 false);
    CMN_IMPL_PROPERTY(GlobalSettings, BlockKeepMarkingAfterDragAndDrop, true);
    CMN_IMPL_PROPERTY(GlobalSettings, BlockDelAndBSDelete,              true);
    CMN_IMPL_PROPERTY(GlobalSettings, BlockTypingOverwrite,             true);
    CMN_IMPL_PROPERTY(GlobalSettings, BlockTabIndent,                   true);
    CMN_IMPL_PROPERTY(GlobalSettings, ColBlockDeleteSpaceAfterMove,     true);
    CMN_IMPL_PROPERTY(GlobalSettings, ColBlockCursorToStartAfterPaste,  true);
    CMN_IMPL_PROPERTY(GlobalSettings, ColBlockEditMode,                 true);
    CMN_IMPL_PROPERTY(GlobalSettings, MouseSelectionDelimiters,         "");

    CMN_IMPL_PROPERTY(GlobalSettings, FileLocking,                      true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileMemMapForBig,                 true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileMemMapThreshold,              2);
    CMN_IMPL_PROPERTY(GlobalSettings, FileSaveOnSwith,                  false);
    CMN_IMPL_PROPERTY(GlobalSettings, FileDetectChanges,                true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileReloadChanges,                false);
    CMN_IMPL_PROPERTY(GlobalSettings, FileAutoscrollAfterReload,        true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileOverwriteReadonly,            false);
    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupV2,                     ebmBackupDirectory);
    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupName,                   "<NAME>.~<EXT>");
    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupDirectoryV2,            "");

    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupSpaceManagment,         false);
    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupKeepDays,               30);
    CMN_IMPL_PROPERTY(GlobalSettings, FileBackupFolderMaxSize,          50);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceAutosaveInterval,        10);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceAutosaveFileLimit,       10);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceAutosaveDeleteOnExit,    false);

    CMN_IMPL_PROPERTY(GlobalSettings, AllowMultipleInstances,           false);
    CMN_IMPL_PROPERTY(GlobalSettings, NewDocOnStartup,                  true);
    CMN_IMPL_PROPERTY(GlobalSettings, MaximizeFirstDocument,            true);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkDirFollowsDocument,           true);
    CMN_IMPL_PROPERTY(GlobalSettings, SaveCurPosAndBookmarks,           true);
    CMN_IMPL_PROPERTY(GlobalSettings, SaveMainWinPosition,              true);
    CMN_IMPL_PROPERTY(GlobalSettings, DoubleClickCloseTab,              false);
    CMN_IMPL_PROPERTY(GlobalSettings, Locale,                           "English");
    CMN_IMPL_PROPERTY(GlobalSettings, KeymapLayout,                     "Default");

    CMN_IMPL_PROPERTY(GlobalSettings, DefaultClass,                     "Text");
    CMN_IMPL_PROPERTY(GlobalSettings, UseIniFile,                       false);

    CMN_IMPL_PROPERTY(GlobalSettings, FileManagerTooltips,              true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileManagerPreviewInTooltips,     true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileManagerPreviewLines,          10);
    CMN_IMPL_PROPERTY(GlobalSettings, FileManagerShellContexMenu,       true);
    CMN_IMPL_PROPERTY(GlobalSettings, FileManagerShellContexMenuFilter, "(^tsvn.*$)|(^.*cvs.*$)|(^properties$)");

    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceFileSaveInActiveOnExit,  false);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceDetectChangesOnOpen,     true);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceUseRelativePath,         false);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceShowNameInTitleBar,      true);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceShowNameInStatusBar,     false);
    CMN_IMPL_PROPERTY(GlobalSettings, WorkspaceDoNotCloseFilesOnOpen,   false); // hidden, not for all

    CMN_IMPL_PROPERTY(GlobalSettings, HistoryRestoreEditorState,        true);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryFiles,                     1000);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryWorkspaces,                100);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryFilesInRecentMenu,         6);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryWorkspacesInRecentMenu,    6);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryQSWorkspacesInRecentMenu,  6);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryOnlyFileNameInRecentMenu,      false);
    CMN_IMPL_PROPERTY(GlobalSettings, HistoryOnlyWorkspaceNameInRecentMenu, true);

    CMN_IMPL_PROPERTY(GlobalSettings, SyntaxGutter,                     true);


GlobalSettings::GlobalSettings ()
{
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
    // the following option have to be skipped on loading externally updated settings
    // ie we don't like to change these properties for all running instances
    CMN_GET_PROPERTY_AWARE(LineNumbers)  .opt = PO_LOAD_ON_INIT_ONLY;
    CMN_GET_PROPERTY_AWARE(VisibleSpaces).opt = PO_LOAD_ON_INIT_ONLY;
    CMN_GET_PROPERTY_AWARE(Ruler)        .opt = PO_LOAD_ON_INIT_ONLY;
    CMN_GET_PROPERTY_AWARE(ColumnMarkers).opt = PO_LOAD_ON_INIT_ONLY;
    CMN_GET_PROPERTY_AWARE(IndentGuide)  .opt = PO_LOAD_ON_INIT_ONLY;
}


int GlobalSettings::GetSysPrintMeasuremnt ()
{
    char buff[4];

    if (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, buff, sizeof(buff)))
    {
        return (buff[0] == '1') ? 1 : 0;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// 	ClassSettings

    CMN_IMPL_PROPERTY_BINDER(ClassSettings);
    CMN_IMPL_PROPERTY(ClassSettings, Name,              "Text");
    CMN_IMPL_PROPERTY(ClassSettings, Extensions,        "txt lst log");
    CMN_IMPL_PROPERTY(ClassSettings, Language,          "NONE");
    CMN_IMPL_PROPERTY(ClassSettings, Delimiters,        " \t\'\"\\()[]{}+-*/.,!?;:=><%|@&^");
    CMN_IMPL_PROPERTY(ClassSettings, TabSpacing,        4);
    CMN_IMPL_PROPERTY(ClassSettings, IndentSpacing,     4);
    CMN_IMPL_PROPERTY(ClassSettings, IndentType,        0);
    CMN_IMPL_PROPERTY(ClassSettings, TabExpand,         0);
    CMN_IMPL_PROPERTY(ClassSettings, NormalizeKeywords, false);
    CMN_IMPL_PROPERTY(ClassSettings, FileCreateAs,      0);
    CMN_IMPL_PROPERTY(ClassSettings, FilenameTemplate,  "Text&n"); 

//ClassSettings::ClassSettings (const ClassSettings& other)
//{
//    *this = other;
//}
//
//
//ClassSettings& ClassSettings::operator = (const ClassSettings& other)
//{
//    if (this != &other)
//    {
//        //TODO: relace it with a generic copy procedure based on the class properties binder
//        m_Name           = other.m_Name           ;
//        m_Extensions     = other.m_Extensions     ;
//        m_Language       = other.m_Language       ;
//        m_Delimiters     = other.m_Delimiters     ;
//        m_TabSpacing     = other.m_TabSpacing     ;
//        m_IndentSpacing  = other.m_IndentSpacing  ;
//        m_IndentType     = other.m_IndentType     ;
//        m_TabExpand      = other.m_TabExpand      ;
//
//        m_NormalizeKeywords   = other.m_NormalizeKeywords;
//        m_FileNameTemplate    = other.m_FileNameTemplate;
//
//        m_ColumnMarkersSet    = other.m_ColumnMarkersSet;
//        m_VisualAttributesSet = other.m_VisualAttributesSet;
//
//        NotifySettingsChanged();
//    }
//    return *this;
//}


/////////////////////////////////////////////////////////////////////////////
// 	Settings
Settings::Settings (const SettingsManager& settingsManager)
    : m_globalSettings(0),
    m_classSettings(0),
    m_overridenTabSpacing(false),
    m_overridenIndentSpacing(false),
    m_overridenIndentType(false),
    m_overridenTabExpand(false)
{
    settingsManager.InitializeSettings(*this);
    ASSERT(m_globalSettings.get());
    m_globalSettings->AddSubscriber(this);
}

Settings::~Settings ()
{
    try { EXCEPTION_FRAME;

        if (m_globalSettings.get())
            m_globalSettings->RemoveSubscriber(this);
    }
    _DESTRUCTOR_HANDLER_;

    try { EXCEPTION_FRAME;

        if (m_classSettings.get())
            m_classSettings->RemoveSubscriber(this);
    }
    _DESTRUCTOR_HANDLER_;
}

Settings& Settings::operator = (const Settings& other)
{
    if (this != &other)
    {
        m_globalSettings->RemoveSubscriber(this);
        m_globalSettings = other.m_globalSettings;
        m_globalSettings->AddSubscriber(this);

        m_classSettings->RemoveSubscriber(this);
        m_classSettings = other.m_classSettings;
        m_classSettings->AddSubscriber(this);

        m_overridenTabSpacing    = other.m_overridenTabSpacing;
        m_overridenIndentSpacing = other.m_overridenIndentSpacing;
        m_overridenIndentType    = other.m_overridenIndentType;
        m_overridenTabExpand     = other.m_overridenTabExpand;

        m_TabSpacing    = other.m_TabSpacing;
        m_IndentSpacing = other.m_IndentSpacing;
        m_IndentType    = other.m_IndentType;
        m_TabExpand     = other.m_TabExpand;

        NotifySettingsChanged();
    }
    return *this;
}

void Settings::SetClassSetting (ClassSettingsPtr settings, bool notify)
{
    if (m_classSettings.get())
        m_classSettings->RemoveSubscriber(this);

    m_classSettings = settings;

    if (m_classSettings.get())
        m_classSettings->AddSubscriber(this);

    if (notify && settings.get())
        NotifySettingsChanged();
}

/////////////////////////////////////////////////////////////////////////////
// 	SettingsManager
SettingsManager::SettingsManager ()
: m_globalSettings(new GlobalSettings)
{
}

SettingsManager::SettingsManager (const SettingsManager& other)
: m_globalSettings(new GlobalSettings)
{
    *this = other;
}


SettingsManager::~SettingsManager ()
{
    m_classSettingsVector.clear(); // it must be call while m_globalSettings still exists
}


SettingsManager& SettingsManager::operator = (const SettingsManager& other)
{
    ASSERT_EXCEPTION_FRAME;

    *m_globalSettings = *other.m_globalSettings;
    m_templateCollection = other.m_templateCollection;

    ClassSettingsVector::const_iterator
        other_it(other.m_classSettingsVector.begin()),
        other_end(other.m_classSettingsVector.end());

    // it is a not initialized object
    if (m_classSettingsVector.size() == 0)
    {
        for (; other_it != other_end; ++other_it)
        {
            m_classSettingsVector.push_back(ClassSettingsPtr(new ClassSettings(**other_it)));
        }

    }
    // copy from a cloned one
    else if (m_classSettingsVector.size() == other.m_classSettingsVector.size())
    {
        ClassSettingsVector::iterator
            it(m_classSettingsVector.begin()),
            end(m_classSettingsVector.end());

        for (; it != end && other_it != other_end; ++it, ++other_it)
        {
            **it = **other_it;
            (*it)->NotifySettingsChanged();
        }
    }
    else
    {
        THROW_APP_EXCEPTION("SettingsManager: illegal assigment.");
    }

    return *this;
}


ClassSettings& SettingsManager::CreateClass (const std::string& name)
{
    ClassSettingsPtr cls;

    try {
        cls = FindByName(name);
    }
    catch (const Common::AppException&) {}

    if (cls.get())
        THROW_APP_EXCEPTION("class with name " + name + " already exists.");

    cls = ClassSettingsPtr(new ClassSettings);

    m_classSettingsVector.push_back(cls);
    m_classSettingsVector.back()->SetName(name);

    return *cls;
}

ClassSettings& SettingsManager::GetByName (const std::string& name) const
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            return **it;
        }

    THROW_APP_EXCEPTION("class with name " + name + " not found.");
}

void SettingsManager::DestroyClass (const std::string& name)
{
    ClassSettingsVector::iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            m_classSettingsVector.erase(it);
            break;
        }
}

/** @fn inline bool is_ext_supported (const string& _list, const std::string& _ext)
 * @brief Checks if ext exists inside the list of accepted extensions.
 *
 * @arg _list List of the extensions (for example: "txt lst log").
 * @arg _ext Checked extension (for example: "sql").
 * @return true or false - depends on existence of the extension.
 */
inline
bool is_ext_supported (const string& _list, const std::string& _ext)
{
    if (!_ext.empty())
    {

        string list = string(' ' + _list + ' ');
        string::iterator it = list.begin();
        for (; it != list.end(); ++it)
            *it = toupper(*it);

        string ext = string(1, ' ') + _ext + ' '; // bug fix, file extension is not recognized properly if it's shorter then 3 chars
        for (it = ext.begin(); it != ext.end(); ++it)
            *it = toupper(*it);

        return strstr(list.c_str(), ext.c_str()) ? true : false;
    }

    return false;
}

/** @fn ClassSettingsPtr SettingsManager::FindByExt (const std::string& ext, bool _default) const
 * @brief Finds proper template class searching by the extension.
 *
 * @arg ext Searched extension (for example: "sql").
 * @arg _default ???
 * @return Found template class or default (?) class.
 */
ClassSettingsPtr SettingsManager::FindByExt (const std::string& ext, bool _default) const
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if (is_ext_supported((**it).GetExtensions(), ext))
        {
            return *it;
        }

    if (!_default)
        THROW_APP_EXCEPTION("class for extension " + ext + " not found.");

    return m_classSettingsVector.at(0);
}


ClassSettingsPtr SettingsManager::FindByName (const std::string& name) const
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            return *it;
        }

    THROW_APP_EXCEPTION("class with name " + name + " not found.");
}

void SettingsManager::InitializeSettings (Settings& settings) const
{
    settings.m_globalSettings = m_globalSettings;
    ClassSettingsPtr classSettings = GetDefaults();
    settings.SetClassSetting(classSettings, false);
}

void SettingsManager::SetClassSettingsByLang (const std::string& name, Settings& settings) const
{
    ClassSettingsPtr classSettings = FindByName(name);
    settings.SetClassSetting(classSettings, true);
}

void SettingsManager::SetClassSettingsByExt (const std::string& ext, Settings& settings) const
{
    ClassSettingsPtr classSettings = FindByExt(ext, true);
    settings.SetClassSetting(classSettings, true); // bug fix, LanguageSupport is not changed when ClassSettings is set
}

TemplatePtr SettingsManager::GetTemplateByName (const string& name) const
{
    TemplatePtr templ;
    
    try {
        templ = GetTemplateCollection().Find(name);
    }
    catch (const Common::AppException&) {}

    return templ;
}

};//namespace OpenEditor
