/* 
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

// TODO: detect missing properties and assign default values
// TODO: incremental update - better backward/forward compatibility

#include "stdafx.h"
#include <shlwapi.h>
#include "COMMON\ExceptionHelper.h"
#include "COMMON/AppUtilities.h"
#include "OpenEditor\OESettingsXmlStreamer.h"
#include "TinyXml\TinyXml.h"
#include "COMMON\VisualAttributes.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

namespace OpenEditor
{
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;
    using Common::AppLoadTextFromResources;

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 1; // any minor changes

    static const char* ROOT_ELEM            = "OpenEditor";
    static const char* SETTINGS             = "Settings";
    static const char* CLASSES              = "Categories";
    static const char* CLASS                = "Category";
    static const char* VISUAL_ATTRIBUTES    = "VisualAttributes";
    static const char* VISUAL_ATTRIBUTE     = "VisualAttribute";
    static const char* COLUMN_MARKERS       = "ColumnMarkers";
    static const char* COLUMN_MARKER        = "ColumnMarker";

    static const int LOCK_TIMEOUT = 1000;
///////////////////////////////////////////////////////////////////////////
// SettingsXmlStreamer
///////////////////////////////////////////////////////////////////////////
SettingsXmlStreamer::SettingsXmlStreamer (const std::string& filename) 
: m_filename(filename),
  m_fileAccessMutex(FALSE, "GNU.OpenEditor.Settings")
{
}

void SettingsXmlStreamer::operator >> (SettingsManager& mgr)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_filename + '"');

    TiXmlDocument doc;

    if (::PathFileExists(m_filename.c_str()))
    {
        if (!doc.LoadFile(m_filename.c_str()))
            THROW_APP_EXCEPTION("Cannot parse \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());
    }
    else // no problem - will load settings from resources
    {
		string xmlText;
		if (!AppLoadTextFromResources(::PathFindFileName(m_filename.c_str()), RT_HTML, xmlText) || xmlText.empty())
            THROW_APP_EXCEPTION("Cannot load default settings from resources");

        if (!doc.Parse(xmlText.c_str()))
            THROW_APP_EXCEPTION(string("Cannot parse default settings\nERROR: ") + doc.ErrorDesc());
    }

    if (const TiXmlNode* rootElem = doc.FirstChildElement(ROOT_ELEM))
    {
        if (const TiXmlElement* settingsElem = rootElem->FirstChildElement(SETTINGS))
        {
            int majorVer;
            if (settingsElem->QueryIntAttribute("MajorVersion", &majorVer) != TIXML_SUCCESS)
                majorVer = 0;
            if (majorVer != MAJOR_VERSION)
                THROW_APP_EXCEPTION("Cannot load \""  + m_filename + "\".\nERROR: Unsupported settings version.");

            readAwareness(settingsElem, *mgr.GetGlobalSettings());
            if (const TiXmlElement* classesElem = settingsElem->FirstChildElement(CLASSES))
            {
                const TiXmlElement* classElem = classesElem->FirstChildElement(CLASS);
                for (; classElem; classElem = classElem->NextSiblingElement(CLASS))
                {
                    ClassSettingsPtr classSettingsPtr(new ClassSettings);
                    readAwareness(classElem, *classSettingsPtr);

                    if (const TiXmlElement* vasetElem = classElem->FirstChildElement(VISUAL_ATTRIBUTES))
                        read(vasetElem, classSettingsPtr->GetVisualAttributesSet());
                    // else ERROR

                    if (classSettingsPtr->GetName() == "PL/SQL")
                    {
                        try 
                        {
                            classSettingsPtr->GetVisualAttributesSet().FindByName("User Objects");
                        }
                        catch (std::out_of_range&)
                        {
                            VisualAttribute attr;
                            attr.m_Name         = "User Objects";
                            attr.m_FontBold     = 1;
                            attr.m_Mask         = 0x170;
                            attr.m_Foreground   = 0xFF0080;
                            attr.m_Background   = 0;
                            classSettingsPtr->GetVisualAttributesSet().Add(attr);
                        }
                    }

                    try 
                    {
                        classSettingsPtr->GetVisualAttributesSet().FindByName("Highlighting");
                    }
                    catch (std::out_of_range&)
                    {
                        VisualAttribute attrText = classSettingsPtr->GetVisualAttributesSet().FindByName("Text");

                        VisualAttribute attr;
                        attr.m_Name         = "Highlighting";
                        attr.m_Mask         = Common::vaoBackground;
                        attr.m_Foreground   = 0;
                        attr.m_Background   = (attrText.m_Background != 0) ? 0xEDE2D8 : (0xEDE2D8 - RGB(127,127,127));
                        classSettingsPtr->GetVisualAttributesSet().Add(attr);
                    }
                    try 
                    {
                        classSettingsPtr->GetVisualAttributesSet().FindByName("Error Highlighting");
                    }
                    catch (std::out_of_range&)
                    {
                        VisualAttribute attrText = classSettingsPtr->GetVisualAttributesSet().FindByName("Text");

                        VisualAttribute attr;
                        attr.m_Name         = "Error Highlighting";
                        attr.m_Mask         = Common::vaoBackground;
                        attr.m_Foreground   = 0;
                        attr.m_Background   = (attrText.m_Background != 0) ? RGB(255,192,192) : RGB(192,64,64);
                        classSettingsPtr->GetVisualAttributesSet().Add(attr);
                    }


                    if (const TiXmlElement* markersElem = classElem->FirstChildElement(COLUMN_MARKERS))
                        read(markersElem, COLUMN_MARKER, classSettingsPtr->GetColumnMarkersSet());
                    
                    mgr.m_classSettingsVector.push_back(classSettingsPtr);
                }
            }
            else 
                THROW_APP_EXCEPTION("\"" + m_filename + "\" does not contain any category.");
        }
        else 
            THROW_APP_EXCEPTION("\"" + m_filename + "\" does not contain settings.");
    }
}

void SettingsXmlStreamer::read (const TiXmlElement* parentElem, VisualAttributesSet& vaset)
{
    const TiXmlElement* elem = parentElem->FirstChildElement(VISUAL_ATTRIBUTE);
    for (; elem; elem = elem->NextSiblingElement(VISUAL_ATTRIBUTE))
    {
        VisualAttribute va;
        readAwareness(elem, va);
        vaset.Add(va);
    }
}

template <class T> 
void SettingsXmlStreamer::readAwareness (const TiXmlElement* elem, T& inst)
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
void SettingsXmlStreamer::read (const TiXmlElement* parentElem, const char* elemName, vector<T>& vec)
{
    const TiXmlElement* elem = parentElem->FirstChildElement(elemName);
    for (; elem; elem = elem->NextSiblingElement(elemName))
    {
        T v;
        if (elem->Attribute("value", &v))
            vec.push_back(v);
    }
}

//// This is a straightforward implementation of the operator <<
//// which simply overwrites the settings file 
//void SettingsXmlStreamer::operator << (const SettingsManager& mgr)
//{
//    CSingleLock lock(&m_fileAccessMutex);
//    BOOL locked = lock.Lock(LOCK_TIMEOUT);
//    if (!locked) 
//        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_filename + '"');
//
//    TiXmlDocument doc;
//    TiXmlDeclaration decl("1.0", ""/*"Windows-1252"*/, "yes");
//    doc.InsertEndChild(decl);
//
//    std::auto_ptr<TiXmlElement> rootElem(new TiXmlElement(ROOT_ELEM));
//
//    std::auto_ptr<TiXmlElement> settingsElem(new TiXmlElement(SETTINGS));
//    settingsElem->SetAttribute("MajorVersion", MAJOR_VERSION);
//    settingsElem->SetAttribute("MinorVersion", MINOR_VERSION);
//    writeAwareness(*mgr.GetGlobalSettings(), settingsElem.get());
//    
//    std::auto_ptr<TiXmlElement> classesElem(new TiXmlElement(CLASSES));
//
//    int count = mgr.GetClassCount();
//    for (int i = 0; i < count; i++)
//    {
//        ClassSettingsPtr classPtr = mgr.GetClassByPos(i);
//        
//        std::auto_ptr<TiXmlElement> classElem(new TiXmlElement(CLASS));
//        writeAwareness(*classPtr, classElem.get());
//
//        std::auto_ptr<TiXmlElement> vasetElem(new TiXmlElement(VISUAL_ATTRIBUTES));
//        write(classPtr->GetVisualAttributesSet(), vasetElem.get());
//        classElem->LinkEndChild(vasetElem.release());
//        
//        std::auto_ptr<TiXmlElement> markersElem(new TiXmlElement(COLUMN_MARKERS));
//        write(classPtr->GetColumnMarkersSet(), markersElem.get(), COLUMN_MARKER);
//        classElem->LinkEndChild(markersElem.release());
//
//        classesElem->LinkEndChild(classElem.release());
//    }
//
//    settingsElem->LinkEndChild(classesElem.release());
//    rootElem->LinkEndChild(settingsElem.release());
//    doc.LinkEndChild(rootElem.release());
//
//    if (!doc.SaveFile(m_filename.c_str()))
//        THROW_APP_EXCEPTION("Cannot save \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());
//}


    static
    TiXmlElement* getNodeSafe (TiXmlNode* parentElem, const char* name)
    {
        TiXmlElement* elem = parentElem->FirstChildElement(name);
        if (!elem)
        {
            elem = new TiXmlElement(name);
            parentElem->LinkEndChild(elem);
        }
        return elem;
    }
    static 
    TiXmlElement* getNodeSafe (map<string, TiXmlElement*>& elemMap, const string& key, TiXmlElement* parentElem, const char* elemName)
    {
        TiXmlElement* elem = 0;
        map<string, TiXmlElement*>::const_iterator it = elemMap.find(key);
        
        if (it != elemMap.end())
            elem = it->second;
        else
        {
            elem = new TiXmlElement(elemName);
            parentElem->LinkEndChild(elem);
        }
        return elem;
    }
    static
    void buildNodeMap (TiXmlElement* parentElem, const char* name, map<string, TiXmlElement*>& map)
    {
        TiXmlElement* elem = parentElem->FirstChildElement(name);
        for (; elem; elem = elem->NextSiblingElement(name))
        {
            if (const char* name = elem->Attribute("Name"))
                map.insert(make_pair(string(name), elem));
        }
    }

