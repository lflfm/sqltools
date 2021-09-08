/* 
    Copyright (C) 2002, 2017 Aleksey Kochetov

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OETemplates_h__
#define __OETemplates_h__

#include <string>
#include <vector>
#include <map>
#include <arg_shared.h>
#include "OpenEditor/OEHelpers.h"

    #define OET_ATTRIBUTE(T,N) \
        private: \
            T m_##N; \
        public: \
            const T& Get##N () const { return m_##N; }; \
            void  Set##N (const T& val) { m_##N = val; } 

namespace OpenEditor
{
    using std::string;
    using std::vector;
    using std::map;
    using arg::counted_ptr;

    class Template;
    typedef arg::counted_ptr<Template> TemplatePtr;

    class Template 
    {
    public:
        Template ();
        virtual ~Template ()  {}

        struct Entry
        {
            string uuid;
            string name;
            string keyword;
            string text;
            int    minLength;
            int    curLine, curPos;
            int    image;
            mutable bool modified;
            bool deleted;

            Entry () { minLength = curLine = curPos = 0; image =-1; modified = true; deleted = false; }

            bool operator == (const string& _uuid) const { return this->uuid == _uuid; }
        };

        virtual bool ExpandKeyword (int index, string& text, Position& pos);
        virtual bool ExpandKeyword (const string& keyword, string& text, Position& pos);
        virtual bool AfterExpand   (const Entry&, string& /*text*/, Position& /*pos*/) { return true; };
        
        typedef std::vector<Entry>::const_iterator ConstIterator;
        ConstIterator Begin () const;
        ConstIterator End () const;
        int GetCount () const;
        const Entry& GetEntry (int) const;

        const vector<Entry>& GetEntries () const { return m_entries; }

        static void GenUUID (Entry&);

        void AppendEntry (const Entry&);
        void UpdateEntry (int pos, const Entry&);
        void DeleteEntry (int pos);

        bool ValidateUniqueness (const string& name, const string& uuid) const;

        OET_ATTRIBUTE(UINT,   ImageListRes);
        OET_ATTRIBUTE(string, KeyColumnHeader);
        OET_ATTRIBUTE(string, NameColumnHeader);

        bool GetAlwaysListIfAlternative () const          { return m_alwaysListIfAlternative; }; 
        void SetAlwaysListIfAlternative (bool val)        { m_alwaysListIfAlternative = val; m_modified = true; } 
        bool GetModified () const                         { return m_modified; }; 
        void SetModified (bool val) const                 { m_modified = val;  } 
        void ClearModified () const;

    private:
        bool m_alwaysListIfAlternative;
        mutable vector<Entry> m_entries;
        mutable bool m_modified;
    };

    
    
    class TemplateCollection
    {
    public:
        TemplateCollection () {};
        TemplateCollection (const TemplateCollection&);
        TemplateCollection& operator = (const TemplateCollection&);

        TemplatePtr Add (const string& lang);
        const TemplatePtr Find (const string& lang) const;

        typedef std::map<std::string, TemplatePtr>::const_iterator ConstIterator;
        ConstIterator Begin () const;
        ConstIterator End () const;
        int GetCount () const;
        void Clear ();

        bool IsModified () const;
        void ClearModified () const;

    private:
        map<string, TemplatePtr> m_templates;
    };

    
    inline
    Template::Template () : m_KeyColumnHeader("Key"), m_NameColumnHeader("Name"), m_ImageListRes((UINT)-1)
        { m_alwaysListIfAlternative = false; m_modified = false; }

    inline
    const Template::Entry& Template::GetEntry (int inx) const
        { return m_entries.at(inx); }

    inline
    Template::ConstIterator Template::Begin () const
        { return m_entries.begin(); }

    inline
    Template::ConstIterator Template::End () const
        { return m_entries.end(); }
    
    inline
    int Template::GetCount () const
        { return m_entries.size(); }

    inline
    TemplateCollection::ConstIterator TemplateCollection::Begin () const
        { return m_templates.begin(); }

    inline
    TemplateCollection::ConstIterator TemplateCollection::End () const
        { return m_templates.end(); }
    
    inline
    int TemplateCollection::GetCount () const
        { return m_templates.size(); }

    inline
    void TemplateCollection::Clear ()
        { m_templates.clear(); }
};

#endif//__OETemplates_h__
