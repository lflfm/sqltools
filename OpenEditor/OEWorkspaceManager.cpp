/* 
    Copyright (C) 2016 Aleksey Kochetov

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

// 2018-12-09 bug fix, cannot open a workspace with a bookmark beyond eof

/*
TODO#1: 
    - suggest list on open
    - add file class and instance overriden settings (tab/indentation/...)

    - + add size / time to xml file description
    - + file saved w/o extension
    - + open file dialog in last choosen folder
    - + keep history of open files with cursor pos/selection/bookmarks
    - + change setting dialog, parameter to exclude big docs from autosaving
    - + implement the same default backup folder in oe document 
    - + open workspace on drag&drop and ...
    - + manage backup folder size
    - + add new workspace command to config
    - + exclude original empty doc from backup
    - + error handling!
    - + open files after crash
    - + wakeup on shutown
*/

#include "stdafx.h"
#include <algorithm>
#include <utility>
#include <stdlib.h>
#include <tinyxml\tinyxml.h>
#include "OpenEditor/OEWorkspaceManager.h"
#include "OpenEditor/OEDocManager.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/RecentFileList.h"
#include "OpenEditor/FavoritesList.h"
#include "Common/ConfigFilesLocator.h"
#include "Common/OsException.h"
#include "Common/AppUtilities.h"
#include "Common/clock64.h"
#include "Common/MyUtf.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include "ThreadCommunication/Singleton.h"

//#pragma comment(lib, "rpcrt4.lib")

    using namespace Common;
    using namespace OpenEditor;
    using namespace ThreadCommunication;

    #define CHECK_FILE_OPERATION(r) if(!(r)) OsException::CheckLastError();


    typedef ThreadCommunication::Singleton<OEWorkspaceManager, 1> OEWorkspaceManagerSingleton;

    CEvent OEWorkspaceManager::m_eventShutdown;
    CEvent OEWorkspaceManager::m_eventInstantAutosave;

OEWorkspaceManager::OEWorkspaceManager ()
    : m_pDocManager(NULL), 
    m_pDocTemplate(NULL),
    m_workspaceExtension(".oe-workspace"),
    m_snapshotExtension(".oe-snapshot"),
    m_hLockFile(INVALID_HANDLE_VALUE),
    m_pThread(NULL),
    m_onUpdateApplicationTitle(0)
{
}

OEWorkspaceManager::~OEWorkspaceManager ()
{
    if (m_hLockFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hLockFile);
        m_hLockFile = INVALID_HANDLE_VALUE;
        DeleteFile(m_sLockFile.c_str());
    }
}

OEWorkspaceManager& OEWorkspaceManager::Get ()
{
    return OEWorkspaceManagerSingleton::Get();
}

void OEWorkspaceManager::Destroy ()
{
    m_eventShutdown.SetEvent();

    OEWorkspaceManagerSingleton::Destroy();
}

CWinThread* OEWorkspaceManager::Shutdown ()
{
    m_eventShutdown.SetEvent();

    if (m_hLockFile != INVALID_HANDLE_VALUE)
    {
        if (COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceAutosaveDeleteOnExit())
        {
            CString buff;
            DWORD nreadbytes = MAX_PATH;
            SetFilePointer(m_hLockFile, 0, NULL, FILE_BEGIN);
            if (ReadFile(m_hLockFile, buff.GetBuffer(nreadbytes), nreadbytes, &nreadbytes, NULL))
            {
                buff.ReleaseBufferSetLength(nreadbytes);
                if (!buff.IsEmpty())
                    DeleteFile((LPCTSTR)buff);
            }
        }

        CloseHandle(m_hLockFile);
        m_hLockFile = INVALID_HANDLE_VALUE;
        DeleteFile(m_sLockFile.c_str());
    }

    return m_pThread;
}

CWinThread* OEWorkspaceManager::ImmediateShutdown ()
{
    m_eventShutdown.SetEvent();

    if (m_pThread)
        WaitForSingleObject(*m_pThread, 15000); // gives some time for InstantAutosave

    if (m_hLockFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hLockFile);
        m_hLockFile = INVALID_HANDLE_VALUE;
    }
    m_sLockFile.clear(); // keep the file for automatic recovery

    return m_pThread;
}

void OEWorkspaceManager::InstantAutosave ()                             
{ 
    m_eventInstantAutosave.SetEvent(); 
}

    static
    void create_unique_fname (CString& buff)
    {
        const int buff_size = 80;
        time_t t = time((time_t*) 0);
        struct tm* tp = localtime(&t);
        wcsftime(buff.GetBuffer(buff_size), buff_size, L"%Y-%m-%d_%H-%M-%S_pid_", tp);
        buff.ReleaseBuffer();
        LPTSTR ptr = buff.GetBuffer(buff_size);
        size_t length = buff.GetLength();
        _itow(GetCurrentProcessId(), ptr + length, 10);
        buff.ReleaseBuffer();
    }
    

