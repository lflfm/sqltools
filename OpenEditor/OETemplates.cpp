/*
    Copyright (C) 2002, 2017 Aleksey Kochetov

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

// 17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report
// 2017-12-25 added uuid to template entry in order to merge the file with external changes

#include "stdafx.h"
#include <algorithm>
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OETemplates.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

namespace OpenEditor
{

///////////////////////////////////////////////////////////////////////////////
// Template

bool Template::ExpandKeyword (int index, string& text, Position& pos)
{
    if (index >= 0 && index < static_cast<int>(m_entries.size()))
    {
        text        = m_entries[index].text;
        pos.line    = m_entries[index].curLine;
        pos.column  = m_entries[index].curPos;
        
        return AfterExpand(m_entries[index], text, pos);
    }
    return false;
}

bool Template::ExpandKeyword (const string& keyword, string& text, Position& pos)
{
    std::vector<Entry>::const_iterator it = m_entries.begin();

    bool matched = false, alternative = false;
    Entry entry;

    for (; it != m_entries.end(); ++it)
    {
        bool is_eql = !strncmp(it->keyword.c_str(), keyword.c_str(), keyword.size());

        if (is_eql && alternative
        && m_alwaysListIfAlternative)
            return false;

        if (!matched && is_eql
        &&  it->minLength <= static_cast<int>(keyword.size()))
        {
            entry = *it;
            matched = true;

            if (!m_alwaysListIfAlternative)
                break;
        }

        alternative |= is_eql;
    }

    if (matched)
    {
        text        = entry.text;
        pos.line    = entry.curLine;
        pos.column  = entry.curPos;
        AfterExpand(entry, text, pos);
    }
    return matched;;
}


void Template::GenUUID (Entry& entry)
{
    UUID uuid;
    UuidCreate(&uuid);
    char* pszUuid;
    UuidToStringA(&uuid, (RPC_CSTR*)&pszUuid);
    entry.uuid = pszUuid;
    RpcStringFreeA((RPC_CSTR*)&pszUuid);
}

void Template::AppendEntry (const Entry& entry)
{
    m_entries.push_back(entry);
    m_entries.rbegin()->modified = true;
    SetModified(true);
}

void Template::UpdateEntry (int pos, const Entry& entry)
{
    m_entries[pos] = entry;
    m_entries[pos].modified = true;
    SetModified(true);
}

void Template::DeleteEntry (int pos)
{
    m_entries[pos].modified = true;
    m_entries[pos].deleted  = true;
    SetModified(true);
}

bool Template::ValidateUniqueness (const string& name, const string& uuid) const
{
    std::vector<Entry>::const_iterator it = m_entries.begin();
    for (; it != m_entries.end(); ++it)
        if (it->name == name && it->uuid != uuid)
            return false;
    return true;
}

    static bool is_deleted (const Template::Entry& entry) { return entry.deleted; }

void Template::ClearModified () const
{
    std::vector<Entry>::const_iterator it = m_entries.begin();
    for (; it != m_entries.end(); ++it)
        it->modified = false;

    std::remove_if(m_entries.begin(), m_entries.end(), is_deleted);

    SetModified(false);
}

static int _least (const Template::Entry& e1, const Template::Entry& e2)
{
    return e1.name < e2.name;
}

///////////////////////////////////////////////////////////////////////////////
// TemplateCollection

TemplateCollection::TemplateCollection (const TemplateCollection& other)
{
    *this = other;
}

TemplateCollection& TemplateCollection::operator = (const TemplateCollection& other)
{
    if (&other != this)
    {
        m_templates.clear();

        std::map<string, TemplatePtr>::const_iterator
            it = other.m_templates.begin(),
            end = other.m_templates.end();

        for (it; it != end; ++it)
        {
            *(Add(it->first)) = *(it->second);
        }

    }

    return *this;
}

TemplatePtr TemplateCollection::Add (const string& lang)
{
    ASSERT_EXCEPTION_FRAME;

    TemplatePtr ptr(new Template);

    if (m_templates.find(lang) == m_templates.end())
        m_templates.insert(std::map<string, TemplatePtr>::value_type(lang, ptr));
    else
        THROW_APP_EXCEPTION("Template \"" + lang + "\" already exist.");

    return ptr;
}

const TemplatePtr TemplateCollection::Find (const string& lang) const
{
    ASSERT_EXCEPTION_FRAME;

    ConstIterator it = m_templates.find(lang);

    if (it == m_templates.end())
        THROW_APP_EXCEPTION("Template \"" + lang + "\" not found.");

    return it->second;
}

bool TemplateCollection::IsModified () const
{
    std::map<string, TemplatePtr>::const_iterator
        it = m_templates.begin(),
        end = m_templates.end();

    for (it; it != end; ++it)
        if (it->second->GetModified())
            return true;

    return false;
}

void TemplateCollection::ClearModified () const
{
    std::map<string, TemplatePtr>::const_iterator
        it = m_templates.begin(),
        end = m_templates.end();

    for (it; it != end; ++it)
        it->second->ClearModified();
}

};//namespace OpenEditor
