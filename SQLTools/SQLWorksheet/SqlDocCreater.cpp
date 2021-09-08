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

#include "stdafx.h"
#include "SqlDocCreater.h"
#include "SQLToolsSettings.h"
#include "SQLWorksheetDoc.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OutputView.h"
#include "ErrorLoader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using namespace OraMetaDict;

#define TAB_OUT_ARR_SIZE (sizeof m_TableOutput / sizeof m_TableOutput[0])

SqlDocCreater::SqlDocCreater (bool singleDocument, bool useBanner, bool supressGroupByDDL, bool objectNameAsTitle)
: m_Output(true, 32*1024),
  m_singleDocument(singleDocument),
  m_bUseBanner(useBanner),
  m_supressGroupByDDL(supressGroupByDDL),
  m_objectNameAsTitle(objectNameAsTitle),
  m_bPackAsWhole(false),
  m_objectCounter(0)
{
    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
        m_TableOutput[i].Realloc(!i ? 256*1024 : 64*1024);
}

SqlDocCreater::~SqlDocCreater ()
{
    try { EXCEPTION_FRAME;

        Flush(true);
    }
    _DESTRUCTOR_HANDLER_
}

void SqlDocCreater::Clear ()
{
    m_strOwner.Empty();
    m_strName.Empty();
    m_strType.Empty();
    m_Output.Erase();
    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
            m_TableOutput[i].Erase();
    m_objectCounter = 0;
    m_bPackAsWhole = false;
}

void SqlDocCreater::NewLine ()
{
    m_Output.NewLine();
}

    static
    void put_split_line (TextOutput& out, int len = 60)
    {
        out.Put("PROMPT ");
        for (int i(sizeof("PROMPT ")-1); i < len; i++) out.Put("=");
        out.NewLine();
    }

    static
    void put_group_header (TextOutput& out, const char* title, int len = 80)
    {
        put_split_line(out, len);
        out.Put("PROMPT "); out.PutLine(title);
        put_split_line(out, len);
    }

void SqlDocCreater::Put (const DbObject& object, const SQLToolsSettings& settings)
{
    m_strOwner = object.GetOwner();
    m_strName  = object.GetName();
    m_strType  = object.GetTypeStr();
    m_strExt   = object.GetDefExt();

    m_Output.SetLowerDBName(settings.m_LowerNames);

    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
        m_TableOutput[i].SetLowerDBName(settings.m_LowerNames);

    bool splittedOutput = m_singleDocument 
        && (!m_supressGroupByDDL && settings.GetGroupByDDL())
        && !strcmp(m_strType,"TABLE");

    if (!splittedOutput && !object.IsLight() && m_objectCounter)
        m_Output.NewLine();

    bool bSpecWithBody = settings.GetSpecWithBody()
                         && (!strcmp(m_strType,"PACKAGE") || !strcmp(m_strType,"TYPE"));
    bool bBodyWithSpec = !bSpecWithBody && settings.GetBodyWithSpec()
                         && (!strcmp(m_strType,"PACKAGE BODY") || !strcmp(m_strType,"TYPE BODY"));

    if (splittedOutput)
    {
        const Table& table = static_cast<const Table&>(object);

        if (settings.m_TableDefinition)
            table.WriteDefinition(m_TableOutput[0], settings);

        if (settings.m_Comments)
            table.WriteComments(m_TableOutput[0], settings);

        m_TableOutput[0].NewLine();

        TextOutputInMEM out(true, 2*1024);
        out.SetLike(m_TableOutput[0]);

        if (settings.m_Indexes)
            table.WriteIndexes(out, settings);

        if (out.GetLength() > 0)
        {
            m_TableOutput[1] += out;
            //m_TableOutput[1].NewLine();
            out.Erase();
        }

        if (settings.m_Constraints)
            table.WriteConstraints(out, settings, 'C');

        if (out.GetLength() > 0)
        {
            m_TableOutput[2] += out;
            //m_TableOutput[2].NewLine();
            out.Erase();
        }

        if (settings.m_Constraints)
            table.WriteConstraints(out, settings, 'P');

        if (settings.m_Constraints)
            table.WriteConstraints(out, settings, 'U');

        if (out.GetLength() > 0)
        {
            m_TableOutput[3] += out;
            //m_TableOutput[3].NewLine();
            out.Erase();
        }

        if (settings.m_Constraints)
            table.WriteConstraints(out, settings, 'R');

        if (out.GetLength() > 0)
        {
            m_TableOutput[4] += out;
            //m_TableOutput[4].NewLine();
            out.Erase();
        }

        if (settings.m_Triggers)
            table.WriteTriggers(out, settings);

        if (out.GetLength() > 0)
        {
            m_TableOutput[5] += out;
            //m_TableOutput[5].NewLine();
            out.Erase();
        }

        if (settings.m_Grants)
            table.WriteGrants(out, settings);

        if (out.GetLength() > 0)
        {
            m_TableOutput[6] += out;
            //m_TableOutput[6].NewLine();
            out.Erase();
        }
    }
    else if (!(bSpecWithBody || bBodyWithSpec))
    {
        m_nOffset[0] = object.Write(m_Output, settings);
        object.WriteGrants(m_Output, settings);
    }
    else
    {
        m_bPackAsWhole = true;
        const DbObject* pPackage = 0;
        const DbObject* pPackageBody = 0;

        if (bSpecWithBody)
        {
            pPackage = &object;

            try
            {
                if (!strcmp(m_strType,"PACKAGE"))
                    pPackageBody = &object.GetDictionary().LookupPackageBody(m_strOwner, m_strName);
                else
                    pPackageBody = &object.GetDictionary().LookupTypeBody(m_strOwner, m_strName);
            }
            catch (const XNotFound&) {}

        }
        else
        {
            try
            {
                if (!strcmp(m_strType,"PACKAGE BODY"))
                    pPackage = &object.GetDictionary().LookupPackage(m_strOwner, m_strName);
                else
                    pPackage = &object.GetDictionary().LookupType(m_strOwner, m_strName);
            }
            catch (const XNotFound&) {}

            pPackageBody = &object;
        }

        /*if (!m_singleDocument)*/ m_Output.EnableLineCounter(true);

        if (pPackage)
            m_nOffset[0] = pPackage->Write(m_Output, settings);
        else
        {
            m_nOffset[0] = -1;
            m_Output.Put("PROMPT Attention: specification of ");
            m_Output.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            m_Output.Put(" not found!");
            //m_Output.NewLine();
            m_Output.NewLine();
        }

        int nPackSpecLines = m_Output.GetLineCounter();
        /*if (!m_singleDocument)*/ m_Output.EnableLineCounter(false);

        if (pPackageBody)
            m_nOffset[1] = pPackageBody->Write(m_Output, settings) + nPackSpecLines;
        else if (bSpecWithBody && !strcmp(m_strType,"PACKAGE BODY"))
        {
            m_nOffset[1] = -1;
            m_Output.Put("PROMPT Attention: body of ");
            m_Output.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            m_Output.Put(" not found!");
            //m_Output.NewLine();
            m_Output.NewLine();
        }
        else
            m_nOffset[1] = -1;

        if (pPackage)
            pPackage->WriteGrants(m_Output, settings);
    }

    m_objectCounter++;

    Flush();
}