void OEWorkspaceManager::Init (COEDocManager* pDocManager, COEMultiDocTemplate* pDocTemplate, 
    arg::counted_ptr<RecentFileList>& recentFileList, arg::counted_ptr<FavoritesList>&  favoritesList
)
{
    CString error;

    try { EXCEPTION_FRAME;
        
        MessageOnlyWindow::GetWindow(); // let's make sure this window is created in the main thread

        m_pDocManager = pDocManager;
        m_pDocTemplate = pDocTemplate;
        m_pRecentFileList = recentFileList;
        m_pFavoritesList = favoritesList;

        //UUID uuid;
        //UuidCreate(&uuid);
        //char* pszUuid;
        //UuidToString(&uuid, (RPC_CSTR*)&pszUuid);
        //m_sUuid = pszUuid;

        CString buff;
        create_unique_fname(buff);
        m_sUuid = buff;

        m_sLockFile = GetInstanceFolder();
        if (!::PathFileExists(m_sLockFile.c_str()))
            AppCreateFolderHierarchy(m_sLockFile.c_str());

        m_sLockFile += L"\\" + m_sUuid;
        m_hLockFile = CreateFile(m_sLockFile.c_str(), FILE_GENERIC_WRITE|FILE_GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (m_hLockFile == INVALID_HANDLE_VALUE)
        {
            CString lastError;
            AppGetLastError(lastError);
            THROW_APP_EXCEPTION(("Can't create the instance lock file \"" + Common::str(m_sLockFile) + "\"\n\n" + Common::str(lastError)));
        }

        if (m_pThread == NULL)
        {
            m_eventShutdown.ResetEvent();
            m_pThread = AfxBeginThread(ThreadProc, NULL, THREAD_PRIORITY_BELOW_NORMAL);
            TRACE("OEWorkspaceManager: thread started\n");
        }
    }
    catch (std::exception& x)
    {
        error = x.what();
    }
    catch (CException* x)
    {
        CString text;
        x->GetErrorMessage(text.GetBuffer(128), 128);
        text.ReleaseBuffer();
        error = text;
    }
    catch (...)
    {
        error = "Unknown error";
    }

    if (!error.IsEmpty())
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((L"Workspace Backup thread initilization failed with error: \n\n"
            + error + L"\n\nAutosave will be disabled!"), MB_ICONERROR | MB_OK); 
    }
}

    struct GetBackupParam_Note : Note 
    {
        int workspaceAutosaveInterval;
        int workspaceAutosaveFileLimit;
        bool fileBackupSpaceManagment;
        int fileBackupKeepDays;
        int fileBackupFolderMaxSize;
        std::wstring backupFolder;
        bool backupFolderDefaultLocation;

        GetBackupParam_Note () 
        {
            workspaceAutosaveInterval  = 0;
            workspaceAutosaveFileLimit = 0;
            fileBackupKeepDays         = 0;
            fileBackupFolderMaxSize    = 0;
        }

        virtual void Deliver ()
        {
            GlobalSettingsPtr settings = COEDocument::GetSettingsManager().GetGlobalSettings();
            
            workspaceAutosaveInterval  = settings->GetWorkspaceAutosaveInterval ();
            workspaceAutosaveFileLimit = settings->GetWorkspaceAutosaveFileLimit();
            fileBackupSpaceManagment   = settings->GetFileBackupSpaceManagment  ();
            fileBackupKeepDays         = settings->GetFileBackupKeepDays        ();
            fileBackupFolderMaxSize    = settings->GetFileBackupFolderMaxSize   ();

            //bool backupFolderDefaultLocation;
            backupFolder = OEWorkspaceManager::Get().GetBackupFolder(&backupFolderDefaultLocation);
        }
    };

    struct CheckAndAutosave_Note : Note 
    {
        int autosaveInterval;
        std::wstring backupFolder;
        std::wstring fileName;
        CMemFile file;
        HANDLE hLockFile;
        
        CheckAndAutosave_Note () : autosaveInterval(0), hLockFile(INVALID_HANDLE_VALUE) {}

        virtual void Deliver ()
        {
            autosaveInterval = COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceAutosaveInterval();

            if (autosaveInterval > 0)
            {
                bool byDefault;
                backupFolder = OEWorkspaceManager::Get().GetBackupFolder(&byDefault);
                bool folderExists = ::PathFileExists(backupFolder.c_str());
                hLockFile = OEWorkspaceManager::Get().GetLockFileHandle();

                if (!byDefault && !folderExists)
                {
                    MessageBeep((UINT)-1);
                    AfxMessageBox((L"The backup folder \"" + backupFolder + L"\" does not exist!"
                        L"\n\nAutosave will be disabled."
                        L"\n\nPlease choose the backup folder at Settings->Editor->Backup & Autosave.").c_str(), MB_ICONEXCLAMATION|MB_OK) ;
                    autosaveInterval = 0;
                    COEDocument::GetSettingsManager().GetGlobalSettings()->SetWorkspaceAutosaveInterval(autosaveInterval);    
                }

                if (autosaveInterval)
                {
                    try
                    {
                        if (!folderExists)
                            AppCreateFolderHierarchy(backupFolder.c_str());
                    }
                    catch (std::exception& x)
                    {
                        MessageBeep((UINT)-1);
                        AfxMessageBox((L"Create Backup folder failed with error: \n\n"
                            + Common::wstr(x.what()) + L"\n\nAutosave will be disabled!").c_str(), MB_ICONERROR | MB_OK);

                        autosaveInterval = 0;
                        COEDocument::GetSettingsManager().GetGlobalSettings()->SetWorkspaceAutosaveInterval(autosaveInterval);    
                    }

                    if (autosaveInterval)
                    {
                        OEWorkspaceManager::Get().CheckAndAutosave(file);

                        if (file.GetLength() > 0)
                        {
                            CString path;
                            ::PathCombine(path.GetBuffer(MAX_PATH+1), backupFolder.c_str(), OEWorkspaceManager::Get().GetBackupFilename());
                            fileName = path;
                        }
                    }
                }
            }
        }
    };

    struct WorkspaceGetFolders_Note : Note 
    {
        std::wstring instanceFolder, backupFolder;

        virtual void Deliver ()
        {
            instanceFolder = OEWorkspaceManager::Get().GetInstanceFolder();
            backupFolder = OEWorkspaceManager::Get().GetBackupFolder();
        }
    };

    struct WorkspaceOpenAutosaved_Note : Note 
    {
        std::wstring path;
        bool processed;

        WorkspaceOpenAutosaved_Note () : processed(false) {}

        virtual void Deliver ()
        {
            processed = false;

            std::wstring name = ::PathFindFileName(path.c_str());
            const wchar_t* ext = ::PathFindExtension(name.c_str());
            name.resize(ext - name.c_str());

            if (AfxMessageBox((L"Terminated program snapshot \"" + name + L"\" found in Backup folder!\n\nWould you like to restore it?").c_str(), MB_ICONQUESTION|MB_YESNO) == IDYES)
            {
                OEWorkspaceManager::Get().AskToCloseAllDocuments();

                CFile file;
                file.Open(path.c_str(), CFile::modeRead); 
                OEWorkspaceManager::Get().DoOpen(file, path);
                file.Close();

                processed = true;
            }
            else if (AfxMessageBox(L"Would you like to clear the crash status?", MB_ICONQUESTION|MB_YESNO) == IDYES)
            {
                processed = true;
            }
        }
    };

    static
    void check_dead_instances ()
    {
        WorkspaceGetFolders_Note nt;
        MessageOnlyWindow::GetWindow().Send(nt);

        WIN32_FIND_DATA ffdata;
        
        HANDLE hFind = ::FindFirstFile((nt.instanceFolder + L"\\*").c_str(), &ffdata);
        
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do 
            {
                if (!PathIsDirectory(ffdata.cFileName))
                {
                    std::wstring instanceLockFile = nt.instanceFolder + L"\\" + ffdata.cFileName;
                    HANDLE hFile = CreateFile(instanceLockFile.c_str(), FILE_GENERIC_WRITE|FILE_GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        // ok we obtained exclusive access to the file that means the owner is dead
                        // let's try to find and open its autosaved workspace

                        std::wstring workspaceFile;
                        // new way
                        {
                            vector<char> buff;
                            DWORD nreadbytes = MAX_PATH * 3; // for utf8
                            buff.reserve(nreadbytes);
                            if (ReadFile(hFile, buff.data(), nreadbytes, &nreadbytes, NULL))
                            {
                                if (nreadbytes > 0)
                                    workspaceFile = Common::wstr(buff.data(), nreadbytes);
                            }
                        }
                        // old way
                        if (workspaceFile.empty())
                            workspaceFile = nt.backupFolder + L"\\" + ffdata.cFileName 
                                + (LPCTSTR)OEWorkspaceManager::Get().GetWorkspaceExtension();
                        
                        if (::PathFileExists(workspaceFile.c_str()))
                        {
                            WorkspaceOpenAutosaved_Note open_nt;
                            open_nt.path = workspaceFile;
                            MessageOnlyWindow::GetWindow().Send(open_nt, (unsigned)-1, true);
                            CloseHandle(hFile);
                            if (open_nt.processed)
                                DeleteFile(instanceLockFile.c_str());
                            break;
                        }
                        else
                        {
                            // there is no autosaved workspace so cleanup it silently
                            CloseHandle(hFile);
                            DeleteFile(instanceLockFile.c_str());
                        }
                    }
                }
            
            } while(::FindNextFile(hFind, &ffdata));

            ::FindClose(hFind);
        }
    }

        struct cleanup_file_info
        {
            std::wstring filename;
            __int64 time;
            __int64 size;

            cleanup_file_info (const std::wstring& fn, const __int64& tm, const __int64& sz)
                : filename(fn), time(tm), size(sz) {}

            bool operator < (const cleanup_file_info& other) const { return time < other.time; }
        };

