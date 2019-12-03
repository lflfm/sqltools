/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2006 Aleksey Kochetov

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
#ifndef __SqlDocCreater_H__
#define __SqlDocCreater_H__

#include "MetaDict\TextOutput.h"

class SQLToolsSettings;
namespace OraMetaDict { class DbObject; };

class SqlDocCreater
{
    OraMetaDict::TextOutputInMEM 
        m_Output, m_TableOutput[7];

    bool m_singleDocument, m_bUseBanner, m_supressGroupByDDL, m_bPackAsWhole, m_objectNameAsTitle;
    int  m_objectCounter, m_nOffset[2];
    CString m_strOwner, m_strName,
            m_strType, m_strExt;
public:
    SqlDocCreater (bool singleDocument, bool useBanner, bool supressGroupByDDL, bool objectNameAsTitle);
    ~SqlDocCreater ();

    void NewLine ();
    void Put (const OraMetaDict::DbObject&, const SQLToolsSettings&);
    void Flush (bool bForce = false);
    void Clear ();
};

#endif//__SqlDocCreater_H__
