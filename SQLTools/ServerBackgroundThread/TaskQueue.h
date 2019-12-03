/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
#pragma once

#include <list>
#include "arg_shared.h"
#include "ThreadCommunication/Singleton.h"

namespace ServerBackgroundThread
{

class Task : private boost::noncopyable
{
public:
    Task (const char* name, void* owner) 
        : m_name(name), m_silent(false), 
        m_groupId(0), m_owner(owner)
                                            {}
    virtual ~Task ()                        {}

    const string& GetName () const          { return m_name; }
    const string& GetLastError () const     { return m_error; }
    bool  IsOk () const                     { return m_error.empty(); }
    bool  IsSilent () const                 { return m_silent; }
    int   GetGroupId () const               { return m_groupId; }
    void* GetOwner () const                 { return m_owner; } 
    
    void SetName  (const string& name)      { m_name = name; }
    void SetSilent (bool silent)            { m_silent = silent; }
    void SetError (const string& error)     { m_error = error; }
    void SetGroupId (int groupId)           { m_groupId = groupId; }
    void SetOwner (void* owner)             { m_owner = owner; } 

    virtual void DoInBackground     (OciConnect&) = 0;
    virtual void ReturnInForeground ()              {};
    virtual void ReturnErrorInForeground ()         {};
    virtual double GetExecutionTime () const        { return 0; }

protected:
    std::string m_name;
    std::string m_error;
    bool m_silent;
    int m_groupId;
    void* m_owner;
    void* m_superOwner;
};

typedef arg::counted_ptr<Task> TaskPtr;

class TaskQueue : private boost::noncopyable
{
public:
    void Shutdown ();
    void Push (TaskPtr);
    void PushInFront (TaskPtr);
    void PushGroup (std::list<TaskPtr>&);
    TaskPtr Pop ();

    void RemoveGroup (int);

    bool IsShuttedDown () const { return m_shutdown; }
    bool WaitForNewRequest (DWORD dwMilliseconds) const;
    int  GetSize () const;

    void ResetLast ();
    bool HasAnyByOwner (void* owner) const;

    // under consideration
    // FindByTag (void*)

private:
    template <class T, int instance> friend class ThreadCommunication::Singleton;

    TaskQueue () 
        : m_shutdown(false), 
        m_groupIdSeq(0),
        m_lastTaskOwner(0)
        {}

    ~TaskQueue () {}
    
    bool m_shutdown;
    CEvent m_newRequest;
    mutable CCriticalSection m_criticalSection;
    std::list<TaskPtr> m_queue; 
    int m_groupIdSeq;
    void* m_lastTaskOwner;
};

typedef ThreadCommunication::Singleton<TaskQueue, 1> BkgdRequestQueue;
typedef ThreadCommunication::Singleton<TaskQueue, 2> BkgdResultQueue;

#define  FrgdRequestQueue BkgdRequestQueue
#define  FrgdResultQueue  BkgdResultQueue

}; // namespace ServerBackgroundThread

