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
#ifndef __DlgDataExt_h__
#define __DlgDataExt_h__

void DDX_Check      (CDataExchange* pDX, int nIDC, bool& value);
void DDX_Text       (CDataExchange* pDX, int nIDC, std::string& value);
void DDX_Text       (CDataExchange* pDX, int nIDC, std::wstring& value);
void DDV_MaxChars   (CDataExchange* pDX, const std::string& value, int nChars);
void DDV_MaxChars   (CDataExchange* pDX, const std::wstring& value, int nChars);
void DDX_CBString   (CDataExchange* pDX, int nIDC, std::wstring& _value);

#endif//__DlgDataExt_h__
