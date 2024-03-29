//////////////////////////////////////////////////////////////////////
// FileWatch.h: interface for the CFileWatch class.
// These classes are based on Herbert Griebel code (herbertgriebel@yahoo.com)
// His code has been changed but the idea hasn't
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif//_MSC_VER > 1000

#ifndef __FILEWATCH_H__
#define __FILEWATCH_H__

#include <afxmt.h>
#include <map>
#include <set>

    using std::map;
    using std::set;
    using std::string;

/////////////////////////////////////////////////////////////////////////////
// CFileWatch
//      assumption: clients are been created only in main thread

class CFileWatchClient
{
public:
    CFileWatchClient ();
    virtual ~CFileWatchClient ();

protected:
    void StartWatching (LPCTSTR = NULL);
    void StopWatching ();
    void UpdateWatchInfo ();

private:
    friend class CFileWatch;
    virtual void OnWatchedFileChanged () = 0;
    
    std::wstring m_fileName;
    __int64	m_lastWriteTime, m_nFileSize;
    bool m_modified;
    bool m_suspendWatch;
};

class CFileWatch
{
public:
    static void NotifyClients ();
    static void SuspendThread ();
    static void ResumeThread  ();
    static UINT m_msgFileWatchNotify;
    static void GetFileTimeAndSize (CFileWatchClient*, __int64& fileTime, __int64& fileSize);

private:
    friend CFileWatchClient;
    static void StartThread ();
    static UINT Watch (LPVOID);
public:
    static void Stop ();

private:
    static void AddFileToWatch (CFileWatchClient&);
    static void RemoveFileToWatch (CFileWatchClient&);

    static set<CFileWatchClient*> m_clients;
    static map<std::wstring, HANDLE> m_folders;

    static CCriticalSection	m_csDataLock;
    static CEvent			m_EventUpdate;
    static CEvent			m_EventCheck;
    static bool				m_bWatchClosed;
    static bool				m_bStopWatch;
    static CWinThread		*m_pThread;
};


#endif//__FILEWATCH_H__
