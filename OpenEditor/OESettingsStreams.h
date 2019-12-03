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

// 17.01.2005 (ak) simplified settings class hierarchy 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OESettingsStreams_h__
#define __OESettingsStreams_h__

#include "OpenEditor/OEStreams.h"
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OELanguage.h"


namespace OpenEditor
{
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;

    ///////////////////////////////////////////////////////////////////////////
    // SettingsManager Reader/Writter
    ///////////////////////////////////////////////////////////////////////////

    class SettingsManagerReader
    {
    public:
        SettingsManagerReader (InStream& in) : m_in(in), m_skipEnum(false) {}
        virtual ~SettingsManagerReader () {}

        void operator >> (SettingsManager&);

    protected:
        virtual void read (ClassSettingsPtr);
        virtual void read (VisualAttributesSet&);
        virtual void read (VisualAttribute&);

        InStream& m_in;
        int       m_version;
        bool      m_skipEnum;
    };


    class SettingsManagerWriter
    {
    public:
        SettingsManagerWriter (OutStream& out) : m_out(out), m_skipEnum(false) {}
        virtual ~SettingsManagerWriter () {}

        void operator << (const SettingsManager&);

    protected:
        virtual void write (const ClassSettingsPtr);
        virtual void write (const VisualAttributesSet&);
        virtual void write (const VisualAttribute&);

        OutStream& m_out;
        bool       m_skipEnum;
    };

};//namespace OpenEditor


#endif//__OESettingsStreams_h__

