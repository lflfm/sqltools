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

#include "stdafx.h"
#include <algorithm>
#include "Common\RecentFilesListCtrl.h"
#include "OpenEditor/RecentFileList.h"


    using namespace Common;

RecentFilesListCtrl::RecentFilesListCtrl ()
: CManagedListCtrl(m_adapter),
    m_adapter(m_entries),
    m_initialized(false)
{
}

RecentFilesListCtrl::~RecentFilesListCtrl ()
{
}

BEGIN_MESSAGE_MAP(RecentFilesListCtrl, CManagedListCtrl)
END_MESSAGE_MAP()

void RecentFilesListCtrl::Link (counted_ptr<RecentFileList> pRecentFileList) 
{ 
    m_pRecentFileList = pRecentFileList; 
    
    if (m_initialized) // re-initialize!
        Initialize();
}

void RecentFilesListCtrl::Initialize ()
{
    if (m_pRecentFileList.get())
    {
        m_pRecentFileList->AttachControl(this);

        SetSortColumn(2, ListCtrlManager::DESC);
        Refresh(true);
    }
    m_initialized = true;
}

void RecentFilesListCtrl::LoadEntry (const string& path, time_t last_used)
{
    RecentFilesEntry entry;
    entry.name = ::PathFindFileName(path.c_str());
    entry.path = entry.name != path ? path.substr(0, path.size()-entry.name.size()) : "";
    entry.last_used = last_used;

    SHFILEINFO shFinfo;
    if (SHGetFileInfo(path.c_str(), 0, &shFinfo, sizeof(shFinfo), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES))
    {
        entry.image = shFinfo.iIcon;
        DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
    }

    m_entries.push_back(entry);
}

void RecentFilesListCtrl::UpdateEntry (const string& path, time_t last_used)
{
    RecentFilesEntry entry;
    entry.name = ::PathFindFileName(path.c_str());
    entry.path = entry.name != path ? path.substr(0, path.size()-entry.name.size()) : "";
    entry.last_used = last_used;

    RecentFilesCollection::iterator it = std::find(m_entries.begin(), m_entries.end(), entry);
    if (it != m_entries.end())
    {
        it->last_used = entry.last_used;
        
        if (m_initialized)
        {
            OnUpdateEntry(m_entries.end() - it);
            Resort();
        }
    }
    else
    {
        //TODO#2: create map ext to icon
        SHFILEINFO shFinfo;
        if (SHGetFileInfo(path.c_str(), 0, &shFinfo, sizeof(shFinfo), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_USEFILEATTRIBUTES))
        {
            entry.image = shFinfo.iIcon;
            DestroyIcon(shFinfo.hIcon); // we only need the index from the system image list
        }
        m_entries.push_back(entry);

        if (m_initialized)
        {
            OnAppendEntry();
            Resort();
        }
    }
}

void RecentFilesListCtrl::RemoveEntry (unsigned int index)
{
    if (index < m_entries.size())
    {
        if (m_pRecentFileList.get())
            m_pRecentFileList->RemoveDocument(GetPath(index));

        OnDeleteEntry(index);
    }
}

void RecentFilesListCtrl::RemoveEntry (const string& path)
{
    unsigned int index = FindEntry(path);

    if (index < m_entries.size())
        RemoveEntry(index);
    else
        m_pRecentFileList->RemoveDocument(path);
}

string RecentFilesListCtrl::GetPath (unsigned int index) const
{
    if (index < m_entries.size())
        return m_entries.at(index).path + m_entries.at(index).name;

    return string();
}

unsigned int RecentFilesListCtrl::FindEntry (const string& path)  const
{
    unsigned int index(0), count(m_entries.size());

    for (; index < count; ++index)
    {
        if (path == (m_entries.at(index).path + m_entries.at(index).name))
            return index;
    }

    return (unsigned int)-1;
}

