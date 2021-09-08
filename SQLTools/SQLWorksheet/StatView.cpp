/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

//TODO: replace SESSTAT.DAT with xml config 

#include "stdafx.h"
#include <COMMON\AppGlobal.h>
#include "COMMON/AppUtilities.h"
#include "resource.h"
#include "SQLToolsSettings.h"
#include "SQLTools.h"
#include "StatView.h"
#include <OCI8/BCursor.h>
#include <OpenGrid/GridSourceBase.h>
#include "ServerBackgroundThread\TaskQueue.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include <ActivePrimeExecutionNote.h>

using namespace ServerBackgroundThread;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


    using namespace std;
    using Common::AppLoadTextFromResources;

    LPSCSTR cszStatNotAvailable =
        "V$SESSION, V$STATNAME or V$SESSTAT is not accessible!"
        "\nStatistics collection is disabled. If it is necessary,"
        "\nask the DBA to grant the privileges required to access these views.";

    StatSet StatGauge::m_StatSet;
    StatGauge StatGauge::m_theGauge;


void StatSet::Load ()
{
    //std::string path;
    //Global::GetSettingsPath(path);
    //path += "\\sesstat.dat";

	string text;
	if (!AppLoadTextFromResources(L"SESSTAT.DAT", RT_HTML, text) || text.empty())
        THROW_APP_EXCEPTION("Cannot load SESSTAT.DAT from resources");

    m_StatNames.clear();
    m_MapNameToLine.clear();
  
    std::istringstream is(text);
    std::string buffer;
    for (int row = 0; std::getline(is, buffer, '\n'); row++) 
    {
        if (!buffer.empty() && *buffer.rbegin() == '\r')
            buffer.erase(buffer.end()-1);

        m_StatNames.push_back(buffer);
        m_MapNameToLine.insert(std::map<string,int>::value_type(buffer, row));
    }
}

void StatSet::Flush ()
{
    m_StatNames.clear();
    m_MapNameToLine.clear();
}

StatGauge::StatGauge ()
{
}

void StatGauge::Open (OciConnect& connect)
{
    LoadStatSet();

    try 
    {
        {
            OciCursor curs(connect, "select sid from v$session where audsid = userenv('SESSIONID')", 1);
            
            curs.Execute();
            
            if (curs.Fetch())
                curs.GetString(0, m_SID);

            curs.Close();
        }

        m_MapRowToNum.clear();

        OciCursor curs(connect, "select name, statistic# from v$statname", 300);
        
        curs.Execute();
        while (curs.Fetch()) 
        {
            map<string,int>::const_iterator 
                it = m_StatSet.m_MapNameToLine.find(curs.ToString(0));

            if (it != m_StatSet.m_MapNameToLine.end()) 
            {
                m_MapRowToNum.insert(std::map<int,int>::value_type(it->second, curs.ToInt(1)));
            }
        }

        curs.Close();

        Calibrate(connect);
    } 
    catch (...) 
    {
        m_StatisticAccessible = false;
        throw;
    }
    m_StatisticAccessible = true;
}

void StatGauge::Close ()
{
    m_StatisticAccessible = false;
}

void StatGauge::LoadStatistics (OciConnect& connect, map<int,int>& data)
{
    data.clear();

    OciCursor stats(connect, "select statistic#,value from v$sesstat where sid = :sid", 300);
    stats.Bind(":sid", m_SID.c_str());
    stats.Execute();
    while(stats.Fetch()) 
        data.insert(map<int,int>::value_type(stats.ToInt(0), stats.ToInt(1)));
}

void StatGauge::Calibrate (OciConnect& connect)
{
    m_CalibrateData.clear();

    map<int,int> probes[2];
    LoadStatistics(connect, probes[0]);
    LoadStatistics(connect, probes[1]);

    map<int,int>::const_iterator 
        it(probes[0].begin()), end(probes[0].end()),
        it2, end2(probes[1].end());

    for (; it != end; it++)
    {
        it2 = probes[1].find(it->first);
        if (it2 != end2)
        {
            m_CalibrateData.insert(
                map<int,int>::value_type(
                    it->first, it2->second - it->second));
        }
        else
            _ASSERTE(0);
    }
}

void StatGauge::BeginMeasure (OciConnect& connect)
{
    if (m_theGauge.m_StatisticAccessible)
        m_theGauge.LoadStatistics(connect, m_theGauge.m_Statistics[0]);
}

void StatGauge::EndMeasure (OciConnect& connect, vector<int>& data)
{
    if (m_theGauge.m_StatisticAccessible)
    {
        m_theGauge.LoadStatistics(connect, m_theGauge.m_Statistics[1]);
        m_theGauge.GetStatistics(data);
    }
}

void StatGauge::GetStatistics (vector<int>& _data) // format 2 x row(s)
{
    vector<int> data;

    if (m_StatisticAccessible)
    {
        int nrows = m_StatSet.m_StatNames.size();

        for (int row = 0; row < nrows; ++row)
        {
            map<int,int>::const_iterator 
                rowIt = m_MapRowToNum.find(row);

            if (rowIt != m_MapRowToNum.end())
            {
                map<int,int>::const_iterator 
                    deltaIt(m_CalibrateData.find(rowIt->second)),
                    prb1It (m_Statistics[0].find(rowIt->second)),
                    prb2It (m_Statistics[1].find(rowIt->second));

                if (deltaIt != m_CalibrateData.end()
                && prb1It != m_Statistics[0].end()
                && prb2It != m_Statistics[1].end())
                {
                    data.push_back((prb2It->second) - (prb1It->second) - (deltaIt->second));
                    data.push_back(prb2It->second);
                }
                else
                {
                    data.push_back(INT_MAX);
                    data.push_back(INT_MAX);
                }
            }
            else
            {
                data.push_back(INT_MAX);
                data.push_back(INT_MAX);
            }
        }
    }

    data.swap(_data);
}

