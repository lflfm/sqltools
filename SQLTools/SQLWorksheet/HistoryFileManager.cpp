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

#include "stdafx.h"
#include <stdio.h>
#include <vector>
#include <afxmt.h>
#include "SQLTools.h"
#include "HistorySource.h"
#include "HistoryFileManager.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "COMMON\ConfigFilesLocator.h"
#include "COMMON\ExceptionHelper.h"

//TODO: test ALL!!

    using namespace std;
    using namespace OpenEditor;
    using namespace Common;

    const wchar_t* TheCatalogerName        = L"cataloger.dat";
    const wchar_t* TheGlobalKey            = L"<This is a global history file>";
    const wchar_t* TheGlobalFileName       = L"global.dat";
    const int   TheCatalogerVersion     = 1001;
    const int   FILE_NAME_OFFSET        = 16;
    const int   CATALOGER_LOCK_ATTEMPTS = 10;
    const int   CATALOGER_LOCK_TIMEOUT  = 1000;
    const int   GLOBAL_LOCK_ATTEMPTS    = 10;
    const int   GLOBAL_LOCK_TIMEOUT     = 1000;

    const char* TheHistoryFileVersion   = "1003";
    const int   SQL_TEXT_OFFSET_1001    = 20;
    const int   CON_DESC_OFFSET_1002    = 20;
    const int   SQL_TEXT_OFFSET_1002    = 40;

    const int   DURATION_OFFSET         = 20;
    const int   CON_DESC_OFFSET         = 40;
    const int   SQL_TEXT_OFFSET         = 60;
    
    HistoryFileManager HistoryFileManager::theInstance;

    bool file_exists (const wchar_t* filename)
    {
        return AppGetFileAttrs(filename, (DWORD*)0);
    }

    class FileLock
    {
    public:
        FileLock (const std::wstring& file)
            : m_hFile(INVALID_HANDLE_VALUE),
            m_filename(file + L".$lock$")
        {
        }

        bool Lock (int attempts, int timeout)
        {
            for (int i = 0; i < attempts && m_hFile == INVALID_HANDLE_VALUE; i++)
            {
                m_hFile = CreateFile(m_filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                Sleep(timeout/attempts);
            }
            return *this;
        }

        ~FileLock ()
        {
            if (m_hFile != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(m_hFile);
                DeleteFile(m_filename.c_str());
            }
        }

        operator bool () { return m_hFile != INVALID_HANDLE_VALUE ? true : false; }

    private:
        HANDLE m_hFile;
        std::wstring m_filename;
    };


// HistoryFileManager //////////////////////////////////////////////////////////
HistoryFileManager::HistoryFileManager ()   
: m_globalSource(new HistorySource)
{
}

HistoryFileManager::~HistoryFileManager ()  
{
    //try { EXCEPTION_FRAME;

    //    Save(*m_globalSource, TheGlobalKey);
    //}
    //_DESTRUCTOR_HANDLER_;
}

void HistoryFileManager::Flush ()
{
    try { EXCEPTION_FRAME;

        Save(*m_globalSource, TheGlobalKey);
    }
    _DESTRUCTOR_HANDLER_;
}

HistoryFileManager& HistoryFileManager::GetInstance ()
{
    if (theInstance.m_folder.empty())
    {
        ASSERT(!Common::ConfigFilesLocator::GetBaseFolder().empty());
        theInstance.SetFolder((Common::ConfigFilesLocator::GetBaseFolder() + L"\\SqlHistory").c_str());
    }

    return theInstance;
}

void HistoryFileManager::SetFolder (const wchar_t* fldr)
{
    std::wstring folder = fldr;
    m_folder = folder;

    if (folder.size())
    {
        vector<std::wstring> subdirs;
        // check existent path part
        while (!folder.empty() && *(folder.rbegin()) != ':'
        && GetFileAttributes(folder.c_str()) == 0xFFFFFFFF) {
            subdirs.push_back(folder);

            size_t len = folder.find_last_of('\\');
            if (len != string::npos)
                folder.resize(len);
            else
                break;
        }

        // create non-existent path part
        auto it(subdirs.rbegin()), end(subdirs.rend());
        for (; it != end; it++)
            _CHECK_AND_THROW_(CreateDirectory((*it).c_str(), NULL), "Can't create folder!");
    }

}

std::wstring HistoryFileManager::getHistoryFile (const wchar_t* filename)
{
    wstring catalogerFilename = m_folder + L"\\" + TheCatalogerName;

    FileLock lk(catalogerFilename);
    
    if (lk.Lock(CATALOGER_LOCK_ATTEMPTS, CATALOGER_LOCK_TIMEOUT))
    {
        ofstream cataloger;
        bool catalogerExists = file_exists(catalogerFilename.c_str());
        
        cataloger.open(catalogerFilename.c_str(), ios::app);

        if (cataloger.is_open())
        {
            if (!catalogerExists)
                cataloger << TheCatalogerVersion << endl;

            std::wstring histFilename;
            set<std::wstring> allHistFiles;

            if (!lookupHistoryFileName(filename, histFilename, &allHistFiles))
            {
                createHistoryFileName(histFilename, allHistFiles);
                // append a new entry
                _ASSERTE(histFilename.length() < FILE_NAME_OFFSET);

                string buffer = Common::str(histFilename);
                buffer.resize(FILE_NAME_OFFSET, ' ');
                buffer += Common::str(filename);
                cataloger << buffer << endl;
                cataloger.flush();
            }

            return m_folder + L"\\" + histFilename;
        }
    }

    return std::wstring();
}

bool HistoryFileManager::lookupHistoryFileName (const wchar_t* _filename, std::wstring& _histFilename, set<std::wstring>* allHistFiles)
{
    if (_filename == TheGlobalKey) {
        _histFilename = TheGlobalFileName;
        return true;
    }

    ifstream cataloger((m_folder + L"\\" + TheCatalogerName).c_str());

    int version;
    if (!(cataloger >> version))
        return false; //cannot read/read file

    if (version != TheCatalogerVersion)
        return false; //unsupported cataloger version

    string buffer;
    while (getline(cataloger, buffer))
    {
        if (buffer.length() > FILE_NAME_OFFSET)
        {
            string filename = buffer.c_str() + FILE_NAME_OFFSET;
            string histFilename = buffer;
            histFilename.resize(FILE_NAME_OFFSET);
            trim_symmetric(histFilename);

            if (_filename == Common::wstr(filename))
            {
                _histFilename = Common::wstr(histFilename);
                return true;
            }

            // still executing and collecting all of existing history files
            if (allHistFiles)
                allHistFiles->insert(Common::wstr(histFilename));
        }
    }

    return false;
}

void HistoryFileManager::createHistoryFileName (std::wstring& histFilename, const set<std::wstring>& allHistFiles)
{
    for (int i = 1; i > 0; i++) 
    {
        wchar_t buff[40];
        _snwprintf(buff, sizeof(buff)/sizeof(buff[0])-1, L"%08x.dat", i);
        buff[sizeof(buff)/sizeof(buff[0])-1] = 0;
        histFilename = buff;
        
        if (allHistFiles.find(histFilename) == allHistFiles.end())
            break;
    }
}

// this method creates a new history source or returns an globale one
counted_ptr<HistorySource> HistoryFileManager::CreateSource ()
{
    if (GetSQLToolsSettings().GetHistoryGlobal())
    {
        if (!m_globalSource->GetRowCount()) 
            Load(*m_globalSource, TheGlobalKey);

        return m_globalSource;
    }

    return counted_ptr<HistorySource>(new HistorySource);
}

void HistoryFileManager::Load (HistorySource& src, const wchar_t* filename)
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if (settings.GetHistoryEnabled()
    && settings.GetHistoryPersitent())
    {
        if (m_globalSource.get() == &src 
        && filename != TheGlobalKey)
            return;            

        ASSERT(!src.GetRowCount());

        ifstream in;

        std::wstring histFilename;
        if (lookupHistoryFileName(filename, histFilename))
        {
            histFilename = m_folder + L"\\" + histFilename;
            in.open(histFilename.c_str());
        }

        if (in.is_open())
        {
            __int64 lastWriteTime;
            VERIFY(AppGetFileAttrs(histFilename.c_str(), 0, 0, 0, &lastWriteTime));
            src.SetHistoryFileTime(lastWriteTime);

            string buffer;
            getline(in, buffer);
            trim_symmetric(buffer);

            if (buffer == "1001")
            {
                getline(in, buffer);

                _ASSERT(buffer == Common::str(filename));

                while (getline(in, buffer))
                {
                    if (buffer.length() > SQL_TEXT_OFFSET_1001)
                    {
                        string time, text;
                        time.assign(buffer.c_str(), SQL_TEXT_OFFSET_1001-1);
                        to_unprintable_str(buffer.c_str() + SQL_TEXT_OFFSET_1001, text);
                        src.LoadStatement(time, "", "", text, /*atTop=*/true);
                    }
                }
            }
            else if (buffer == "1002")
            {
                getline(in, buffer);

                _ASSERT(buffer == Common::str(filename));

                while (getline(in, buffer))
                {
                    if (buffer.length() > SQL_TEXT_OFFSET_1002)
                    {
                        string time, conn, text;
                        
                        // substr
                        time = buffer.substr(0, CON_DESC_OFFSET_1002-1);
                        trim_symmetric(time);

                        conn = buffer.substr(CON_DESC_OFFSET_1002, SQL_TEXT_OFFSET_1002-CON_DESC_OFFSET_1002);
                        trim_symmetric(conn);

                        to_unprintable_str(buffer.c_str() + SQL_TEXT_OFFSET_1002, text);

                        src.LoadStatement(time, "", conn, text);
                    }
                }
            }
            else if (buffer == TheHistoryFileVersion)
            {
                getline(in, buffer);

                _ASSERT(buffer == Common::str(filename));

                while (getline(in, buffer))
                {
                    if (buffer.length() > SQL_TEXT_OFFSET)
                    {
                        string time, duration, conn, text;
                        
                        // substr
                        time = buffer.substr(0, DURATION_OFFSET-1);
                        trim_symmetric(time);

                        duration = buffer.substr(DURATION_OFFSET, CON_DESC_OFFSET-DURATION_OFFSET);
                        trim_symmetric(duration);

                        conn = buffer.substr(CON_DESC_OFFSET, SQL_TEXT_OFFSET-CON_DESC_OFFSET);
                        trim_symmetric(conn);

                        to_unprintable_str(buffer.c_str() + SQL_TEXT_OFFSET, text);

                        src.LoadStatement(time, duration, conn, text);
                    }
                }
            }
        }
        src.SetModified(false);
    }
}

