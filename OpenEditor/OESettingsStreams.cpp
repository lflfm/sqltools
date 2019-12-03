/* 
    Copyright (C) 2002 Aleksey Kochetov

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

// 17.01.2005 (ak) simplified settings class hierarchy 

#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OESettingsStreams.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

#define OESMS_WRITE_MEMBER(o,p) m_out.write(#p, (o)->m_##p);
#define OESMS_READ_MEMBER(o,p)  m_in.read(#p, (o)->m_##p);
#define OESMS_VER_READ_MEMBER(ver,o,p,def)  \
    if (m_version >= ver) m_in.read(#p, (o)->m_##p); \
    else (o)->m_##p = def;

namespace OpenEditor
{
    using std::hex;
    using std::dec;
    using std::endl;
    using std::getline;
    using namespace Common;

	//! Version of settings file. Increase after adding new field! Use OESMS_VER_READ_MEMBER(new_ver,...) to read added fields.
    const int TheSettingsVersion = 1025;
    /*
        Note: use escape sequences in write(string), improve hex format
    */

/////////////////////////////////////////////////////////////////////////////
// 	SettingsManagerWriter & SettingsManagerReader

void SettingsManagerWriter::operator << (const SettingsManager& mgr)
{
    m_out.write("Version", TheSettingsVersion);

    const GlobalSettingsPtr settings = mgr.GetGlobalSettings();

    OESMS_WRITE_MEMBER(settings, Locale);
    OESMS_WRITE_MEMBER(settings, KeymapLayout);

    OESMS_WRITE_MEMBER(settings, AllowMultipleInstances);
    OESMS_WRITE_MEMBER(settings, NewDocOnStartup);
    OESMS_WRITE_MEMBER(settings, MaximizeFirstDocument);
    OESMS_WRITE_MEMBER(settings, WorkDirFollowsDocument);
    OESMS_WRITE_MEMBER(settings, SaveCurPosAndBookmarks);
    OESMS_WRITE_MEMBER(settings, SaveMainWinPosition);
    OESMS_WRITE_MEMBER(settings, DoubleClickCloseTab);

    OESMS_WRITE_MEMBER(settings, FileLocking);
    OESMS_WRITE_MEMBER(settings, FileMemMapForBig);
    OESMS_WRITE_MEMBER(settings, FileMemMapThreshold);
    OESMS_WRITE_MEMBER(settings, FileSaveOnSwith);
    OESMS_WRITE_MEMBER(settings, FileDetectChanges);
    OESMS_WRITE_MEMBER(settings, FileReloadChanges);
    OESMS_WRITE_MEMBER(settings, FileAutoscrollAfterReload);
    OESMS_WRITE_MEMBER(settings, FileOverwriteReadonly);
    OESMS_WRITE_MEMBER(settings, FileBackup);
    OESMS_WRITE_MEMBER(settings, FileBackupName);
    OESMS_WRITE_MEMBER(settings, FileBackupDirectory);
//	OESMS_WRITE_MEMBER(settings, FileSaveAddTimestamp);
//	OESMS_WRITE_MEMBER(settings, FileSaveAddTimestampFormat);


    OESMS_WRITE_MEMBER(settings, DefFileExtension);
    OESMS_WRITE_MEMBER(settings, UndoLimit);
    OESMS_WRITE_MEMBER(settings, UndoMemLimit);
    OESMS_WRITE_MEMBER(settings, UndoAfterSaving);
    OESMS_WRITE_MEMBER(settings, TruncateSpaces);
    OESMS_WRITE_MEMBER(settings, CursorBeyondEOL);
    OESMS_WRITE_MEMBER(settings, CursorBeyondEOF);
    OESMS_WRITE_MEMBER(settings, LineNumbers);
    OESMS_WRITE_MEMBER(settings, VisibleSpaces);
    OESMS_WRITE_MEMBER(settings, Ruler);
    OESMS_WRITE_MEMBER(settings, TimestampFormat);
    OESMS_WRITE_MEMBER(settings, EOFMark);
    OESMS_WRITE_MEMBER(settings, HomeGotoFirstColumn);
    OESMS_WRITE_MEMBER(settings, ColumnMarkers);
    OESMS_WRITE_MEMBER(settings, IndentGuide);

    OESMS_WRITE_MEMBER(settings, BlockKeepMarking);
    OESMS_WRITE_MEMBER(settings, BlockKeepMarkingAfterDragAndDrop);
    OESMS_WRITE_MEMBER(settings, BlockDelAndBSDelete);
    OESMS_WRITE_MEMBER(settings, BlockTypingOverwrite);
    OESMS_WRITE_MEMBER(settings, BlockTabIndent);
    OESMS_WRITE_MEMBER(settings, ColBlockDeleteSpaceAfterMove);
    OESMS_WRITE_MEMBER(settings, ColBlockCursorToStartAfterPaste);
    OESMS_WRITE_MEMBER(settings, ColBlockEditMode);
    OESMS_WRITE_MEMBER(settings, MouseSelectionDelimiters);

    OESMS_WRITE_MEMBER(settings, PrintBlackAndWhite);
    OESMS_WRITE_MEMBER(settings, PrintHeader);
    OESMS_WRITE_MEMBER(settings, PrintFooter);
    OESMS_WRITE_MEMBER(settings, PrintMarginMeasurement);
    OESMS_WRITE_MEMBER(settings, PrintLeftMargin);
    OESMS_WRITE_MEMBER(settings, PrintRightMargin);
    OESMS_WRITE_MEMBER(settings, PrintTopMargin);
    OESMS_WRITE_MEMBER(settings, PrintBottomMargin);

    int count = mgr.GetClassCount();
    m_out.write("Classes", count);

    Stream::Section sect(m_out, "Cls");
    for (int i(0); i < count; i++)
    {
        Stream::Section sect2(m_out, i, m_skipEnum);
        write(mgr.GetClassByPos(i));
    }
    // end of Cls
}


