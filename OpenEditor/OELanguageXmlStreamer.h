/* 
    Copyright (C) 2005 Aleksey Kochetov

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
#include "OpenEditor/OELanguage.h"

    class TiXmlElement;                                                 

namespace OpenEditor
{
    ///////////////////////////////////////////////////////////////////////////
    // LanguageXmlStreamer
    ///////////////////////////////////////////////////////////////////////////

    class LanguageXmlStreamer
    {
    public:
        LanguageXmlStreamer (const std::string& filename) : m_filename(filename) {}

        void operator >> (LanguagesCollection&);
        void operator << (const LanguagesCollection&);

    private:
        void read  (const TiXmlElement*, Language&);
        void read  (const TiXmlElement*, LanguageKeywordMapPtr&, vector<string>&, bool caseSensiteve);

        void write (const Language&, TiXmlElement*);
        void write (const LanguageKeywordMapPtr&, const vector<string>&, TiXmlElement*);

        const std::string m_filename;
    };

};//namespace OpenEditor

