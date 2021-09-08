#pragma once
#include "XmlStreamerBase.h"

class ConnectData;

class ConnectDataXmlStreamer : protected XmlStreamerBase
{
public:
    ConnectDataXmlStreamer (const std::wstring& filename, bool backup = false);
    
    // only write requires password
    void SetMasterPassword (const std::string& masterPassword) { m_masterPassword = masterPassword; }
    void SetOverridePassword (bool overridePassword = true)    { m_overridePassword = overridePassword; }

    void operator >> (ConnectData& data)       { XmlStreamerBase::read(&data); }
    ConnectDataXmlStreamer& operator << (const ConnectData& data) { XmlStreamerBase::write(&data); return *this; }

    using XmlStreamerBase::fileExists;

	ConnectDataXmlStreamer& operator << (ConnectDataXmlStreamer& (__cdecl *_Pfn)(ConnectDataXmlStreamer&))
    {	
		return ((*_Pfn)(*this)); // call manipulator
	}

    void EnableBackup (bool backup) {  enableBackup(backup); }

private:
    bool m_overridePassword;
    std::string m_masterPassword;
    virtual void read  (const TiXmlDocument&, void* ctx);
    virtual void write (TiXmlDocument&, bool, const void* ctx);
};