void SqlDocCreater::Flush (bool bForce)
{
    if (m_objectCounter
    && (bForce || !m_singleDocument))
    {
        CDocTemplate* pDocTemplate = theApp.GetPLSDocTemplate();
        ASSERT(pDocTemplate);

        if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
        {
            ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);
            ASSERT(pDoc->m_pEditor);

            pDoc->DefaultFileFormat();

            std::vector<int> bookmarks;

            if (m_singleDocument && !m_strType.CompareNoCase("TABLE"))
            {
                LPSCSTR cszBanners[TAB_OUT_ARR_SIZE] =
                {
                    "TABLE DEFINITIONS",
                    "INDEXES",
                    "CHECK CONSTRAINTS",
                    "PRIMARY AND UNIQUE CONSTRAINTS",
                    "FOREIGN CONSTRAINTS",
                    "TRIGGERS",
                    "GRANTS",
                };

                if (m_TableOutput[0].GetPosition())
                {
                    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
                    {
                        if (m_TableOutput[i].GetPosition())
                        {
                            if (m_bUseBanner && GetSQLToolsSettings().m_GeneratePrompts)
                            {
                                put_group_header(m_Output, cszBanners[i]);
                                m_Output.NewLine();
                            }

                            int line = m_Output.CountLines();
                            
                            if (GetSQLToolsSettings().m_GeneratePrompts
                            && i != TAB_OUT_ARR_SIZE-1) // it is not grant section
                                line++;
                            
                            bookmarks.push_back(line);

                            m_Output += m_TableOutput[i];
                            //m_Output.NewLine();
                        }
                    }
                }
            }

            pDoc->SetText(m_Output, m_Output.GetLength());
            pDoc->SetModifiedFlag(FALSE);
            pDoc->m_LoadedFromDB = true;
            pDoc->SetEncodingCodepage(CP_UTF8);

            switch (pDoc->GetSettings().GetEncoding())
            {
            case OpenEditor::feANSI     :
                pDoc->SetFileEncodingCodepage(CP_ACP, 0);
                break;
            case OpenEditor::feUTF8     :
                pDoc->SetFileEncodingCodepage(CP_UTF8, 0);
                break;
            case OpenEditor::feUTF8BOM  :
                pDoc->SetFileEncodingCodepage(CP_UTF8, 3);
                break;
            case OpenEditor::feUTF16BOM :
                pDoc->SetFileEncodingCodepage(OpenEditor::CP_UTF16, 2);
                break;
            }
            pDoc->SetModifiedFlag(FALSE);
            pDoc->ClearUndo();

            pDoc->m_pOutput->Clear();
            if (m_objectNameAsTitle)
            {
                pDoc->m_newPathName  = m_strName + '.' + m_strExt;

                CString title(m_strOwner);
                if (!m_strOwner.IsEmpty()) title += '.';
                title += pDoc->m_newPathName;
                pDoc->SetTitle(title);

                pDoc->m_pOutput->Clear();
                if (!m_bPackAsWhole)
                    ErrorLoader::SubmitLoadTask(DocumentProxy(*pDoc), m_strOwner, m_strName, m_strType, m_nOffset[0]);
                else
                {
                    if (m_nOffset[0] != -1)
                    {
                        pDoc->m_pEditor->SetQueueBookmark(m_nOffset[0]);
                        ErrorLoader::SubmitLoadTask(DocumentProxy(*pDoc), m_strOwner, m_strName, "PACKAGE", m_nOffset[0]);
                    }

                    if (m_nOffset[1] != -1)
                    {
                        pDoc->m_pEditor->SetQueueBookmark(m_nOffset[1]);
                        ErrorLoader::SubmitLoadTask(DocumentProxy(*pDoc), m_strOwner, m_strName, "PACKAGE BODY", m_nOffset[1]);
                    }
                }

                pDoc->ActivateTab(pDoc->m_pOutput);
                pDoc->ActivateEditor();
            }
            else
            {
                std::vector<int>::const_iterator it(bookmarks.begin()), end(bookmarks.end());
                for (; it != end; it++)
                    pDoc->m_pEditor->SetQueueBookmark(*it);
            }

        }

        Clear();
    }
}
