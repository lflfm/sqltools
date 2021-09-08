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

#include <afxmt.h>
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OELanguage.h"

    class TiXmlElement;

namespace OpenEditor
{
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;

    ///////////////////////////////////////////////////////////////////////////
    // SettingsXmlStreamer
    ///////////////////////////////////////////////////////////////////////////

    class SettingsXmlStreamer
    {
    public:
        SettingsXmlStreamer (const std::wstring& filename);

        void operator >> (SettingsManager&);
        void operator << (const SettingsManager&);

    private:
        const std::wstring m_filename;
        CMutex m_fileAccessMutex;   // tinyxml uses fstream access
                                    // so we have to use the mutex to lock the file 
                                    // for concurrent access

        void read (const TiXmlElement*, VisualAttributesSet&);
        template <class T> static void readAwareness  (const TiXmlElement*, T&);
        template <class T> static void read (const TiXmlElement*, const char* elemName, vector<T>&);

        void write (const VisualAttributesSet&, TiXmlElement*);
        template <class T> static void writeAwareness  (const T&, TiXmlElement*);
        template <class T> static void write (const vector<T>&, TiXmlElement*, const char* elemName);
    };

};//namespace OpenEditor