void HistoryFileManager::Save (HistorySource& src, const wchar_t* filename)
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if (src.IsModified()
    && settings.GetHistoryEnabled()
    && settings.GetHistoryPersitent())
    {
        if (m_globalSource.get() == &src
        && filename != TheGlobalKey)
            return;           

        std::wstring histFilename = getHistoryFile(filename);
        FileLock lk(histFilename);
   
        if (lk.Lock(GLOBAL_LOCK_ATTEMPTS, GLOBAL_LOCK_TIMEOUT))
        {
            bool saved = false;

            if (::PathFileExists(histFilename.c_str()))
            {
                __int64 lastWriteTime;
                VERIFY(AppGetFileAttrs(histFilename.c_str(), 0, 0, 0, &lastWriteTime));
           
                if (src.GetHistoryFileTime() != lastWriteTime) // merging
                {
                    std::wstring tmpHistFilename = histFilename + L".$temp$";

                    {
                        stringstream s1;
                        save(src, filename, s1);

                        ifstream s2(histFilename.c_str());

                        ofstream out(tmpHistFilename.c_str());

                        if (merge(s1, s2, out))
                        {
                            s2.close();
                            out.close();
                            VERIFY(::DeleteFile(histFilename.c_str()));
                            VERIFY(::MoveFile(tmpHistFilename.c_str(), histFilename.c_str()));
                            saved = true;
                        }
                    }
                }
            }

            if (!saved)
            {
                ofstream out(histFilename.c_str());
               
                if (out.is_open())
                    save(src, filename, out);
            }

            src.SetModified(false);
        }
    }
}

