//////////////////////////////////////////////////////////////////////
//
// Probably it's better using ReadDirectoryChangesW, see FWATCH.C
//
// FileWatch.cpp: implementation of the CFileWatch class.
// These classes are based on Herbert Griebel code (herbertgriebel@yahoo.com)
// His code has been changed but the idea hasn't
//////////////////////////////////////////////////////////////////////

// 26.08.2002 bug fix, unnecessary reloading outside-updated files
// 23.12.2002 bug fix, detect file changing does not work
//            it still does not work properly on Win95/Win98 
//            but I'm not going to fix it

#include "stdafx.h"
#include "FileWatch.h"
#include <COMMON/SEException.h>
#include <COMMON/AppUtilities.h>
#include <ThreadCommunication\MessageOnlyWindow.h>
using namespace ThreadCommunication;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static
bool getFileAttrs (const std::wstring& fileName, __int64 &lastWriteTime, __int64 &fileSize)
{
    DWORD attrs;
    __int64 creationTime;
    return Common::AppGetFileAttrs(fileName.c_str(), &attrs, &fileSize, &creationTime, &lastWriteTime);
}

static
std::wstring getFileFolder (const std::wstring& fileName)
{
    TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
    _tsplitpath(fileName.c_str(), szDrive, szDir, NULL, NULL);
    return std::wstring(szDrive) + szDir;
}

static
bool isFolder (const std::wstring& lpszFileName)
{
    return !lpszFileName.empty() && *(lpszFileName.rbegin()) == '\\';
}

/////////////////////////////////////////////////////////////////////////////
// CFileWatchClient

CFileWatchClient::CFileWatchClient ()
{
    m_modified = false;
    m_nFileSize = m_lastWriteTime = 0;

    CFileWatch::m_csDataLock.Lock();
    CFileWatch::m_clients.insert(this);
    CFileWatch::m_csDataLock.Unlock();
}

CFileWatchClient::~CFileWatchClient ()
{
    try { EXCEPTION_FRAME;

        CFileWatch::RemoveFileToWatch(*this);
        CFileWatch::m_csDataLock.Lock();
        CFileWatch::m_clients.erase(this);
        CFileWatch::m_csDataLock.Unlock();
    }
    _DESTRUCTOR_HANDLER_;
}

void CFileWatchClient::StartWatching (LPCTSTR file)
{
    if (file && m_fileName != file)
    {
        CFileWatch::RemoveFileToWatch(*this);
        CFileWatch::m_csDataLock.Lock();
        m_fileName = file;
        CFileWatch::m_csDataLock.Unlock();
    }
    CFileWatch::AddFileToWatch(*this);
}

void CFileWatchClient::StopWatching ()
{
    CFileWatch::RemoveFileToWatch(*this);
}

void CFileWatchClient::UpdateWatchInfo ()
{
    getFileAttrs(m_fileName, m_lastWriteTime, m_nFileSize);
}

/////////////////////////////////////////////////////////////////////////////
// CFileWatch

set<CFileWatchClient*>   CFileWatch::m_clients;
map<std::wstring, HANDLE> CFileWatch::m_folders;
CCriticalSection	CFileWatch::m_csDataLock;
CEvent				CFileWatch::m_EventUpdate;
CEvent				CFileWatch::m_EventCheck;
bool				CFileWatch::m_bStopWatch = false;
UINT				CFileWatch::m_msgFileWatchNotify = WM_USER;
CWinThread*			CFileWatch::m_pThread = NULL;


void CFileWatch::NotifyClients ()
{
    //CFileWatch::m_csDataLock.Lock();

    std::set<CFileWatchClient*>::iterator 
        it = m_clients.begin(), end = m_clients.end();

    for (; it != end; ++it)
        if ((*it)->m_modified == true)
        {
            (*it)->OnWatchedFileChanged();
            (*it)->m_modified = false;
        }

    //CFileWatch::m_csDataLock.Unlock();
}

void CFileWatch::SuspendThread () 
{ 
    m_csDataLock.Lock();
    TRACE("FileWatch: SuspendThread\n");
    if (m_pThread && !m_bStopWatch) 
        m_pThread->SuspendThread(); 
    m_csDataLock.Unlock();
}

