/* 
    Copyright (C) 2003,2005 Aleksey Kochetov

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
    14.06.2005 the implementation has been moved into Common namespace
*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEPlSqlParser_h__
#define __OEPlSqlParser_h__

#include <COMMON/PlSqlParser.h>


namespace OpenEditor
{
    using namespace Common::PlsSql;

    class PlSqlParser : public Common::PlsSql::PlSqlParser
    {
    public:
        PlSqlParser (SyntaxAnalyser&);
        
        static const char* GetStringToken (EToken token);

        static bool IsPlSqlToken (EToken token);
        static bool IsScriptToken (EToken token);
    };

};

#endif//__OEPlSqlParser_h__
