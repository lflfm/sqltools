/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2014 Aleksey Kochetov

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
#include "SessionCache.h"
#include "DbBrowser\ObjectTree_Builder.h"
#include "OpenEditor\OETemplates.h"
#include "SQLWorksheet\AutocompleteTemplate.h"
#include <OpenEditor\OELanguage.h>
#include "OCI8\Connect.h"
#include "OCI8\BCursor.h"
#include "SQLTools.h"
#include "SQLUtilities.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

//TODO#TEST: retest this class

    using namespace OpenEditor;
    using namespace ObjectTree;

    SessionCachePtr SessionCache::m_sessionCache(0);
    string SessionCache::m_cookie("SessionCache");

    static
    int object_type_to_image_index (const string& object_type)
    {
        if (object_type.size() > 0)
        {
            switch (object_type.at(0))
            {
            case 'F': 
                if (object_type == "FUNCTION") return IDII_FUNCTION;
            case 'P': 
                if (object_type == "PACKAGE")   return IDII_PACKAGE;
                if (object_type == "PROCEDURE") return IDII_PROCEDURE;
            case 'S': 
                if (object_type == "SEQUENCE") return IDII_SEQUENCE; 
                if (object_type == "SYNONYM" ) return IDII_SYNONYM ; 
            case 'T': 
                if (object_type == "TABLE") return IDII_TABLE;    
                if (object_type == "TYPE")  return IDII_TYPE;     
            case 'V': 
                if (object_type == "VIEW")  return IDII_VIEW;     
            }
        }
        return IDII_UNKNOWN;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////
    
    static const char* lookupQuery1 = 
        "SELECT DISTINCT object_name, object_type"
        " FROM sys.all_objects "
        " WHERE owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA')"
        " AND object_type IN ('TABLE', 'VIEW', 'PACKAGE', 'PROCEDURE', 'FUNCTION', 'INDEX', 'SEQUENCE', 'SYNONYM', 'TRIGGER', 'TYPE')"
        " AND object_name is NOT NULL"
        " AND object_name = Upper(object_name)";

    static const char* lookupQuery1b = 
        " AND generated = 'N'";

    static const char* lookupQuery2 = 
        // added because 10g's all_objects does not have synonyms for CURRENT_SCHEMA if it was altered
        " UNION ALL SELECT synonym_name, 'SYNONYM' object_type"
        " FROM sys.all_synonyms "
        " WHERE owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA')"
        " AND synonym_name = Upper(synonym_name)"
        " AND SYS_CONTEXT('USERENV', 'SESSION_USER') <> SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA')";

    struct SrvBkLoadObjectCache : ServerBackgroundThread::Task
    {
        ObjectTree::TreeNodePoolPtr m_objectPool;           // it is thread safe
        OpenEditor::TemplatePtr     m_autocompleteTemplate; // built in background  
        LanguageKeywordMapPtr       m_newKeywordMap;
        int                         m_userObjectsGroup;
        bool                        m_force;
        bool                        m_skip;

        SrvBkLoadObjectCache (ObjectTree::TreeNodePoolPtr objectPool, OpenEditor::TemplatePtr autocompleteTemplate, bool force) 
            : ServerBackgroundThread::Task("Populate ObjectCache", 0), 
            m_objectPool(objectPool),
            m_autocompleteTemplate(autocompleteTemplate),
            m_newKeywordMap(new LanguageKeywordMap),
            m_userObjectsGroup(-1),
            m_force(force),
            m_skip(false)
        {
            CWaitCursor wait;

            LanguagePtr lang = LanguagesCollection::Find("PL/SQL");
            
            const vector<string>& groups = lang->GetKeywordGroups();
            for (int i = 0; i < (int)groups.size(); ++i) {
                if (groups.at(i) == "User Objects") {
                    m_userObjectsGroup = i;
                    break;
                }
            }

            LanguageKeywordMapPtr orgKeywordMap = lang->GetLanguageKeywordMap();

            LanguageKeywordMapConstIterator 
                it = orgKeywordMap->begin(),
                end = orgKeywordMap->end();
            for (it; it != end; ++it)
                if (it->second.groupIndex != m_userObjectsGroup)
                    m_newKeywordMap->insert(pair<std::wstring, LanguageKeyword>(it->first, it->second));
        }

        void DoInBackground (OciConnect& connect)
        {
            if (connect.IsSlow() || !connect.IsDatabaseOpen()) 
            { 
                m_skip = true;
                return;
            }

            string cookie_val;
            connect.GetSessionCookie(SessionCache::m_cookie, cookie_val);

            if (!(m_force || cookie_val != "loaded"))
            { 
                m_skip = true;
                return;
            }

            try
            {
                ActivePrimeExecutionOnOff onOff;

                string lookupQuery = lookupQuery1;

                // TODO: check if it is required for 8i or 9i
                if (connect.GetVersion() >= OCI8::esvServer10X)
                {
                    lookupQuery += lookupQuery1b;
                }
                
                if (connect.GetVersion() >= OCI8::esvServer81X)
                {
                    string user, schema;
                    connect.GetCurrentUserAndSchema(user, schema);
                    if (user != schema)
                        lookupQuery += lookupQuery2;
                }
                lookupQuery += " ORDER BY 1";

                OciCursor cursor(connect, lookupQuery.c_str());
                cursor.Execute();

                vector<Template::Entry> entries;

                LanguageKeyword lkw;
                lkw.groupIndex = m_userObjectsGroup;
                while (cursor.Fetch())
                {
                    string buff;
                    cursor.GetString(0, buff);
                    lkw.keyword = Common::wstr(buff);
                    m_newKeywordMap->insert(pair<std::wstring, LanguageKeyword>(lkw.keyword, lkw));
                    
                    Template::Entry entry;
                    cursor.GetString(1, entry.name);
                    if (entry.name != "INDEX" && entry.name != "TRIGGER")
                    {
                        cursor.GetString(0, entry.keyword);
                        entry.keyword = SQLUtilities::GetSafeDatabaseObjectName(entry.keyword);
                        entry.text = entry.keyword;
                        entry.curPos = entry.text.length();
                        entry.minLength = entry.curPos + 1; // to handle names like TEST TEST_PKG 
                        entry.image = object_type_to_image_index(entry.name);
                        entries.push_back(entry);
                    }
                }
            
                TemplatePtr templ(new AutocompleteTemplate(m_objectPool));
                templ->SetImageListRes(IDB_SQL_GENERAL_LIST);
                // calculation of min input length for quick expand
                for (vector<Template::Entry>::iterator it1 = entries.begin(); it1 != entries.end(); ++it1)
                {
                    string keyword = it1->keyword;
                
                    if (entries.size() < 5000) // TODO: make that configurable
                    {
                        // TODO: try to use it1 as the start point
                        vector<Template::Entry>::const_iterator begin(entries.begin()), end(entries.end());
                        for (vector<Template::Entry>::const_iterator it2 = entries.begin(); it2 != entries.end(); ++it2)
                        {
                            if (!strncmp(keyword.c_str(), it2->keyword.c_str(), 3)) {
                                if(begin == entries.begin())
                                    begin = it2;
                            } else {
                                if (begin != entries.begin()) {
                                    end = it2;
                                    break;
                                }
                            }
                        }

                        while (keyword.length() > 2)
                        {
                            int match_counter = 0; 
                            for (vector<Template::Entry>::const_iterator it2 = begin; it2 != end; ++it2)
                            {
                                if (keyword[0] == it2->keyword[0]
                                && !strncmp(keyword.c_str(), it2->keyword.c_str(), keyword.length()))
                                    match_counter++;
                            }
                            if (match_counter > 1)
                                break;
                            keyword.resize(keyword.length()-1);
                        }
                    }

                    it1->minLength = keyword.length() + 1;
                    templ->AppendEntry(*it1);
                }
                m_autocompleteTemplate = templ;

                connect.SetSessionCookie(SessionCache::m_cookie, "loaded"); 
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            if (!m_skip)
            {
                CWaitCursor wait;

                SessionCache::GetSessionCache()->m_autocompleteTemplate = m_autocompleteTemplate;

                LanguagePtr lang = LanguagesCollection::Find("PL/SQL");
                lang->SetLanguageKeywordMap(m_newKeywordMap);

                AfxGetMainWnd()->RedrawWindow(NULL, NULL, RDW_INVALIDATE|RDW_NOERASE|RDW_NOFRAME|RDW_ALLCHILDREN);
            }
            else
                SetSilent(true);
        }
    };

///////////////////////////////////////////////////////////////////////////////////////////////////
    
SessionCache::SessionCache ()
{
}

SessionCache::~SessionCache ()
{
}

SessionCachePtr SessionCache::GetSessionCache ()
{
    if (!m_sessionCache.get())
        m_sessionCache = SessionCachePtr(new SessionCache);

    return m_sessionCache;
}


void SessionCache::OnConnect ()
{
    if (GetSessionCache().get())
    {
        m_sessionCache->m_objectPool.reset(new TreeNodePool);
        m_sessionCache->m_autocompleteTemplate.reset(new AutocompleteTemplate(m_sessionCache->m_objectPool));    
        m_sessionCache->doReloadObjectCache(false);
    }
}

void SessionCache::OnDisconnect ()
{
    if (GetSessionCache().get())
    {
        m_sessionCache->m_objectPool.reset(0);
        m_sessionCache->m_autocompleteTemplate.reset(0);    
        m_sessionCache->resetLanguageKeywordMap(); 
    }
}

void SessionCache::Reload (bool force)
{
    if (GetSessionCache().get())
        m_sessionCache->doReloadObjectCache(force);
}

void SessionCache::InitIfNecessary ()
{
    if (GetSessionCache().get())
    {
        if (!m_sessionCache->m_objectPool.get())
            m_sessionCache->m_objectPool.reset(new TreeNodePool);
        if (!m_sessionCache->m_autocompleteTemplate.get())
            m_sessionCache->m_autocompleteTemplate.reset(new AutocompleteTemplate(m_sessionCache->m_objectPool));    
    }
}

void SessionCache::doReloadObjectCache (bool force)
{
    ServerBackgroundThread::BkgdRequestQueue::Get().Push(ServerBackgroundThread::TaskPtr(new SrvBkLoadObjectCache(m_objectPool, m_autocompleteTemplate, force)));
}

void SessionCache::resetLanguageKeywordMap ()
{
    CWaitCursor wait;

    LanguagePtr lang = LanguagesCollection::Find("PL/SQL");
            
    int userObjectsGroup = -1;
    const vector<string>& groups = lang->GetKeywordGroups();
    for (int i = 0; i < (int)groups.size(); ++i) {
        if (groups.at(i) == "User Objects") {
            userObjectsGroup = i;
            break;
        }
    }

    LanguageKeywordMapPtr orgKeywordMap = lang->GetLanguageKeywordMap();
    LanguageKeywordMapPtr newKeywordMap(new LanguageKeywordMap);

    LanguageKeywordMapConstIterator 
        it = orgKeywordMap->begin(),
        end = orgKeywordMap->end();
    for (it; it != end; ++it)
        if (it->second.groupIndex != userObjectsGroup)
            newKeywordMap->insert(pair<std::wstring, LanguageKeyword>(it->first, it->second));

    lang->SetLanguageKeywordMap(newKeywordMap);
    AfxGetMainWnd()->RedrawWindow(NULL, NULL, RDW_INVALIDATE|RDW_NOERASE|RDW_NOFRAME|RDW_ALLCHILDREN);
}

OpenEditor::TemplatePtr 
SessionCache::GetAutocompleteSubobjectTemplate (string& object)
{
    CWaitCursor wait;

    return OpenEditor::TemplatePtr(new AutocompleteSubobjectTemplate(m_sessionCache->m_objectPool, object));
}

