/* 
	SQLTools is a tool for Oracle database developers and DBAs.
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

#include "stdafx.h"
#include "TinyXml\TinyXml.h"
#include "XmlStreamerBase.h"
#include "COMMON/AppUtilities.h"
#include "Common/ConfigFilesLocator.h"
#include "Common/VisualAttributes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using Common::ConfigFilesLocator;
using Common::VisualAttributesSet;
using Common::VisualAttribute;
using Common::AppLoadTextFromResources;

static const char* VISUAL_ATTRIBUTES = "VisualAttributes";
static const char* VISUAL_ATTRIBUTE  = "VisualAttribute";

///////////////////////////////////////////////////////////////////////////
// XmlStreamerBase
///////////////////////////////////////////////////////////////////////////
XmlStreamerBase::XmlStreamerBase (const std::string& filename, bool backup) 
: m_backup(backup), 
m_filename(filename),
m_fileAccessMutex(FALSE, ("GNU.SQLTools." + filename).c_str())
{
    ASSERT(m_filename.c_str() == ::PathFindFileName(m_filename.c_str()));
}

bool XmlStreamerBase::fileExists () const
{
    string path;
    return ConfigFilesLocator::GetFilePath(m_filename.c_str(), path);
}

void XmlStreamerBase::read (void* ctx)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_filename + '"');

    TiXmlDocument doc;

    string path;
    bool exists = ConfigFilesLocator::GetFilePath(m_filename.c_str(), path);

    if (exists)
    {
        if (!doc.LoadFile(path.c_str()))
            THROW_APP_EXCEPTION("Cannot parse \"" + path + "\"\nERROR: " + doc.ErrorDesc());
    }
    else // no problem - will load settings from resources
    {
		string xmlText;
		if (!AppLoadTextFromResources(::PathFindFileName(m_filename.c_str()), RT_HTML, xmlText) || xmlText.empty())
            THROW_APP_EXCEPTION("Cannot load default settings from resources");

        if (!doc.Parse(xmlText.c_str()))
            THROW_APP_EXCEPTION(string("Cannot parse default settings\nERROR: ") + doc.ErrorDesc());
    }

    read(doc, ctx);

    // create a settings file
    if (!exists && !doc.SaveFile(path.c_str()))
        THROW_APP_EXCEPTION("Cannot save \"" + path + "\"\nERROR: " + doc.ErrorDesc());
}


void XmlStreamerBase::write (const void* ctx)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_filename + '"');

    TiXmlDocument doc;

    string path;
    bool exists = ConfigFilesLocator::GetFilePath(m_filename.c_str(), path);

    if (!exists)
        doc.InsertEndChild(TiXmlDeclaration("1.0", ""/*"Windows-1252"*/, "yes"));
    else if (!doc.LoadFile(path.c_str()))
        THROW_APP_EXCEPTION("Cannot read \"" + path + "\"\nERROR: " + doc.ErrorDesc());

    if (exists && m_backup)
        if (!::CopyFile(path.c_str(), (path + ".old").c_str(), FALSE))
            THROW_APP_EXCEPTION("Cannot backup file \"" + path + "\"");

    write(doc, !exists, ctx);

    if (!doc.SaveFile(path.c_str()))
        THROW_APP_EXCEPTION("Cannot save \"" + path + "\"\nERROR: " + doc.ErrorDesc());
}

TiXmlElement* XmlStreamerBase::getNodeSafe (TiXmlNode* parent, const char* name)
{
    TiXmlElement* elem = parent->FirstChildElement(name);
    if (!elem)
    {
        elem = new TiXmlElement(name);
        parent->LinkEndChild(elem);
    }
    return elem;
}

void XmlStreamerBase::writeVersion (TiXmlElement* elem, int majorVersion, int minorVersion)
{
    elem->SetAttribute("MajorVersion", majorVersion);
    elem->SetAttribute("MinorVersion", minorVersion);
}

void XmlStreamerBase::validateMajorVersion (const TiXmlElement* elem, int majorVersion) const
{
    int majorVer;
    if (elem->QueryIntAttribute("MajorVersion", &majorVer) != TIXML_SUCCESS)
        majorVer = 0;
    if (majorVer != majorVersion)
        THROW_APP_EXCEPTION("Cannot load \""  + m_filename + "\".\nERROR: Unsupported settings version.");
}

void XmlStreamerBase::buildNodeMap (const TiXmlElement* parentElem, const char* elemName, const char* attrName, map<string, const TiXmlElement*>& map)
{
    const TiXmlElement* elem = parentElem->FirstChildElement(elemName);
    for (; elem; elem = elem->NextSiblingElement(elemName))
    {
        if (const char* name = elem->Attribute(attrName))
            map.insert(make_pair(string(name), elem));
    }
}

void XmlStreamerBase::buildNodeMap (TiXmlElement* parentElem, const char* elemName, const char* attrName, map<string, TiXmlElement*>& map)
{
    TiXmlElement* elem = parentElem->FirstChildElement(elemName);
    for (; elem; elem = elem->NextSiblingElement(elemName))
    {
        if (const char* name = elem->Attribute(attrName))
            map.insert(make_pair(string(name), elem));
    }
}

TiXmlElement* XmlStreamerBase::getNodebyMap (map<string, TiXmlElement*>& elemMap, const char* key, TiXmlElement* parentElem, const char* elemName)
{
    TiXmlElement* elem = 0;
    map<string, TiXmlElement*>::const_iterator it = elemMap.find(key);
    
    if (it != elemMap.end())
        elem = it->second;
    else
    {
        elem = new TiXmlElement(elemName);
        parentElem->LinkEndChild(elem);
    }
    return elem;
}


void XmlStreamerBase::write (const VisualAttributesSet& vaset, TiXmlElement* parentElem)
{
    map<string, TiXmlElement*> elemMap;
    buildNodeMap(parentElem, VISUAL_ATTRIBUTE, "Name", elemMap);

    for (int i = 0, count = vaset.GetCount(); i < count; i++)
    {
        TiXmlElement* vaElem = getNodebyMap(elemMap, vaset[i].m_Name.c_str(), parentElem, VISUAL_ATTRIBUTE);
        writeAwareness(vaset[i], vaElem);
    }
}

void XmlStreamerBase::read (const TiXmlElement* parentElem, VisualAttributesSet& vaset)
{
    map<string,int> vaMap;
    for (int i = 0, count = vaset.GetCount(); i < count; ++i)
        vaMap[vaset[i].m_Name] = i;

    const TiXmlElement* elem = parentElem->FirstChildElement(VISUAL_ATTRIBUTE);
    for (; elem; elem = elem->NextSiblingElement(VISUAL_ATTRIBUTE))
    {
        if (const char* name = elem->Attribute("Name"))
        {
            map<string,int>::const_iterator it = vaMap.find(name);
            if (it != vaMap.end())
                readAwareness(elem, vaset[it->second]);
        }
    }
}