void SettingsManagerReader::operator >> (SettingsManager& mgr)
{
    m_version = 0;
    m_in.read("Version", m_version);

    _CHECK_AND_THROW_(m_version >= 1011 && m_version <= TheSettingsVersion, "Unsupported settings version!");

    GlobalSettingsPtr settings = mgr.GetGlobalSettings();

    OESMS_VER_READ_MEMBER(1013, settings, Locale, "English");
    OESMS_VER_READ_MEMBER(1013, settings, KeymapLayout, "Default");

    OESMS_READ_MEMBER(settings, AllowMultipleInstances);
    OESMS_READ_MEMBER(settings, NewDocOnStartup);
    OESMS_READ_MEMBER(settings, MaximizeFirstDocument);
    OESMS_READ_MEMBER(settings, WorkDirFollowsDocument);
    OESMS_READ_MEMBER(settings, SaveCurPosAndBookmarks);
    OESMS_READ_MEMBER(settings, SaveMainWinPosition);
    OESMS_VER_READ_MEMBER(1023, settings, DoubleClickCloseTab, false);

    OESMS_READ_MEMBER(settings, FileLocking);
    OESMS_READ_MEMBER(settings, FileMemMapForBig);
    OESMS_READ_MEMBER(settings, FileMemMapThreshold);
    OESMS_READ_MEMBER(settings, FileSaveOnSwith);
    OESMS_READ_MEMBER(settings, FileDetectChanges);
    OESMS_READ_MEMBER(settings, FileReloadChanges);
    OESMS_VER_READ_MEMBER(1013, settings, FileAutoscrollAfterReload, false);
    OESMS_READ_MEMBER(settings, FileOverwriteReadonly);
    OESMS_READ_MEMBER(settings, FileBackup);
    OESMS_READ_MEMBER(settings, FileBackupName);
    OESMS_VER_READ_MEMBER(1013, settings, FileBackupDirectory, "");
//	OESMS_VER_READ_MEMBER(1023, settings, FileSaveAddTimestamp, false);
//	OESMS_VER_READ_MEMBER(1023, settings, FileSaveAddTimestampFormat, "yyyymmdd");


    OESMS_VER_READ_MEMBER(1013, settings, DefFileExtension, "txt");
    OESMS_READ_MEMBER(settings, UndoLimit);
    OESMS_VER_READ_MEMBER(1013, settings, UndoMemLimit, 1000);
    OESMS_READ_MEMBER(settings, UndoAfterSaving);
    OESMS_READ_MEMBER(settings, TruncateSpaces);

    if (m_version < 1018)
    {
        bool virtualSpace = false;
        m_in.read("VirtualSpace", virtualSpace);
        settings->m_CursorBeyondEOL = 
        settings->m_CursorBeyondEOF = virtualSpace;
    }
    else
    {
        OESMS_VER_READ_MEMBER(1018, settings, CursorBeyondEOL, false);
        OESMS_VER_READ_MEMBER(1018, settings, CursorBeyondEOF, false);
    }

    OESMS_VER_READ_MEMBER(1019, settings, LineNumbers, false);
    OESMS_VER_READ_MEMBER(1019, settings, VisibleSpaces, false);
    OESMS_VER_READ_MEMBER(1019, settings, Ruler, true);
    OESMS_VER_READ_MEMBER(1019, settings, TimestampFormat, "dd.mm.yyyy hh24:mi");
    OESMS_VER_READ_MEMBER(1020, settings, EOFMark, true);
    OESMS_VER_READ_MEMBER(1024, settings, HomeGotoFirstColumn, false);
    OESMS_VER_READ_MEMBER(1021, settings, ColumnMarkers, true);
    OESMS_VER_READ_MEMBER(1025, settings, IndentGuide, true);

    OESMS_READ_MEMBER(settings, BlockKeepMarking);
    OESMS_VER_READ_MEMBER(1015, settings, BlockKeepMarkingAfterDragAndDrop, true);
    OESMS_READ_MEMBER(settings, BlockDelAndBSDelete);
    OESMS_READ_MEMBER(settings, BlockTypingOverwrite);
    OESMS_READ_MEMBER(settings, BlockTabIndent);
    OESMS_READ_MEMBER(settings, ColBlockDeleteSpaceAfterMove);
    OESMS_READ_MEMBER(settings, ColBlockCursorToStartAfterPaste);
    OESMS_VER_READ_MEMBER(1022, settings, ColBlockEditMode, true);
    OESMS_VER_READ_MEMBER(1016, settings, MouseSelectionDelimiters, "");

    OESMS_READ_MEMBER(settings, PrintBlackAndWhite);
    OESMS_READ_MEMBER(settings, PrintHeader);
    OESMS_READ_MEMBER(settings, PrintFooter);
    OESMS_READ_MEMBER(settings, PrintMarginMeasurement);
    OESMS_READ_MEMBER(settings, PrintLeftMargin);
    OESMS_READ_MEMBER(settings, PrintRightMargin);
    OESMS_READ_MEMBER(settings, PrintTopMargin);
    OESMS_READ_MEMBER(settings, PrintBottomMargin);

    int count = 0;
    m_in.read("Classes", count);
    mgr.m_classSettingsVector.clear();
    mgr.m_classSettingsVector.reserve(count);

    Stream::Section sect(m_in, "Cls");
    for (int i(0); i < count; i++)
    {
        ClassSettingsPtr classSettingsPtr(new ClassSettings);
        Stream::Section sect(m_in, i, m_skipEnum);
        read(classSettingsPtr);
        mgr.m_classSettingsVector.push_back(classSettingsPtr);
    }
    // end of Cls
}


