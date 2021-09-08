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
#include "SQLToolsSettingsXmlStreamer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 1; // any minor changes

    static const char* ROOT_ELEM    = "SQLTools";
    static const char* SETTINGS     = "Settings";
    static const char* GUI_SETTINGS = "GUI-Settings";
    static const char* DDL_SETTINGS = "DDL-Settings";

///////////////////////////////////////////////////////////////////////////
// SQLToolsSettingsXmlStreamer
///////////////////////////////////////////////////////////////////////////
SQLToolsSettingsXmlStreamer::SQLToolsSettingsXmlStreamer (const std::wstring& filename, bool backup) 
: XmlStreamerBase(filename, backup)
{
}

void SQLToolsSettingsXmlStreamer::read  (const TiXmlDocument& doc, void* ctx)
{
    SQLToolsSettings* settings = reinterpret_cast<SQLToolsSettings*>(ctx);

    if (const TiXmlNode* rootElem = doc.FirstChildElement(ROOT_ELEM))
    {
        if (const TiXmlElement* settingsElem = rootElem->FirstChildElement(SETTINGS))
        {
            validateMajorVersion(settingsElem, MAJOR_VERSION);
            readAwareness(settingsElem, *settings);

            if (const TiXmlElement* ddlElem = settingsElem->FirstChildElement(DDL_SETTINGS))
                readAwareness(ddlElem, *static_cast<OraMetaDict::WriteSettings*>(settings));

            if (const TiXmlElement* guiElem = settingsElem->FirstChildElement(GUI_SETTINGS))
            {
                map<string, const TiXmlElement*> elemMap;
                buildNodeMap(guiElem, "Window", "Name", elemMap);

                // read only matched by name
                SQLToolsSettings::Vasets::iterator it = settings->m_vasets.begin();
                for (; it != settings->m_vasets.end(); ++it)
                {
                    map<string, const TiXmlElement*>::const_iterator it2 = elemMap.find(it->GetName());
                    if (it2 != elemMap.end())
                        XmlStreamerBase::read(it2->second, *it);
                }
            }
        }
        else 
            THROW_APP_EXCEPTION(L"\"" + getFilename() + L"\" does not contain settings.");
    }
}

void SQLToolsSettingsXmlStreamer::write (TiXmlDocument& doc, bool /*fresh*/, const void* ctx)
{
    const SQLToolsSettings* settings = reinterpret_cast<const SQLToolsSettings*>(ctx);

    TiXmlElement* rootElem = getNodeSafe(&doc, ROOT_ELEM);
    TiXmlElement* settingsElem = getNodeSafe(rootElem, SETTINGS);
    
    validateMajorVersion(settingsElem, MAJOR_VERSION);
    writeVersion(settingsElem, MAJOR_VERSION, MINOR_VERSION);

    writeAwareness(*settings, settingsElem);

    TiXmlElement* ddlElem = getNodeSafe(settingsElem, DDL_SETTINGS);
    writeAwareness(*static_cast<const OraMetaDict::WriteSettings*>(settings), ddlElem);
    
    TiXmlElement* guiElem = getNodeSafe(settingsElem, GUI_SETTINGS);
    map<string, TiXmlElement*> elemMap;
    buildNodeMap(guiElem, "Window", "Name", elemMap);


    SQLToolsSettings::Vasets::const_iterator it = settings->m_vasets.begin();
    for (; it != settings->m_vasets.end(); ++it)
    {
        TiXmlElement* winElem = getNodebyMap(elemMap, it->GetName(), guiElem, "Window");
        winElem->SetAttribute("Name", it->GetName());
        XmlStreamerBase::write(*it, winElem);
    }
}
