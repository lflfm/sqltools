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

// 28.08.2002 improvement, double key accelerator, for example "Ctrl+K,B"
// 24.09.2007 some improvements taken from sqltools++ by Randolf Geist

#pragma once
#ifndef __GUICommandDictionary_h__
#define __GUICommandDictionary_h__

#include <string>
#include <map>
#include <vector>
#include <istream>


namespace Common
{
    using std::string;
    using std::map;
    using std::istream;
    using std::vector;

    typedef unsigned short Command;
    typedef unsigned short VKey;

class GUICommandDictionary
{
public:
    static Command m_firstDblKeyAccelCommandId;

    static void InsertCommand (const char*, Command);
    static bool BuildAcceleratorTable (std::istream&, HACCEL& accelTable);
    static void AddAccelDescriptionToMenu (HMENU&, unsigned int extraSpaces = 0);

    static bool GetDblKeyDescription (int dblKeyAccelInx, string&, VKey = 0);
    static bool GetDblKeyAccelCmdId  (int dblKeyAccelInx, VKey, Command&);
    static void GetDblKeyCommnds (int dblKeyAccelInx, std::vector<std::pair<Command,string> >&commands);

    static bool GetCommandAccelLabel (Command, string&);
	static HACCEL GetMenuAccelTable (HMENU& hMenu);
};

};//namespace Common

#endif//__GUICommandDictionary_h__
