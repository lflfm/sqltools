/* 
    Copyright (C) 2002-2017 Aleksey Kochetov

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

/*
    09.03.2003 bug fix, backup file problem if a destination is the current folder
    14.04.2003 bug fix, SetModifiedFlag clears an undo history
    06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db 
               if "save files when switching tasks" is activated.
    21.01.2004 bug fix, LockContent/open/save file error handling chanded to avoid paranoic bug reporting
    10.06.2004 improvement, the program asks to continue w/o locking if it cannot lock a file
    30.06.2004 bug fix, unknown exception on "save as", if a user doesn't have permissions to rewrite the destination 
    17.01.2005 (ak) simplified settings class hierarchy 
    28.03.2005 bug fix, (ak) if file are locked, an unrecoverable error occurs on "Reload"
    30.03.2005 bug fix, a new bug introduced by the previous bug fix
    2011.10.17 bug fix, file custom.keymap is missing
    2016.06.28 improvement, added SetFileFormat
    2016.09.01 improvement, Implemented "Rename / Move" command (internally it calls SaveAs and then deletes the original file)
    2016.09.01 bug fix, Save (File) fails if file backup does not succeed. The backup failure can be ignored now.
    2017-12-25 improvement, the program automatically reloads the template file if it was changed externally
    2017-12-25 improvement, added quick access to template dialog using context menu
*/

#include "stdafx.h"
#include <fstream>
#include "Shlwapi.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/AppGlobal.h"
#include <COMMON/ExceptionHelper.h>
#include "COMMON\StrHelpers.h"
#include "COMMON/VisualAttributesPage.h"
#include "COMMON/PropertySheetMem.h"
#include "Common/OsException.h"
#include "OpenEditor/OEDocument.h"
#include "Common/InputDlg.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/OEPrintPageSetup.h"
#include "OpenEditor/OEEditingPage.h"
#include "OpenEditor/OEBlockPage.h"
#include "OpenEditor/OEClassPage.h"
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OEFileInfoDlg.h"
#include "OpenEditor/OEFileInfoDlg.h"
#include "OpenEditor/OEGeneralPage.h"
#include "OpenEditor/OEFilePage.h"
#include "OpenEditor/OEWorkspacePage.h"
#include "OpenEditor/OEFileManagerPage.h"
#include "OpenEditor/OEBackupPage.h"
#include "OpenEditor/OEPropPage.h"
#include "OpenEditor/OETemplatesPage.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEOverwriteFileDlg.h"
#include "Common/ConfigFilesLocator.h"
#include "OpenEditor/RecentFileList.h"
#include "Common/MyUtf.h"
#include "WindowsCodepage.h"
#include <tinyxml\tinyxml.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COEDocument

using namespace std;
using namespace Common;
using namespace OpenEditor;

#define CHECK_FILE_OPERATION(r) if(!(r)) OsException::CheckLastError();

SettingsManager*COEDocument::m_pSettingsManager = NULL;
Searcher        COEDocument::m_searcher(true);
bool            COEDocument::m_enableOpenUnexisting = false;

IMPLEMENT_DYNCREATE(COEDocument, CDocument)

BEGIN_MESSAGE_MAP(COEDocument, CDocument)
    //{{AFX_MSG_MAP(COEDocument)
    ON_COMMAND(ID_EDIT_PRINT_PAGE_SETUP, OnEditPrintPageSetup)
    ON_COMMAND(ID_EDIT_FILE_SETTINGS, OnEditFileSettings)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
    ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD, OnUpdate_FileReload)
    ON_UPDATE_COMMAND_UI(ID_FILE_SYNC_LOCATION, OnUpdate_FileLocation)
    ON_UPDATE_COMMAND_UI(ID_FILE_COPY_LOCATION, OnUpdate_FileLocation)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_ENCODING,  OnUpdate_EncodingInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_FileTypeInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_LINES, OnUpdate_FileLinesInd)

    ON_COMMAND(ID_EDIT_VIEW_LINE_NUMBERS, OnEditViewLineNumbers)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_LINE_NUMBERS, OnUpdate_EditViewLineNumbers)

    ON_COMMAND(ID_EDIT_VIEW_SYNTAX_GUTTER, OnEditViewSyntaxGutter)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_SYNTAX_GUTTER, OnUpdate_EditViewSyntaxGutter)

    ON_COMMAND(ID_EDIT_VIEW_WHITE_SPACE, OnEditViewWhiteSpace)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_WHITE_SPACE, OnUpdate_EditViewWhiteSpace)
    ON_COMMAND(ID_EDIT_VIEW_RULER, OnEditViewRuler)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_RULER,  OnUpdate_EditViewRuler)
    ON_COMMAND(ID_EDIT_VIEW_COLUMN_MARKERS, OnEditViewColumnMarkers)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_COLUMN_MARKERS,  OnUpdate_EditViewColumnMarkers)
    ON_COMMAND(ID_EDIT_VIEW_INDENT_GUIDE, OnEditViewIndentGuide)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_INDENT_GUIDE,  OnUpdate_EditViewIndentGuide)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)

    ON_COMMAND(ID_FILE_CLOSE_OTHERS, OnFileCloseOthers)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE_OTHERS, OnUpdate_FileCloseOthers)
    ON_COMMAND(ID_FILE_RENAME, OnFileRename)
    ON_COMMAND(ID_FILE_COPY_LOCATION, OnFileCopyLocation)
    ON_UPDATE_COMMAND_UI(ID_FILE_COPY_LOCATION, OnUpdate_FileCopyLocation)
    ON_COMMAND(ID_FILE_COPY_NAME, OnFileCopyName)

    ON_COMMAND(ID_EDIT_FILE_FORMAT_WINOWS, OneditFileFormatWinows)
    ON_COMMAND(ID_EDIT_FILE_FORMAT_UNIX  , OneditFileFormatUnix  )
    ON_COMMAND(ID_EDIT_FILE_FORMAT_MAC   , OneditFileFormatMac   )

    ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_FILE_FORMAT_WINOWS, ID_EDIT_FILE_FORMAT_MAC, OnUpdate_FileFormat) 

    ON_COMMAND(ID_EDIT_ENCODING_ANSI, OnEditEncodingAnsi)
    ON_COMMAND(ID_EDIT_ENCODING_OEM,  OnEditEncodingOem)
    ON_COMMAND(ID_EDIT_ENCODING_UTF8, OnEditEncodingUtf8)

    ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_ENCODING_ANSI, ID_EDIT_ENCODING_UTF8, OnUpdate_Encoding) 
    ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_ENCODING_CODEPAGE_FIRST, ID_EDIT_ENCODING_CODEPAGE_LAST, OnUpdate_EncodingCodepage) 
    ON_COMMAND_EX_RANGE(ID_EDIT_ENCODING_CODEPAGE_FIRST, ID_EDIT_ENCODING_CODEPAGE_LAST, OnEncodingCodepage)

    ON_COMMAND(ID_FILE_ENCODING_ANSI      , OnFileEncodingAnsi)     
    ON_COMMAND(ID_FILE_ENCODING_OEM       , OnFileEncodingOem)     
    ON_COMMAND(ID_FILE_ENCODING_UTF8      , OnFileEncodingUtf8)
    ON_COMMAND(ID_FILE_ENCODING_UTF8_BOM  , OnFileEncodingUtf8wBom)
    ON_COMMAND(ID_FILE_ENCODING_UTF16_BOM , OnFileEncodingUtf16wBom)

    ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_ENCODING_ANSI, ID_FILE_ENCODING_UTF16_BOM, OnUpdate_FileEncoding) 
    ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_ENCODING_CODEPAGE_FIRST, ID_FILE_ENCODING_CODEPAGE_LAST, OnUpdate_FileEncodingCodepage) 
    ON_COMMAND_EX_RANGE(ID_FILE_ENCODING_CODEPAGE_FIRST, ID_FILE_ENCODING_CODEPAGE_LAST, OnFileEncodingCodepage)

    // Standard Mail Support
    ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
    ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COEDocument construction/destruction

COEDocument::COEDocument()
: m_settings(getSettingsManager())
{
    try { EXCEPTION_FRAME;

        m_vmdata = NULL;
        m_vmsize = 0;
        m_orgModified = false;
        m_newOrSaveAs = false;
        m_extensionInitialized = false;
        m_orgFileTime = 0;
        m_orgFileSize = 0;
        m_storage.SetSettings(&m_settings);
        m_storage.SetSearcher(&m_searcher);
    }
    _OE_DEFAULT_HANDLER_;
}


