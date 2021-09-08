#include "stdafx.h"
#include <codecvt>
#include "MyUtf.h"

namespace Common
{

std::wstring wstr (const std::string& str)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

std::wstring wstr (const char* str)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(std::string(str));
}

std::wstring wstr (const char* str, int len)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(std::string(str, len));
}

std::wstring wstr_from_ansi (const std::string& str)
{
    int length = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), 0, 0);
    auto wbuffer = std::make_unique<wchar_t[]>(length+1);
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), wbuffer.get(), length);
    wbuffer[length] = 0;
    return std::wstring(wbuffer.get(), length);
}


std::string str (const std::wstring& wstr)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

std::string str (const wchar_t* wstr)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(std::wstring(wstr));
}

std::string str (const wchar_t* wstr, int len)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(std::wstring(wstr, len));
}

std::string str (const CString& cstr)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(std::wstring(cstr));
}

bool is_combining_implementation (wchar_t ch)
{
    static const wchar_t combining_ranges[] = 
    { 
        0x0300, 0x0361, 
        0x0483, 0x0486, 
        0x0903, 0x0903, 
        0x093E, 0x0940, 
        0x0949, 0x094C,
        0x0982, 0x0983,
        0x09BE, 0x09C0,
        0x09C7, 0x09CC,
        0x09D7, 0x09D7,
        0x0A3E, 0x0A40,
        0x0A83, 0x0A83,
        0x0ABE, 0x0AC0,
        0x0AC9, 0x0ACC,
        0x0B02, 0x0B03,
        0x0B3E, 0x0B3E,
        0x0B40, 0x0B40,
        0x0B47, 0x0B4C,
        0x0B57, 0x0B57,
        0x0B83, 0x0B83,
        0x0BBE, 0x0BBF,
        0x0BC1, 0x0BCC,
        0x0BD7, 0x0BD7,
        0x0C01, 0x0C03,
        0x0C41, 0x0C44,
        0x0C82, 0x0C83,
        0x0CBE, 0x0CBE,
        0x0CC0, 0x0CC4,
        0x0CC7, 0x0CCB,
        0x0CD5, 0x0CD6,
        0x0D02, 0x0D03,
        0x0D3E, 0x0D40,
        0x0D46, 0x0D4C,
        0x0D57, 0x0D57,
        0x0F7F, 0x0F7F,
        0x20D0, 0x20E1, 
        0x3099, 0x309A,
        0xFE20, 0xFE23, 
        0xffff, 0xffff, 
    };

    const wchar_t* p = combining_ranges + 1;
   
    while(*p < ch) 
       p += 2;

    --p;

    if((ch >= *p) && (ch <= *(p+1)))
        return true;
   
    return false;
}

};