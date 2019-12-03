/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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
#include <fstream>
#include <set>
#include <arg_shared.h>

    class HistorySource;
    using arg::counted_ptr;

class HistoryFileManager
{
public:
    // get singleton
    static HistoryFileManager& GetInstance ();

    // set history folder, create/open cataloger
    void SetFolder (const char*);

    void Load  (HistorySource&, const char* filename);
    void Save  (HistorySource&, const char* filename);

    // this method creates a new history source or returns an globale one
    counted_ptr<HistorySource> CreateSource ();
    
    void Flush (); 

private:
    HistoryFileManager (); 
    ~HistoryFileManager ();

    std::string getHistoryFile (const char*);
    bool lookupHistoryFileName (const char*, std::string&, std::set<std::string>* = 0);
    void createHistoryFileName (std::string&, const std::set<std::string>&);

    void save  (HistorySource&, const char* filename, std::ostream&);
    bool merge (std::istream&, std::istream&, std::ostream&);

    std::string m_folder;
    counted_ptr<HistorySource> m_globalSource;

    static HistoryFileManager theInstance;
};

