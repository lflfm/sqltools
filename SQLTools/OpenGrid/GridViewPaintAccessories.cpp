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

#include "stdafx.h"
#include "GridViewPaintAccessories.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace OG2
{
    using namespace std;
    using namespace Common;

    typedef map<string, CGridViewPaintAccessoriesPtr> CGridViewPaintAccessoriesMap;
    
    static CGridViewPaintAccessoriesMap g_PaintAccessoriesMap;


CGridViewPaintAccessoriesPtr CGridViewPaintAccessories::GetPaintAccessories (const std::string& category)
{
    CGridViewPaintAccessoriesMap::iterator it = g_PaintAccessoriesMap.find(category);
    
    if (g_PaintAccessoriesMap.end() == it)
    {
        CGridViewPaintAccessoriesPtr ptr(new CGridViewPaintAccessories);

        g_PaintAccessoriesMap.insert(CGridViewPaintAccessoriesMap::value_type(category, ptr));

        return ptr;
    }

    return it->second;
}

CGridViewPaintAccessories::CGridViewPaintAccessories ()
{ 
}

#pragma message ("\tImprovement: suppress redundant notifications")
void CGridViewPaintAccessories::OnSettingsChanged (const VisualAttributesSet& set_)
{
    CFont* pfont = set_.FindByName("Text").NewFont();
    m_Font.DeleteObject();
    m_Font.Attach((HFONT)*pfont);
    pfont->Detach();
    delete pfont;
}

}//namespace OG2

