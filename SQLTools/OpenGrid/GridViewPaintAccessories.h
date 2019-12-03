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
#ifndef __GridViewPaintAccessories_h__
#define __GridViewPaintAccessories_h__

#include <map>
#include <string>
#include <arg_shared.h>
#include "COMMON/VisualAttributes.h"

namespace OG2
{
    using arg::counted_ptr;
    struct CGridViewPaintAccessories;
    typedef counted_ptr<CGridViewPaintAccessories> CGridViewPaintAccessoriesPtr;
    
struct CGridViewPaintAccessories
{
    CFont m_Font;

    static CGridViewPaintAccessoriesPtr GetPaintAccessories (const std::string&);

    CGridViewPaintAccessories ();

    void OnSettingsChanged (const Common::VisualAttributesSet& set);

};

}//namespace OG2

#endif//__GridViewPaintAccessories_h__
