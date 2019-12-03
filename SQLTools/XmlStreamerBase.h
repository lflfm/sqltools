/*
TODO: move this class in OpenEditor package and re-use for settings sreamers including OpenEditor
*/

/* 
	SQLTools is a tool for Oracle database developers and DBAs.
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

#ifndef __XmlStreamerBase_h__
#define __XmlStreamerBase_h__

#include <afxmt.h>
#include <vector>

    class TiXmlNode;
    class TiXmlElement;
    class TiXmlDocument;
    namespace Common { class VisualAttributesSet; };

///////////////////////////////////////////////////////////////////////////
// XmlStreamerBase
///////////////////////////////////////////////////////////////////////////

class XmlStreamerBase
{
protected:
    XmlStreamerBase (const std::string& filename, bool backup);

    bool fileExists () const;
    void enableBackup (bool backup) { m_backup = backup; }

    void read  (void* ctx);
    void write (const void* ctx);

    virtual void read  (const TiXmlDocument&, void* ctx) = 0;
    virtual void write (TiXmlDocument&, bool fresh, const void* ctx) = 0;

    template <class T> static void readAwareness  (const TiXmlElement*, T&);
    template <class T> static void writeAwareness  (const T&, TiXmlElement*);
    void read  (const TiXmlElement*, Common::VisualAttributesSet&);
    void write (const Common::VisualAttributesSet&, TiXmlElement*);


    static void writeVersion (TiXmlElement* elem, int majorVersion, int minorVersion);
    void validateMajorVersion (const TiXmlElement* elem, int majorVersion) const;
    static TiXmlElement* getNodeSafe (TiXmlNode* parent, const char* name);
    static void buildNodeMap (TiXmlElement* parentElem, const char*, const char*, std::map<std::string, TiXmlElement*>&);
    static void buildNodeMap (const TiXmlElement* parentElem, const char*, const char*, std::map<std::string, const TiXmlElement*>&);
    static TiXmlElement* getNodebyMap (std::map<std::string, TiXmlElement*>&, const char* key, TiXmlElement*, const char*);

    const std::string& getFilename () const { return m_filename;}

private:
    bool m_backup;
    const std::string m_filename;
    CMutex m_fileAccessMutex;   // tinyxml uses fstream access
                                // so we have to use the mutex to lock the file 
                                // for concurrent access
    static const int LOCK_TIMEOUT = 1000;
};


template <class T> 
void XmlStreamerBase::readAwareness (const TiXmlElement* elem, T& inst)
{
    T::AwareCollection& awares = T::GetAwareCollection();
    T::AwareCollectionIterator it = awares.begin();
    for (; it != awares.end(); ++it)
    {
        if (const char* str = elem->Attribute((*it)->getName().c_str()))
            (*it)->setAsString(inst, str);
        else
            (*it)->initialize(inst);
    }
}

template <class T> 
void XmlStreamerBase::writeAwareness (const T& inst, TiXmlElement* elem)
{
    T::AwareCollection& awares = T::GetAwareCollection();
    T::AwareCollectionIterator it = awares.begin();
    for (; it != awares.end(); ++it)
    {
        std::string buffer;
        (*it)->getAsString(inst, buffer);
        elem->SetAttribute((*it)->getName().c_str(), buffer.c_str());
    }
}

#endif//__XmlStreamerBase_h__