#include <Common/StrHelpers.h>

    static
    void cleanup_backup_folder (const std::wstring& backupFolder, int fileBackupKeepDays, int fileBackupFolderMaxSize)
    {
        CTime cutTime = CTime::GetCurrentTime() - CTimeSpan(fileBackupKeepDays, 0, 0, 0);
        SYSTEMTIME sysCutTime;
        if (cutTime.GetAsSystemTime(sysCutTime))
        {
            __int64 totalSize = 0, keepSize = 0;
            vector<cleanup_file_info> files;

            FILETIME fileCutTime;
            SystemTimeToFileTime(&sysCutTime, &fileCutTime);

            WIN32_FIND_DATA ffdata;
            HANDLE hFind = ::FindFirstFile((backupFolder + L"\\*").c_str(), &ffdata);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do 
                {
                    if (!PathIsDirectory(ffdata.cFileName))
                    {
                        FILETIME filetime;
                        FileTimeToLocalFileTime(&ffdata.ftLastWriteTime, &filetime);
                        //SYSTEMTIME systemTime;
                        //FileTimeToSystemTime(&filetime, &systemTime);

                        LARGE_INTEGER sz;
                        sz.HighPart = ffdata.nFileSizeHigh;
                        sz.LowPart  = ffdata.nFileSizeLow;
                        totalSize += sz.QuadPart;

                        // changed logic to keep at least # of days
                        if (CompareFileTime(&filetime, &fileCutTime) == 1)
                        {
                            CString buff;
                            Common::filetime_to_string(filetime, buff);
                            TRACE("%s %s to keep\n", (LPCTSTR)buff, ffdata.cFileName);
                            keepSize += sz.QuadPart;
                        }
                        else
                        {
                            LARGE_INTEGER tm;
                            tm.HighPart = ffdata.ftLastWriteTime.dwHighDateTime;
                            tm.LowPart  = ffdata.ftLastWriteTime.dwLowDateTime;
                            files.push_back(cleanup_file_info(ffdata.cFileName, tm.QuadPart, sz.QuadPart));
                        }
                    }

                } while(::FindNextFile(hFind, &ffdata));
                ::FindClose(hFind);
            }

            __int64 maxSize = __int64(fileBackupFolderMaxSize) * 1024 * 1024;

            if (totalSize > maxSize)
            {
                std::sort(files.begin(), files.end());

                vector<cleanup_file_info>::const_reverse_iterator it = files.rbegin();

                // skipping files that fit in limit
                for (; it != files.rend(); ++it)
                {
                    CString buff;
                    LARGE_INTEGER tm;
                    tm.QuadPart = it->time;
                    FILETIME filetime = { (DWORD)tm.LowPart, (DWORD)tm.HighPart };
                    Common::filetime_to_string(filetime, buff);
                    TRACE("%s %s to skip\n", (LPCTSTR)buff, it->filename.c_str());

                    if (keepSize + it->size > maxSize)
                        break;

                    keepSize += it->size;
                }

                // deleteing the rest
                for (; it != files.rend(); ++it)
                {
                    CString buff;
                    LARGE_INTEGER tm;
                    tm.QuadPart = it->time;
                    FILETIME filetime = { (DWORD)tm.LowPart, (DWORD)tm.HighPart };
                    Common::filetime_to_string(filetime, buff);
                    TRACE("%s %s to delete\n", (LPCTSTR)buff, it->filename.c_str());

                    DeleteFile((backupFolder + L"\\" + it->filename).c_str());
                }

            }
        }
    }


UINT OEWorkspaceManager::ThreadProc (LPVOID)
{
    CString error;

    try { EXCEPTION_FRAME;

        while (!AfxGetApp()->m_pMainWnd) // FRM2
            Sleep(100);

        check_dead_instances();

        clock64_t lastCleanupClock, lastSaveClock;
        lastCleanupClock = lastSaveClock = clock64();

        CArray<HANDLE, const HANDLE&> events;
        events.Add(m_eventShutdown);
        events.Add(m_eventInstantAutosave);

        bool instantAutosave = false;

        while (1)
        {
            GetBackupParam_Note paramNote;
            MessageOnlyWindow::GetWindow().Send(paramNote);

            clock64_t currClock = clock64();

            if (instantAutosave || paramNote.workspaceAutosaveInterval)
            {
                double interval = double(currClock - lastSaveClock) / CLOCKS_PER_SEC /*/ 60*/;

                if (instantAutosave || interval > paramNote.workspaceAutosaveInterval)
                {
                    CheckAndAutosave_Note backupNote;
                    MessageOnlyWindow::GetWindow().Send(backupNote);

                    if (backupNote.file.GetLength() > 0)
                    {
                        // creating the new backup file
                        HANDLE m_file = CreateFile(backupNote.fileName.c_str(), FILE_GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        CHECK_FILE_OPERATION(m_file != INVALID_HANDLE_VALUE);
                        DWORD written, length = (DWORD)backupNote.file.GetLength();
                        std::unique_ptr<BYTE[]> data(backupNote.file.Detach());
                        BOOL retVal = WriteFile(m_file, data.get(), length, &written, 0);
                        CHECK_FILE_OPERATION(retVal);
                        FlushFileBuffers(m_file);
                        CloseHandle(m_file);
                        // saving the last backup file name in the lock file
                        SetFilePointer(backupNote.hLockFile, 0, NULL, FILE_BEGIN);
                        
                        std::string utf8buff = Common::str(backupNote.fileName); // save in UTF-8 for compatibility with older versions
                        WriteFile(backupNote.hLockFile, utf8buff.c_str(), utf8buff.size(), &written, 0);

                        SetEndOfFile(backupNote.hLockFile);
                        FlushFileBuffers(backupNote.hLockFile);
                        // removing prevoius bakup file
                        static std::wstring previous;
                        if (!previous.empty() && previous != backupNote.fileName)
                            DeleteFile(previous.c_str());
                        previous = backupNote.fileName;
                    }

                    lastSaveClock = currClock;
                }
            }

            // check folder size and remove older files
            if (!instantAutosave && paramNote.fileBackupSpaceManagment)
            {
                double interval = double(currClock - lastCleanupClock) / CLOCKS_PER_SEC;
                
                if (interval > 60 * 5) // check every 5 min
                {
                    cleanup_backup_folder(paramNote.backupFolder, paramNote.fileBackupKeepDays, paramNote.fileBackupFolderMaxSize);
                    lastCleanupClock = currClock;
                }
            }

            if (instantAutosave)
                instantAutosave = false;

            DWORD retVal = WaitForMultipleObjects(events.GetSize(), events.GetData(), FALSE,
                !paramNote.workspaceAutosaveInterval ? 1000 : paramNote.workspaceAutosaveInterval * 1000);

            if (retVal == WAIT_OBJECT_0 || retVal == WAIT_FAILED) // shutdown
            {
                break;
            }
            else if (retVal == WAIT_OBJECT_0 + 1) // autosave requested
            {
                instantAutosave = true;
                m_eventInstantAutosave.ResetEvent();
            }

        } // while (1)
    }
    catch (std::exception& x)
    {
        error = x.what();
    }
    catch (CException* x)
    {
        CString text;
        x->GetErrorMessage(text.GetBuffer(128), 128);
        text.ReleaseBuffer();
        error = text;
    }
    catch (...)
    {
        error = "Unknown error";
    }

    if (!error.IsEmpty())
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((L"Workspace Backup thread failed with error: \n\n"
            + error + L"\n\nAutosave will be disabled!"), MB_ICONERROR | MB_OK); 
    }

    return 0;
}

    UINT OEWorkspaceManager::m_workspaceClipboardFormat = 0;

