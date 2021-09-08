#include "stdafx.h"
#include "TinyXml\TinyXml.h"
#include "ConnectData.h"
#include "ConnectDataXmlStreamer.h"

    using namespace std;

    static const int MAJOR_VERSION = 1; // indicates incompatibility 
    static const int MINOR_VERSION = 1; // any minor changes

    static const char* ROOT_ELEM = "SQLTools";
    static const char* SETTINGS  = "Settings";
    static const char* CONN_LIST = "Connections";
    static const char* CONN_ELEM = "Connection";

ConnectDataXmlStreamer::ConnectDataXmlStreamer (const std::wstring& filename, bool backup)
    : XmlStreamerBase(filename, backup), 
    m_overridePassword(false)
{
}

void ConnectDataXmlStreamer::read (const TiXmlDocument& doc, void* ctx)
{
    ConnectData* data = reinterpret_cast<ConnectData*>(ctx);

    if (const TiXmlNode* rootElem = doc.FirstChildElement(ROOT_ELEM))
    {
        if (const TiXmlElement* settingsElem = rootElem->FirstChildElement(SETTINGS))
        {
            validateMajorVersion(settingsElem, MAJOR_VERSION);
            readAwareness(settingsElem, *data);

            if (const TiXmlElement* connListElem = settingsElem->FirstChildElement(CONN_LIST))
            {
                const TiXmlElement* connElem = connListElem->FirstChildElement(CONN_ELEM);
                for (; connElem; connElem = connElem->NextSiblingElement(CONN_ELEM))
                {
                    ConnectEntry entry;
                    readAwareness(connElem, entry);
                    entry.m_status = ConnectEntry::UPTODATE;
                    data->m_entries.push_back(entry);
                }
            }
        }
        else 
            THROW_APP_EXCEPTION(L"\"" + getFilename() + L"\" does not contain connection information.");
    }
}

void ConnectDataXmlStreamer::write (TiXmlDocument& doc, bool createNew, const void* ctx)
{
    const ConnectData* data = reinterpret_cast<const ConnectData*>(ctx);

    TiXmlElement* rootElem = getNodeSafe(&doc, ROOT_ELEM);
    TiXmlElement* settingsElem = getNodeSafe(rootElem, SETTINGS);
    
    if (!createNew) 
    {
        validateMajorVersion(settingsElem, MAJOR_VERSION);
        if (!m_overridePassword)
        {
            string probe;
            if (const char* _probe = settingsElem->Attribute("Probe"))
                data->Decrypt(m_masterPassword, _probe, probe); // will throw if invalid password
        }
    }
    writeVersion(settingsElem, MAJOR_VERSION, MINOR_VERSION);

    writeAwareness(*data, settingsElem);

    TiXmlElement* connListElem = getNodeSafe(settingsElem, CONN_LIST);

    // save modified CIs only, usually only one
    vector<ConnectEntry>::const_iterator it = data->m_entries.begin();
    for (; it != data->m_entries.end(); ++it)
    {
        ASSERT(it->m_status != ConnectEntry::UNDEFINED);

        if (it->m_status != ConnectEntry::UPTODATE) 
        {
            TiXmlElement* connElem = connListElem->FirstChildElement(CONN_ELEM);
            for (; connElem; connElem = connElem->NextSiblingElement(CONN_ELEM))
            {
                ConnectEntry entry;
                readAwareness(connElem, entry);
                if (ConnectEntry::IsSignatureEql(*it, entry))
                {
                    if (it->m_status == ConnectEntry::DELETED)
                    {
                        connListElem->RemoveChild(connElem);
                        connElem = 0;
                    }
                    else
                    {
                        writeAwareness(*it, connElem);
                        it->m_status = ConnectEntry::UPTODATE;
                    }
                    break;
                }

            }
            if (it->m_status == ConnectEntry::MODIFIED) 
            {
                /*TiXmlElement**/ connElem = new TiXmlElement(CONN_ELEM);
                connListElem->LinkEndChild(connElem);
                writeAwareness(*it, connElem);
                it->m_status = ConnectEntry::UPTODATE;
            }
        }
    }
}
