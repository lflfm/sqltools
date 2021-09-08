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
#include "ExtractDDLSettingsXmlStreamer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 1; // any minor changes

    static const char* ROOT_ELEM   = "SQLTools";
    static const char* SETTINGS    = "DDL-Settings";

///////////////////////////////////////////////////////////////////////////
// ExtractDDLSettingsXmlStreamer
///////////////////////////////////////////////////////////////////////////
ExtractDDLSettingsXmlStreamer::ExtractDDLSettingsXmlStreamer (const std::wstring& filename, bool backup) 
: XmlStreamerBase(filename, backup)
{
}

void ExtractDDLSettingsXmlStreamer::read  (const TiXmlDocument& doc, void* ctx)
{
    ExtractDDLSettings* settings = reinterpret_cast<ExtractDDLSettings*>(ctx);

    if (const TiXmlNode* rootElem = doc.FirstChildElement(ROOT_ELEM))
    {
        if (const TiXmlElement* settingsElem = rootElem->FirstChildElement(SETTINGS))
        {
            validateMajorVersion(settingsElem, MAJOR_VERSION);
            readAwareness(settingsElem, *(OraMetaDict::WriteSettings*)settings);
            readAwareness(settingsElem, *settings);
        }
        else 
            THROW_APP_EXCEPTION("\"" + Common::str(getFilename()) + "\" does not contain settings.");
    }
}

void ExtractDDLSettingsXmlStreamer::write (TiXmlDocument& doc, bool /*fresh*/, const void* ctx)
{
    const ExtractDDLSettings* settings = reinterpret_cast<const ExtractDDLSettings*>(ctx);

    TiXmlElement* rootElem = getNodeSafe(&doc, ROOT_ELEM);
    TiXmlElement* settingsElem = getNodeSafe(rootElem, SETTINGS);
    
    validateMajorVersion(settingsElem, MAJOR_VERSION);
    writeVersion(settingsElem, MAJOR_VERSION, MINOR_VERSION);

    writeAwareness(*(OraMetaDict::WriteSettings*)settings, settingsElem);
    writeAwareness(*settings, settingsElem);
}
