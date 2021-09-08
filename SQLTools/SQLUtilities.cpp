/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2014 Aleksey Kochetov

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
#include "SQLUtilities.h"
#include "SQLTools.h"
#include <OpenEditor/OELanguage.h> // for SQL keywords
#include "OpenEditor/OEDocument.h" // for OpenEditor::SettingsManager

using namespace OpenEditor;

namespace SQLUtilities
{

    string GetSafeDatabaseObjectName (const string& name, bool forceLower)
    {
        string ret_val;

        if (!name.empty())
        {
            bool regular = true;
            bool is_public = false;
    
            if (name.at(0) < 'A' || name.at(0) > 'Z') 
            {
                regular = false;
            } 
            else 
            {
                string::const_iterator it = name.begin();
                for (; it  != name.end(); ++it)
                {
                    if (!((*it >= 'A' && *it <= 'Z') 
                    || (*it >= '0' && *it <= '9') 
                    || *it == '_' || *it == '$' || *it == '#')) 
                    {
                        regular = false;
                        break;
                    }
                }
            }

            if (regular) 
            {
                static LanguageKeywordMapPtr s_languageKeywordMap;

                try
                {
                    if (!s_languageKeywordMap.get())
                        s_languageKeywordMap = LanguagesCollection::Find("PL/SQL")->GetLanguageKeywordMap();
                }
                catch (const std::logic_error&)
                {
                    _RAISE(std::logic_error("Text output stream error. Cannot get PL/SQL language support."));
                }

                LanguageKeywordMapConstIterator it = s_languageKeywordMap->find(Common::wstr(name));
                if (it != s_languageKeywordMap->end() && it->second.groupIndex == 0) // only for SQL Keywords
                {
                    is_public = name == "PUBLIC";
                    regular = false;
                }
            }

            if (regular || is_public) 
            {
                if ((forceLower || GetSQLToolsSettings().m_LowerNames) && !is_public) 
                {
                    for (string::const_iterator it = name.begin(); it != name.end(); ++it)
                        ret_val.push_back(static_cast<char>(tolower(*it)));
                } 
                else
                    ret_val = name;
            } 
            else
            {
                ret_val = '\"' + name + '\"';
            }
        }
        return ret_val;
    }

    string GetSafeDatabaseObjectFullName (const string& schema, const string& name)
    {
        string ret_val;

        if (GetSQLToolsSettings().m_ShemaName && !schema.empty())
        {
            ret_val = GetSafeDatabaseObjectName(schema);
            ret_val += '.';
        }
        ret_val += GetSafeDatabaseObjectName(name);

        return ret_val;
    }

}
