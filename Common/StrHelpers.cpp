/* 
    Copyright (C) 2002 Aleksey Kochetov

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

/*
    31.03.2003 bug fix, "Month" is not recognized as a valid token for date conversion
*/

#include "stdafx.h"
#include <string>
#include <vector>
//#include <locale>
#include <iomanip>
#include <sstream>
#include "StrHelpers.h"
#include "Fastmap.h"
#include "MyUtf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace Common 
{
    using namespace std;
    
    void to_printable_str (const char* from, string& _to)
    {
        string to;

        for (; from && *from; from++)
        {
            switch (*from)
            {
            case '\a': to += "\\a";  break; // \a Bell (alert) 
            case '\b': to += "\\b";  break; // \b Backspace 
            case '\f': to += "\\f";  break; // \f Formfeed 
            case '\n': to += "\\n";  break; // \n New line 
            case '\r': to += "\\r";  break; // \r Carriage return 
            case '\t': to += "\\t";  break; // \t Horizontal tab 
            case '\v': to += "\\v";  break; // \v Vertical tab 
//            case '\'': to += "\'";   break; // \' Single quotation mark 
//            case '\"': to += "\"";   break; // \" Double quotation mark 
            case '\\': to += "\\\\"; break; // \\ Backslash 
//            case '\?': to += "\?";   break; // \? Literal question mark 
            default:
                if (isprint(*from))
                    to += (char)*from;
                else // \ddd ASCII character in decimal notation
                {
                    char buff[20];
                    to += "\\";
                    itoa(*from, buff, 10);
                    for (int j(0), len(3-strlen(buff)); j < len; j++)
                        to += '0';
                    to += buff;
                }
            }
        }
        _to = to;
    }

    void to_unprintable_str (const char* from, string& _to, bool skipEscDgt)
    {
        string to;

        for (; *from; from++)
        {
            unsigned char ch = *from;

            if (*from == '\\')
            {
                switch (from[1])
                {
                case 'a':  ch = '\a'; from++; break; // \a Bell (alert) 
                case 'b':  ch = '\b'; from++; break; // \b Backspace 
                case 'f':  ch = '\f'; from++; break; // \f Formfeed 
                case 'n':  ch = '\n'; from++; break; // \n New line 
                case 'r':  ch = '\r'; from++; break; // \r Carriage return 
                case 't':  ch = '\t'; from++; break; // \t Horizontal tab 
                case 'v':  ch = '\v'; from++; break; // \v Vertical tab 
                case '\'': ch = '\''; from++; break; // \' Single quotation mark 
                case '\"': ch = '\"'; from++; break; // \" Double quotation mark 
                case '\\': ch = '\\'; from++; break; // \\ Backslash 
                case '?':  ch = '\?'; from++; break; // \? Literal question mark 
                default:
                    if (!skipEscDgt)
                    {
                        char digits[4];
                        for (int j(0); j < 3 && isdigit(from[j+1]); j++)
                            digits[j] = from[j+1];
                
                        if (j > 0)
                        {
                            digits[j] = 0;
                            ch = static_cast<char>(static_cast<unsigned>(atoi(digits)));
                            from += j; 
                        }
                        else
                            _ASSERTE(0);
                    }
                }
            }
            to += ch;
        }
        _to = to;
    }

    void trim_symmetric (string& str, const char* skip)
    {
        if (!str.empty()) {
            string::size_type beg = str.find_first_not_of(skip);
            string::size_type end = str.find_last_not_of(skip);

            if (beg != string::npos || end != string::npos) {
                if (string::npos == beg) beg = 0;
                if (string::npos == end) end = str.size();
                str = str.substr(beg, end - beg + 1);
            }
        }
    }

    void trim_symmetric (wstring& str, const wchar_t* skip )
    {
        if (!str.empty()) {
            auto beg = str.find_first_not_of(skip);
            auto end = str.find_last_not_of(skip);

            if (beg != wstring::npos || end != wstring::npos) {
                if (wstring::npos == beg) beg = 0;
                if (wstring::npos == end) end = str.size();
                str = str.substr(beg, end - beg + 1);
            }
        }
    }

    string sub_str (const string& text, int startLine, int startCol, int length)
    {
        string::const_iterator it = text.begin();
        for (int line = 0, col = 0; it != text.end(); ++it, col++)
        {
            if (line == startLine && col == startCol)
                return string(it, length != -1 ? it + length : text.end());
            else if (*it == '\n') 
                { col = -1; line++; }
        }
        return string();
    }

    string sub_str (const string& text, int startLine, int startCol, int endLine, int endCol)
    {
        string buffer;
        string::const_iterator it = text.begin();
        for (int line = 0, col = 0; !(line == endLine && col == endCol) && it != text.end(); ++it, col++)
        {
            if ((line > startLine) || (line == startLine && col >= startCol))
                buffer += *it;
            
            if (*it == '\n') 
                { col = -1; line++; }
        }
        return buffer;
    }

    /////////////////////////////////////////////////////////////////////////

    void Substitutor::ResetContent ()
    {
        m_result.erase();
        m_whatFind.clear(); 
        m_withReplace.clear();
    }

    void Substitutor::AddPair (const char* find, const char* replace)
    {
        m_whatFind.push_back(find); 
        m_withReplace.push_back(replace);
    }

    void Substitutor::AddPair (const string& find, const string& replace)
    {
        m_whatFind.push_back(find); 
        m_withReplace.push_back(replace);
    }

    Substitutor& Substitutor::operator << (const char* input)
    {
        Fastmap<bool> fast_map;

        vector<string>::const_iterator 
            it(m_whatFind.begin()), end(m_whatFind.end());

        if (m_casesensitive)
        {
            for (; it != end; ++it)
                fast_map[(*it)[0]] = true;
        }
        else
        {
            for (; it != end; ++it)
                fast_map[static_cast<unsigned char>(toupper((*it)[0]))] = true;
        }

        const char* chunk_begin = input;
        const char* chunk_end   = input;

        while (*chunk_end)
        {
            if (m_casesensitive && fast_map[*chunk_end]
            || (!m_casesensitive && fast_map[static_cast<unsigned char>(toupper(*chunk_end))])) 
            {
                bool hit = false;
                it = m_whatFind.begin();

                for (int i(0); it != end; ++it, i++) 
                {
                    const string& str = (*it);

                    if ((m_casesensitive && !strncmp(chunk_end, str.c_str(), str.size()))
                    || (!m_casesensitive && !strnicmp(chunk_end, str.c_str(), str.size())))
                    {
                        hit = true;
                        break;
                    }
                }

                if (hit)
                {
                    m_result.append(chunk_begin, chunk_end - chunk_begin);
                    m_result.append(m_withReplace[i].c_str(), m_withReplace[i].size());
                    chunk_end  += m_whatFind[i].size();
                    chunk_begin = chunk_end;
                    continue;
                }
            }
            chunk_end++;
        }

        m_result.append(chunk_begin);

        return *this;
    }

    void date_c_to_oracle (const char* from, string& to)
    {
        Substitutor subst;
        subst.AddPair("%d", "dd"  );
        subst.AddPair("%m", "mm"  );
        subst.AddPair("%B", "month" );
        subst.AddPair("%b", "mon" );
        subst.AddPair("%Y", "yyyy");
        subst.AddPair("%y", "yy"  );
        subst.AddPair("%H", "hh24");
        subst.AddPair("%I", "hh12");
        subst.AddPair("%p", "am"  );
        subst.AddPair("%M", "mi"  );
        subst.AddPair("%S", "ss"  );
        subst << from;
        to = subst.GetResult();
    }

    void date_oracle_to_c (const char* from, string& to)
    {
        Substitutor subst(false);
        subst.AddPair("dd"  , "%d");
        subst.AddPair("mm"  , "%m");
         // 31.03.2003 bug fix, "Month" is not recognized as a valid token for date conversion
        subst.AddPair("month","%B");
        subst.AddPair("mon" , "%b");
        subst.AddPair("yyyy", "%Y");
        subst.AddPair("yy"  , "%y");
        subst.AddPair("rrrr", "%Y");
        subst.AddPair("rr"  , "%y");
        subst.AddPair("hh24", "%H");
        subst.AddPair("hh12", "%I");
        subst.AddPair("am"  , "%p");
        subst.AddPair("mi"  , "%M");
        subst.AddPair("ss"  , "%S");
        subst << from;
        to = subst.GetResult();
    }

    void to_upper_str (const char* from, string& _to)
    {
        string to;
        for (const char* ptr = from; ptr && *ptr; ptr++)
            to += toupper(*ptr);
        _to = to;
    }

    void to_upper_str (const wchar_t* from, wstring& _to)
    {
        wstring to;
        for (const wchar_t* ptr = from; ptr && *ptr; ptr++)
            to += (wchar_t)towupper(*ptr);
        _to = to;
    }

    void to_lower_str (const char* from, string& _to)
    {
        string to;
        for (const char* ptr = from; ptr && *ptr; ptr++)
            to += tolower(*ptr);
        _to = to;
    }

    void make_safe_filename (const char* from, string& to)
    {
        Substitutor subst;
        subst.AddPair("\\","%5c");
        subst.AddPair("/","%2f");
        subst.AddPair(":","%3a");
        subst.AddPair("*","%2a");
        subst.AddPair("?","%3f");
        subst.AddPair("\"","%22");
        subst.AddPair("<","%3c");
        subst.AddPair(">","%3e");
        subst.AddPair("|","%7c");
        subst << from;
        to = subst.GetResult();
    }

    void make_safe_filename (const wchar_t* from, wstring& to)
    {
        string _to;
        make_safe_filename(str(from).c_str(), _to);
        to = wstr(_to);
    }

    void make_safe_filename (const char* from, CString& to)
    {
        string _to;
        make_safe_filename(from, _to);
        to = wstr(_to).c_str();
    }

    void make_safe_filename (const wchar_t* from, CString& to)
    {
        string _to;
        make_safe_filename(str(from).c_str(), _to);
        to = wstr(_to).c_str();
    }

    void filetime_to_string (FILETIME& filetime, CString& str)
    {
        SYSTEMTIME systemTime;
        const int buffLen = 40;
        TCHAR* buff = str.GetBuffer(buffLen);

        FileTimeToLocalFileTime(&filetime, &filetime);
        FileTimeToSystemTime(&filetime, &systemTime);

        int dateLen = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, NULL, buff, buffLen);
        buff[dateLen - 1] = ' ';
        int timeLen = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systemTime, NULL, buff + dateLen, buffLen - dateLen);

        str.ReleaseBuffer(dateLen + timeLen);
    }


    void format_number (CString& str, unsigned int number, const wchar_t* units, bool noZero)
    {
        if (number)
        {
            wostringstream o;

            char separator = ',';//use_facet<numpunct<char> >(o.getloc()).thousands_sep();

            if (unsigned int gb = number/1000000000)
                o << gb << separator << setfill(L'0') << setw(3);
            if (unsigned int mb = (number - number/1000000000*1000000000)/1000000)
                o << mb << separator  << setfill(L'0') << setw(3);
            if (unsigned int kb = (number - number/1000000*1000000)/1000)
                o << kb << separator  << setfill(L'0') << setw(3);
            if (unsigned int b = (number - number/1000*1000)/1)
                o << b;

            o << units;

            str = o.str().c_str();
        }
        else if (!noZero)
        {
            str = "0";
            str += units;
        }
    }

    bool is_blank_line (const char *str, const int len)
    {
        if (!str) return true;

        for (int i = 0; i < len; i++)
            if (!isspace(*(str + i)))
                return false;

        return true;
    }

    bool is_blank_line (const wchar_t *str, const int len)
    {
        if (!str) return true;

        for (int i = 0; i < len; i++)
            if (!iswspace(*(str + i)))
                return false;

        return true;
    }

    string print_time (double seconds)
    {
        ostringstream out;

        int n_hours = int(seconds)/3600;
        int n_minutes = (int(seconds) - n_hours*3600)/60;
        int n_seconds = int(seconds) - n_hours*3600 - n_minutes*60;
        double n_fraction = seconds - n_hours*3600 - n_minutes*60 - n_seconds;
        char s_fraction[20];
        _snprintf(s_fraction, sizeof(s_fraction), "%.3f", n_fraction);
        if (n_hours > 0)
            out << setw(2) << setfill('0') << n_hours
                << ':' << setw(2) << setfill('0') << n_minutes
                << ':' << setw(2) << setfill('0') << n_seconds
                << &s_fraction[1]
                << " (" << seconds << " sec.)";
        else if (n_minutes > 0)
            out << setw(2) << setfill('0') << n_minutes
                << ':' << setw(2) << setfill('0') << n_seconds
                << &s_fraction[1]
                << " (" << seconds << " sec.)";
        else if (n_seconds > 0)
            out << seconds << " sec.";
        else
            out << int(n_fraction * 1000) << " ms";

        return out.str();
    }

    bool get_line (const wchar_t* str, int len, int& position, std::wstring& line, bool& cr)
    {
        bool retVal = false;
    
        line.erase();
        cr = false;

        while (position < len && str[position])
        {
            retVal = true;

            switch (str[position])
            {
            case '\r':
                if (position + 1 < len &&  str[position + 1] == '\n') 
                    position++;
            case '\n':
                position++;
                cr = true;
                break;
            }

            if (cr) break;

            line += str[position++];
        }

        return retVal;
    }
}// END namespace Common
