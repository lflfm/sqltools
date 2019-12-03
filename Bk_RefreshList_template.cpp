    friend struct BackgroundTask_Refresh<Type>List;
    ClusterCollection m_data;

#include "ServerBackgroundThread\TaskQueue.h"
using namespace ServerBackgroundThread;


	struct BackgroundTask_Refresh<Type>List : Task
    {
        <Type>Collection m_data;
        DBBrowser<Type>List& m_list;
        string m_schema;

        BackgroundTask_Refresh<Type>List(DBBrowser<Type>List& list)
            : Task("Refreshing <Type>List"),
            m_list(list)
        {
            m_schema = list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            std::swap(m_list.m_data, m_data);

            m_list.Common::CManagedListCtrl::Refresh(true);
            m_list.SetDirty(false);
        }
    };


void DBBrowserTableList::Refresh (bool force)
{
    if (IsDirty() || force)
    {
        DeleteAllItems();
        m_data.clear();

        TaskQueue::GetRequestQueue().Push(TaskPtr(new BackgroundTask_Refresh<Type>List(*this)));

        Common::CManagedListCtrl::Refresh(false);
        SetDirty(false);
    }
}

	struct BackgroundTask_Refresh<Type>ListEntry : Task
    {
        int m_entry;
        <Type>Entry m_dataEntry;
        DBBrowser<Type>List& m_list;
        string m_schema;

        BackgroundTask_Refresh<Type>ListEntry (DBBrowser<Type>List& list, int entry)
            : Task("Refreshing <Type>ListEntry"),
            m_list(list),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry))
        {
            m_schema = list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            m_list.m_data.at(m_entry) = m_dataEntry;
            m_list.OnUpdateEntry(m_entry);
        }
    };

void ...::RefreshEntry (int entry)
{
    TaskQueue::GetRequestQueue().Push(TaskPtr(new BackgroundTask_Refresh<Type>ListEntry(*this, entry)));
}