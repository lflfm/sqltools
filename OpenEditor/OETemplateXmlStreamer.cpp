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

// 2017-12-25 reimplemented save procedure based on uuid in order to merge deleted and renames entries properly


#include "stdafx.h"
#include <COMMON/ExceptionHelper.h>
#include "COMMON/AppUtilities.h"
#include "OpenEditor\OETemplateXmlStreamer.h"
#include "OpenEditor\OESettings.h"
#include "TinyXml\TinyXml.h"
#include "COMMON/MyUtf.h"
#include "TiXmlUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OpenEditor
{
    using Common::AppLoadTextFromResources;

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 2; // any minor changes

    static const int LOCK_TIMEOUT = 1000;

    // helper procedures
    inline void writeXml (TiXmlElement* elem, const char* name, int val) { 
        elem->SetAttribute(name, val);
    }
    inline void writeXml (TiXmlElement* elem, const char* name, bool val) { 
        elem->SetAttribute(name, val); 
    }
    inline void writeXml (TiXmlElement* elem, const char* name, const string& val) { 
        elem->SetAttribute(name, val.c_str()); 
    }
    inline void readXml (const TiXmlElement* elem, const char* name, int& val) {
        if (elem->QueryIntAttribute(name, &val) != TIXML_SUCCESS)
            val = 0;
    }
    inline void readXml (const TiXmlElement* elem, const char* name, bool& val) {
        int _val;
        val = (elem->QueryIntAttribute(name, &_val) == TIXML_SUCCESS) ? _val : false;
    }
    inline void readXml (const TiXmlElement* elem, const char* name, string& val) {
        val = elem->Attribute(name);
    }

/////////////////////////////////////////////////////////////////////////////

TemplateXmlStreamer::TemplateXmlStreamer (const std::wstring& filename) 
    : m_filename(filename),
    m_fileAccessMutex(FALSE, L"GNU.OpenEditor.Languges")
{
}

    static
    TiXmlElement* getNodeSafe (TiXmlNode* parentElem, const char* name)
    {
        TiXmlElement* elem = parentElem->FirstChildElement(name);
        if (!elem)
        {
            elem = new TiXmlElement(name);
            parentElem->LinkEndChild(elem);
        }
        return elem;
    }
    static 
    TiXmlElement* findNode (TiXmlElement* parent, const char* name, const char* attr, const char* value)
    {
        TiXmlElement* elem = parent->FirstChildElement(name);

        while (elem) 
        {
            if (const char* val = elem->Attribute(attr))
                if (!stricmp(value, val))
                    return elem;
            
            elem = elem->NextSiblingElement(name);
        }

        return 0;
    }
    static 
    TiXmlElement* getNodeSafe (TiXmlElement* parent, const char* name, const char* attr, const char* value)
    {
        TiXmlElement* elem = findNode(parent, name, attr, value);
        
        if (!elem)
        {
            elem = new TiXmlElement(name);
            parent->LinkEndChild(elem);
        }
        return elem;
    }

/*
    The method reads and parses the file and then overwrites only known settings
    keeeping any extention unchanged. 
    In theory it allows two different versions of the program 
    to use the same configuration file.
*/
void TemplateXmlStreamer::operator << (const SettingsManager& mgr)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION("Cannot get exclusive access to \"" + Common::str(m_filename) + '"');

    TiXmlBase::SetCondenseWhiteSpace(false);

    TiXmlDocument doc;
    if (!::PathFileExists(m_filename.c_str()))
        doc.InsertEndChild(TiXmlDeclaration("1.0", ""/*"Windows-1252"*/, "yes"));
    else if (!TiXmlUtils::LoadFile(doc, m_filename.c_str()))
        THROW_APP_EXCEPTION("Cannot read \"" + Common::str(m_filename) + "\"\nERROR: " + doc.ErrorDesc());

    TiXmlNode* rootElem = getNodeSafe(&doc, "OpenEditor");
    TiXmlElement* templsElem = getNodeSafe(rootElem, "Templates");

    int majorVer;
    if (templsElem->QueryIntAttribute("MajorVersion", &majorVer) != TIXML_SUCCESS)
        majorVer = MAJOR_VERSION;
    if (majorVer != MAJOR_VERSION)
        THROW_APP_EXCEPTION("Cannot load \""  + Common::str(m_filename) + "\".\nERROR: Unsupported settings version.");

    writeXml(templsElem, "MajorVersion", MAJOR_VERSION);
    writeXml(templsElem, "MinorVersion", MINOR_VERSION);

    TemplateCollection::ConstIterator 
        colIt  = mgr.m_templateCollection.Begin(),
        colEnd = mgr.m_templateCollection.End();

    if (mgr.m_templateCollection.IsModified())
    {
        for (; colIt != colEnd; ++colIt)
        {
            if (colIt->second->GetModified())
            {
                TiXmlElement* langElem = getNodeSafe(templsElem, "Language", "Name", colIt->first.c_str());
                writeXml(langElem, "Name", colIt->first);
                writeXml(langElem, "AlwaysListIfAlternative", colIt->second->GetAlwaysListIfAlternative());

                Template::ConstIterator 
                    templIt  = colIt->second->Begin(),
                    templEnd = colIt->second->End();

                for (; templIt != templEnd; ++templIt)
                {
                    if (templIt->deleted)
                    {
                        if (TiXmlElement* templElem = findNode(langElem, "Template", "Uuid", templIt->uuid.c_str()))
                            langElem->RemoveChild(templElem);
                    }
                    else if (templIt->modified)
                    {
                        TiXmlElement* templElem = getNodeSafe(langElem, "Template", "Uuid", templIt->uuid.c_str());

                        writeXml(templElem, "Name",      templIt->name);
                        writeXml(templElem, "Keyword",   templIt->keyword);
                        writeXml(templElem, "MinLength", templIt->minLength);
                        writeXml(templElem, "CurLine",   templIt->curLine);
                        writeXml(templElem, "CurPos",    templIt->curPos);
                        writeXml(templElem, "Uuid",      templIt->uuid);

                        if (TiXmlNode* textNode = templElem->FirstChild())
                        {
                            if (textNode->Type() == TiXmlNode::TINYXML_TEXT)
                                textNode->SetValue((templIt->text.c_str()));
                            else
                                templElem->LinkEndChild(new TiXmlText((templIt->text.c_str())));
                        }
                        else
                            templElem->LinkEndChild(new TiXmlText((templIt->text.c_str())));
                    }
                }
            }
        }

        if (!TiXmlUtils::SaveFile(doc, m_filename.c_str()))
            THROW_APP_EXCEPTION("Cannot save \"" + Common::str(m_filename) + "\"\nERROR: " + doc.ErrorDesc());

        mgr.m_templateCollection.ClearModified();
    }
}

