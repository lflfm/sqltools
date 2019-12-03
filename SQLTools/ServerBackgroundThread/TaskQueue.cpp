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
#include "stdafx.h"
#include <afxmt.h>
#include "TaskQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace ServerBackgroundThread
{

void TaskQueue::Shutdown ()
{
    m_shutdown = true;
    m_newRequest.SetEvent();
}

void TaskQueue::Push (TaskPtr ptr)
{
    if (!m_shutdown)
    {
        CSingleLock lk(&m_criticalSection, TRUE);

        m_queue.push_back(ptr);

        m_newRequest.SetEvent();
    }
}

void TaskQueue::PushInFront (TaskPtr ptr)
{
    if (!m_shutdown)
    {
        CSingleLock lk(&m_criticalSection, TRUE);

        m_queue.push_front(ptr);

        m_newRequest.SetEvent();
    }
}

void TaskQueue::PushGroup (std::list<TaskPtr>& group)
{
    if (!m_shutdown)
    {
        CSingleLock lk(&m_criticalSection, TRUE);

        int groupId = ++m_groupIdSeq;

        std::list<TaskPtr>::iterator it = group.begin();
        for (; it != group.end(); ++it)
        {
            (*it)->SetGroupId(groupId);
            m_queue.push_back(*it);
        }

        m_newRequest.SetEvent();
    }
}

TaskPtr TaskQueue::Pop ()
{
    CSingleLock lk(&m_criticalSection, TRUE);

    TaskPtr ptr(0);

    if (!m_queue.empty())
    {
        ptr = m_queue.front();
        m_queue.pop_front();

        if (ptr.get())
            m_lastTaskOwner = ptr->GetOwner();
    }

    if (m_queue.empty())
        m_newRequest.ResetEvent();

    return ptr;
}

void TaskQueue::RemoveGroup (int groupId)
{
    CSingleLock lk(&m_criticalSection, TRUE);

    std::list<TaskPtr> _queue;

    std::list<TaskPtr>::iterator it = m_queue.begin();
    for (; it != m_queue.end(); ++it)
        if ((*it)->GetGroupId() != groupId)
            _queue.push_back(*it);

    m_queue.swap(_queue);
}

bool TaskQueue::WaitForNewRequest (DWORD dwMilliseconds) const 
{ 
    return WaitForSingleObject(m_newRequest, dwMilliseconds) == WAIT_OBJECT_0; 
}

int TaskQueue::GetSize () const
{
    CSingleLock lk(&m_criticalSection, TRUE);
    return m_queue.size();
}

void TaskQueue::ResetLast ()
{
    CSingleLock lk(&m_criticalSection, TRUE);

    m_lastTaskOwner = 0;
 }

bool TaskQueue::HasAnyByOwner (void* owner) const
{
    if (!owner) 
        return false;

    CSingleLock lk(&m_criticalSection, TRUE);

    if (m_lastTaskOwner == owner)
        return true;

    std::list<TaskPtr>::const_iterator it = m_queue.begin();
    for (; it != m_queue.end(); ++it)
        if ((*it)->GetOwner() == owner)
            return true;

    return false;
}

}; // namespace ServerBackgroundThread