void SettingsManagerWriter::write (const ClassSettingsPtr settings)
{
    OESMS_WRITE_MEMBER(settings, Name);
    OESMS_WRITE_MEMBER(settings, Extensions);
    OESMS_WRITE_MEMBER(settings, Language);

    OESMS_WRITE_MEMBER(settings, Delimiters);
    OESMS_WRITE_MEMBER(settings, TabSpacing);
    OESMS_WRITE_MEMBER(settings, IndentSpacing);
    OESMS_WRITE_MEMBER(settings, IndentType);
    OESMS_WRITE_MEMBER(settings, TabExpand);

    //OESMS_WRITE_MEMBER(settings, LineNumbers);
    //OESMS_WRITE_MEMBER(settings, VisibleSpaces);
    OESMS_WRITE_MEMBER(settings, NormalizeKeywords);
    OESMS_WRITE_MEMBER(settings, FileCreateAs);
    OESMS_WRITE_MEMBER(settings, ColumnMarkersSet);

    write(settings->GetVisualAttributesSet());
}


void SettingsManagerReader::read  (ClassSettingsPtr settings)
{
    OESMS_READ_MEMBER(settings, Name);
    OESMS_READ_MEMBER(settings, Extensions);
    OESMS_READ_MEMBER(settings, Language);

    OESMS_READ_MEMBER(settings, Delimiters);
    OESMS_READ_MEMBER(settings, TabSpacing);
    OESMS_READ_MEMBER(settings, IndentSpacing);
    OESMS_READ_MEMBER(settings, IndentType);
    OESMS_READ_MEMBER(settings, TabExpand);

    if (m_version >= 1011 && m_version < 1019)
    {
        bool dummy;
        m_in.read("LineNumbers", dummy);
        m_in.read("VisibleSpaces", dummy);
    }

    OESMS_VER_READ_MEMBER(1014, settings, NormalizeKeywords, false);
    OESMS_READ_MEMBER(settings, FileCreateAs);

    if (m_version >= 1019 && m_version < 1021)
    {
        bool dummy;
        m_in.read("ColumnMarkers", dummy);
    }

    OESMS_VER_READ_MEMBER(1019, settings, ColumnMarkersSet, vector<int>(1, 80));

    read(settings->GetVisualAttributesSet());

    if (m_version < 1017
    && settings->GetName() == "PL/SQL")
    {
        VisualAttributesSet& set_ = settings->GetVisualAttributesSet();
        try 
        {
            set_.FindByName("Other Keywords");
        }
        catch (const std::out_of_range&)
        {
            VisualAttribute attr;
            attr.m_Name       = "Other Keywords";
            attr.m_Mask       = vaoFontBold|vaoFontItalic|vaoFontUnderline|vaoForeground;
            attr.m_FontBold   = true;
            attr.m_Foreground = 0xC00000;
            set_.Add(attr);
        }
    }
}


