/*
    Copyright (C) 2004 Aleksey Kochetov

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
#include <COMMON\SimpleDragDataSource.h>
#include <COMMON\MyUtf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Common
{

SimpleDragDataSource::SimpleDragDataSource (const wchar_t* str)
{
    m_strBuffer = str;
    DelayRenderData(CF_UNICODETEXT);
}

SimpleDragDataSource::SimpleDragDataSource (const char* str)
{
    m_strBuffer = wstr(str).c_str();
    DelayRenderData(CF_UNICODETEXT);
}

BOOL SimpleDragDataSource::OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
{
    if (lpFormatEtc->cfFormat == CF_UNICODETEXT)
    {
        bool bShift   = 0xFF00 & GetKeyState(VK_SHIFT) ? true : false;
        bool bControl = 0xFF00 & GetKeyState(VK_CONTROL) ? true : false;

        if (bShift)
            m_strBuffer += ',';
        if (bShift && !bControl)
            m_strBuffer += ' ';
        if (bControl)
            m_strBuffer += '\n';

        size_t size = sizeof(TCHAR) * (m_strBuffer.GetLength() + 1);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size);
        if (hGlobal)
        {
            if (void* buff = GlobalLock(hGlobal))
            {
                memcpy(buff, (LPCTSTR)m_strBuffer, size);
                GlobalUnlock(hGlobal);
                *phGlobal = hGlobal;
                return TRUE;
            }
            GlobalFree(hGlobal);
        }
    }
    return FALSE;
}

//SimpleDragDataSourceExt::SimpleDragDataSourceExt (const char* str, const char* strPivateText, CLIPFORMAT clipFormat)
//    : SimpleDragDataSource (str),
//    m_clipFormat(clipFormat)
//{
//    m_strPrivateText = strPivateText;
//    DelayRenderData(clipFormat);
//}
//
//BOOL SimpleDragDataSourceExt::OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
//{
//    if (lpFormatEtc->cfFormat == m_clipFormat)
//    {
//        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, m_strPrivateText.GetLength() + 1);
//        if (hGlobal)
//        {
//            if (char* buff = (char*)GlobalLock(hGlobal))
//            {
//                strcpy(buff, m_strPrivateText);
//                GlobalUnlock(hGlobal);
//                *phGlobal = hGlobal;
//                return TRUE;
//            }
//            GlobalFree(hGlobal);
//        }
//    }
//    
//    return SimpleDragDataSource::OnRenderGlobalData(lpFormatEtc, phGlobal);
//}

}//namespace Common
