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
#ifndef __BindParser_h__
#define __BindParser_h__

#include "PlsSqlParser.h"

/** @brief Class for identifying bind variables in SQL.
 */
class BindParser : PlsSqlParser
{
public:
	BindParser (SyntaxAnalyser* analyser, TokenMapPtr tokenMap) : PlsSqlParser (analyser, tokenMap) {};
    bool PutLine (int line, const char*, int length);
};

#endif//__PlsSqlParser_h__