void HistoryFileManager::save (HistorySource& src, const wchar_t* filename, std::ostream& out)
{
    out << TheHistoryFileVersion << endl;
    out << filename << endl;

    int ncount = src.GetRowCount();
    
    for (int i = 0; i < ncount; i++)
    {
        string buffer;
        to_printable_str(src.GetTime(i).c_str(), buffer);
        buffer.resize(DURATION_OFFSET, ' ');
        out << buffer;

        to_printable_str(src.GetDuration(i).c_str(), buffer);
        buffer.resize(CON_DESC_OFFSET-DURATION_OFFSET, ' ');
        out << buffer;

        to_printable_str(src.GetConnectDesc(i).c_str(), buffer);
        buffer.resize(SQL_TEXT_OFFSET-CON_DESC_OFFSET, ' ');
        out << buffer;

        to_printable_str(src.GetSqlStatement(i).c_str(), buffer);
        out << buffer << endl;
    }
}

bool HistoryFileManager::merge (istream& in1, istream& in2, ostream& out)
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();
    bool distinct = settings.GetHistoryDistinct();

    set<string> statements;

    string filename, buffer[2];
    istream* in[2] = { &in1, &in2 };

    // check version
    for (int i = 0; i < 2; i++)
    {
        getline(*in[i], buffer[i]);
        trim_symmetric(buffer[i]);
        if (buffer[i] != TheHistoryFileVersion)
            return false;
    }

    // skip file name
    for (int i = 0; i < 2; i++)
    {
        getline(*in[i], buffer[i]);
        filename = buffer[i];
        buffer[i].clear();
    }

    // init out stream
    out << TheHistoryFileVersion << endl;
    out << filename << endl;

    while (true)
    {
        for (int i = 0; i < 2; i++)
            if (buffer[i].empty()) 
                getline(*in[i], buffer[i]);
        
        int index = -1;

        if (!buffer[0].empty() && !buffer[1].empty())
            index = (strncmp(buffer[0].c_str(), buffer[1].c_str(), DURATION_OFFSET) > 0) ? 0 : 1;
        else if (!buffer[0].empty())
            index = 0;
        else if (!buffer[1].empty())
            index = 1;
        else
            break;

        string sttm = buffer[index].substr(SQL_TEXT_OFFSET);
        if (!distinct || statements.find(sttm) == statements.end())
        {
            if (distinct) statements.insert(sttm);
            out << buffer[index] << endl;
        }
        buffer[index].clear();
    }

    return true;
}
