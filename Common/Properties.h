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

#pragma once
#include <vector>
#include <sstream>
#include "MyUtf.h"

enum EPropertyAwareOpt
{
    PO_LOAD_ON_INIT_ONLY = 0x01,
};

template <class T>
struct PropertyAware
{
    unsigned int opt;
    PropertyAware () : opt(0) {};
    virtual ~PropertyAware () {};
    virtual std::string getName () const = 0;
    virtual void initialize (T&) const = 0;
    virtual void getAsString (const T&, std::string&) const = 0;
    virtual void setAsString (T&, const char*) const = 0;
};


template <class T>
class PropertyBinder {
public:
    PropertyBinder () {}
    void add (PropertyAware<T>* aware) { m_awares.push_back(aware); }
    const std::vector<PropertyAware<T>*>& GetAwares () const { return m_awares; }
    void initialize (T& inst) const {
        std::vector<PropertyAware<T>*>::const_iterator it;
        for (it = m_awares.begin(); it != m_awares.end(); ++it)
            (*it)->initialize(inst);
    }
private:
    PropertyBinder (const PropertyBinder&);
    void operator = (const PropertyBinder&);
    std::vector<PropertyAware<T>*> m_awares;
};


template <class T, class V>
struct PropertyAwareImpl : PropertyAware<T>
{
    typedef const V& (T::*GetPfm)() const;
    typedef void (T::*SetPfm)(const V&);
    
    std::string m_name;
    GetPfm m_getPfm;
    SetPfm m_setPfm;
    V m_defVal;

    PropertyAwareImpl (PropertyBinder<T>& binder, const char* name, GetPfm getPfm, SetPfm setPfm, const V& defVal) 
        : m_name(name), m_getPfm(getPfm), m_setPfm(setPfm), m_defVal(defVal) { binder.add(this); }

    std::string getName () const { return m_name; }

    void initialize (T& inst) const { (inst.*m_setPfm)(m_defVal); }

    void getAsString (const T&, std::string&) const;
    void setAsString (T&, const char*) const;

    static void to_string (const std::string& val, std::string& str)   { str = val; }
    static void to_string (const std::wstring& val, std::string& str)  { str = Common::str(val); }
    static void to_string (int val, std::string& str)                  { char buff[80]; itoa(val, buff, 10); str = buff; }
    static void to_string (__int64 val, std::string& str)              { char buff[80]; _i64toa(val, buff, 10); str = buff; }
    static void to_string (unsigned int val, std::string& str)         { char buff[80]; ultoa(val, buff, 10); str = buff; }
    static void to_string (double val, std::string& str)               { char buff[80]; _gcvt(val, 12, buff); str = buff; }
    static void to_string (bool val, std::string& str)                 { str = val ? "1" : "0"; }
    static void to_string (unsigned __int64 val, std::string& str)     { char buff[80]; _ui64toa(val, buff, 10); str = buff; }
#if _MFC_VER < 0x0800
    static void to_string (time_t val, std::string& str)               { char buff[80]; _ui64toa(val, buff, 10); str = buff; }
#endif

    static void from_string (const char* str, std::string& val)        { val = str; }
    static void from_string (const char* str, std::wstring& val)       { val = Common::wstr(str); }
    static void from_string (const char* str, int& val)                { val = str ? atoi(str) : 0; }
    static void from_string (const char* str, __int64& val)            { val = str ? _atoi64(str) : 0; }
    static void from_string (const char* str, unsigned int& val)       { val = str ? (unsigned int)_atoi64(str) : 0U; }
    static void from_string (const char* str, double& val)             { val = str ? atof(str) : .0; }
    static void from_string (const char* str, bool& val)               { val = str ? ((atoi(str) != 0) ? true : false) : false; }
    static void from_string (const char* str, unsigned __int64& val)   { val = str ? _atoi64(str) : 0U; }
#if _MFC_VER < 0x0800
    static void from_string (const char* str, time_t& val)             { char* dummy; val = (time_t)(str ? _strtoui64(str, &dummy, 10) : 0U); }
#endif
};

template <class T, class V>
void PropertyAwareImpl<T,V>::getAsString (const T& inst, std::string& str) const 
{ 
    to_string((inst.*m_getPfm)(), str);
}

template <class T, class V>
void PropertyAwareImpl<T,V>::setAsString (T& inst, const char* str) const
{
    V val;
    from_string(str, val);
    (inst.*m_setPfm)(val);
}

#define CMN_GET_THIS_PROPERTY_BINDER  \
    m_propertyBinder
#define CMN_GET_PROPERTY_BINDER(cls)  \
    cls::m_propertyBinder
#define CMN_GET_PROPERTY_AWARE(name) \
    m_propertyAware_##name

#define CMN_DECL_PROPERTY_BINDER(cls) \
    typedef cls PropertyOwner; \
    typedef const std::vector<PropertyAware<cls>*> AwareCollection; \
    typedef cls::AwareCollection::const_iterator AwareCollectionIterator; \
    static AwareCollection& GetAwareCollection () { return m_propertyBinder.GetAwares(); } \
    static PropertyBinder<cls> m_propertyBinder

#define CMN_IMPL_PROPERTY_BINDER(cls) \
    PropertyBinder<cls> cls::m_propertyBinder

#define CMN_DECL_PROPERTY(access,type,name) \
    access: \
    type m_##name; \
    private: \
    const type& _get_##name () const { return m_##name; } \
    void _set_##name (const type& val) { m_##name = val; } \
    static struct PropertyAware_##name : PropertyAwareImpl<PropertyOwner,type> { \
        PropertyAware_##name (const type& defVal) \
            : PropertyAwareImpl<PropertyOwner,type> (m_propertyBinder, #name, &_get_##name, &_set_##name, defVal) {} \
    } m_propertyAware_##name;

#define CMN_DECL_PUBLIC_PROPERTY(type,name)  \
    CMN_DECL_PROPERTY(public,type,name)
#define CMN_DECL_PRIVATE_PROPERTY(type,name) \
    CMN_DECL_PROPERTY(private,type,name)

#define CMN_IMPL_PROPERTY(cls,name,def) \
    cls::PropertyAware_##name cls::m_propertyAware_##name(def)

//class Test
//{
//public:
//    CMN_CMN_DECL_PROPERTY_BINDER(Test);
//    CMN_DECL_PUBLIC_PROPERTY(Test, int,         Integer);
//    CMN_DECL_PUBLIC_PROPERTY(Test, std::string, String );
//    CMN_DECL_PUBLIC_PROPERTY(Test, bool,        Boolean);
//};
//
//CMN_CMN_IMPL_PROPERTY_BINDER(Test);
//CMN_IMPL_PROPERTY(Test, Integer, 111     );
//CMN_IMPL_PROPERTY(Test, String , "simple");
//CMN_IMPL_PROPERTY(Test, Boolean, true    );
//
//int main (int argc, char* argv[])
//{
//    Test test;
//    test.m_Integer(999);
//    Test::m_propertyBinder.initialize(test);
//
//    const std::vector<PropertyAware<Test>*> awares = CMN_GET_PROPERTY_BINDER(Test).GetAwares();
//    std::vector<PropertyAware<Test>*>::const_iterator it;
//    for (it = awares.begin(); it != awares.end(); ++it)
//    {
//        std::string buff;
//        (*it)->getAsString(test, buff);
//        std::cout << (*it)->getName() << " = " << buff  << std::endl;
//    }
//
//	return 0;
//}