UINT OEWorkspaceManager::GetWorkspaceClipboardFormat ()
{
    if (!m_workspaceClipboardFormat)
        m_workspaceClipboardFormat = RegisterClipboardFormat(L"OpenEditor.Workspace");

    return m_workspaceClipboardFormat;
}

    static const char master_signature[] = "{OpenEditor.Workspace.V1}";

    enum record_type { rt_description, rt_data, rt_eof };

    struct record
    {
        char signature[sizeof(master_signature)-1];
        char file_type  [5]; 
        char spacer_1      ;
        char file_length[8];
        char spacer_2      ;

        record () 
        { 
            memset(this, 0, sizeof(*this)); 
        }
        
        record (unsigned int length, record_type rt) 
        { 
            memset(this, ' ', sizeof(*this)); 

            spacer_1 = '{';

            char buffer[sizeof(file_length)+1];
            sprintf_s(buffer, sizeof(buffer), "%08X", length);
            memcpy(file_length, buffer, sizeof(file_length)); 

            spacer_2 = '}';

            memcpy(signature, master_signature, sizeof(master_signature)-1); 

            switch (rt)
            {
            case rt_description:
                memcpy(&file_type, "{DSC}", sizeof(file_type)); 
                break;
            case rt_data:
                memcpy(&file_type, "{DAT}", sizeof(file_type)); 
                break;
            case rt_eof:
                memcpy(&file_type, "{EOF}", sizeof(file_type)); 
                break;
            }
        }

        operator int () const { return memcmp(&signature, master_signature, sizeof(master_signature)-1) == 0; }

        bool is_desciption () const { return memcmp(&file_type, "{DSC}", sizeof("{DSC}")-1) == 0; }
        bool is_data () const { return memcmp(&file_type, "{DAT}", sizeof("{DAT}")-1) == 0; }
        bool is_eof () const { return memcmp(&file_type, "{EOF}", sizeof("{EOF}")-1) == 0; }

        unsigned int get_file_length ()
        {
            return (unsigned int)strtol(file_length, NULL, 16);
        }
    };

int OEWorkspaceManager::AskToCloseAllDocuments ()
{
    if (int dcount = m_pDocManager->GetDocumentCount())
    {
        if (dcount == 1)
        {
            POSITION pos = m_pDocTemplate->GetFirstDocPosition();
            if (pos != NULL)
            {
                CDocument* pDoc = m_pDocTemplate->GetNextDoc(pos);
                if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
                {
                    unsigned int action = pOEDoc->GetStorage().GetActionId().ToUInt();
                    if (action == Sequence::START_VAL) // ignore single empty doc
                    {
                        pOEDoc->OnCloseDocument();
                        return 0;
                    }
                }
            }
        }

        if (AfxMessageBox(L"Would you like to close all open files?", MB_YESNO|MB_ICONQUESTION) == IDYES)
            m_pDocManager->SaveAndCloseAllDocuments();
    }

    return m_pDocManager->GetDocumentCount();
}

    static
    void SetAttribute (TiXmlElement* elem, const char* attr, unsigned __int64 value)
    {
        char buff[80];
        _i64toa(value, buff, 10);
        elem->SetAttribute(attr, buff);
    }

    CString make_relative_path (const CString& base, const CString& path)
    {
        TCHAR result[MAX_PATH];
        if (::PathIsSameRoot(base, path)
        && ::PathRelativePathTo(result, base, FILE_ATTRIBUTE_DIRECTORY, path, FILE_ATTRIBUTE_NORMAL))
            return result;

        return path;
    }

#include "COMMON/WorkbookMDIFrame.h" // for saving documents in order of wookbook tabs

void OEWorkspaceManager::BuildDocumentList (std::vector<COEDocument*>& docs)
{
    docs.clear();
    // getting the list of documents in the order of creation
    POSITION pos = m_pDocTemplate->GetFirstDocPosition();
    while (pos != NULL)
    {
        CDocument* pDoc = m_pDocTemplate->GetNextDoc(pos);
        if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
            docs.push_back(pOEDoc);
    }

    try // building the list in display order
    {
        std::vector<COEDocument*> orderedDocs;
        if (CWnd* pWnd = AfxGetMainWnd())
            if (CWorkbookMDIFrame* pFrame = DYNAMIC_DOWNCAST(CWorkbookMDIFrame, pWnd))
            {
                std::vector<CMDIChildWnd*> children;
                pFrame->GetOrderedChildren(children);

                for (std::vector<CMDIChildWnd*>::iterator it = children.begin(); it != children.end(); ++it)
                {
                    CDocument * pDoc = (*it)->GetActiveDocument();
                    if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
                    {
                        if (std::find(docs.begin(), docs.end(), pOEDoc) != docs.end()
                        && (std::find(orderedDocs.begin(), orderedDocs.end(), pOEDoc) == orderedDocs.end()))
                            orderedDocs.push_back(pOEDoc);
                    }
                }
            }

        if (docs.size() == orderedDocs.size()) // basic check if the ordered result is good
            std::swap(docs, orderedDocs);
    }
    catch (...) {} // ingnore, if we have non-ordered documents, it's still good for backup
}