/*
    The method reads and parses the file and then overwrites only known settings
    keeping any extention unchanged. 
    In theory it allows two different versions of the program 
    to use the same configuration file.
*/
void SettingsXmlStreamer::operator << (const SettingsManager& mgr)
{
    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION(string("Cannot get exclusive access to \"") + m_filename + '"');

    TiXmlDocument doc;
    if (!::PathFileExists(m_filename.c_str()))
        doc.InsertEndChild(TiXmlDeclaration("1.0", ""/*"Windows-1252"*/, "yes"));
    else if (!doc.LoadFile(m_filename.c_str()))
        THROW_APP_EXCEPTION("Cannot read \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());

    TiXmlNode* rootElem = getNodeSafe(&doc, ROOT_ELEM);
    TiXmlElement* settingsElem = getNodeSafe(rootElem, SETTINGS);
    
    int majorVer;
    if (settingsElem->QueryIntAttribute("MajorVersion", &majorVer) != TIXML_SUCCESS)
        majorVer = MAJOR_VERSION;
    if (majorVer != MAJOR_VERSION)
        THROW_APP_EXCEPTION("Cannot load \""  + m_filename + "\".\nERROR: Unsupported settings version.");

    settingsElem->SetAttribute("MajorVersion", MAJOR_VERSION);
    settingsElem->SetAttribute("MinorVersion", MINOR_VERSION);

    writeAwareness(*mgr.GetGlobalSettings(), settingsElem);

    TiXmlElement* classesElem = getNodeSafe(settingsElem, CLASSES);

    map<string, TiXmlElement*> classes;
    buildNodeMap(classesElem, CLASS, classes);

    for (int i = 0, count = mgr.GetClassCount(); i < count; i++)
    {
        ClassSettingsPtr classPtr = mgr.GetClassByPos(i);
        
        TiXmlElement* classElem = getNodeSafe(classes, classPtr->GetName(), classesElem, CLASS); 
        writeAwareness(*classPtr, classElem);

        TiXmlElement* vasetElem = getNodeSafe(classElem, VISUAL_ATTRIBUTES);
        write(classPtr->GetVisualAttributesSet(), vasetElem);
        
        TiXmlElement* markersElem = classElem->FirstChildElement(COLUMN_MARKERS);
        if (markersElem)
            classElem->RemoveChild(markersElem);
        
        markersElem = new TiXmlElement(COLUMN_MARKERS);
        classElem->LinkEndChild(markersElem);

        write(classPtr->GetColumnMarkersSet(), markersElem, COLUMN_MARKER);
    }

    if (!doc.SaveFile(m_filename.c_str()))
        THROW_APP_EXCEPTION("Cannot save \"" + m_filename + "\"\nERROR: " + doc.ErrorDesc());
}

//// This is a straightforward implementation of the method write
//// which simply overwrites the settings 
//void SettingsXmlStreamer::write (const VisualAttributesSet& vaset, TiXmlElement* vasetElem)
//{
//    for (int i = 0, count = vaset.GetCount(); i < count; i++)
//    {
//        std::auto_ptr<TiXmlElement> vaElem(new TiXmlElement(VISUAL_ATTRIBUTE));
//        writeAwareness(vaset[i], vaElem.get());
//        vasetElem->LinkEndChild(vaElem.release());
//    }
//}
//
void SettingsXmlStreamer::write (const VisualAttributesSet& vaset, TiXmlElement* vasetElem)
{
    map<string, TiXmlElement*> vaelements;
    buildNodeMap(vasetElem, VISUAL_ATTRIBUTE, vaelements);

    for (int i = 0, count = vaset.GetCount(); i < count; i++)
    {
        TiXmlElement* vaElem = getNodeSafe(vaelements, vaset[i].m_Name, vasetElem, VISUAL_ATTRIBUTE);
        writeAwareness(vaset[i], vaElem);
    }
}

template <class T> 
void SettingsXmlStreamer::writeAwareness (const T& inst, TiXmlElement* elem)
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

template <class T> 
void SettingsXmlStreamer::write (const vector<T>& vec, TiXmlElement* parentElem, const char* elemName)
{
    vector<T>::const_iterator it = vec.begin();
    for (; it != vec.end(); ++it)
    {
        std::auto_ptr<TiXmlElement> elem(new TiXmlElement(elemName));
        elem->SetAttribute("value", *it);
        parentElem->LinkEndChild(elem.release());
    }
}

};//namespace OpenEditor