void CFileWatch::ResumeThread () 
{ 
    m_csDataLock.Lock();
    TRACE("FileWatch: ResumeThread\n");
    if (m_pThread) 
    {
        m_EventCheck.SetEvent();
        m_pThread->ResumeThread(); 
    }
    m_csDataLock.Unlock();
}

void CFileWatch::GetFileTimeAndSize (CFileWatchClient* client, __int64& fileTime, __int64& fileSize)
{ 
    if (client)
    {
        m_csDataLock.Lock();
        fileTime = client->m_lastWriteTime; 
        fileSize = client->m_nFileSize; 
        m_csDataLock.Unlock();
    }
}

void CFileWatch::StartThread ()
{
    if (m_pThread == NULL)
    {
//		if (m_msgFileWatchNotify == 0)
//			m_msgFileWatchNotify = ::RegisterWindowMessage("Kochware.OpenEditor.FileWatchNotify");

        m_bStopWatch = false;
        m_EventUpdate.SetEvent();
        m_pThread = AfxBeginThread(Watch, NULL, THREAD_PRIORITY_LOWEST);
        TRACE("FileWatch: thread started\n");
    }
}

void CFileWatch::Stop ()
{
    if (m_pThread)
    {
        m_csDataLock.Lock();
        m_bStopWatch = true;
        m_EventUpdate.SetEvent();
        ResumeThread();
        m_csDataLock.Unlock();

        for (int i(0); m_pThread && i < 100; i++)
            Sleep(10);
    }
}

void CFileWatch::AddFileToWatch (CFileWatchClient& client)
{
    m_csDataLock.Lock();

    _ASSERTE(m_clients.find(&client) != m_clients.end());

    __int64 ftLastWriteTime = 0, nFileSize = 0;
    std::wstring sFolder = getFileFolder(client.m_fileName);

    if (isFolder(client.m_fileName) || getFileAttrs(client.m_fileName, ftLastWriteTime, nFileSize))
    {
        client.m_suspendWatch = false;
        TRACE("FileWatch: Add File = %s\n", client.m_fileName.c_str());
        TRACE("FileWatch: time=%I64d, size=%I64d\n", ftLastWriteTime, nFileSize);
        client.m_lastWriteTime = ftLastWriteTime;
        client.m_nFileSize = nFileSize;
        // new directory to watch?
        auto it = m_folders.find(sFolder);
        if (it == m_folders.end())
        {
            m_EventUpdate.SetEvent();


            OSVERSIONINFO osinfo;
            osinfo.dwOSVersionInfoSize = sizeof(osinfo);
            ::GetVersionEx(&osinfo);

            std::wstring folder = sFolder;

            // for Win95 compatibility
            if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
            && isFolder(folder) 
            && folder.length() && *folder.rbegin() == '\\') 
                folder.resize(folder.length()-1);

            HANDLE hChangeNotification 
                = FindFirstChangeNotification(folder.c_str(), FALSE, 
                    FILE_NOTIFY_CHANGE_FILE_NAME
                    |FILE_NOTIFY_CHANGE_LAST_WRITE
                    |FILE_NOTIFY_CHANGE_SIZE
                    );

            ASSERT(hChangeNotification != INVALID_HANDLE_VALUE);

            if (hChangeNotification != INVALID_HANDLE_VALUE)
                m_folders[sFolder] = hChangeNotification;
        }

        StartThread();
    }

    m_csDataLock.Unlock();
}

void CFileWatch::RemoveFileToWatch (CFileWatchClient& client)
{
    m_csDataLock.Lock();

    _ASSERTE(m_clients.find(&client) != m_clients.end());

    std::wstring sFolder = getFileFolder(client.m_fileName);

    std::set<CFileWatchClient*>::const_iterator it = m_clients.begin();
    for(int counter = 0; it != m_clients.end(); ++it)
        if (sFolder == getFileFolder((*it)->m_fileName))
            counter++;

    if (counter == 1)
    {
        m_EventUpdate.SetEvent();
        FindCloseChangeNotification(m_folders[sFolder]);
        m_folders.erase(sFolder);
    }
    
    client.m_suspendWatch = true;
    TRACE("FileWatch: Suspend watch, File = %s\n", client.m_fileName.c_str());

    m_csDataLock.Unlock();
    
    if (m_folders.empty())
        Stop();
}

    struct FileWatchNotifyClients_Note : Note 
    {
        FileWatchNotifyClients_Note () {}
        virtual void Deliver () { CFileWatch::NotifyClients(); }
    };