void OEWorkspaceManager::DoSave (CFile& file, bool use_relative, const std::wstring& filepath)
{
    CWaitCursor wait;

    CString base;
    if (!filepath.empty())
        if (LPCTSTR fname = ::PathFindFileName(filepath.c_str()))
            base = CString(filepath.c_str(), fname - filepath.c_str());

    int workspaceAutosaveFileLimit = COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceAutosaveFileLimit();

    CDocument* pActiveDoc = 0;

    if (CWnd* pWnd = AfxGetMainWnd())
        if (CMDIFrameWnd* pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, pWnd))
            if (CMDIChildWnd* pMDIChild = pFrame->MDIGetActive())
                pActiveDoc = pMDIChild->GetActiveDocument();

    std::vector<COEDocument*> docs;
    BuildDocumentList(docs);
    for (std::vector<COEDocument*>::iterator it = docs.begin(); it != docs.end(); ++it)
    {
        if (COEDocument* pOEDoc = *it)
        {
            TiXmlDocument doc;
            doc.LinkEndChild(new TiXmlDeclaration("1,0","0","0"));

            TiXmlElement* fileEl = new TiXmlElement("File");
            fileEl->SetAttribute("title", Common::str(pOEDoc->GetOrgTitle()));
            
            CString path = pOEDoc->GetPathName();
            if (use_relative && !base.IsEmpty() && !path.IsEmpty())
                path = make_relative_path(base, path);
            fileEl->SetAttribute("path", Common::str(path));

            bool modified = pOEDoc->IsModified();
            fileEl->SetAttribute("modified", modified ? 1 : 0);
            
            if (modified && !path.IsEmpty())
            {
                __int64 fileTime, fileSize;
                pOEDoc->GetOrgFileTimeAndSize(fileTime, fileSize);
                SetAttribute(fileEl, "file_time", fileTime);
                SetAttribute(fileEl, "file_size", fileSize);
            }

            if (pOEDoc == pActiveDoc)
                fileEl->SetAttribute("active", 1);

            if (it == docs.begin())
                fileEl->SetAttribute("untitled_count", m_pDocTemplate->GetUntitledCount());

            {
                TiXmlElement* encodingEl = new TiXmlElement("Encoding");
                pOEDoc->SaveEncoding(*encodingEl);
                fileEl->LinkEndChild(encodingEl);
            }

            {
                std::vector<int> bookmarks;
                pOEDoc->GetStorage().GetBookmarkedLines(bookmarks, eBmkGroup1);
                if (!bookmarks.empty())
                {
                    TiXmlElement* bmksEl = new TiXmlElement("Bookmarks");

                    std::vector<int>::const_iterator it2 = bookmarks.begin();
                    for (; it2 != bookmarks.end(); ++it2)
                    {
                        TiXmlElement* bmkEl = new TiXmlElement("Bookmark");
                        bmkEl->SetAttribute("line", *it2);
                        bmksEl->LinkEndChild(bmkEl);
                    }

                    fileEl->LinkEndChild(bmksEl);
                }
            }

            {
                bool empty = true;
                const RandomBookmarkArray& bookmarks = pOEDoc->GetStorage().GetRandomBookmarks();

                TiXmlElement* bmksEl = new TiXmlElement("RandomBookmark");

                for (unsigned char i = RandomBookmark::FIRST; i < RandomBookmark::LAST; ++i)
                {
                    int line = bookmarks.GetLine(RandomBookmark(i));
                    if (line != -1)
                    {
                        TiXmlElement* bmkEl = new TiXmlElement("Bookmark");
                        bmkEl->SetAttribute("id",   (int)i);
                        bmkEl->SetAttribute("line", line);
                        bmksEl->LinkEndChild(bmkEl);
                        empty = false;
                    }
                }

                if (!empty)
                    fileEl->LinkEndChild(bmksEl);
                else
                    delete bmksEl;
            }

            POSITION vpos = pOEDoc->GetFirstViewPosition();
            CView* pView = pOEDoc->GetNextView(vpos);
            if (COEditorView* pOEView = DYNAMIC_DOWNCAST(COEditorView, pView))
            {
                OpenEditor::Position pos = pOEView->GetPosition();
                int topmostLine = pOEView->GetTopmostLine();
                int topmostColumn = pOEView->GetTopmostColumn();

                int blockMode = (int)pOEView->GetBlockMode();
                int isAltColumnar = pOEView->IsAltColumnarMode() ? 1 : 0; 
                
                OpenEditor::Square selection;
                pOEView->GetSelection(selection);

                TiXmlElement* viewEl = new TiXmlElement("View");

                viewEl->SetAttribute("pos_line", pos.line);
                viewEl->SetAttribute("pos_column", pos.column);
                viewEl->SetAttribute("topmost_line", topmostLine);
                viewEl->SetAttribute("topmost_column", topmostColumn);

                viewEl->SetAttribute("block_mode", blockMode);
                viewEl->SetAttribute("alt_columnar", isAltColumnar);

                if (!selection.is_empty())
                {
                    viewEl->SetAttribute("sel_start_line"  , selection.start.line  );
                    viewEl->SetAttribute("sel_start_column", selection.start.column);
                    viewEl->SetAttribute("sel_end_line"    , selection.end.line    );
                    viewEl->SetAttribute("sel_end_column"  , selection.end.column  );
                }

                fileEl->LinkEndChild(viewEl);
            }
            
            doc.LinkEndChild(fileEl);

            {
                std::ostringstream out;
                out << doc;
                string buff;
                buff.swap(out.str());

                file.Write(&record(buff.length(), rt_description), sizeof(record));
                file.Write(buff.c_str(), buff.length());
            }

            if (path.IsEmpty() || modified)
            {
                // let's skip files bigger than workspaceAutosaveFileLimit Mb
                unsigned long length = pOEDoc->GetStorage().GetTextLengthA();
                if (length <= (unsigned)workspaceAutosaveFileLimit * 1024 * 1024)
                {
                    std::unique_ptr<char[]> buffer(new char[length]);
                    length = pOEDoc->GetStorage().GetTextA(buffer.get(), length);

                    file.Write(&record(length, rt_data), sizeof(record));
                    file.Write(buffer.get(), length);
                }
            }
        }
    }

    file.Write(&record(0, rt_eof), sizeof(record));
}

