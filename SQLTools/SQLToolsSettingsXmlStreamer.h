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

#ifndef __SQLToolsSettingsXmlStreamer_h__
#define __SQLToolsSettingsXmlStreamer_h__

#include <afxmt.h>
#include <vector>
#include "SQLToolsSettings.h"
#include "XmlStreamerBase.h"


///////////////////////////////////////////////////////////////////////////
// SQLToolsSettingsXmlStreamer
///////////////////////////////////////////////////////////////////////////

class SQLToolsSettingsXmlStreamer : XmlStreamerBase
{
public:
    SQLToolsSettingsXmlStreamer (const std::string& filename, bool backup = false);

    void operator >> (SQLToolsSettings& settings)           { XmlStreamerBase::read(&settings); }
    void operator << (const SQLToolsSettings& settings)     { XmlStreamerBase::write(&settings); }

private:
    virtual void read  (const TiXmlDocument&, void* ctx);
    virtual void write (TiXmlDocument&, bool, const void* ctx);
};

#endif//__SQLToolsSettingsXmlStreamer_h__