UINT CFileWatch::Watch (LPVOID)
{
    // I cannot use the application's exception handling 
    // because it hangs on dialog box - it must be fixed

    //Common::SEException::InstallSETranslator();
    //set_terminate(Common::terminate);

    //try { EXCEPTION_FRAME;

    CArray<HANDLE, const HANDLE&> arHandle;
    arHandle.Add(m_EventUpdate);
    arHandle.Add(m_EventCheck);

    for (;;)
    {
        BOOL changeDetected = FALSE;

        // wait for event or notification
        DWORD dwResult = WaitForMultipleObjects(arHandle.GetSize(), arHandle.GetData(), FALSE, INFINITE);
        m_csDataLock.Lock();
        TRACE("FileWatch: Notification\n");

        if (m_bStopWatch)
            break;

        int nObject = dwResult - WAIT_OBJECT_0;
        if (nObject == 0)
        {
            TRACE("FileWatch: Update\n");
            m_EventUpdate.ResetEvent();

            // refresh list
            arHandle.SetSize(2);

            auto it = m_folders.begin();
            for(; it != m_folders.end(); ++it)
                arHandle.Add(it->second);
        }
        else if (nObject>0 && nObject<arHandle.GetSize())
        {
            if (nObject == 1)
            {
                TRACE("FileWatch: Check on Activation\n");
                m_EventCheck.ResetEvent();
            }

            auto folderIt = m_folders.begin();
            for(; folderIt != m_folders.end(); ++folderIt)
            {
                if (nObject == 1 || folderIt->second == arHandle[nObject])
                {
                    TRACE("FileWatch: Notification Directory = %s\n", folderIt->first.c_str());

                    std::set<CFileWatchClient*>::const_iterator clientIt = m_clients.begin();
                    for(; clientIt != m_clients.end(); ++clientIt)
                    {
                        if (getFileFolder((*clientIt)->m_fileName) == folderIt->first)
                        {
                            TRACE("FileWatch: Folder File = %s\n", (*clientIt)->m_fileName.c_str());
                            __int64 ftLastWriteTime = 0, nFileSize = 0;
                            if (isFolder((*clientIt)->m_fileName) 
                            || (!((*clientIt)->m_suspendWatch)
                            && getFileAttrs((*clientIt)->m_fileName, ftLastWriteTime, nFileSize) 
                            && ((*clientIt)->m_lastWriteTime != ftLastWriteTime 
                                || ((*clientIt)->m_nFileSize!= nFileSize))))
                            {
                                TRACE("FileWatch: *Changed File = %s\n", (*clientIt)->m_fileName.c_str());
                                TRACE("FileWatch:   oldval time=%I64d, size=%I64d\n", (*clientIt)->m_lastWriteTime, (*clientIt)->m_nFileSize);
                                TRACE("FileWatch:   newval time=%I64d, size=%I64d\n", ftLastWriteTime, nFileSize);
                                (*clientIt)->m_modified = true;
                                changeDetected = TRUE;
                            }
                        }
                    }

                    // AfxGetApp()->PostThreadMessage(m_msgFileWatchNotify, 0, 0);
                    // PostThreadMessage is unreliable notification method so it was replaced with Send
                }
            }
            if (nObject != 1)
                FindNextChangeNotification(arHandle[nObject]);
        }
        m_csDataLock.Unlock();

        if (changeDetected)
            try {
                MessageOnlyWindow::GetWindow().Send(FileWatchNotifyClients_Note());
            } catch (...) {}
    }
    
    auto folderIt = m_folders.begin();
    for(; folderIt != m_folders.end(); ++folderIt)
        FindCloseChangeNotification(folderIt->second);
    m_folders.clear();
    m_pThread = NULL;
    m_csDataLock.Unlock();

    TRACE("FileWatch: Quit\n");
    //}
    //catch (CException* x)           { DEFAULT_HANDLER(x); }
    //catch (const std::exception& x) { DEFAULT_HANDLER(x); }
    //catch (...)                     { DEFAULT_HANDLER_ALL; }

    return 0;
}
