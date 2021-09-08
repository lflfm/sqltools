/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 2020 Aleksey Kochetov

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

#include <iostream>
#include <string>
#include <vector>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <windows.h>
#include <lmerr.h>

#include "Common/MemoryMappedFile.h"
#include "Common/OsException.h"
using namespace Common;
#include "TextEncodingDetect/text_encoding_detect.h"
using namespace AutoIt::Common;

#define BOOST_REGEX_NO_LIB
#define BOOST_REGEX_STATIC_LINK
#include <boost/cregex.hpp>
using namespace boost;


namespace GrepImplementation
{
using namespace std;

// trim from start (in place)
static inline void ltrim(wstring &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
        return !isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(wstring &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(wstring &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline wstring ltrim_copy(wstring s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline wstring rtrim_copy(wstring s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline wstring trim_copy(wstring s) {
    trim(s);
    return s;
}

static inline wstring trim_qoutes (wstring s) {
    if (!s.empty())
    {
        if (*s.begin() == '"')
        {
            if (s.length() < 2 || *s.rbegin() != '"')
                throw exception("quoted wstring is not closed properly!");

            return s.substr(1, s.length() - 2);
        }
    }
    return s;
}

static
vector<wstring> make_list (const wstring& input, bool folders, const char deleimeter = ';')
{
    wstring buffer;
    vector<wstring> result;
    bool quoted = false;

    for (auto it = input.begin(); it != input.end(); ++it)
    {
        if (*it == '"')
        {
          quoted = !quoted;
        }
        if (!quoted && *it == deleimeter)
        {
            buffer = trim_qoutes(trim_copy(buffer));
            if (!buffer.empty())
            {
                if (folders && *buffer.rbegin() != '\\')
                    buffer += '\\';
                if (find(result.begin(), result.end(), buffer) == result.end())
                    result.push_back(buffer);
                buffer.clear();
            }
            continue;
        }
        buffer += *it;
    }

    buffer = trim_qoutes(trim_copy(buffer));
    if (!buffer.empty())
    {
        if (folders && *buffer.rbegin() != '\\')
            buffer += '\\';
        if (find(result.begin(), result.end(), buffer) == result.end())
            result.push_back(buffer);
        buffer.clear();
    }

    return result;
}

static
wstring get_last_error ()
{
    wstring result;

    HMODULE 	hModule 	= NULL; 
    LPTSTR		lpszMessage	= NULL;
    DWORD		cbBuffer	= NULL;
    DWORD       dwErrCode   = ::GetLastError();

    if ((dwErrCode >= NERR_BASE) && (dwErrCode <= MAX_NERR /*NERR_BASE+899*/))
    {
        hModule = ::LoadLibraryEx( L"netmsg.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    }
    
    cbBuffer = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                               FORMAT_MESSAGE_IGNORE_INSERTS | 
                               FORMAT_MESSAGE_FROM_SYSTEM | // always consider system table
                               ((hModule != NULL) ? FORMAT_MESSAGE_FROM_HMODULE : 0), 
                               hModule, // module to get message from (NULL == system)
                               dwErrCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
                               (LPTSTR) &lpszMessage, 0, NULL );
    
    //ERROR_RESOURCE_LANG_NOT_FOUND

    if (cbBuffer)
        result = (LPCTSTR)lpszMessage;

    ::LocalFree(lpszMessage);
    if (hModule) ::FreeLibrary(hModule);

    if (result.empty())
        result = L"Unknown system error!";// + dwErrCode;

    return result;
}


static
bool is_folder (const wstring& path, wstring& error)
{
    DWORD dwFileAttributes = GetFileAttributes(path.c_str());

    if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

    //error = L"Cannot open folder or file \"" + file + L"\"!";
    error = get_last_error();

    return false;
}

template <typename FuncType, typename ErrorHandler>
void for_each_folder (const wstring& folders, const wstring& masks, bool recursively, FuncType func, ErrorHandler handler)
{
    vector<wstring> folderList = make_list(folders, true);
    vector<wstring> maskList = make_list(masks, false, ',');

    for (auto folderIt = folderList.begin(); folderIt != folderList.end(); ++folderIt)
        for_each_folder(*folderIt, maskList, recursively, func, handler);

}

template <typename FuncType, typename ErrorHandler>
void for_each_folder (const wstring& folder, const vector<wstring>& maskList, bool recursively, FuncType func, ErrorHandler handler)
{
    //wcout << L"for_each_folder for " << folder << endl;

    wstring error;
    if (is_folder(folder, error))
    {
        for (auto maskIt = maskList.begin(); maskIt != maskList.end(); ++maskIt)
        {
            wstring pattern = folder;
            pattern += *maskIt;

            {
                WIN32_FIND_DATA findFileData;
                memset(&findFileData, 0, sizeof findFileData);
                HANDLE hFind = FindFirstFile(pattern.c_str(), &findFileData);

                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        if(!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                            func(folder + findFileData.cFileName);
                        else
                            ;
                    }
                    while (FindNextFile(hFind, &findFileData));

                    if (hFind != INVALID_HANDLE_VALUE)
                        FindClose(hFind);
                }
            }
        }

        if (recursively)
        {
            wstring pattern = folder;
            pattern += L"*.*";

            WIN32_FIND_DATA findFileData;
            memset(&findFileData, 0, sizeof findFileData);
            HANDLE hFind = FindFirstFile(pattern.c_str(), &findFileData);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                    && wcscmp(L".", findFileData.cFileName)
                    && wcscmp(L"..", findFileData.cFileName))
                        for_each_folder(folder + findFileData.cFileName + L"\\", maskList, recursively, func, handler);
                }
                while (FindNextFile(hFind, &findFileData));

                if (hFind != INVALID_HANDLE_VALUE)
                    FindClose(hFind);
            }
        }
    }
    else
    {
        //wcerr << L"Error while processing the folder " << folder << endl << error << endl;
        handler(folder, error);
    }
}

const int CP_UTF16 = 1200;

template <typename FuncType, typename ErrorHandler>
void for_each_file (const wstring& file, FuncType func, ErrorHandler handler)
{
    try // open a file 
    {
        Common::MemoryMappedFile mmfile;
        mmfile.Open(file.c_str(), emmoRead|emmoShareRead|emmoShareWrite, 0);
        
        unsigned long length = mmfile.GetDataLength();
        void* data = mmfile.GetData();

        int codepage = 0, bom = 0;

        switch (TextEncodingDetect().DetectEncoding((unsigned char*)data, length))
        {
        case TextEncodingDetect::Encoding::UTF8_BOM:   // UTF8 with BOM
            codepage = CP_UTF8;
            bom = 3;
            break;
        case TextEncodingDetect::Encoding::UTF8_NOBOM: // UTF8 without BOM
            codepage = CP_UTF8;
            bom = 0;
            break;
        case TextEncodingDetect::Encoding::UTF16_LE_BOM:// UTF16 LE with BOM
            codepage = CP_UTF16;
            bom = 2;
            break;
        case TextEncodingDetect::Encoding::ASCII:      // 0-127
        case TextEncodingDetect::Encoding::ANSI:       // 0-255
        default:
            codepage = CP_ACP;
            bom = 0;
            break;
        };

        //wcout << file << L" length = " << length << endl;

        if (codepage != CP_UTF16) 
            for_each_line((const char*)data + bom, length - bom, codepage, func);
        else // wide characters
            for_each_line((const wchar_t*)((const char*)data + bom), (length - bom)/2, codepage, func);
    }
    catch (const OsException& e)
    {
        //wcerr << L"Error while processing the file " << file << endl << e.what() << endl;
        handler(file, Common::wstr(e.what()));
    }
}

template <class char_type>
struct line_delim
{
    char_type bytes[2];
};

template <class char_type>
static line_delim<char_type> get_line_delim (const char_type* text, unsigned long length)
{
    const char_type* ptr = text;
    for (; ptr && *ptr != '\n' && *ptr != '\r' && ptr < text + length; ptr++)
        ;

    line_delim<char_type> delim;
    delim.bytes[0] = '\n';
    delim.bytes[1] = '\r';

    if (ptr < text + length)
    {
        if (ptr[0] == '\n')
        {
            if (ptr + 1 < text + length && ptr[1] == '\r')
            {
                delim.bytes[0] = '\n';
                delim.bytes[1] = '\r';
            }
            else
            {
                delim.bytes[0] = '\n';
                delim.bytes[1] = 0;
            }
        }
        else if (ptr[0] == '\r')
        {
            if (ptr + 1 < text + length && ptr[1] == '\n')
            {
                delim.bytes[0] = '\r';
                delim.bytes[1] = '\n';
            }
            // else should not happen - use default
       }
    }
    //else use default

    return delim;
}

template <class char_type, typename FuncType>
void for_line (unsigned line, const char_type* ptr, int len, int codepage, FuncType func)
{
    int length = ::MultiByteToWideChar(codepage, 0, ptr, len, 0, 0);
    auto wbuffer = std::make_unique<wchar_t[]>(length+1);
    ::MultiByteToWideChar(codepage, 0, ptr, len, wbuffer.get(), length);
    wbuffer[length] = 0;
    func(line, wstring(wbuffer.get(), length));
}

template <class char_type, typename FuncType>
void for_line (unsigned line, const wchar_t* ptr, int len, int /*codepage*/, FuncType func)
{
    //wcout << line << L"\t" << wstring(ptr, len) << endl;
    func(line, wstring(ptr, len));
}

template <class char_type, typename FuncType>
static void for_each_line (const char_type* text, unsigned long length, int codepage, FuncType func)
{
    line_delim<char_type> delim = get_line_delim(text, length);

    int pos = 0;
    const char_type* ptr = text;
    unsigned line(0);

    while (ptr + pos < text + length)
    {
        if (ptr[pos] == delim.bytes[0]
        // 20.11.2003 bug fix, handling unregular eol, e.c. single '\n' for MSDOS files
        || ptr[pos] == delim.bytes[1])
        {
            for_line<char_type, FuncType>(line, ptr, pos, codepage, func);

            if (delim.bytes[1] && ((ptr + pos + 1) < text + length))
            {
                if (ptr[pos + 1] == delim.bytes[1])
                    pos++;
                else
                    ; // do nothing, ignore unregular eol
            }

            ptr += pos + 1;
            pos = 0;
            line++;
        }
        else
            pos++;
    }

    if (pos) // ptr is not empty
        for_line<char_type, FuncType>(line, ptr, pos, codepage, func);
}


struct regexp_cxt
{
    regex_t     pattern;
    regmatch_t  match[10];
    bool        wholeword;

    regexp_cxt ()  { ::memset(this, 0, sizeof(*this)); }
    ~regexp_cxt () { try { regfree(&pattern); } _DESTRUCTOR_HANDLER_; }

    void compile (wstring what, bool plain, bool icase, bool ww)
    {
        wholeword = ww;

        wstring expression;

        if (!plain)
        {
            expression = what;
        }
        else
        {
            auto it = what.begin();
            for (; it != what.end(); ++it)
            {
                switch (*it) // 17/05/2002 bug fix, cannot search for $,[,],^
                {
                case '.': case '*': case '\\':
                case '[': case ']':
                case '^': case '$':
                    expression += '\\';
                }
                expression += *it;
            }
        }

        int error = regcomp(&pattern, expression.c_str(), (!plain ? REG_EXTENDED : 0) | (!icase ? 0 :REG_ICASE));

        if (error)
        {
            wchar_t buff[256];
            buff[0] = 0;
            regerror(error, &pattern, buff, sizeof buff);

            THROW_APP_EXCEPTION(string("Regular expression error:\n") + Common::str(buff));
        }
    }

    bool find (const wchar_t* str, int len, int& start, int& end)
    {
        match[0].rm_so = 0; // from
        match[0].rm_eo = len; // to

        if (!regexec(&pattern, str, sizeof match / sizeof match[0], match, REG_STARTEND))


        while ((match[0].rm_so < len)
        && !regexec(&pattern, str, sizeof match / sizeof match[0], match, !match[0].rm_so ? REG_STARTEND : REG_NOTBOL|REG_STARTEND))
        {
            if (wholeword)
            {
                if ((match[0].rm_so > 0 && iswalnum(str[match[0].rm_so - 1]))
                || (match[0].rm_eo < len && iswalnum(str[match[0].rm_eo])))
                {
                    match[0].rm_so++;
                    match[0].rm_eo = len;
                    continue;
                }
            }

            start = match[0].rm_so;
            end   = match[0].rm_eo;
            return true;
        }
        return false;
    }
};

} // namespace GrepImplementation