COEDocument::~COEDocument()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        if (m_vmdata) 
        {
            VirtualFree(m_vmdata, 0, MEM_RELEASE);
            m_vmsize = 0;
        }
        m_storage.Clear(true);
        m_mmfile.Close();
    }
    _DESTRUCTOR_HANDLER_;
}


BOOL COEDocument::IsModified()
{
    return m_storage.IsModified() ? TRUE : FALSE;
}


RecentFileList* COEDocument::GetRecentFileList()
{
    if (CDocTemplate* templ = GetDocTemplate())
        if (COEMultiDocTemplate* oeTempl = dynamic_cast<COEMultiDocTemplate*>(templ))
            return oeTempl->GetRecentFileList();

    return 0;
}

BOOL COEDocument::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    switch (m_settings.GetEncoding())
    {
    case feANSI     :
        m_openEncoding =
        m_saveEncoding = Encoding(CP_ACP, 0);
        m_storage.SetCodepage(m_openEncoding.codepage, false);
        break;
    case feUTF8     :
        m_openEncoding =
        m_saveEncoding = Encoding(CP_UTF8, 0);
        m_storage.SetCodepage(m_openEncoding.codepage, false);
        break;
    case feUTF8BOM  :
        m_openEncoding =
        m_saveEncoding = Encoding(CP_UTF8, 3);
        m_storage.SetCodepage(m_openEncoding.codepage, false);
        break;
    case feUTF16BOM :
        m_openEncoding =
        m_saveEncoding = Encoding(CP_UTF16, 2);
        m_storage.SetCodepage(CP_UTF8, false);
        break;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// COEDocument othes methods

void COEDocument::SetPathName (LPCWSTR lpszPathName, BOOL bAddToMRU)
{
    CDocument::SetPathName(lpszPathName, bAddToMRU);

    LPCWSTR extension = PathFindExtension(lpszPathName);
    
    if (extension) 
        ++extension;

    if (!m_extensionInitialized || extension != m_extension)
    {
        m_extension = extension;
        if (!m_extensionInitialized
        || AfxMessageBox(L"Filename extension changed. Auto-setup for this extension?", MB_YESNO|MB_ICONQUESTION) == IDYES)
            getSettingsManager().SetClassSettingsByExt(Common::str(m_extension), m_settings);
        m_extensionInitialized = true;
    }
}


void COEDocument::SetTitle (LPCWSTR lpszTitle)
{
    m_orgTitle = lpszTitle;
    m_orgModified = m_storage.IsModified();

    CDocument::SetTitle(m_orgModified ? m_orgTitle + L" *" : m_orgTitle);
}


void COEDocument::UpdateTitle ()
{
    if (m_orgModified != m_storage.IsModified())
    {
        m_orgModified = m_storage.IsModified();
        CDocument::SetTitle(m_orgModified ? m_orgTitle + L" *" : m_orgTitle);
    }
}

BOOL COEDocument::OnIdle (LONG)
{
    if (!getSettingsManager().GetGlobalSettings()->GetSyntaxGutter())
        return FALSE;

    return m_storage.OnIdle() ? TRUE : FALSE;
}

void COEDocument::OnCloseDocument ()
{
    try { EXCEPTION_FRAME; // added exception handler because the method called from CMFCToolBar

        if (RecentFileList* rfl = GetRecentFileList())
            rfl->OnCloseDocument(this);

        __super::OnCloseDocument();
    }
    _OE_DEFAULT_HANDLER_;
}

enum ROF_CHECK { ROF_CANCEL, ROF_SAVE, ROF_SAVE_AS };

ROF_CHECK checkAndRemoveReadonlyAttribute (const CString& filename, bool overwriteReadonlyWithoutConfirmation)
{
    DWORD dwAttrib = filename.IsEmpty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributes(filename);
    
    if (dwAttrib != INVALID_FILE_ATTRIBUTES 
    && dwAttrib & FILE_ATTRIBUTE_READONLY)
    {
        if (overwriteReadonlyWithoutConfirmation)
        {
            SetFileAttributes(filename, dwAttrib & (~FILE_ATTRIBUTE_READONLY));
        }
        else
        {
            switch (OEOverwriteFileDlg(filename).DoModal())
            {
            case IDCANCEL:  // cancel saving
                return ROF_CANCEL;
            case IDYES:     // try "Save As"
                return ROF_SAVE_AS;
            case IDNO:      // remove write-protection and continue
                SetFileAttributes(filename, dwAttrib & (~FILE_ATTRIBUTE_READONLY));
            }
        }
    }
    return ROF_SAVE;
}

BOOL COEDocument::DoFileSave ()
{
    if (checkAndRemoveReadonlyAttribute(m_strPathName, m_settings.GetFileOverwriteReadonly()) != ROF_CANCEL
    && CDocument::DoFileSave())
    {
        if (!m_settings.GetUndoAfterSaving())
            m_storage.ClearUndo();

        return TRUE;
    }

    return FALSE;
}


BOOL COEDocument::DoSave (LPCWSTR lpszPathName, BOOL bReplace)
{
    return doSave2(lpszPathName, bReplace, FALSE);
}

#include "TextEncodingDetect/text_encoding_detect.h"
using namespace AutoIt::Common;

static COEDocument::Encoding detect_encoding (void* data, unsigned long length, bool ascii_as_utf8)
{
    COEDocument::Encoding encoding;

    switch (TextEncodingDetect().DetectEncoding((unsigned char*)data, length))
    {
    case TextEncodingDetect::Encoding::ASCII:      // 0-127
        if (ascii_as_utf8)
        {
            encoding.codepage = CP_UTF8;
            encoding.bom = 0;
            break;
        }
    case TextEncodingDetect::Encoding::ANSI:       // 0-255
    default:
        encoding.codepage = CP_ACP;
        encoding.bom = 0;
        break;
    case TextEncodingDetect::Encoding::UTF8_BOM:   // UTF8 with BOM
        encoding.codepage = CP_UTF8;
        encoding.bom = 3;
        break;
    case TextEncodingDetect::Encoding::UTF8_NOBOM: // UTF8 without BOM
        encoding.codepage = CP_UTF8;
        encoding.bom = 0;
        break;
    case TextEncodingDetect::Encoding::UTF16_LE_BOM:// UTF16 LE with BOM
        encoding.codepage = CP_UTF16;
        encoding.bom = 2;
        break;
    };

    return encoding;
}

static void to_utf8 (MemoryMappedFile& mmfile, COEDocument::Encoding encoding, LPVOID& vmdata, unsigned long& vmlen)
{
    unsigned long length = mmfile.GetDataLength();
    const wchar_t* wbuff = ((const wchar_t*)mmfile.GetData()) + encoding.bom / sizeof(wchar_t);
    unsigned long wlen = (length - encoding.bom) / sizeof(wchar_t);
    vmlen = WideCharToMultiByte(CP_UTF8, 0, wbuff, wlen, NULL, 0, NULL, NULL);
    vmdata = VirtualAlloc(NULL, vmlen, MEM_COMMIT, PAGE_READWRITE);
    if (WideCharToMultiByte(CP_UTF8, 0, wbuff, wlen, (char*)vmdata, vmlen, NULL, NULL) != (int)vmlen)
        throw OsException(0, L"UTF-16 to UTF-8 conversion error!");
}

// 28.03.2005 bug fix, (ak) if file are locked, an unrecoverable error occurs on "Reload"
void COEDocument::loadFile (LPCWSTR path, bool reload, bool external, bool modified)
{
    ASSERT_EXCEPTION_FRAME;

    LPVOID vmdata = 0;
    unsigned long vmsize = 0;
    bool irrecoverable = false; // if it's true and exception occurs, cleanup is required

    try // ok, read it
    {
        CWaitCursor wait;

        bool locking = m_settings.GetFileLocking();

        // reload if file is locked
        // 30.03.2005 bug fix, a new bug introduced by the previous bug fix
        if (reload && locking && m_mmfile.IsOpen()) 
        {
            if (m_openEncoding.codepage != m_saveEncoding.codepage)
            {
                m_storage.SetCodepage(m_openEncoding.codepage, false);
                m_saveEncoding.codepage = m_openEncoding.codepage;
            }

            if (m_openEncoding.codepage == CP_UTF16)
            {
                to_utf8(m_mmfile, m_openEncoding, vmdata, vmsize);
                irrecoverable = true;
                m_storage.SetText((const char*)vmdata, vmsize, true, reload, external);
                swap(m_vmdata, vmdata);
                swap(m_vmsize, vmsize);
                if (vmdata) VirtualFree(vmdata, 0, MEM_RELEASE);
                vmsize = 0;
                irrecoverable = false;
            }
            else
            {
                unsigned long length = m_mmfile.GetDataLength();
                m_storage.SetText((const char*)m_mmfile.GetData() + m_openEncoding.bom, length - m_openEncoding.bom, true, reload, external);
            }

            if (!modified)
                SetModifiedFlag(FALSE);

            return;
        }

        if (!locking && m_settings.GetFileMemMapForBig())
        {
            DWORD attrs;
            __int64 fileSize;
            if (Common::AppGetFileAttrs(path, &attrs, &fileSize) 
            && fileSize > __int64(m_settings.GetFileMemMapThreshold()) * (1024 * 1024))
            {
                locking = true;
            }
        }

        MemoryMappedFile mmfile;
        try // open a file 
        {
            unsigned options = locking ? emmoRead|emmoWrite|emmoShareRead : emmoRead|emmoShareRead|emmoShareWrite;
            mmfile.Open(path, options, 0);
        }
        catch (const OsException& x)
        {
            if (locking 
            && (x.GetErrCode() == ERROR_ACCESS_DENIED || x.GetErrCode() == ERROR_SHARING_VIOLATION))
            {
                LPCWSTR text = m_settings.GetFileLocking()
                    ? L"Cannot lock the file \"%s\".\nWould you like to continue without locking?"
                    : L"Cannot lock the file \"%s\".\nMemory mapped file access is disabled.\nWould you like to continue?";

                WCHAR buffer[10 * MAX_PATH];
                _snwprintf(buffer,  sizeof(buffer)/sizeof(buffer[0]), text, path);
                buffer[sizeof(buffer)/sizeof(buffer[0])-1] = 0;

                if (AfxMessageBox(buffer, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                {
                    locking = false;
                    unsigned options = emmoRead|emmoShareRead|emmoShareWrite;
                    mmfile.Open(path, options, 0);
                }
                else
                    AfxThrowUserException(); // it's the most silent exception
            }
            else
            THROW_APP_EXCEPTION("Cannot open file \"" + Common::str(path) + "\".\n\n" + x.what());
        }

        unsigned long length = mmfile.GetDataLength();
        
        if (!reload)
        {
            m_saveEncoding = 
            m_openEncoding = detect_encoding (mmfile.GetData(), length, 
                m_settings.GetEncoding() == feUTF8 && m_settings.GetAsciiAsUtf8());
        }

        if (locking) // it's equal to using of mm
        {
            irrecoverable = true;
            MemoryMappedFile::Swap(mmfile, m_mmfile);

            if (m_openEncoding.codepage != CP_UTF16)
            {
                if (!reload || m_openEncoding.codepage != m_saveEncoding.codepage)
                {
                    m_storage.SetCodepage(m_openEncoding.codepage, false);
                    m_saveEncoding.codepage = m_openEncoding.codepage;
                }

                m_storage.SetText(((const char*)m_mmfile.GetData()) + m_openEncoding.bom, length - m_openEncoding.bom, true, reload, external);
            }
            else
            {
                to_utf8(m_mmfile, m_openEncoding, vmdata, vmsize);
                if (!reload || m_openEncoding.codepage != m_saveEncoding.codepage)
                {
                    m_storage.SetCodepage(CP_UTF8, false);
                    m_saveEncoding.codepage = m_openEncoding.codepage;
                }
                m_storage.SetText((const char*)vmdata, vmsize, true, reload, external);
                swap(m_vmdata, vmdata);
                swap(m_vmsize, vmsize);
            }
            
            if (!modified)
                SetModifiedFlag(FALSE);
        }
        else
        {
            if (length > 0)
            {
                if (m_openEncoding.codepage != CP_UTF16)
                {
                    vmdata = VirtualAlloc(NULL, length, MEM_COMMIT, PAGE_READWRITE);
                    memcpy(vmdata, mmfile.GetData(), length);
                    irrecoverable = true;
                    if (!reload || m_openEncoding.codepage != m_saveEncoding.codepage)
                    {
                        m_storage.SetCodepage(m_openEncoding.codepage, false);
                        m_saveEncoding.codepage = m_openEncoding.codepage;
                    }
                    m_storage.SetText((const char*)vmdata + m_openEncoding.bom, length - m_openEncoding.bom, true, reload, external);
                    swap(vmdata, m_vmdata);
                    m_vmsize = length;
                }
                else
                {
                    to_utf8(mmfile, m_openEncoding, vmdata, vmsize);
                    irrecoverable = true;
                    if (!reload || m_openEncoding.codepage != m_saveEncoding.codepage)
                    {
                        m_storage.SetCodepage(CP_UTF8, false);
                        m_saveEncoding.codepage = m_openEncoding.codepage;
                    }
                    m_storage.SetText((const char*)vmdata, vmsize, true, reload, external);
                    swap(m_vmdata, vmdata);
                    swap(m_vmsize, vmsize);
                }
            }
            else
            {
                m_storage.SetText("", 0, false, reload, external);
            }
            if (!modified)
                SetModifiedFlag(FALSE);
        }
        if (vmdata) VirtualFree(vmdata, 0, MEM_RELEASE);
        vmdata = 0;
    }
    catch (...)
    { 
        try { 
            if (vmdata) VirtualFree(vmdata, 0, MEM_RELEASE);
            vmdata = 0;
        } catch (...) {}
        
        if (irrecoverable)
        { 
            try { m_storage.SetText("", 0, false, false, false, true); } catch (...) {}
            try { if (m_vmdata) VirtualFree(m_vmdata, 0, MEM_RELEASE); m_vmdata = 0; m_vmsize = 0; } catch (...) {}
            try { m_mmfile.Close(); } catch (...) {}
        }
        throw;
    }
}


BOOL COEDocument::OnOpenDocument (LPCWSTR lpszPathName) 
{
    try { EXCEPTION_FRAME;

        _ASSERTE(!m_mmfile.IsOpen() && !m_vmdata);

        DWORD dummy;
        if (m_enableOpenUnexisting 
        && !AppGetFileAttrs(lpszPathName, &dummy))
        {
            if (COEDocument::OnNewDocument())
            {
                SetPathName(lpszPathName);
                return TRUE;
            }
            return FALSE;
        }

        loadFile(lpszPathName);

    }
    catch (const CUserException*)
    {
        // silent termination because user was notified
        return FALSE;
    }
    catch (const std::exception& x)   
    { 
        DEFAULT_HANDLER(x); 
        return FALSE;
    }
    catch (...)   
    { 
        DEFAULT_HANDLER_ALL; 
        return FALSE;
    }

    CFileWatchClient::StartWatching(lpszPathName);

    CFileWatch::GetFileTimeAndSize(this, m_orgFileTime, m_orgFileSize);

    // views have not initialized yet
    //if (RecentFileList* rfl = GetRecentFileList())
    //    rfl->OnOpenDocument(this);

    return TRUE;
}

BOOL COEDocument::OnSaveDocument (LPCWSTR lpszPathName)
{
    bool storage_invalidated = false;
    WCHAR tmpname[_MAX_PATH];

    try { EXCEPTION_FRAME;

        static bool skip_backup = false;
        if (!skip_backup)
        {
            CString error;
            try 
            {
                backupFile(lpszPathName); // create backup file
            }
            catch (const std::exception& x)   
            {
                error = "Cannot create a backup file!\n\n";
                error += x.what();
                error += "\n";
                error += "Would you like to ignore and continue?";
            }
            catch (...)
            {
                error = "Cannot create a backup file!\n\n";
                error += "Would you like to ignore and continue?";
            }
            if (!error.IsEmpty())
            {
                MessageBeep(MB_ICONERROR);
                if (AfxMessageBox(error, MB_ICONERROR | MB_YESNO) != IDYES)
                    throw OsException(0, L"File backup error!");

                skip_backup = true;
            }
        }

        if (!m_storage.IsLocked())
            m_storage.TruncateSpaces();     // truncate spaces before size allocation
        
        unsigned long length = 
            m_saveEncoding.codepage != CP_UTF16
            ? m_storage.GetTextLengthA() + m_saveEncoding.bom
            : m_storage.GetTextLengthW() * sizeof(wchar_t) + m_saveEncoding.bom;
        
        // create mm file and copy data
        MemoryMappedFile outfile;

        // if we using mm file then we have to use an intermediate file for file saving
        if (m_mmfile.IsOpen())
        {
            WCHAR *name, path[_MAX_PATH];
            // get the directory for the intermediate file
            CHECK_FILE_OPERATION(::GetFullPathName(lpszPathName, sizeof(path)/sizeof(path[0]), path, &name));
            *name = 0; // cut file name
            // get a temporary name for the intermediate file
            CHECK_FILE_OPERATION(::GetTempFileName(path, L"OE~", 0, tmpname));
            // open output mm file
            outfile.Open(tmpname, emmoCreateAlways|emmoWrite|emmoShareRead, length);
        }
        else
        {
            // suspend file watching
            CFileWatchClient::StopWatching();
            // open output mm file
            outfile.Open(lpszPathName, emmoCreateAlways|emmoWrite|emmoShareRead, length);
        }

        char* ptr = (char*)outfile.GetData();

        if (m_saveEncoding.bom == 3) // utf-8
        {
            ptr[0] = '\xEF';
            ptr[1] = '\xBB';
            ptr[2] = '\xBF';
        }
        else if (m_saveEncoding.bom == 2) // utf-16 le
        {
            ptr[0] = '\xFF';
            ptr[1] = '\xFE';
        }

        // copy text to output mm file
        if (m_saveEncoding.codepage != CP_UTF16)
            m_storage.GetTextA(ptr + m_saveEncoding.bom, length - m_saveEncoding.bom);
        else
            m_storage.GetTextW((wchar_t*)(ptr + m_saveEncoding.bom), (length - m_saveEncoding.bom) / sizeof(wchar_t));

        outfile.Close();

        m_openEncoding = m_saveEncoding;
        m_storage.SetCodepage((m_saveEncoding.codepage != CP_UTF16) ? m_saveEncoding.codepage : CP_UTF8, false);

        if (m_mmfile.IsOpen())
        {
            storage_invalidated = true;
            // suspend file watching
            CFileWatchClient::StopWatching();
            m_mmfile.Close();
            // delete the original file we have to ignore failure for SaveAs operation
            int del_err = ::DeleteFile(lpszPathName) ? 0 : ::GetLastError();
            
            // rename tmp file to the original
            if (!::MoveFile(tmpname, lpszPathName))
            { 
                std::wstring err_desc;
                OsException::GetError(del_err ? del_err : ::GetLastError(), err_desc);

                loadFile(tmpname, true/*reload*/, false /*external*/, true/*modified*/);
                CFileWatchClient::StartWatching(tmpname);

                CString message = L"Cannot save file \"";
                message += lpszPathName;
                message += L"\".\n";
                message += err_desc.c_str();
                AfxMessageBox(message, MB_OK|MB_ICONSTOP);
                return TRUE;
            }
        }

        // open a new copy and reload a file
        loadFile(lpszPathName, true/*reload*/);
        
        // resume file watching
        CFileWatchClient::StartWatching(lpszPathName);
    }
    catch (const OsException& x)   
    { 
        if (storage_invalidated) m_storage.Clear(true);
        CString message = L"Cannot save file \"";
        message += lpszPathName;
        message += L"\".\n\n";
        message += x.what();
        AfxMessageBox(message, MB_OK|MB_ICONSTOP);
        return FALSE;
    }
    catch (const std::exception& x)   
    { 
        if (storage_invalidated) m_storage.Clear(true);
        DEFAULT_HANDLER(x);
        CString message = L"Cannot save file \"";
        message += lpszPathName;
        message += L"\".";
        AfxMessageBox(message, MB_OK|MB_ICONSTOP);
        return FALSE;
    }
    catch (...) 
    { 
        if (storage_invalidated) m_storage.Clear(true);
        DEFAULT_HANDLER_ALL;
        CString message = L"Cannot save file \"";
        message += lpszPathName;
        message += L"\".";
        AfxMessageBox(message, MB_OK|MB_ICONSTOP);
        return FALSE;
    }

    return TRUE;
}


/** @fn BOOL COEDocument::doSave (LPCWSTR lpszPathName, BOOL bReplace)
 * @brief Setup 'Save as...' dialog and call saving.
 *
 * @arg lpszPathName Path name where to save document file.
 * if lpszPathName is NULL then the user will be prompted (SaveAs)
 * note: lpszPathName can be different than 'm_strPathName'
 * if 'bReplace' is TRUE will change file name if successful (SaveAs)
 * if 'bReplace' is FALSE will not change path name (SaveCopyAs)
 *
 * @return True On successful writing the file.
 */
BOOL COEDocument::doSave (LPCWSTR lpszPathName, BOOL bReplace, BOOL bMove)
{
    CString newName = lpszPathName;
    
    if (newName.IsEmpty())
    {
        // 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db 
        //            if "save files when switching takss" is activated.
        //newName = m_strPathName;
        GetNewPathName(newName);

        if (bReplace && newName.IsEmpty())
        {
            newName = m_orgTitle;
            make_safe_filename(newName, newName);

            if (newName.Find (_T('.')) == -1)
            {
                // append the default suffix if there is one
                int iStart = 0;
                CString strExt = Common::wstr(m_settings.GetExtensions()).c_str();
                newName += '.' + strExt.Tokenize(_T(" ;,"), iStart);
            }
        }

        if (!AfxGetApp()->DoPromptFileName(newName, 
                bReplace ? (bMove ? IDS_RENAME_DIALOG_TITLE : AFX_IDS_SAVEFILE) : (AFX_IDS_SAVEFILECOPY),
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, NULL/*pTemplate*/)
            )
            return FALSE; // don't even attempt to save

        // 2016.06.06 bug fix, a new file saved w/o default extention
        CString fileName = ::PathFindFileName(newName);
        if (bReplace && fileName.Find (_T('.')) == -1)
        {
            // append the default suffix if there is one
            int iStart = 0;
            CString strExt = Common::wstr(m_settings.GetExtensions()).c_str();
            newName += '.' + strExt.Tokenize(_T(" ;,"), iStart);
        }
    }

    CWaitCursor wait;

    switch (checkAndRemoveReadonlyAttribute(newName, m_settings.GetFileOverwriteReadonly()))
    {
    case ROF_CANCEL: 
        return FALSE;
    case ROF_SAVE_AS: 
        return DoSave(NULL, bReplace);
    case ROF_SAVE: 
        if (!OnSaveDocument(newName))
        {
            // 30.06.2004 bug fix, unknown exception on "save as", if a user doesn't have permissions to rewrite the destination 
            return FALSE;
        }
    }

    // reset the title and change the document name
    if (bReplace)
        SetPathName(newName);

    return TRUE;        // success
}

BOOL COEDocument::doSave2 (LPCWSTR lpszPathName, BOOL bReplace, BOOL bMove)
{
    try
    {
        if (!lpszPathName) m_newOrSaveAs = true;
        BOOL retVal = doSave(lpszPathName, bReplace, bMove);
        m_newOrSaveAs = false;

        OnDocumentEvent(onAfterSaveDocument);

        if (RecentFileList* rfl = GetRecentFileList())
            rfl->OnSaveDocument(this);

        return retVal;
    }
    catch (...)
    {
        m_newOrSaveAs = false;
        throw;
    }
}

/** @fn void COEDocument::backupFile (LPCWSTR lpszPathName)
 * @brief Do the backup only first time after opening (or first saving) the file.
 *
 * @arg lpszPathName Path name where to save backup document file.
 * @return Nothing or THROW_APP_EXCEPTION on error in CopyFile() WinAPI function.
 */
void COEDocument::backupFile (LPCWSTR lpszPathName)
{
    if (!PathFileExists(lpszPathName)) // new file while sending email
        return;

    EBackupMode backupMode = (EBackupMode)m_settings.GetFileBackupV2();

    if (!m_newOrSaveAs &&  backupMode != ebmNone && lpszPathName)
    {
        CString backupPath;

        switch (backupMode)
        {
        case ebmCurrentDirectory:
            {
                CString path, name, ext;
                    
                CString buff(lpszPathName);
                ::PathRemoveFileSpec(buff.LockBuffer()); buff.UnlockBuffer();
                path = buff;

                ext = ::PathFindExtension(lpszPathName) + 1;

                buff = ::PathFindFileName(lpszPathName);
                ::PathRemoveExtension(buff.LockBuffer()); buff.UnlockBuffer();
                name = buff;

                Common::Substitutor substr;
                substr.AddPair("<NAME>", Common::str(name));
                substr.AddPair("<EXT>",  Common::str(ext));
                substr << m_settings.GetFileBackupName().c_str();

                //backupPath = path;
                //backupPath += L"\\";  
                //backupPath += Common::to_wstr(substr.GetResult()).c_str();
                ::PathCombine(backupPath.GetBuffer(MAX_PATH), path, Common::wstr(substr.GetResult()).c_str());
                backupPath.ReleaseBuffer();
            }
            break; // 09.03.2003 bug fix, backup file problem if a destination is the current folder
        case ebmBackupDirectory:
            {
                CString path = Common::wstr(m_settings.GetFileBackupDirectoryV2()).c_str();

                if (path.IsEmpty())
                {
                    ::PathCombine(path.GetBuffer(MAX_PATH), ConfigFilesLocator::GetBaseFolder().c_str(), L"Backup");
                    path.ReleaseBuffer();
                }

                if (!::PathFileExists(path))
                    AppCreateFolderHierarchy(path);

                ::PathCombine(backupPath.GetBuffer(MAX_PATH), path, ::PathFindFileName(lpszPathName));
                backupPath.ReleaseBuffer();
            }
            break;
        }

        if (m_lastBackupPath.CompareNoCase(backupPath) != 0 // only for the first attemption
        && GetPathName().CompareNoCase(backupPath) != 0)  // avoiding copy the file into itself
        {
            // DO the backup by copying 'old' file (only first time since opening!)
            if(::CopyFile(lpszPathName, backupPath, FALSE) == 0) 
            {
                std::wstring os_error, err_desc;
                OsException::GetLastError(os_error);
                err_desc = L"FROM: \"";
                err_desc += lpszPathName;
                err_desc += L"\"\nTO: \"" + backupPath;
                err_desc += L"\"\nERROR: ";
                err_desc += os_error;

                THROW_APP_EXCEPTION(err_desc);
            } else {
                m_lastBackupPath = backupPath;
            }
        }
    }
}

BOOL COEDocument::SaveModified ()
{
    return CDocument::SaveModified();
}

void COEDocument::SetModifiedFlag (BOOL bModified)
{
    if (!bModified)
        m_storage.SetSavedUndoPos(); // 14.04.2003 bug fix, SetModifiedFlag clears an undo history
    else
        m_storage.SetUnconditionallyModified();
}

void COEDocument::ClearUndo ()
{
    m_storage.ClearUndo();
}

void COEDocument::OnEditPrintPageSetup()
{
    SettingsManager mgr(getSettingsManager());

    if (COEPrintPageSetup(mgr).DoModal() == IDOK)
    {
        getSettingsManager() = mgr;
        SaveSettingsManager();
    }
}

void COEDocument::SetText (LPVOID vmdata, unsigned long length)
{
    m_storage.Clear(false);
    m_mmfile.Close();
    if (m_vmdata) 
    {
        VirtualFree(m_vmdata, 0, MEM_RELEASE);
        m_vmsize = 0;
    }

    m_vmdata = vmdata;
    m_vmsize = length;
    m_storage.SetText((const char*)m_vmdata, length, true/*use_buffer*/, false/*reload*/, false/*external*/);
}

// FRM called from OEWorkspaceManager, probably encoding should be passed too
void COEDocument::SetText (const char* text, unsigned long length)
{
    LPVOID vmdata = VirtualAlloc(NULL, length, MEM_COMMIT, PAGE_READWRITE);
    
    try { EXCEPTION_FRAME;

        memcpy(vmdata, text, length);
        m_vmsize = length;
        SetText(vmdata, length);
    }
    catch (...)
    {
        if (vmdata != m_vmdata)
        {
            VirtualFree(vmdata, 0, MEM_RELEASE);
            m_vmsize = 0;
        }
    }
}

#include "OpenEditor/OESettingsXmlStreamer.h"
#include "OpenEditor/OELanguageXmlStreamer.h"
#include "OpenEditor/OETemplateXmlStreamer.h"
#include "Common/ConfigFilesLocator.h"

    class TemplateFileWatcher : public CFileWatchClient
    {
        virtual void OnWatchedFileChanged ()
        {
            COEDocument::ReloadTemplates();
        }
    public:
        using CFileWatchClient::StartWatching;
        using CFileWatchClient::StopWatching;
        using CFileWatchClient::UpdateWatchInfo;
    };

    TemplateFileWatcher& getTemplateFileWatcher ()
    {
        // this way it will be initialized on the first call
        static TemplateFileWatcher s_templateFileWatcher;
        return s_templateFileWatcher;
    }

void COEDocument::LoadSettingsManager ()
{
    std::wstring path;
    
    bool exists = ConfigFilesLocator::GetFilePath(L"languages.xml", path); 
    LanguageXmlStreamer(path) >> LanguagesCollection();
    if (!exists) LanguageXmlStreamer(path) << LanguagesCollection();

    exists = ConfigFilesLocator::GetFilePath(L"templates.xml", path); 
    getTemplateFileWatcher().StopWatching();
    TemplateXmlStreamer(path) >> getSettingsManager(); // that can upgrade the existing file
    getTemplateFileWatcher().UpdateWatchInfo();
    getTemplateFileWatcher().StartWatching(path.c_str());


    exists = ConfigFilesLocator::GetFilePath(L"settings.xml", path); 
    SettingsXmlStreamer(path) >> getSettingsManager();
    if (!exists) SettingsXmlStreamer(path) << getSettingsManager();

    exists = ConfigFilesLocator::GetFilePath(L"custom.keymap", path); 
    if (!exists)
    {
        string text;
        if (AppLoadTextFromResources(L"custom.keymap", L"TEXT", text))
            std::ofstream(path.c_str()) << text;
    }
}

void COEDocument::ReloadTemplates ()
{
    std::wstring path;
    ConfigFilesLocator::GetFilePath(L"templates.xml", path); 
    getTemplateFileWatcher().StopWatching();
    TemplateXmlStreamer(path) >> getSettingsManager(); // that can upgrade the existing file
    getTemplateFileWatcher().UpdateWatchInfo();
    getTemplateFileWatcher().StartWatching(path.c_str());
}

    static bool g_isBackupDone = false;
    static void backup_file (const wstring& file)
    {
        if (::PathFileExists(file.c_str()))
        {
            if (!::CopyFile(file.c_str(), (file + L".old").c_str(), FALSE))
                THROW_APP_EXCEPTION("Cannot backup file \"" + Common::str(file) + "\"");
        }
    }

void COEDocument::SaveSettingsManager ()
{
    std::wstring path;
    
    ConfigFilesLocator::GetFilePath(L"settings.xml", path); 
    if (!g_isBackupDone) backup_file(path);
    SettingsXmlStreamer(path) << getSettingsManager();

    ConfigFilesLocator::GetFilePath(L"templates.xml", path); 
    if (!g_isBackupDone) backup_file(path);
    getTemplateFileWatcher().StopWatching();
    TemplateXmlStreamer(path) << getSettingsManager();
    getTemplateFileWatcher().UpdateWatchInfo();
    getTemplateFileWatcher().StartWatching(path.c_str());

    g_isBackupDone = true;
}

bool COEDocument::ShowSettingsDialog (SettingsDialogCallback* callback)
{
    string category;
    if (CWnd* pWnd = AfxGetMainWnd())
    {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)))
        {
            if (CMDIChildWnd * pChild = ((CMDIFrameWnd*)pWnd)->MDIGetActive())
                if (CDocument* pActiveDoc = pChild->GetActiveDocument())
                    if (pActiveDoc->IsKindOf(RUNTIME_CLASS(COEDocument)))
                        category = ((COEDocument*)pActiveDoc)->GetSettings().GetName();
        }
        else if (pWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        {
            if (CDocument* pActiveDoc = ((CFrameWnd*)pWnd)->GetActiveDocument())
                if (pActiveDoc->IsKindOf(RUNTIME_CLASS(COEDocument)))
                    category = ((COEDocument*)pActiveDoc)->GetSettings().GetName();
        }        
    }

    SettingsManager mgr(getSettingsManager());

    static UINT gStartPage = 0;
    Common::CPropertySheetMem sheet(L"Settings", gStartPage);
    sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
    sheet.SetTreeWidth(200);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    COEGeneralPage   gp(mgr);                  
    COEFilePage      fp(mgr);                  
    OEWorkspacePage  wp(mgr);                  
    COEFileManagerPage mp(mgr);                
    COEBackupPage    ap(mgr);                  
    COEEditingPage   ep(mgr);                  
    COEBlockPage     bp(mgr);                  
    COEClassPage     cp(mgr, category.c_str());
    COETemplatesPage tp(mgr, category.c_str());

    std::vector<VisualAttributesSet*> vasets;
    for (int i = 0, ilimit = mgr.GetClassCount() ; i < ilimit; i++)
    {
        ClassSettingsPtr settings = mgr.GetClassByPos(i);
        VisualAttributesSet& vaset = settings->GetVisualAttributesSet();
        vaset.SetName(settings->GetName());
        vasets.push_back(&vaset);
    }

    CVisualAttributesPage vp(vasets, category.c_str());
    
    if (callback) 
        callback->BeforePageAdding(sheet);

    sheet.AddPage(&gp);
    sheet.AddPage(&fp);
    sheet.AddPage(&wp);
    sheet.AddPage(&mp);
    sheet.AddPage(&ap);
    sheet.AddPage(&ep);
    sheet.AddPage(&bp);
    sheet.AddPage(&cp);
    sheet.AddPage(&tp);
    sheet.AddPage(&vp);

    if (callback) 
        callback->AfterPageAdding(sheet);

    if (sheet.DoModal() == IDOK)
    {
        getSettingsManager() = mgr;
        SaveSettingsManager();
        if (callback) 
            callback->OnOk();
        return true;
    }

    if (callback) 
        callback->OnCancel();
   
    return false;
}

void COEDocument::ShowTemplateDialog (OpenEditor::TemplatePtr, int index, const std::wstring& text)
{
    string category;
    if (CWnd* pWnd = AfxGetMainWnd())
    {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CMDIFrameWnd)))
        {
            if (CMDIChildWnd * pChild = ((CMDIFrameWnd*)pWnd)->MDIGetActive())
                if (CDocument* pActiveDoc = pChild->GetActiveDocument())
                    if (pActiveDoc->IsKindOf(RUNTIME_CLASS(COEDocument)))
                        category = ((COEDocument*)pActiveDoc)->GetSettings().GetName();
        }
        else if (pWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        {
            if (CDocument* pActiveDoc = ((CFrameWnd*)pWnd)->GetActiveDocument())
                if (pActiveDoc->IsKindOf(RUNTIME_CLASS(COEDocument)))
                    category = ((COEDocument*)pActiveDoc)->GetSettings().GetName();
        }        
    }

    SettingsManager mgr(getSettingsManager());

    CPropertySheet sheet;//("Settings");
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    COETemplatesPage tp(mgr, category.c_str(), index);
    tp.m_psp.pszTitle = L"Templates";
    tp.m_psp.dwFlags |= PSP_USETITLE;
    tp.SetNewTemplateText(Common::str(text));

    sheet.AddPage(&tp);

    if (sheet.DoModal() == IDOK)
    {
        getSettingsManager() = mgr;
        SaveSettingsManager();
    }
}

