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

#pragma once
#ifndef __STATVIEW_H__
#define __STATVIEW_H__

#include <map>
#include <set>
#include <vector>
#include <string>
#include <OpenGrid/GridView.h>

    class CSQLToolsApp;
    using std::set;
    using std::map;
    using std::vector;
    using std::string;
    using namespace OG2;

    struct StatSet  
    {
        vector<string>  m_StatNames;
        map<string,int> m_MapNameToLine;

        bool IsEmpty () const { return m_StatNames.empty(); }

        void Load  ();
        void Flush ();
    };

    class StatGauge 
    {
    public:
        StatGauge ();

        void LoadStatSet ()                           { if (m_StatSet.IsEmpty()) m_StatSet.Load(); };
        void OnChangeStatSet ();

        bool StatisticAccessible ()                   { return m_StatisticAccessible; }
        const vector<string>& GetStatNames () const   { return m_StatSet.m_StatNames; }

        void Open  (OciConnect& connect);
        void Close ();

        void LoadStatistics (OciConnect& connect, map<int,int>&);
        void Calibrate (OciConnect& connect);

        static void BeginMeasure (OciConnect& connect);
        static void EndMeasure   (OciConnect& connect, vector<int>& data);
        
        void GetStatistics (vector<int>& data); // format 2 x row(s)

        static StatGauge m_theGauge;

    private:
        static StatSet m_StatSet;

        bool m_StatisticAccessible;

        map<int,int> m_MapRowToNum, 
                     m_CalibrateData;
        map<int,int> m_Statistics[2];

        string m_SID;
    };


class CStatView : public StrGridView
{
    friend struct BackgroundTask_OpenStatGauge;
public:
	CStatView();
	~CStatView();

    void LoadStatNames (const vector<string>&);
    void LoadStatistics (const vector<int>&);

    bool IsEmpty () const { return m_pManager->m_pSource->GetCount(edVert) ? false : true; }

    static void OpenAll  ();
    static void CloseAll ();

private:
    static CImageList m_imageList;
    static set<CStatView*> m_GridFamily;

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#endif//__STATVIEW_H__
