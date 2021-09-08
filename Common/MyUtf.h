#pragma once

#include <string>

namespace Common
{

std::wstring wstr (const std::string& str);
std::wstring wstr (const char* str);
std::wstring wstr (const char* str, int len);
std::wstring wstr_from_ansi (const std::string& str);

std::string str (const std::wstring& wstr);
std::string str (const wchar_t* wstr);
std::string str (const wchar_t* wstr, int len);
std::string str (const CString& wstr);

inline
bool is_right_to_left_lang (wchar_t ch)
{
    return 
        0x0590  <= ch && ch <= 0x05FF  // Hebrew
    || 0x0600  <= ch && ch <= 0x06FF  // Arabic
    || 0x0750  <= ch && ch <= 0x077F  // Arabic Supplement
    || 0xFB50  <= ch && ch <= 0xFDFF  // Arabic Presentation forms-A
    || 0xFE70  <= ch && ch <= 0xFEFF  // Arabic Presentation forms-B
    || 0x103A0 <= ch && ch <= 0x103DF // Old Persian
    ;
}

bool is_combining_implementation (wchar_t ch);

};