void COEDocument::OnEditFileSettings()
{
    COEFileInfoDlg infoPage;

    unsigned usage, allocated, undo;
    m_storage.GetMemoryUsage(usage, allocated, undo);

    format_number(infoPage.m_Lines,           m_storage.GetLineCount(), L" lines", true);
    format_number(infoPage.m_MemoryUsage,     usage                   , L" bytes", true);
    format_number(infoPage.m_MemoryAllocated, allocated               , L" bytes", true);
    format_number(infoPage.m_UndoMemoryUsage, undo                    , L" bytes", true);

    infoPage.m_Path = GetPathName();

    if (!infoPage.m_Path.IsEmpty())
    {
        HANDLE hFile = CreateFile(
            infoPage.m_Path,        // file name
            0,                      // access mode
            FILE_SHARE_READ,        // share mode
            NULL,                   // SD
            OPEN_EXISTING,          // how to create
            FILE_ATTRIBUTE_NORMAL,  // file attributes
            NULL                    // handle to template file
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            BY_HANDLE_FILE_INFORMATION fileInformation;

            if (GetFileInformationByHandle(hFile, &fileInformation))
            {
                if (!fileInformation.nFileSizeHigh)
                    format_number(infoPage.m_DriveUsage, fileInformation.nFileSizeLow, L" bytes", false);

                filetime_to_string (fileInformation.ftCreationTime,  infoPage.m_Created);
                filetime_to_string (fileInformation.ftLastWriteTime, infoPage.m_LastModified);
            }

            CloseHandle(hFile);
        }
    }

    Settings settings(getSettingsManager());
    settings = m_settings;
    COEPropPage propPage(getSettingsManager(), settings);

    static UINT gStartPage = 0;
    POSITION pos = GetFirstViewPosition();
    Common::CPropertySheetMem sheet(L"File Settings", gStartPage, GetNextView(pos));
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;

    sheet.AddPage(&propPage);
    sheet.AddPage(&infoPage);

    if (sheet.DoModal() == IDOK)
    {
        m_settings = settings;
    }
}

