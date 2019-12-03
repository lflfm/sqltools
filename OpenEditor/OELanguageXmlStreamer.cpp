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

// TODO: error handling

#include "stdafx.h"
#include <sstream>
#include "TinyXml\TinyXml.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "COMMON/AppUtilities.h"
#include "OpenEditor\OELanguageXmlStreamer.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

namespace OpenEditor
{
    using namespace Common;

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 1; // any minor changes

    // write helper procedures
    inline void writeXml (TiXmlElement* elem, const char* name, int val) { 
        elem->SetAttribute(name, val); 
    }
    inline void writeXml (TiXmlElement* elem, const char* name, bool val) { 
        elem->SetAttribute(name, val); 
    }
    inline void writeXml (TiXmlElement* elem, const char* name, const string& val) { 
        elem->SetAttribute(name, val.c_str()); 
    }
    template <class TCollection> 
    void writeXml (TiXmlElement* parentElem, const char* elemName, TCollection collection) {
        TCollection::const_iterator it = collection.begin();
        for (; it != collection.end(); ++it) {
            std::auto_ptr<TiXmlElement> elem(new TiXmlElement(elemName));
            writeXml(elem.get(), "value", *it);
            parentElem->LinkEndChild(elem.release());
        }
    }
    // read helper procedures
    inline void readXml (const TiXmlElement* elem, const char* name, int& val) {
        if (elem->QueryIntAttribute(name, &val) != TIXML_SUCCESS)
            val = 0;
    }
    inline void readXml (const TiXmlElement* elem, const char* name, bool& val) {
        int _val;
        val = (elem->QueryIntAttribute(name, &_val) == TIXML_SUCCESS) ? _val : false;
    }
    inline void readXml (const TiXmlElement* elem, const char* name, string& val) {
        if (const char* _val = elem->Attribute(name))
            val = _val;
    }
    inline void readXml (const TiXmlElement* parentElem, const char* elemName, set<string>& collection) {
        const TiXmlElement* elem = parentElem->FirstChildElement(elemName);
        for (; elem; elem = elem->NextSiblingElement(elemName))
            if (const char* _val = elem->Attribute("value"))
                collection.insert(_val);
    }
    inline void readXml (const TiXmlElement* parentElem, const char* elemName, vector<string>& collection) {
        const TiXmlElement* elem = parentElem->FirstChildElement(elemName);
        for (; elem; elem = elem->NextSiblingElement(elemName))
            if (const char* _val = elem->Attribute("value"))
                collection.push_back(_val);
    }


/////////////////////////////////////////////////////////////////////////////
// LanguageXmlStreamer

void LanguageXmlStreamer::operator << (const LanguagesCollection&)
{
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", ""/*"Windows-1252"*/, "yes");
    doc.InsertEndChild(decl);

    std::auto_ptr<TiXmlElement> rootElem(new TiXmlElement("OpenEditor"));
    std::auto_ptr<TiXmlElement> langsElem(new TiXmlElement("Languages"));
    writeXml(langsElem.get(), "MajorVersion", MAJOR_VERSION);
    writeXml(langsElem.get(), "MinorVersion", MINOR_VERSION);
    
    std::vector<LanguagePtr>::const_iterator it = LanguagesCollection::m_Languages.begin();
    for (; it != LanguagesCollection::m_Languages.end(); ++it)
    {
        std::auto_ptr<TiXmlElement> langElem(new TiXmlElement("Language"));
        write(**it, langElem.get());
        langsElem->LinkEndChild(langElem.release());
    }

    rootElem->LinkEndChild(langsElem.release());
    doc.LinkEndChild(rootElem.release());

    if (!doc.SaveFile(m_filename.c_str()))
        throw Common::AppException("Cannot save \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());
}

void LanguageXmlStreamer::write (const Language& lang, TiXmlElement* langElem)
{
    writeXml(langElem, "Name",              lang.m_name);
    writeXml(langElem, "Delimiters",        lang.m_delimiters);
    writeXml(langElem, "CaseSensiteve",     lang.m_caseSensiteve);
    writeXml(langElem, "CommentPairFirst",  lang.m_commentPair.first );
    writeXml(langElem, "CommentPairLast",   lang.m_commentPair.second);
    writeXml(langElem, "EndLineComment",    lang.m_endLineComment);
    writeXml(langElem, "EndLineCommentCol", lang.m_endLineCommentCol);
    writeXml(langElem, "EscapeChar",        lang.m_escapeChar);
    writeXml(langElem, "StringPairFirst",   lang.m_stringPair.first );
    writeXml(langElem, "StringPairLast",    lang.m_stringPair.second);
    writeXml(langElem, "CharPairFirst",     lang.m_charPair.first );
    writeXml(langElem, "CharPairLast",      lang.m_charPair.second);

    std::auto_ptr<TiXmlElement> comElem(new TiXmlElement("StartLineComments"));
    writeXml(comElem.get(), "StartLineComment", lang.m_startLineComment);
    langElem->LinkEndChild(comElem.release());

    std::auto_ptr<TiXmlElement> setsElem(new TiXmlElement("MatchBraceSets"));
    vector<vector<string> >::const_iterator it = lang.m_matchBraces.begin();
    for (; it != lang.m_matchBraces.end(); ++it)
    {
        std::auto_ptr<TiXmlElement> setElem(new TiXmlElement("MatchBraceSet"));
        writeXml(setElem.get(), "Brace", *it);
        setsElem->LinkEndChild(setElem.release());
    }
    langElem->LinkEndChild(setsElem.release());

    write(lang.m_LanguageKeywordMap, lang.m_keywordGroups, langElem);
}


void LanguageXmlStreamer::write (const LanguageKeywordMapPtr& keywordMap, const vector<string>& groups, TiXmlElement* langElem)
{
    std::auto_ptr<TiXmlElement> keywordGroupsElem(new TiXmlElement("KeywordGroups"));

    std::vector<string>::const_iterator groupIt = groups.begin();
    for (int groupIndex(0); groupIt != groups.end(); ++groupIt, groupIndex++)
    {
        std::auto_ptr<TiXmlElement> keywordGroupElem(new TiXmlElement("KeywordGroup"));
        writeXml(keywordGroupElem.get(), "Name", *groupIt);

        std::ostringstream out;
        LanguageKeywordMapConstIterator it = keywordMap->begin();
        for (int keywordIndex = 0; it != keywordMap->end(); ++it)
            if (groupIndex == it->second.groupIndex)
            {
                if (keywordIndex) out << ' ';
                out << it->second.keyword;
                keywordIndex++;
            }

        std::auto_ptr<TiXmlText> text(new TiXmlText(out.str().c_str()));
        keywordGroupElem->LinkEndChild(text.release());
        
        keywordGroupsElem->LinkEndChild(keywordGroupElem.release());
    }

    langElem->LinkEndChild(keywordGroupsElem.release());
}

void LanguageXmlStreamer::operator >> (LanguagesCollection&)
{
    LanguagesCollection::m_Languages.clear();

    TiXmlDocument doc;

    if (::PathFileExists(m_filename.c_str()))
    {
        if (!doc.LoadFile(m_filename.c_str()))
            THROW_APP_EXCEPTION("Cannot parse \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());
    }
    else // no problem - will load settings from resources
    {
		string xmlText;
		if (!AppLoadTextFromResources(::PathFindFileName(m_filename.c_str()), RT_HTML, xmlText) || xmlText.empty())
            THROW_APP_EXCEPTION("Cannot load language settings from resources");

        if (!doc.Parse(xmlText.c_str()))
            THROW_APP_EXCEPTION(string("Cannot parse language settings\nERROR: ") + doc.ErrorDesc());
    }

    if (const TiXmlNode* rootElem = doc.FirstChildElement("OpenEditor"))
    {
        if (const TiXmlElement* langsElem = rootElem->FirstChildElement("Languages"))
        {
            int majorVer = -1;
            readXml(langsElem, "MajorVersion", majorVer);
            if (majorVer != MAJOR_VERSION)
                throw Common::AppException("Cannot load \""  + m_filename + "\".\nERROR: Unsupported languages version.");

            const TiXmlElement* langElem = langsElem->FirstChildElement("Language");
            for (; langElem; langElem = langElem->NextSiblingElement("Language"))
            {
                LanguagePtr lang = new Language;
                read(langElem, *lang);
                LanguagesCollection::m_Languages.push_back(lang);
            }
        }
    }
}

void LanguageXmlStreamer::read (const TiXmlElement* langElem, Language& lang)
{
    readXml(langElem, "Name",              lang.m_name);
    readXml(langElem, "Delimiters",        lang.m_delimiters);
    readXml(langElem, "CaseSensiteve",     lang.m_caseSensiteve);
    readXml(langElem, "CommentPairFirst",  lang.m_commentPair.first );
    readXml(langElem, "CommentPairLast",   lang.m_commentPair.second);
    readXml(langElem, "EndLineComment",    lang.m_endLineComment);
    readXml(langElem, "EndLineCommentCol", lang.m_endLineCommentCol);
    readXml(langElem, "EscapeChar",        lang.m_escapeChar);
    readXml(langElem, "StringPairFirst",   lang.m_stringPair.first );
    readXml(langElem, "StringPairLast",    lang.m_stringPair.second);
    readXml(langElem, "CharPairFirst",     lang.m_charPair.first );
    readXml(langElem, "CharPairLast",      lang.m_charPair.second);

    lang.m_startLineComment.clear();
    if (const TiXmlElement* comElem = langElem->FirstChildElement("StartLineComments"))
        readXml(comElem, "StartLineComment", lang.m_startLineComment);

    lang.m_matchBraces.clear();
    if (const TiXmlElement* setsElem = langElem->FirstChildElement("MatchBraceSets"))
    {
        const TiXmlElement* setElem = setsElem->FirstChildElement("MatchBraceSet");
        for (; setElem; setElem = setElem->NextSiblingElement("MatchBraceSet"))
        {
            lang.m_matchBraces.push_back(vector<string>());
            readXml(setElem, "Brace", *lang.m_matchBraces.rbegin());
        }
    }

    read(langElem, lang.m_LanguageKeywordMap, lang.m_keywordGroups, lang.m_caseSensiteve);

    if (lang.GetName() == "PL/SQL")
    {
        vector<string>::const_iterator it = lang.m_keywordGroups.begin();
        for (; it != lang.m_keywordGroups.end(); ++it)
            if (*it == "User Objects")
                break;

        if (it == lang.m_keywordGroups.end())
            lang.m_keywordGroups.push_back("User Objects");

        {
        string::size_type pos = lang.m_delimiters.find('@');
        if (pos != string::npos)
            lang.m_delimiters.erase(pos, 1);
        }
    }
    else if (lang.GetName() == "XML")
    {
        // 2018-01-04 bug fix, broken XML highlighting after "<!-- anything-->"
        if (lang.m_delimiters.find('-') == string::npos)
            lang.m_delimiters += '-';
    }
}

void LanguageXmlStreamer::read (const TiXmlElement* langElem, LanguageKeywordMapPtr& keywordMap, vector<string>& keywordGroups, bool caseSensiteve)
{
    keywordGroups.clear();
    keywordMap->clear();

    if (const TiXmlElement* keywordGroupsElem = langElem->FirstChildElement("KeywordGroups"))
    {
        const TiXmlElement* keywordGroupElem = keywordGroupsElem->FirstChildElement("KeywordGroup");
        for (; keywordGroupElem; keywordGroupElem = keywordGroupElem->NextSiblingElement("KeywordGroup"))
        {
            if (const TiXmlNode* textNode = keywordGroupElem->FirstChild())
            {
                if (textNode->Type() == TiXmlNode::TINYXML_TEXT)
                {
                    int groupIndex = keywordGroups.size();
                    keywordGroups.push_back(keywordGroupElem->Attribute("Name"));
                    
                    std::string buffer = textNode->Value();
                    std::string::iterator it = buffer.begin();
                    for (; it != buffer.end(); ++it)
                        switch (*it) {
                        case '\n': 
                        case '\r': 
                            *it = ' ';
                        }

                    std::istringstream in(buffer);

                    string key;
                    LanguageKeyword keyword;
                    while (getline(in, keyword.keyword, ' '))
                    {
                        keyword.groupIndex = groupIndex;
                        key = keyword.keyword;

                        trim_symmetric(key);

                        if (!key.empty())
                        {
                            if (!caseSensiteve)
                                for (string::iterator it = key.begin(); it != key.end(); ++it)
                                    *it = toupper(*it);

                            if (keywordMap->find(key) == keywordMap->end())
                                keywordMap->insert(pair<string, LanguageKeyword>(key, keyword));
                            else
                                TRACE1("\tWARNING: skip duplicated keyword \"%s\"\n", key.c_str());
                        }
                    }
                }
            }
        }
    }
}

};//namespace OpenEditor

