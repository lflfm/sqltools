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

#pragma once
#ifndef __VisualAttributes_h__
#define __VisualAttributes_h__

#include <string>
#include <vector>
#include <set>
#include <Common\Properties.h>

class CFont;

namespace Common
{
    using std::string;
    using std::vector;
    using std::set;


enum VisualAttributeOption
{
    vaoFontFixedPitch = 0x01,
    vaoFontName       = 0x02,
    vaoFontSize       = 0x04,
    vaoFontBold       = 0x10,
    vaoFontItalic     = 0x20,
    vaoFontUnderline  = 0x40,
    vaoForeground     = 0x100,
    vaoBackground     = 0x200,

    vaoFont   = vaoFontName|vaoFontSize|vaoFontBold|vaoFontItalic|vaoFontUnderline,
    vaoColor  = vaoForeground|vaoBackground,
    vaoAll    = vaoFont|vaoColor
};

///////////////////////////////////////////////////////////////////////////////
//  VisualAttribute
///////////////////////////////////////////////////////////////////////////////

struct VisualAttribute
{
    VisualAttribute ();

    VisualAttribute (const VisualAttribute&);

    VisualAttribute ( // is it used?
        const char* name,
        const char* font, int size = 11,
        bool bold = false, bool italic = false, bool underline = false,
        COLORREF foreground = -1, COLORREF background = -1,
        unsigned mask = vaoFont
        );

    VisualAttribute& operator = (const VisualAttribute&);

    void Clear ();
    void Construct (VisualAttribute& dest, const VisualAttribute& def);
    static void Construct (int count, COLORREF* dest, const COLORREF* src, const COLORREF* def);

    CFont* NewFont () const;
    static int PixelToPoint (int pixels);
    static int PointToPixel (int points);

    CMN_DECL_PROPERTY_BINDER(VisualAttribute);
    CMN_DECL_PUBLIC_PROPERTY(std::string,   Name         );
    CMN_DECL_PUBLIC_PROPERTY(std::string,   FontName     );
    CMN_DECL_PUBLIC_PROPERTY(int,           FontSize     );
    CMN_DECL_PUBLIC_PROPERTY(bool,          FontBold     );
    CMN_DECL_PUBLIC_PROPERTY(bool,          FontItalic   );
    CMN_DECL_PUBLIC_PROPERTY(bool,          FontUnderline);
    CMN_DECL_PUBLIC_PROPERTY(unsigned,      Foreground   );
    CMN_DECL_PUBLIC_PROPERTY(unsigned,      Background   );
    CMN_DECL_PUBLIC_PROPERTY(unsigned,      Mask         );

public:
    //string m_Name;
    //string m_FontName;
    //int    m_FontSize;
    //bool   m_FontBold, m_FontItalic, m_FontUnderline;
    //COLORREF m_Foreground, m_Background;
    //unsigned m_Mask;
};


///////////////////////////////////////////////////////////////////////////////
//  VisualAttributesSet
///////////////////////////////////////////////////////////////////////////////

class VisualAttributesSet
{
public:
    VisualAttributesSet () {};
    VisualAttributesSet (const char* name, int size);

    VisualAttributesSet& operator = (const VisualAttributesSet&);

    const char* GetName () const;
    const char* GetName (int index) const;
    void SetName (const char* name);
    void SetName (const string& name);

    int  GetCount () const;
    int  Add (const VisualAttribute&);
    void Clear ();

    VisualAttribute& operator [] (int index)             { return m_attrs.at(index); }
    const VisualAttribute& operator [] (int index) const { return m_attrs.at(index); }

    const VisualAttribute& FindByName (const char* name) const;

protected:
    string m_name;
    vector<VisualAttribute> m_attrs;
};


///////////////////////////////////////////////////////////////////////////////
//  VisualAttributesSet inline methods
///////////////////////////////////////////////////////////////////////////////

inline
const char* VisualAttributesSet::GetName () const
{
    return m_name.c_str();
}

inline
const char* VisualAttributesSet::GetName (int index) const
{
    return m_attrs.at(index).m_Name.c_str();
}

inline
void VisualAttributesSet::SetName (const char* name)
{
    m_name = name;
}

inline
void VisualAttributesSet::SetName (const string& name)
{
    m_name = name;
}


inline
int VisualAttributesSet::GetCount () const
{
    return m_attrs.size();
}

inline
int VisualAttributesSet::Add (const VisualAttribute& attr)
{
    m_attrs.push_back(attr);
    return m_attrs.size() - 1;
}

inline
void VisualAttributesSet::Clear ()
{
    m_attrs.clear();
}

}//namespace Common

#endif//__VisualAttributes_h__