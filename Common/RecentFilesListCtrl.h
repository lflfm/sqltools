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

#pragma once

#include <string>
#include <arg_shared.h>
#include "ManagedListCtrl.h"

    using arg::counted_ptr;
    class RecentFileList;

    struct RecentFilesEntry
    {
        std::wstring path          ;
        std::wstring name          ;
        time_t last_used           ;

        RecentFilesEntry () : deleted(false), image(-1) {}
        bool operator == (const RecentFilesEntry& other) const { return path == other.path && name == other.name; }
        bool deleted;
        int image;
    };

    typedef std::vector<RecentFilesEntry> RecentFilesCollection;

    class RecentFilesAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const RecentFilesCollection& m_entries;

        enum
        {
            e_name     = 0, 
            e_path     , 
            e_last_used,
            e_count    ,
        };

    public:
        RecentFilesAdapter (const RecentFilesCollection& entries) : m_entries(entries) {}

        const RecentFilesEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return e_count; }

        virtual Type getColType (int col) const {
            switch (col) {
            case e_name      : return String;   
            case e_path      : return String;      
            case e_last_used : return Date;     
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case e_name      : return L"Name";   
            case e_path      : return L"Path";      
            case e_last_used : return L"Last used";     
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case e_name      : return getStr(data(row).name          );
            case e_path      : return getStr(data(row).path          );
            case e_last_used : return getStrTimeT(data(row).last_used);
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case e_name      : return comp(data(row1).name     , data(row2).name     );
            case e_path      : return comp(data(row1).path     , data(row2).path     );
            case e_last_used : return comp(data(row1).last_used, data(row2).last_used);
            }
            return 0;
        }

        virtual int getImageIndex (int row) const { return data(row).image; }
    };


class RecentFilesListCtrl : public Common::CManagedListCtrl
{
    counted_ptr<RecentFileList> m_pRecentFileList;
    RecentFilesCollection m_entries;
    RecentFilesAdapter m_adapter;
    bool m_initialized;

public:
    RecentFilesListCtrl ();
    virtual ~RecentFilesListCtrl ();

    void Link (counted_ptr<RecentFileList> pRecentFileList);

    bool IsInitialized() const { return m_initialized; }
    void Initialize();

    void LoadEntry (const std::wstring& path, time_t last_used);
    void UpdateEntry (const std::wstring& path, time_t last_used);
    
    void RemoveEntry (unsigned int);
    void RemoveEntry (const std::wstring& path);

    std::wstring GetPath (unsigned int) const;
    unsigned int FindEntry (const std::wstring& path) const;

protected:
	DECLARE_MESSAGE_MAP()
};


