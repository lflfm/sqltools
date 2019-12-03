/* 
    Copyright (C) 2003-2015 Aleksey Kochetov

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

#include "Common/FixedArray.h"
#include "OpenEditor/OEPlsSqlSyntax.h"

namespace OpenEditor
{ 
    using Common::FixedArray;
    typedef FixedArray<Token, 7> RecognitionPattern;

    struct ScriptFactory
    {
        // returns false if recognition is not possible anymore 
        static void RecognizeAndCreate (const RecognitionPattern&, std::auto_ptr<SyntaxNode>&);
        static void Create (const RecognitionPattern&, std::auto_ptr<SyntaxNode>&);
        static void feed (const RecognitionPattern&, SyntaxNode&);
    };
    
    struct DmlSqlFactory
    {
        static void RecognizeAndCreate (const Token&, std::auto_ptr<SyntaxNode>&);
    };

    struct PlSqlFactory
    {
        static void RecognizeAndCreate (const Token&, std::auto_ptr<SyntaxNode>&);
    };

    typedef DmlSqlFactory DdlSqlFactory;
    typedef DmlSqlFactory ExpressionFactory;
};
