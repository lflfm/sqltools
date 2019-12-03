/* 
    Copyright (C) 2002 Aleksey Kochetov

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
#include "OpenEditor/OELanguage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OpenEditor
{
    
void Language::SetLanguageKeywordMap (LanguageKeywordMapPtr keywordMap) 
{ 
    m_LanguageKeywordMap->clear(); 

    LanguageKeywordMapConstIterator it = keywordMap->begin();
    LanguageKeywordMapConstIterator end = keywordMap->end();

    for (it; it != end; ++it)
    {
        ASSERT(it->second.groupIndex < (int)m_keywordGroups.size());
        if (it->second.groupIndex < (int)m_keywordGroups.size())
            m_LanguageKeywordMap->insert(pair<string, LanguageKeyword>(it->first, it->second));
    }
}

    vector<LanguagePtr> LanguagesCollection::m_Languages;

const LanguagePtr LanguagesCollection::Find (const string& name)
{
    ASSERT_EXCEPTION_FRAME;

    std::vector<LanguagePtr>::const_iterator it = m_Languages.begin();
    for (; it != m_Languages.end(); ++it)
        if ((*it)->GetName() == name)
            return *it;

    THROW_APP_EXCEPTION(string("Language \"") + name + "\" not found.");
}

vector<string> LanguagesCollection::GetAll ()
{
    vector<string> names;

    std::vector<LanguagePtr>::const_iterator it = m_Languages.begin();
    for (; it != m_Languages.end(); ++it)
        names.push_back((*it)->GetName());

    return names;
}

void LanguagesCollection::Update (LanguagePtr lang)
{
    std::vector<LanguagePtr>::iterator it = m_Languages.begin();
    for (; it != m_Languages.end(); ++it)
        if ((*it)->GetName() == lang->GetName())
        {
            *it = lang;
            return;
        }

    m_Languages.push_back(lang);
}

void LanguagesCollection::Delete (const char* name)
{
    std::vector<LanguagePtr>::iterator it = m_Languages.begin();
    for (; it != m_Languages.end(); ++it)
        if ((*it)->GetName() == name)
        {
            m_Languages.erase(it);
            return;
        }
}

};//namespace OpenEditor