/////////////////////////////////////////////////////////////////////////////
// CStatView
set<CStatView*> CStatView::m_GridFamily;

	struct BackgroundTask_OpenStatGauge : Task
    {
        vector<string> m_names;
        bool m_failure;

        BackgroundTask_OpenStatGauge () : Task("OpenStatGauge", 0), m_failure(false)
        {
            //SetSilent(true);
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;
                StatGauge::m_theGauge.Open(connect);
            }
            catch (const OciException& x)
            {
                m_failure = true;
                MessageBeep(MB_ICONHAND);
                AfxMessageBox(Common::wstr((x == 942) ? cszStatNotAvailable : x.what()).c_str());
            }

            m_names = StatGauge::m_theGauge.GetStatNames();
        }

        void ReturnInForeground ()
        {
            if (!m_failure)
            {
                std::set<CStatView*>::iterator it = CStatView::m_GridFamily.begin();

                for (; it != CStatView::m_GridFamily.end(); ++it)
                    (*it)->LoadStatNames(m_names);
            }
            else
            {
                GetSQLToolsSettingsForUpdate().SetSessionStatistics(false);

                std::set<CStatView*>::iterator it = CStatView::m_GridFamily.begin();

                for (; it != CStatView::m_GridFamily.end(); ++it)
                    (*it)->Clear();
            }
        }
    };

void CStatView::OpenAll ()
{
    FrgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_OpenStatGauge));
}

	struct BackgroundTask_CloseStatGauge : Task
    {
        BackgroundTask_CloseStatGauge () : Task("CloseStatGauge", 0)
        {
            //SetSilent(true);
        }

        void DoInBackground (OciConnect&)
        {
            StatGauge::m_theGauge.Close();
        }
    };

void CStatView::CloseAll ()
{
    FrgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_CloseStatGauge));

    std::set<CStatView*>::iterator it = m_GridFamily.begin();

    for (; it != m_GridFamily.end(); ++it)
        (*it)->Clear();
}

void CStatView::LoadStatNames (const vector<string>& names)
{
    if (m_pStrSource->GetCount(edVert))
        Clear();

    vector<string>::const_iterator it = names.begin();
    for (; it != names.end(); ++it) 
    {
        int row = m_pStrSource->Append();
        m_pStrSource->SetString(row, 0, *it);
        m_pStrSource->SetImageIndex(row, 6);
    }

    if (m_hWnd)
        m_pManager->Refresh();
}

void CStatView::LoadStatistics (const vector<int>& data)
{
    vector<int>::const_iterator it = data.begin();

    int nrows = m_pStrSource->GetCount(edVert);
        
    for (int row = 0; row < nrows && it != data.end(); ++row, ++it) 
    {
        char szBuff[18];
        m_pStrSource->SetString(row, 1, (*it != INT_MAX) ? itoa(*it, szBuff, 10) : "");
        m_pStrSource->SetString(row, 2, (*(++it) != INT_MAX) ? itoa(*it, szBuff, 10) : "");
        m_pStrSource->SetImageIndex(row, (*it != INT_MAX) ? 6 : 3);
    }

    m_pManager->Refresh();
}

    CImageList CStatView::m_imageList;

	struct BackgroundTask_GetStatNames : Task
    {
        HWND m_hWnd;
        CStatView* m_view;
        vector<string> m_names;

        BackgroundTask_GetStatNames (CStatView* view) 
            : Task("GetStatNames", 0),
            m_hWnd(*view),
            m_view(view)
        {
            SetSilent(true);
        }

        void DoInBackground (OciConnect&)
        {
            m_names = StatGauge::m_theGauge.GetStatNames();
        }

        void ReturnInForeground ()
        {
            if (IsWindow(m_hWnd))
                m_view->LoadStatNames(m_names);
        }
    };

CStatView::CStatView ()
{
    SetVisualAttributeSetName("Statistics Window");

    _ASSERTE(m_pStrSource);
    m_pStrSource->SetCols(3);
    m_pStrSource->SetColCharWidth(0, 32);
    m_pStrSource->SetColCharWidth(1, 16);
    m_pStrSource->SetColCharWidth(2, 16);
    m_pStrSource->SetColHeader(0, "Name");
    m_pStrSource->SetColHeader(1, "Last");
    m_pStrSource->SetColAlignment(1, ecaRight);
    m_pStrSource->SetColHeader(2, "Total");
    m_pStrSource->SetColAlignment(2, ecaRight);
    m_pStrSource->SetShowRowEnumeration(false);
    m_pStrSource->SetShowTransparentFixCol(true);

    m_pStrSource->SetFixSize(eoVert, edHorz, 3);

    m_pManager->m_Options.m_HorzLine  = false;
    m_pManager->m_Options.m_VertLine  = false;
    m_pManager->m_Options.m_RowSelect = true;
    m_pManager->m_Options.m_ExpandLastCol = false;

    if (!m_imageList.m_hImageList)
        m_imageList.Create(IDB_PANE_ICONS, 16, 64, RGB(0,255,0));

    m_pStrSource->SetImageList(&m_imageList, FALSE);

    m_GridFamily.insert(this);
}

CStatView::~CStatView ()
{
    try { EXCEPTION_FRAME;
        m_GridFamily.erase(this);
    } _DESTRUCTOR_HANDLER_;
}

BEGIN_MESSAGE_MAP(CStatView, StrGridView)
    ON_WM_CREATE()
END_MESSAGE_MAP()


int CStatView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (StrGridView::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (GetSQLToolsSettings().GetSessionStatistics())
        FrgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_GetStatNames(this)));

    return 0;
}