void COEDocument::OnWatchedFileChanged ()
{
    try { EXCEPTION_FRAME;

        // ignore notification if it's a disabled feature
        // TODO#2: Notify about changes but do not re-load the content if IsContentLocked()
        if (!IsContentLocked() && m_settings.GetFileDetectChanges())
        {
            CString prompt = GetPathName();
            prompt += "\n\nThis file has been modified outside of the editor."
                      "\nDo you want to reload it";
            prompt += !IsModified() ? "?" : " and lose the changes made in the editor?";

            if ((m_settings.GetFileReloadChanges() && !IsModified())
            || AfxMessageBox(prompt, 
                !IsModified() ? MB_YESNO|MB_ICONQUESTION : MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
            {
                vector<COEditorView*> viewsToScroll;
                if (m_settings.GetFileAutoscrollAfterReload())
                {
                    POSITION pos = GetFirstViewPosition();
                    while (pos != NULL) 
                    {
                        CView* pView = GetNextView(pos);
                        if (pView && (pView->IsKindOf(RUNTIME_CLASS(COEditorView))))
                        {
                            Position curPos = ((COEditorView*)pView)->GetPosition();
                            if (curPos.line >= m_storage.GetLineCount()-1)
                                viewsToScroll.push_back(((COEditorView*)pView));
                        }
                    }
                }
                Global::SetStatusText(CString(L"Reloading file \"") + GetPathName() + L"\" ...");
                CFileWatchClient::UpdateWatchInfo();
                loadFile(GetPathName(), true/*reload*/, true/*external*/);
                Global::SetStatusText(CString(L"File \"") + GetPathName() + L"\" reloaded.");

                if (viewsToScroll.size())
                {
                    
                    for (std::vector<COEditorView*>::iterator it = viewsToScroll.begin();
                        it != viewsToScroll.end(); ++it)
                        (*it)->MoveToBottom();
                }

            }
            else // do not want to see the same notofication again
                CFileWatchClient::UpdateWatchInfo();
        }
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnFileReload ()
{
    try { EXCEPTION_FRAME;

        if (!IsContentLocked() && !GetPathName().IsEmpty())
        {
            CString prompt = L"Do you want to reload \n\n" + GetPathName() 
                    + L"\n\nand lose the changes made in the editor?";

            if (!IsModified() || AfxMessageBox(prompt, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
            {
                Global::SetStatusText(CString(L"Reloading file \"") + GetPathName() + L"\" ...");
                loadFile(GetPathName(), true/*reload*/, true/*external*/);
                Global::SetStatusText(CString(L"File \"") + GetPathName() + L"\" reloaded.");
            }
        }
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnUpdate_FileReload (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!IsContentLocked() && !GetPathName().IsEmpty());
}

void COEDocument::OnUpdate_FileLocation (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty());
}

void COEDocument::OnUpdate_EncodingInd (CCmdUI* pCmdUI)
{
    LPCWSTR text = 0;

    if (m_saveEncoding.codepage == CP_UTF16)
        text = L" UTF16 ";
    else
    {
        int codepage = m_storage.GetCodepage();
        switch (m_storage.GetCodepage())
        {
        case CP_ACP:  
            text = L" ANSI "; 
            break;
        case CP_OEMCP:  
            text = L" OEM "; 
            break;
        case CP_UTF8: 
            text = L" UTF8 "; 
            break;
        default:
            text = WindowsCodepage::GetCodepageId(codepage).name.c_str();
            break;
        }
    }

    if (text) 
        pCmdUI->SetText(text);
    pCmdUI->Enable(TRUE);
}

void COEDocument::OnUpdate_FileTypeInd (CCmdUI* pCmdUI)
{
    pCmdUI->SetText(m_storage.GetFileFormatName());
    pCmdUI->Enable(TRUE);
}

void COEDocument::OnUpdate_FileLinesInd (CCmdUI* pCmdUI)
{
    TCHAR buff[80]; 
    buff[sizeof(buff)/sizeof(buff[0])-1] = 0;
    _snwprintf(buff, sizeof(buff)/sizeof(buff[0])-1, L" Lines: %d ", m_storage.GetLineCount());
    pCmdUI->SetText(buff);
    pCmdUI->Enable(TRUE);
}

void COEDocument::OnEditViewWhiteSpace()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetVisibleSpaces(!settings->GetVisibleSpaces());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewWhiteSpace(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetVisibleSpaces() ? TRUE : FALSE);
}

void COEDocument::OnEditViewLineNumbers ()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetLineNumbers(!settings->GetLineNumbers());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewLineNumbers (CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetLineNumbers() ? TRUE : FALSE);
}

void COEDocument::OnEditViewSyntaxGutter ()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetSyntaxGutter(!settings->GetSyntaxGutter());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewSyntaxGutter (CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetSyntaxGutter() ? TRUE : FALSE);
    pCmdUI->Enable(m_storage.GetLanguageSupport().get() && m_storage.GetLanguageSupport()->HasExtendedSupport());
}

void COEDocument::OnEditViewRuler ()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetRuler(!settings->GetRuler());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewRuler (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetRuler() ? TRUE : FALSE);
}

void COEDocument::OnEditViewColumnMarkers ()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetColumnMarkers(!settings->GetColumnMarkers());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewColumnMarkers (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetColumnMarkers() ? TRUE : FALSE);
}

void COEDocument::OnEditViewIndentGuide ()
{
    GlobalSettingsPtr settings = getSettingsManager().GetGlobalSettings();
    settings->SetIndentGuide(!settings->GetIndentGuide());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewIndentGuide (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetIndentGuide() ? TRUE : FALSE);
}

void COEDocument::OnFileOpen ()
{
    CString path = GetPathName();
    if (m_settings.GetWorkDirFollowsDocument() && !path.IsEmpty())
    {
        PathRemoveFileSpec(path.LockBuffer());
        path.UnlockBuffer();
    }
    AfxGetApp()->OpenDocumentFile(!path.IsEmpty() ? (LPCWSTR)path : NULL); // COEDocManager will open a file dialog using this path
}

void COEDocument::OnFileCloseOthers ()
{
    CDocTemplate* pDocTempl = GetDocTemplate();
    ASSERT_KINDOF(COEMultiDocTemplate, pDocTempl);
    if (pDocTempl->IsKindOf(RUNTIME_CLASS(COEMultiDocTemplate)))
        ((COEMultiDocTemplate*)pDocTempl)->SaveAndCloseAllDocuments(this);
}

void COEDocument::OnUpdate_FileCloseOthers (CCmdUI* pCmdUI)
{
    CDocTemplate* pDocTempl = GetDocTemplate();
    ASSERT_KINDOF(COEMultiDocTemplate, pDocTempl);
    if (pDocTempl->IsKindOf(RUNTIME_CLASS(COEMultiDocTemplate)))
        pCmdUI->Enable(((COEMultiDocTemplate*)pDocTempl)->GetDocumentCount() > 1);
}

void COEDocument::OnFileRename ()
{
    CString path = GetPathName();

    if (path.IsEmpty()) // new document
    {
        Common::CInputDlg dlg;
        dlg.m_prompt = L"Rename title to:";
        dlg.m_value  = GetTitle();
        if (dlg.DoModal() == IDOK)
            SetTitle(dlg.m_value.c_str());
    }
    else // existing file
    {
        if (doSave2(NULL, TRUE, TRUE))
        {
            if (path != GetPathName())
            {
                if (!::DeleteFile(path))
                {
                    std::wstring os_error;
                    OsException::GetLastError(os_error);

                    MessageBeep(MB_ICONERROR);
                    AfxMessageBox((L"Cannot delete the original file!\n\nERROR: " + os_error).c_str(), MB_ICONERROR | MB_OK);
                }
            }
        }
    }
}

void COEDocument::OnFileCopyLocation ()
{
    try { EXCEPTION_FRAME;
        Common::AppCopyTextToClipboard((LPCWSTR)GetPathName());
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnFileCopyName ()
{
    try { EXCEPTION_FRAME;

        CString buff = (LPCWSTR)GetPathName();
        
        if (!buff.IsEmpty())
        {
            buff = ::PathFindFileName(buff);
        }
        else
        {
            buff = GetTitle();
            buff.TrimRight(L" *");
        }

        Common::AppCopyTextToClipboard(buff);
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnUpdate_FileCopyLocation (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty());
}

void COEDocument::OneditFileFormatWinows ()
{
    m_storage.SetFileFormat(effDos);
}

void COEDocument::OneditFileFormatUnix ()
{
    m_storage.SetFileFormat(effUnix);
}

void COEDocument::OneditFileFormatMac ()
{
    m_storage.SetFileFormat(effMac);
}

void COEDocument::OnUpdate_FileFormat (CCmdUI* pCmdUI)
{
    if (pCmdUI->m_pMenu)
    {
        pCmdUI->m_pMenu->CheckMenuRadioItem(
            ID_EDIT_FILE_FORMAT_WINOWS, ID_EDIT_FILE_FORMAT_MAC,
            m_storage.IsMsDosFormat() ? ID_EDIT_FILE_FORMAT_WINOWS 
                : (m_storage.IsUnixFormat() ? ID_EDIT_FILE_FORMAT_UNIX : ID_EDIT_FILE_FORMAT_MAC),
            MF_BYCOMMAND
            );
    }
    else
    {
        switch (pCmdUI->m_nID)
        {
        case ID_EDIT_FILE_FORMAT_WINOWS: pCmdUI->SetRadio(m_storage.IsMsDosFormat()); break;
        case ID_EDIT_FILE_FORMAT_UNIX  : pCmdUI->SetRadio(m_storage.IsUnixFormat()); break;
        case ID_EDIT_FILE_FORMAT_MAC   : pCmdUI->SetRadio(m_storage.IsMacFormat()); break;
        }
    }
}

void COEDocument::SaveEncoding (TiXmlElement& elem) const
{
    elem.SetAttribute("codepage",      m_storage.GetCodepage());
    elem.SetAttribute("open_codepage", m_openEncoding.codepage);
    elem.SetAttribute("open_bom",      m_openEncoding.bom);
    elem.SetAttribute("save_codepage", m_openEncoding.codepage);
    elem.SetAttribute("save_bom",      m_openEncoding.bom);
}

void COEDocument::RestoreEncoding (const TiXmlElement& elem)
{
    int cp = 0;
    elem.Attribute("codepage", &cp);
    m_storage.SetCodepage(cp, false);
    elem.Attribute("open_codepage", &m_openEncoding.codepage);
    elem.Attribute("open_bom",      &m_openEncoding.bom);
    elem.Attribute("save_codepage", &m_openEncoding.codepage);
    elem.Attribute("save_bom",      &m_openEncoding.bom);
}

int COEDocument::GetEncodingCodepage () const
{
    return m_storage.GetCodepage();
}

void COEDocument::SetEncodingCodepage (int codepage)
{
    if (m_saveEncoding.codepage != CP_UTF16 && m_saveEncoding.bom == 0)
    {
        m_storage.SetCodepage(codepage, m_storage.CanUndo() ? true :  false);
        m_saveEncoding.codepage = codepage;
    }
}

void COEDocument::OnEditEncodingAnsi ()
{
    SetEncodingCodepage(CP_ACP);
}

void COEDocument::OnEditEncodingOem ()
{
    SetEncodingCodepage(CP_OEMCP);
}

void COEDocument::OnEditEncodingUtf8 ()
{
    SetEncodingCodepage(CP_UTF8);
}

void COEDocument::OnUpdate_Encoding (CCmdUI* pCmdUI)
{
    UINT selected = (UINT)-1;

    switch (m_storage.GetCodepage())
    {
    case CP_ACP:   selected = ID_EDIT_ENCODING_ANSI; break;
    case CP_OEMCP: selected = ID_EDIT_ENCODING_OEM; break;
    case CP_UTF8:  
    case CP_UTF16: selected = ID_EDIT_ENCODING_UTF8; break;
    }

    if (pCmdUI->m_pMenu)
        pCmdUI->m_pMenu->CheckMenuRadioItem(ID_EDIT_ENCODING_ANSI, ID_EDIT_ENCODING_UTF8, selected, MF_BYCOMMAND);
    else if (ID_EDIT_ENCODING_ANSI <= pCmdUI->m_nID && pCmdUI->m_nID <= ID_EDIT_ENCODING_UTF8)
        pCmdUI->SetRadio(pCmdUI->m_nID == selected);

    pCmdUI->Enable(m_saveEncoding.codepage != CP_UTF16 && m_saveEncoding.bom == 0);
}

void COEDocument::OnUpdate_EncodingCodepage (CCmdUI* pCmdUI)
{
	ENSURE_ARG(pCmdUI != NULL);

    BOOL enabled = m_saveEncoding.codepage != CP_UTF16 && m_saveEncoding.bom == 0;

    if (pCmdUI->m_nID == ID_EDIT_ENCODING_CODEPAGE_FIRST
    && pCmdUI->m_pMenu && pCmdUI->m_pMenu->GetMenuItemCount() == 1)
    {
        pCmdUI->m_pMenu->DeleteMenu(0, MF_BYPOSITION);

        const std::vector<WindowsCodepage>& codepages = WindowsCodepage::GetAll();
        std::wstring group = codepages.size() > 0 ? codepages.front().group : std::wstring();

        for (auto it = codepages.begin(); it != codepages.end(); ++it)
        {
            if (group != it->group)
		        pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++, MF_STRING | MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, L"");
		    
            pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++, 
                enabled ? MF_STRING | MF_BYPOSITION : MF_STRING | MF_BYPOSITION | MF_GRAYED, 
                pCmdUI->m_nID++, it->longName.c_str());
            
            group = it->group;
        }

	    pCmdUI->m_nIndex--; // point to last menu added
	    pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();
	    pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
    }

    int inx = WindowsCodepage::GetIndexId(m_storage.GetCodepage());

    if (pCmdUI->m_pMenu)
            pCmdUI->m_pMenu->CheckMenuRadioItem(
                ID_EDIT_ENCODING_CODEPAGE_FIRST, 
                ID_EDIT_ENCODING_CODEPAGE_LAST, 
                (inx != -1) ? ID_EDIT_ENCODING_CODEPAGE_FIRST + inx : -1, 
                MF_BYCOMMAND);
    else if (inx != -1) 
        pCmdUI->SetRadio(pCmdUI->m_nID == UINT(ID_EDIT_ENCODING_CODEPAGE_FIRST + inx));

    pCmdUI->Enable(enabled);
}

BOOL COEDocument::OnEncodingCodepage (UINT nID)
{
    if (m_saveEncoding.codepage != CP_UTF16 && m_saveEncoding.bom == 0)
    {
        const WindowsCodepage& codepage = WindowsCodepage::GetCodepageIndex(nID - ID_EDIT_ENCODING_CODEPAGE_FIRST);
        if (codepage.id != 0)
            SetEncodingCodepage(codepage.id);
    }
    return TRUE;
}

COEDocument::UndoFileEncoding::UndoFileEncoding (COEDocument& document, Encoding oldEncoding, Encoding newEncoding)
    :m_document(document), m_oldEncoding(oldEncoding), m_newEncoding(newEncoding)
{
}

void COEDocument::UndoFileEncoding::Undo (UndoContext& cxt) const
{
    cxt.m_onlyStorageAffected = true;
    m_document.m_saveEncoding = m_oldEncoding;
}

void COEDocument::UndoFileEncoding::Redo (UndoContext& cxt) const
{
    cxt.m_onlyStorageAffected = true;
    m_document.m_saveEncoding = m_newEncoding;
}

unsigned COEDocument::UndoFileEncoding::GetMemUsage() const
{
    return sizeof(*this);
}

void COEDocument::SetFileEncodingCodepage (int codepage, int bom)
{
    if (m_saveEncoding.codepage != codepage || m_saveEncoding.bom != bom)
    {
        OpenEditor::Storage::UndoGroup undoGroup(m_storage);

        Encoding oldOne = m_saveEncoding;
        m_saveEncoding.codepage = codepage;
        m_saveEncoding.bom = bom;

        m_storage.pushInUndoStack(new UndoFileEncoding(*this, oldOne, m_saveEncoding));

        m_storage.ConvertToCodepage(m_saveEncoding.codepage != CP_UTF16 ? m_saveEncoding.codepage : CP_UTF8);
    }
}

void COEDocument::OnFileEncodingAnsi ()
{
    SetFileEncodingCodepage(CP_ACP, 0);
}

void COEDocument::OnFileEncodingOem ()
{
    SetFileEncodingCodepage(CP_OEMCP, 0);
}

void COEDocument::OnFileEncodingUtf8 ()
{
    SetFileEncodingCodepage(CP_UTF8, 0);
}

void COEDocument::OnFileEncodingUtf8wBom ()
{
    SetFileEncodingCodepage(CP_UTF8, 3);
}

void COEDocument::OnFileEncodingUtf16wBom ()
{
    SetFileEncodingCodepage(CP_UTF16, 2);
}

void COEDocument::OnUpdate_FileEncoding (CCmdUI* pCmdUI)
{
    UINT selected = (UINT)-1;

    switch (m_saveEncoding.codepage)
    {
    case CP_ACP:   selected = ID_FILE_ENCODING_ANSI; break;
    case CP_OEMCP: selected = ID_FILE_ENCODING_OEM; break;
    case CP_UTF8:  selected = m_saveEncoding.bom > 0 ? ID_FILE_ENCODING_UTF8_BOM : ID_FILE_ENCODING_UTF8; break;
    case CP_UTF16: selected = ID_FILE_ENCODING_UTF16_BOM; break;
    }

    if (pCmdUI->m_pMenu)
        pCmdUI->m_pMenu->CheckMenuRadioItem(ID_FILE_ENCODING_ANSI, ID_FILE_ENCODING_UTF16_BOM, selected, MF_BYCOMMAND);
    else if (ID_FILE_ENCODING_ANSI <= pCmdUI->m_nID && pCmdUI->m_nID <= ID_FILE_ENCODING_UTF16_BOM)
        pCmdUI->SetRadio(pCmdUI->m_nID == selected);
}

void COEDocument::OnUpdate_FileEncodingCodepage (CCmdUI* pCmdUI)
{
	ENSURE_ARG(pCmdUI != NULL);

    if (pCmdUI->m_nID == ID_FILE_ENCODING_CODEPAGE_FIRST
    && pCmdUI->m_pMenu && pCmdUI->m_pMenu->GetMenuItemCount() == 1)
    {
        pCmdUI->m_pMenu->DeleteMenu(0, MF_BYPOSITION);

        const std::vector<WindowsCodepage>& codepages = WindowsCodepage::GetAll();
        std::wstring group = codepages.size() > 0 ? codepages.front().group : std::wstring();

        for (auto it = codepages.begin(); it != codepages.end(); ++it)
        {
            if (group != it->group)
		        pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++, MF_STRING | MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, L"");
		    pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++, MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++, it->longName.c_str());
            group = it->group;
        }

	    pCmdUI->m_nIndex--; // point to last menu added
	    pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();
	    pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
    }

    int inx = WindowsCodepage::GetIndexId(m_saveEncoding.codepage);

    if (pCmdUI->m_pMenu)
            pCmdUI->m_pMenu->CheckMenuRadioItem(
                ID_FILE_ENCODING_CODEPAGE_FIRST, 
                ID_FILE_ENCODING_CODEPAGE_LAST, 
                (inx != -1) ? ID_FILE_ENCODING_CODEPAGE_FIRST + inx : -1, 
                MF_BYCOMMAND);
    else if (inx != -1) 
        pCmdUI->SetRadio(pCmdUI->m_nID == UINT(ID_FILE_ENCODING_CODEPAGE_FIRST + inx));
}

BOOL COEDocument::OnFileEncodingCodepage (UINT nID)
{
    const WindowsCodepage& codepage = WindowsCodepage::GetCodepageIndex(nID - ID_FILE_ENCODING_CODEPAGE_FIRST);

    if (codepage.id != 0)
        SetFileEncodingCodepage(codepage.id, 0);

    return TRUE;
}

COEditorView* COEDocument::GetFirstEditorView ()
{
   POSITION pos = GetFirstViewPosition();
   
   while (pos != NULL)
   {
        CView *pView = GetNextView(pos);
        if (pView && (pView->IsKindOf(RUNTIME_CLASS(COEditorView))))
            return (COEditorView*)pView;
   }

   return nullptr;
}
