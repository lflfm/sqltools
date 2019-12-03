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

// 13.03.2005 (ak) R1105003: Bind variables

#pragma once
#ifndef __BindVariableTypes_h__
#define __BindVariableTypes_h__

enum EBindVariableTypes
{
    EBVT_UNKNOWN = 0, // for initialization
    EBVT_VARCHAR2,
    EBVT_NUMBER,
    EBVT_CLOB,
    EBVT_DATE,
    EBVT_REFCURSOR
};

#endif//__BindVariableTypes_h__