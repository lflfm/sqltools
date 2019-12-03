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

#pragma once
#ifndef __SIMPLEDRAGDATASOURCE_H__
#define __SIMPLEDRAGDATASOURCE_H__
#include <afxole.h>

namespace Common
{

    class SimpleDragDataSource : public COleDataSource
    {
        CString m_strBuffer;
    public:
        SimpleDragDataSource (const char*);
        virtual BOOL OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal);
    };

    class SimpleDragDataSourceExt : public SimpleDragDataSource
    {
        CString m_strPrivateText;
        CLIPFORMAT m_clipFormat;

    public:
        SimpleDragDataSourceExt (const char*, const char*, CLIPFORMAT = CF_PRIVATEFIRST);
        virtual BOOL OnRenderGlobalData (LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal);
    };

}

#endif//__SIMPLEDRAGDATASOURCE_H__
