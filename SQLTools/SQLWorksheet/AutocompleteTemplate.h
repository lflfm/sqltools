/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
#include "OpenEditor/OETemplates.h"

    namespace ObjectTree 
    {
        class TreeNodePool;
    };
    typedef arg::counted_ptr<ObjectTree::TreeNodePool> TreeNodePoolPtr;


struct AutocompleteTemplate : OpenEditor::Template
{    
    AutocompleteTemplate (TreeNodePoolPtr);

    virtual bool AfterExpand (const Entry&, string& text, OpenEditor::Position& pos);

private:
    TreeNodePoolPtr m_poolPtr;
    CEvent          m_bkgrSyncEvent;
};


struct AutocompleteSubobjectTemplate : OpenEditor::Template
{    
    AutocompleteSubobjectTemplate (TreeNodePoolPtr, const string& object);

private:
    TreeNodePoolPtr m_poolPtr;
    CEvent          m_bkgrSyncEvent;
};