void OEWorkspaceManager::DoOpen (CFile& file, const std::wstring& filepath)
{
    CWaitCursor wait;

    CString base;
    if (!filepath.empty())
        if (LPCTSTR fname = ::PathFindFileName(filepath.c_str()))
            base = CString(filepath.c_str(), fname - filepath.c_str());

    record rec;
    int bytes = file.Read(&rec, sizeof(rec));

    if (bytes && bytes != sizeof(rec))
        THROW_APP_EXCEPTION("OpenWorkspace: Unexpected end of file!");

    if (!rec)
        THROW_APP_EXCEPTION("OpenWorkspace: read error!");

    int untitledCount = 0;
    CDocument* pActiveDocument = 0;
    bool detectChanges = COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceDetectChangesOnOpen();

    while (bytes && rec && !rec.is_eof())
    {
        unsigned int desc_length = rec.get_file_length();
        std::unique_ptr<char[]> desc_buffer(new char[desc_length + 1]);

        if (desc_length != file.Read(desc_buffer.get(), desc_length))
            THROW_APP_EXCEPTION("OpenWorkspace: Unexpected end of file!");

        desc_buffer.get()[desc_length] = 0;

        TiXmlDocument doc;
        doc.Parse(desc_buffer.get());
        if (doc.Error())
            THROW_APP_EXCEPTION("OpenWorkspace: xml record parsing error!");

        OpenEditor::Position pos = { 0, 0 };
        int topmostLine = 0, topmostColumn = 0;
        int blockMode = ebtUndefined;
        int isAltColumnar = 0; 
        OpenEditor::Square selection = { 0, 0, 0, 0 };

        if (const TiXmlElement* fileEl = doc.FirstChildElement("File"))
        {
            bytes = file.Read(&rec, sizeof(rec));
            if (bytes != sizeof(rec))
                THROW_APP_EXCEPTION("OpenWorkspace: Unexpected end of file!");
            if (!rec)
                THROW_APP_EXCEPTION("OpenWorkspace: read error!");

            std::wstring path;
            
            try
            {
                path = Common::wstr(fileEl->Attribute("path"));
            }
            catch (std::range_error&)
            {
                path = Common::wstr_from_ansi(fileEl->Attribute("path"));
            }

            if (!path.empty() && ::PathIsRelative(path.c_str()))
            {
                TCHAR buff[_MAX_PATH];
                if (::PathCombine(buff, base, path.c_str()))
                    path = buff;
            }
            
            if (CDocument* pDoc = m_pDocTemplate->OpenWorkspaceFile(!path.empty() ? path.c_str() : NULL))
            {
                if (path.empty())
                {
                    wstring title;
                    try
                    {
                        title = Common::wstr(fileEl->Attribute("title"));
                    }
                    catch (std::range_error&)
                    {
                        title = Common::wstr_from_ansi(fileEl->Attribute("title"));
                    }
                    pDoc->SetTitle(title.c_str());
                }
            
                int modified = 0;
                fileEl->Attribute("modified", &modified);

                int active = 0;
                fileEl->Attribute("active", &active);
                if (active) 
                    pActiveDocument = pDoc;

                fileEl->Attribute("untitled_count", &untitledCount);

                if (const TiXmlElement* viewEl = fileEl->FirstChildElement("View"))
                {
                    viewEl->Attribute("pos_line", &pos.line);
                    viewEl->Attribute("pos_column", &pos.column);
                    viewEl->Attribute("topmost_line", &topmostLine);
                    viewEl->Attribute("topmost_column", &topmostColumn);
                    viewEl->Attribute("block_mode", &blockMode);
                    viewEl->Attribute("alt_columnar", &isAltColumnar);

                    viewEl->Attribute("sel_start_line"  , &selection.start.line  );
                    viewEl->Attribute("sel_start_column", &selection.start.column);
                    viewEl->Attribute("sel_end_line"    , &selection.end.line    );
                    viewEl->Attribute("sel_end_column"  , &selection.end.column  );
                }

                std::vector<int> bookmarks;
                if (const TiXmlElement* bookmarksEl = fileEl->FirstChildElement("Bookmarks"))
                {
                    if (const TiXmlElement* bookmarkEl = bookmarksEl->FirstChildElement("Bookmark"))
                    {
                        while (bookmarkEl)
                        {
                            int line = -1;
                            if (bookmarkEl->Attribute("line", &line) && line != -1)
                                bookmarks.push_back(line);

                            bookmarkEl = bookmarkEl->NextSiblingElement("Bookmark");
                        }
                    }
                }

                std::vector<pair<int,int>> randomBookmarks;
                if (const TiXmlElement* bookmarksEl = fileEl->FirstChildElement("RandomBookmark"))
                {
                    if (const TiXmlElement* bookmarkEl = bookmarksEl->FirstChildElement("Bookmark"))
                    {
                        while (bookmarkEl)
                        {
                            int id = -1, line = -1;
                            if (bookmarkEl->Attribute("id", &id) && id != -1
                            && bookmarkEl->Attribute("line", &line) && line != -1)
                                randomBookmarks.push_back(std::make_pair(id, line));

                            bookmarkEl = bookmarkEl->NextSiblingElement("Bookmark");
                        }
                    }
                }

                if (rec.is_data() && rec.get_file_length())
                {
                    unsigned int file_length = rec.get_file_length();
                    std::unique_ptr<char[]> data_buffer(new char[file_length]);

                    if (file_length != file.Read(data_buffer.get(), file_length))
                        THROW_APP_EXCEPTION("OpenWorkspace: Unexpected end of file!");

                    if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
                    {
                        bool skipModified = false;

                        if (!path.empty())
                        {
                            __int64 fileTime = 0, fileSize = 0;
                            if (const char* buff = fileEl->Attribute("file_time"))
                                fileTime = _atoi64(buff);
                            if (const char* buff = fileEl->Attribute("file_size"))
                                fileSize = _atoi64(buff);
                            
                            __int64 currFileTime = 0, currFileSize = 0;
                            CFileWatch::GetFileTimeAndSize(pOEDoc, currFileTime, currFileSize);

                            if (detectChanges
                            && (fileTime != currFileTime || fileSize != currFileSize))
                            {
                                std::wstring prompt = L"The file " + path + L" has been changed since the last workspace saving."
                                        L"\n\nDo you want to reload it and lose the changes made in the editor?";

                                if (AfxMessageBox(prompt.c_str(), MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                                    skipModified = true;
                            }

                            if (!skipModified)
                                pOEDoc->SetOrgFileTimeAndSize(fileTime, fileSize);
                        }

                        if (!skipModified)
                        {
                            if (const TiXmlElement* encodingEl = fileEl->FirstChildElement("Encoding"))
                                pOEDoc->RestoreEncoding(*encodingEl);

                            pOEDoc->SetText(data_buffer.get(), file_length);

                            if (modified)
                                pOEDoc->SetModifiedFlag(TRUE);
                        }
                    }
                }

                if (COEDocument* pOEDoc = DYNAMIC_DOWNCAST(COEDocument, pDoc))
                {
                    int nlines = pOEDoc->GetStorage().GetLineCount();
                    {
                        vector<int>::const_iterator it = bookmarks.begin();
                        for (; it != bookmarks.end(); ++it)
                            if (*it < nlines) // 2018-12-09 bug fix, cannot open a workspace with a bookmark beyond eof
                                pOEDoc->GetStorage().SetBookmark(*it, eBmkGroup1, true);
                    }
                    {
                        vector<pair<int,int>>::const_iterator it = randomBookmarks.begin();
                        for (; it != randomBookmarks.end(); ++it)
                            if (it->second < nlines) // 2018-12-09 bug fix, cannot open a workspace with a bookmark beyond eof
                                pOEDoc->GetStorage().SetRandomBookmark(RandomBookmark((unsigned char)it->first), it->second, true);
                    }
                }

                {                                                                                                                             
                    POSITION vpos = pDoc->GetFirstViewPosition();
                    CView* pView = pDoc->GetNextView(vpos);
                    if (COEditorView* pOEView = DYNAMIC_DOWNCAST(COEditorView, pView))
                    {
                        if (blockMode != ebtUndefined)
                            pOEView->SetBlockMode(blockMode == ebtColumn ? ebtColumn : ebtStream, 
                                blockMode == ebtColumn ? (isAltColumnar ? true : false) : false);

                        pOEView->MoveTo(pos, true);

                        if (!selection.is_empty())
                            pOEView->SetSelection(selection);

                        pOEView->SetTopmostLine(topmostLine);
                        pOEView->SetTopmostColumn(topmostColumn);
                    }
                }
            }
        }

        if (rec && rec.is_data())
            bytes = file.Read(&rec, sizeof(rec));
        if (bytes && bytes != sizeof(rec))
            THROW_APP_EXCEPTION("OpenWorkspace: Unexpected end of file!");
    }

    if (untitledCount)
        m_pDocTemplate->SetUntitledCount(untitledCount);

    if (pActiveDocument)
    {
        POSITION vpos = pActiveDocument->GetFirstViewPosition();
        CView* pView = pActiveDocument->GetNextView(vpos);
        if (CFrameWnd* pFrame = pView->GetParentFrame()) 
        {
            if (pFrame->IsKindOf(RUNTIME_CLASS(CMDIChildWnd)))
                pFrame->GetParent()->PostMessage(WM_MDIACTIVATE, (WPARAM)pFrame->m_hWnd);
        }
    }
}

void OEWorkspaceManager::WorkspaceCopy ()
{
    CSharedFile mf(GMEM_MOVEABLE|GMEM_DDESHARE);
        
    DoSave(mf, false);

    if (::OpenClipboard(NULL))
    {
        ::EmptyClipboard();

        HGLOBAL hMem = mf.Detach();

        if (hMem)
            ::SetClipboardData(m_workspaceClipboardFormat, hMem);

        ::CloseClipboard();
    }
}

void OEWorkspaceManager::WorkspacePaste ()
{
    if (IsClipboardFormatAvailable(m_workspaceClipboardFormat))
    {
        if (OEWorkspaceManager::Get().HasActiveWorkspace())
            OEWorkspaceManager::Get().WorkspaceCloseActive();

        if (m_pDocManager->GetDocumentCount() > 0)
            AskToCloseAllDocuments();

        if (::OpenClipboard(NULL))
        {
            if (HGLOBAL hMem = ::GetClipboardData(m_workspaceClipboardFormat))
            {
                CMemFile mf((BYTE*)::GlobalLock(hMem), ::GlobalSize(hMem));
                try 
                {
                    if ((HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE)
                    {
                        m_workspaceFile.Close();
                        m_workspacePath.clear();
                        m_activeWorkspaceSavedState.clear();

                        if (m_onUpdateApplicationTitle)
                            m_onUpdateApplicationTitle();
                    }

                    DoOpen(mf, std::wstring());
                }
                catch (...)
                {
                    ::GlobalUnlock(hMem);
                    throw;
                }

                ::GlobalUnlock(hMem);
            }
        }
    }
}

CString OEWorkspaceManager::GetWorkspaceFilter () const
{
    CString filter;
    // append the "*.TXT" TXT files filter
    filter += L"Workspace Files (*"+ GetWorkspaceExtension() + L")";
    filter += (TCHAR)'\0';   // next string please
    filter += L"*" + GetWorkspaceExtension();
    filter += (TCHAR)'\0';   // last string
    // append the "*.*" all files filter
    filter += L"All Files (*.*)";
    filter += (TCHAR)'\0';   // next string please
    filter += _T("*.*");
    filter += (TCHAR)'\0';   // last string
    filter += '\0'; // close
    return filter;
}

CString OEWorkspaceManager::GetSnapshotFilter () const
{
    CString filter;
    // append the "*.TXT" TXT files filter
    filter += L"AS/QS Snapshot Files (*"+ GetSnapshotExtension() + L")";
    filter += (TCHAR)'\0';   // next string please
    filter += L"*" + GetSnapshotExtension();
    filter += (TCHAR)'\0';   // last string
    // append the "*.TXT" TXT files filter
    filter += L"Workspace Files (*"+ GetWorkspaceExtension() + L")";
    filter += (TCHAR)'\0';   // next string please
    filter += L"*" + GetWorkspaceExtension();
    filter += (TCHAR)'\0';   // last string
    // append the "*.*" all files filter
    filter += L"All Files (*.*)";
    filter += (TCHAR)'\0';   // next string please
    filter += _T("*.*");
    filter += (TCHAR)'\0';   // last string
    filter += '\0'; // close
    return filter;
}

CString OEWorkspaceManager::GetWorkspaceExtension () const
{
    return m_workspaceExtension;
}

CString OEWorkspaceManager::GetSnapshotExtension () const
{
    return m_snapshotExtension;
}

CString OEWorkspaceManager::GetWorkspaceFilename (LPCTSTR format) const
{
    CString fname;
    time_t t = time((time_t*) 0);
    struct tm* tp = localtime(&t);
    wcsftime(fname.GetBuffer(80), 80, format, tp);
    fname.ReleaseBuffer();
    fname += GetWorkspaceExtension();
    return fname;
}

CString OEWorkspaceManager::GetBackupFilename () const
{
    //CString fname = m_sUuid.c_str();
    //fname += GetSnapshotExtension();

    CString fname;

    // if there s an active workspace then use its name as a prefix
    if ((HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE && !m_workspacePath.empty())
    {
        CString name;
        if (name = ::PathFindFileName(m_workspacePath.c_str()))
            if (LPWSTR ext = ::PathFindExtension(name.GetBuffer()))
            {
                if (*ext == '.') *ext = 0;
                name.ReleaseBuffer();

                CString buffer;
                create_unique_fname(buffer);
                fname = name + L"_" + buffer + GetSnapshotExtension();
            }
    }
    
    if (fname.IsEmpty())
    {
        CString buffer;
        create_unique_fname(buffer);
        fname = L"AS_" + buffer + GetSnapshotExtension();
    }

    return fname;
}

std::wstring OEWorkspaceManager::GetInstanceFolder () const
{
    return ConfigFilesLocator::GetBaseFolder() + L"\\Instances";
}

std::wstring OEWorkspaceManager::GetBackupFolder (bool* byDefault) const
{
    std::wstring backupFolder = Common::wstr(COEDocument::GetSettingsManager().GetGlobalSettings()->GetFileBackupDirectoryV2());

    if (backupFolder.empty())
    {
        backupFolder = ConfigFilesLocator::GetBaseFolder() + L"\\Backup";
        if (byDefault) *byDefault = true;
    }
    else
    {
        if (byDefault) *byDefault = false;
    }

    return backupFolder;
}

    static
    std::wstring get_backup_fname (const std::wstring& fname)
    {
        std::wstring backupFname;

        GlobalSettingsPtr settings = COEDocument::GetSettingsManager().GetGlobalSettings();
        EBackupMode backupMode = (EBackupMode)settings->GetFileBackupV2();

        if (backupMode != ebmNone && !fname.empty())
        {
            std::wstring buffer;

            switch (backupMode)
            {
            case ebmCurrentDirectory:
                {
                    LPTSTR ext = PathFindExtension(fname.c_str());
                    if ((unsigned)(ext - fname.c_str()) < fname.size())
                    {
                        buffer = fname;
                        buffer.insert(ext - fname.c_str(), L".prev");
                        backupFname = buffer;
                    }
                }
                break;
            case ebmBackupDirectory:
                {
                    buffer = Common::wstr(settings->GetFileBackupDirectoryV2());

                    if (buffer.empty())
                        buffer = ConfigFilesLocator::GetBaseFolder() + L"\\Backup";

                    if (!::PathFileExists(buffer.c_str()))
                        AppCreateFolderHierarchy(buffer.c_str());

                    if (buffer.rbegin() != buffer.rend() && *buffer.rbegin() != '\\')
                        buffer += '\\';

                    buffer += ::PathFindFileName(fname.c_str());
                    backupFname = buffer;
                }
                break;
            }
        }

        return backupFname;
    }

    static
    void backup_file (const std::wstring& fname)
    {
        std::wstring backupFname = get_backup_fname(fname);
        if (!fname.empty() && !backupFname.empty() && backupFname != fname) // FRM2
        {
            if (::CopyFile(fname.c_str(), backupFname.c_str(), FALSE) == 0) 
            {
                //TODO#2: offer ignore backup error while saving a file
                // get from the system information about error
                std::wstring err_desc;
                OsException::GetLastError(err_desc);
                std::wstring error = L"Cannot create backup file: \"" + backupFname + L"\",\n\n"+ err_desc;
                THROW_APP_EXCEPTION(error);
            }
        }
    }


void OEWorkspaceManager::WorkspaceSave (bool saveAs)
{
    BOOL overwrite = TRUE;

    if (saveAs || m_workspacePath.empty())
    {
        overwrite = FALSE;

        CFileDialog dlgFile(FALSE, NULL, NULL,
            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING,
            NULL, NULL, 0);

        CString path;
        
        if (!m_workspacePath.empty())
            path = m_workspacePath.c_str(); 
        else 
            path = (m_lastFolder.empty()) ? GetWorkspaceFilename(L"%Y-%m-%d_%H-%M") : (m_lastFolder + (LPCTSTR)GetWorkspaceFilename(L"%Y-%m-%d_%H-%M")).c_str(); 

        CString filter = GetWorkspaceFilter();
        dlgFile.m_ofn.lpstrFilter = filter;
        dlgFile.m_ofn.lpstrTitle = L"Save Workspace";
        dlgFile.m_ofn.nMaxFile   = _MAX_PATH;
        dlgFile.m_ofn.lpstrFile  = path.GetBuffer(dlgFile.m_ofn.nMaxFile);

        if (dlgFile.DoModal() == IDOK)
        {
            path.ReleaseBuffer();
            LPCTSTR fname = PathFindFileName((LPTSTR)(LPCTSTR)path);
            if (fname != (LPCTSTR)path)
                m_lastFolder = std::wstring((LPCTSTR)path, fname - (LPCTSTR)path); 

            LPCTSTR fileExt = PathFindExtension(path);
            if (!fileExt || !_tcslen(fileExt))
                path += GetWorkspaceExtension();

            // On SaveAs make sure it is not the same path
            if (saveAs && (HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE && !m_workspacePath.empty())
            {
                TCHAR buffer1[MAX_PATH], buffer2[MAX_PATH];
                memset(buffer1, 0, sizeof(buffer1));
                memset(buffer2, 0, sizeof(buffer2));
                PathCanonicalize(buffer1, m_workspacePath.c_str());
                PathCanonicalize(buffer2, path);
                overwrite = !wcsncmp(buffer1, buffer2, sizeof(buffer1)/sizeof(buffer1[0]));
            }

            if (!overwrite)
            {
                if ((HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE)
                    m_workspaceFile.Close();

                CFileException exception;
                if (!m_workspaceFile.Open(path, CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite, &exception))
                    CFileException::ThrowOsError(exception.m_lOsError, path);
                
                DoSave(m_workspaceFile, COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceUseRelativePath(), (LPCTSTR)path);
                m_workspacePath = path;

                if (m_onUpdateApplicationTitle)
                    m_onUpdateApplicationTitle();

                if (m_pRecentFileList.get())
                    m_pRecentFileList->OnSaveWorkspace(m_workspacePath.c_str());

                m_pDocTemplate->GetWorkspaceState(m_activeWorkspaceSavedState);
            }
        }
    }

    if (overwrite)
    {
        backup_file(m_workspacePath); // TODO#2: backup any existing file

        m_workspaceFile.SeekToBegin();
        m_workspaceFile.SetLength(0);
        DoSave(m_workspaceFile, COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceUseRelativePath(), m_workspacePath);
        FlushFileBuffers(m_workspaceFile);

        if (m_onUpdateApplicationTitle)
            m_onUpdateApplicationTitle();

        if (m_pRecentFileList.get()) // not sure if it is needed here
            m_pRecentFileList->OnSaveWorkspace(m_workspacePath.c_str());

        m_pDocTemplate->GetWorkspaceState(m_activeWorkspaceSavedState);
    }
}

void OEWorkspaceManager::WorkspaceSaveQuick ()
{
    CString fname, path;
    create_unique_fname(fname);
    fname = L"QS-" + fname;
    fname += GetSnapshotExtension();
    ::PathCombine(path.GetBuffer(MAX_PATH+1), GetBackupFolder().c_str(), fname);

    CFile file;
    file.Open(path, CFile::modeCreate | CFile::modeWrite); 
    DoSave(file, false);
    file.Close();

    m_pDocTemplate->GetWorkspaceState(m_activeWorkspaceSavedState);

    m_pFavoritesList->AddWorkspaceQuickSave(Common::str(path));
}

void OEWorkspaceManager::WorkspaceOpen ()
{
    CFileDialog dlgFile(TRUE, NULL, NULL,
        OFN_FILEMUSTEXIST | OFN_ENABLESIZING,
        NULL, NULL, 0);

    CString path = m_lastFolder.c_str(), filter = GetWorkspaceFilter();

    dlgFile.m_ofn.lpstrFilter = filter;
    dlgFile.m_ofn.lpstrTitle  = L"Open Workspace";
    dlgFile.m_ofn.nMaxFile    = _MAX_PATH;
    dlgFile.m_ofn.lpstrFile   = path.GetBuffer(dlgFile.m_ofn.nMaxFile);

    if (dlgFile.DoModal() == IDOK)
    {
        path.ReleaseBuffer();
        WorkspaceOpen((LPCTSTR)path, false);
    }
}
    
void OEWorkspaceManager::WorkspaceOpenAutosaved ()
{
    std::wstring backupFolder = Common::wstr(COEDocument::GetSettingsManager().GetGlobalSettings()->GetFileBackupDirectoryV2());

    if (backupFolder.empty())
        backupFolder = ConfigFilesLocator::GetBaseFolder() + L"\\Backup\\";
    else
    {
        if (*backupFolder.rbegin() != '\\')
            backupFolder += '\\';
    }

    CFileDialog dlgFile(TRUE, NULL, NULL,
        OFN_FILEMUSTEXIST | OFN_ENABLESIZING | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT,
        NULL, NULL, 0);

    CString path = backupFolder.c_str(), filter = GetSnapshotFilter();

    dlgFile.m_ofn.lpstrFilter = filter;
    dlgFile.m_ofn.lpstrTitle  = L"Open AS/QS Snapshot";
    dlgFile.m_ofn.nMaxFile    = _MAX_PATH;
    dlgFile.m_ofn.lpstrFile   = path.GetBuffer(dlgFile.m_ofn.nMaxFile);

    if (dlgFile.DoModal() == IDOK)
    {
        path.ReleaseBuffer();
        WorkspaceOpen(path, true);
    }
}

void OEWorkspaceManager::WorkspaceOpen (LPCTSTR lpszPath, bool _snapshot)
{
    bool doNotClose = COEDocument::GetSettingsManager().GetGlobalSettings()->GetWorkspaceDoNotCloseFilesOnOpen();
    bool snapshot = _snapshot || doNotClose;

    if (HasActiveWorkspace())
        WorkspaceCloseActive();

    std::wstring workspacePath = lpszPath;

    if (!workspacePath.empty())
    {
        bool cleanStart = (!doNotClose && AskToCloseAllDocuments() == 0) ? true : false;

        if ((HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE)
        {
            m_workspaceFile.Close();
            m_workspacePath.clear();
            m_activeWorkspaceSavedState.clear();

            if (m_onUpdateApplicationTitle)
                m_onUpdateApplicationTitle();
        }

        if (snapshot)
        {
            CFile file;
            file.Open(workspacePath.c_str(), CFile::modeRead); 
            DoOpen(file, workspacePath);
            file.Close();
        }
        else
        {
            CFileException exception;
            if (m_workspaceFile.Open(workspacePath.c_str(), CFile::modeReadWrite|CFile::shareDenyWrite, &exception))
            {
                DoOpen(m_workspaceFile, workspacePath);
                m_workspacePath = workspacePath;

                if (m_onUpdateApplicationTitle)
                    m_onUpdateApplicationTitle();
            }
            else
            {
                if (exception.m_lOsError != ERROR_SHARING_VIOLATION)
                    CFileException::ThrowOsError(exception.m_lOsError, workspacePath.c_str());

                if (AfxMessageBox(L"The workspace is locked by another program!"
                    L"\n\nWould you like to open the workspace without making it active?", MB_ICONEXCLAMATION|MB_YESNO) == IDYES
                    )
                {
                    CFile file;
                    CFileException exception2;
                    if (file.Open(workspacePath.c_str(), CFile::modeRead|CFile::shareDenyNone, &exception2))
                    {
                        DoOpen(file, workspacePath);
                        file.Close();
                    }
                    else
                    {
                        CFileException::ThrowOsError(exception2.m_lOsError, workspacePath.c_str());
                    }
                }
                else
                    AfxThrowUserException();
            }

            if (m_pRecentFileList.get())
                m_pRecentFileList->OnOpenWorkspace(workspacePath);

            if (cleanStart)
                m_pDocTemplate->GetWorkspaceState(m_activeWorkspaceSavedState);
            else
                m_activeWorkspaceSavedState.clear(); // the loaded workspace has already been modified

            LPCTSTR fname = PathFindFileName(workspacePath.c_str());
            if (int len = fname - workspacePath.c_str())
                m_lastFolder = workspacePath.substr(0, len); 
        }
    }
}

void OEWorkspaceManager::WorkspaceCloseActive ()
{
    if ((HANDLE)m_workspaceFile != INVALID_HANDLE_VALUE)
    {
        WorkspaceState lastState;
        m_pDocTemplate->GetWorkspaceState(lastState);

        if (!COEMultiDocTemplate::WorkspaceStateEqual(m_activeWorkspaceSavedState, lastState))
        {
            std::wstring name = ::PathFindFileName(m_workspacePath.c_str());
            LPTSTR ext = ::PathFindExtension(name.c_str());
            name.resize(ext - name.c_str());

            switch (AfxMessageBox((L"Would you like to save the workspace \"" + name + L"\" before closing it?"
                L"\n\nAll open files will be closed.").c_str(), MB_ICONQUESTION|MB_YESNOCANCEL)
            )
            {
            case IDCANCEL:
                AfxThrowUserException();
            case IDYES:
                WorkspaceSave(false);
                break;
            case IDNO:
                break;
            }
        }

        m_workspaceFile.Close();
        m_workspacePath.clear();
        m_activeWorkspaceSavedState.clear();

        if (m_onUpdateApplicationTitle)
            m_onUpdateApplicationTitle();

        m_pDocManager->CloseAllDocuments(FALSE);
    }
}

void OEWorkspaceManager::CheckAndAutosave (CFile& file)
{
    WorkspaceState lastState;
    m_pDocTemplate->GetWorkspaceState(lastState);

    if (!COEMultiDocTemplate::WorkspaceStateEqual(m_lastBackupState, lastState, true/*ignoreClosed*/))
    {
        DoSave(file, false);
        m_lastBackupState.swap(lastState);
    }
}