void SettingsManagerWriter::write (const VisualAttributesSet& set_)
{
    Stream::Section sect(m_out, "VA", m_skipEnum);
    m_out.write("Name", set_.GetName());

    int count = set_.GetCount();
    m_out.write("Count", count);

    for (int i(0); i < count; i++)
    {
        Stream::Section sect(m_out, i, m_skipEnum);
        write(set_[i]);
    }
}


void SettingsManagerReader::read  (VisualAttributesSet& set_)
{
    Stream::Section sect(m_in, "VA");

    string setName;
    m_in.read("Name", setName);
    set_.SetName(setName);

    int count = 0;
    m_in.read("Count", count);
    set_.Clear();

    for (int i(0); i < count; i++)
    {
        VisualAttribute attr;
        Stream::Section sect(m_in, i, m_skipEnum);
        read(attr);
        set_.Add(attr);

        if (m_version <= 1003 && attr.m_Name == "Selected Text")
        {
            attr.m_Name = "Current Line";
            attr.m_Mask = Common::vaoBackground;
            attr.m_Background = RGB(255,255,127);
            set_.Add(attr);
        }
    }
}


void SettingsManagerWriter::write (const VisualAttribute& attr)
{
    OESMS_WRITE_MEMBER(&attr, Name);
    OESMS_WRITE_MEMBER(&attr, FontName);
    OESMS_WRITE_MEMBER(&attr, FontSize);
    OESMS_WRITE_MEMBER(&attr, FontBold);
    OESMS_WRITE_MEMBER(&attr, FontItalic);
    OESMS_WRITE_MEMBER(&attr, FontUnderline);
    OESMS_WRITE_MEMBER(&attr, Foreground);
    OESMS_WRITE_MEMBER(&attr, Background);
    OESMS_WRITE_MEMBER(&attr, Mask);
}


void SettingsManagerReader::read  (VisualAttribute& attr)
{
    OESMS_READ_MEMBER(&attr, Name);
    OESMS_READ_MEMBER(&attr, FontName);
    OESMS_READ_MEMBER(&attr, FontSize);
    OESMS_READ_MEMBER(&attr, FontBold);
    OESMS_READ_MEMBER(&attr, FontItalic);
    OESMS_READ_MEMBER(&attr, FontUnderline);
    OESMS_READ_MEMBER(&attr, Foreground);
    OESMS_READ_MEMBER(&attr, Background);
    OESMS_READ_MEMBER(&attr, Mask);

    if (!(attr.m_Mask & vaoFontName     )) attr.m_FontName.clear();
    if (!(attr.m_Mask & vaoFontSize     )) attr.m_FontSize       = 0;
    if (!(attr.m_Mask & vaoFontBold     )) attr.m_FontBold       = false;
    if (!(attr.m_Mask & vaoFontItalic   )) attr.m_FontItalic     = false;
    if (!(attr.m_Mask & vaoFontUnderline)) attr.m_FontUnderline  = false;
    if (!(attr.m_Mask & vaoForeground   )) attr.m_Foreground     = 0;
    if (!(attr.m_Mask & vaoBackground   )) attr.m_Background     = 0;
}

};//namespace OpenEditor