void TemplateXmlStreamer::operator >> (SettingsManager& mgr)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + Common::str(m_filename) + '"');

    TiXmlBase::SetCondenseWhiteSpace(false);

    TiXmlDocument doc;
    if (::PathFileExists(m_filename.c_str()))
    {
        if (!TiXmlUtils::LoadFile(doc, m_filename.c_str()))
            THROW_APP_EXCEPTION("Cannot parse \"" + Common::str(m_filename) + "\"\nERROR: " + doc.ErrorDesc());
    }
    else // no problem - will load settings from resources
    {
        string xmlText;
        if (!AppLoadTextFromResources(::PathFindFileName(m_filename.c_str()), RT_HTML, xmlText) || xmlText.empty())
            THROW_APP_EXCEPTION("Cannot load templates from resources");

        if (!doc.Parse(xmlText.c_str()))
            THROW_APP_EXCEPTION(string("Cannot parse templates\nERROR: ") + doc.ErrorDesc());
    }

    bool fileModified = false;

    if (TiXmlNode* rootElem = doc.FirstChildElement("OpenEditor"))
    {
        if (TiXmlElement* langsElem = rootElem->FirstChildElement("Templates"))
        {
            int majorVer;
            readXml(langsElem, "MajorVersion", majorVer);
            if (majorVer != MAJOR_VERSION)
                THROW_APP_EXCEPTION("Cannot load \""  + Common::str(m_filename) + "\".\nERROR: Unsupported templates version.");

            mgr.m_templateCollection.Clear();

            TiXmlElement* langElem = langsElem->FirstChildElement("Language");
            for (; langElem; langElem = langElem->NextSiblingElement("Language"))
            {
                string language;
                readXml(langElem, "Name", language);
                TemplatePtr templ = mgr.m_templateCollection.Add(language);

                bool templModified = false;
                TiXmlElement* templElem = langElem->FirstChildElement("Template");
                for (; templElem; templElem = templElem->NextSiblingElement("Template"))
                {
                    Template::Entry entry;
                    readXml(templElem, "Name",      entry.name);
                    readXml(templElem, "Keyword",   entry.keyword);
                    readXml(templElem, "MinLength", entry.minLength);
                    readXml(templElem, "CurLine",   entry.curLine);
                    readXml(templElem, "CurPos",    entry.curPos);

                    if (const TiXmlNode* textNode = templElem->FirstChild())
                    {
                        if (textNode->Type() == TiXmlNode::TINYXML_TEXT)
                            entry.text = textNode->Value();
                    }

                    readXml(templElem, "Uuid", entry.uuid);
                    if (entry.uuid.empty())
                    {
                        Template::GenUUID(entry);
                        writeXml(templElem, "Uuid", entry.uuid);
                        templModified = true;
                    }
                    else
                        entry.modified = false;

                    templ->AppendEntry(entry);
                }

                fileModified |= templModified;

                if (!templModified)
                    templ->ClearModified();
            }
        }
    }

    if (fileModified)
        if (!TiXmlUtils::SaveFile(doc, m_filename.c_str()))
            THROW_APP_EXCEPTION("Cannot save \"" + Common::str(m_filename) + "\"\nERROR: " + doc.ErrorDesc());
}

};//namespace OpenEditor
