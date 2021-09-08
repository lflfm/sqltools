/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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
#include "SQLTools.h"
#include "AutocompleteTemplate.h"
#include "DbBrowser\ObjectTree_Builder.h"
#include "DbBrowser\FindObjectsTask.h"
#include <ActivePrimeExecutionNote.h>

using namespace OpenEditor;
using namespace ObjectTree;
using namespace ServerBackgroundThread;

AutocompleteTemplate::AutocompleteTemplate (TreeNodePoolPtr poolPtr)
    : m_poolPtr(poolPtr)
{ 
    SetKeyColumnHeader("Name");
    SetNameColumnHeader("Type");
}

    struct BackgroundTask_FindAndLoadProc : ObjectTree::FindObjectsTask
    {
        CEvent& m_event;
        TreeNodePoolPtr m_poolPtr;
        TreeNode* m_object;

        BackgroundTask_FindAndLoadProc (CEvent& event, TreeNodePoolPtr poolPtr, const std::string& input)
            : FindObjectsTask(input, 0),
            m_event(event),
            m_poolPtr(poolPtr),
            m_object(0)
        {
            m_silent = true;
        }

        void DoInBackground (OciConnect& connect)
        {
            FindObjectsTask::DoInBackground(connect);

            if (m_result.size() > 0)
            {
                TreeNode* object = m_poolPtr->CreateObject(m_result.at(0));

                if (object && !object->IsComplete())
                {
                    ActivePrimeExecutionOnOff onOff;
                    object->Load(connect);
                }

                m_object = object;
            }

            m_event.SetEvent();
        }

        void ReturnInForeground ()
        {
        }
    };

    static bool pump_messages_and_wait_for_event (CEvent& event, int timeout)
    {
        clock64_t startTime = clock64();
        bool ready = false;
        MSG Msg;
        while (GetMessage(&Msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);

            if (WaitForSingleObject(event, 0) == WAIT_OBJECT_0)
            {
                ready = true;
                break;
            }

            if ((clock64() - startTime) > timeout)
                break;            
        }

        return ready;
    }

bool AutocompleteTemplate::AfterExpand (const Entry& entry, string& text, Position& pos)
{
    if (entry.name == "PROCEDURE" || entry.name == "FUNCTION")
    {
        CWaitCursor wait;

        m_bkgrSyncEvent.ResetEvent();
        BackgroundTask_FindAndLoadProc* task = new BackgroundTask_FindAndLoadProc(m_bkgrSyncEvent, m_poolPtr, text);
        TaskPtr taskPtr(task);

        BkgdRequestQueue::Get().Push(taskPtr);

        if (pump_messages_and_wait_for_event(m_bkgrSyncEvent, 5000))
        {
            if (task->m_object)
            {
                TextCtx ctx;
                //ctx.m_form = etfCOLUMN;
                ctx.m_form = etfLINE;
                task->m_object->GetTextToCopy(ctx);

                text = ctx.m_text;
                string::size_type inx = text.find_first_of("=>");

                if (inx != string::npos)
                {
                    // taking in consideration multi-line formats
                    bool multiline = false;
                    for (string::size_type i = 0, col = 0, line = 0; i < inx && text.at(i); i++)
                    {
                        if (text.at(i) == '\n') {
                            multiline = true;
                            col = 0;
                            line++;
                        } else
                            col++;
                    }
                    pos.column = col + 3; 
                    pos.line = line; 
                    //pos.column = inx + 3;
                }
                else
                {
                    pos.column = text.length();
                    if (ctx.m_form == etfCOLUMN)
                        --pos.column; // because the text ends with \n 
                }
            }
        }
        else
            AfxMessageBox(L"Timeout error while loading object(s).\nPlease try again.", MB_ICONERROR | MB_OK); 
    }

    return true;
}

    struct BackgroundTask_FindAndLoad : ObjectTree::FindObjectsTask
    {
        CEvent& m_event;
        TreeNodePoolPtr m_poolPtr;
        vector<TreeNode*> m_children;

        BackgroundTask_FindAndLoad (CEvent& event, TreeNodePoolPtr poolPtr, const std::string& input)
            : FindObjectsTask(input, 0),
            m_event(event),
            m_poolPtr(poolPtr)
        {
            m_silent = true;
        }

        void DoInBackground (OciConnect& connect)
        {
            FindObjectsTask::DoInBackground(connect);

            if (m_result.size() > 0)
            {
                ActivePrimeExecutionOnOff onOff;

                if (Object* object = m_poolPtr->CreateObject(m_result.at(0)))
                {
                    if (!object->IsComplete())
                        object->Load(connect);

                    string type = object->GetType();

                    if (Synonym* syn = dynamic_cast<Synonym*>(object))
                    {
                        ObjectDescriptor desc;
                        desc.owner = syn->GetRefOwner();
                        desc.name  = syn->GetRefName();
                        desc.type  = syn->GetRefType();

                        if (syn->GetRefDBLink().empty())
                        {
                            if (Object* refObject = m_poolPtr->CreateObject(desc))
                            {
                                if (!refObject->IsComplete())
                                    refObject->Load(connect);
                                object = refObject;
                                type = object->GetType();
                            }
                        }
                    }

                    if (type == "PACKAGE" || type == "TYPE" || type == "TABLE" || type == "VIEW")
                    {
                        vector<TreeNode*> children;

                        unsigned int ncount = object->GetChildrenCount();
                        for (unsigned int i = 0; i < ncount; ++i)
                        {
                            if (TreeNode* child = object->GetChild(i))
                                if (dynamic_cast<Column*>(child) || dynamic_cast<PackageProcedure*>(child))
                                    children.push_back(child);
                        }

                        swap(m_children, children);
                    }
                }
            }

            m_event.SetEvent();
        }

        void ReturnInForeground ()
        {
        }
    };

AutocompleteSubobjectTemplate::AutocompleteSubobjectTemplate (TreeNodePoolPtr poolPtr, const string& object)
    : m_poolPtr(poolPtr)
{
	CWaitCursor wait;

    SetKeyColumnHeader("Name");
    SetNameColumnHeader("Type");
    SetImageListRes(IDB_SQL_GENERAL_LIST);

    m_bkgrSyncEvent.ResetEvent();
    BackgroundTask_FindAndLoad* task = new BackgroundTask_FindAndLoad(m_bkgrSyncEvent, m_poolPtr, object);
    TaskPtr taskPtr(task);

    BkgdRequestQueue::Get().Push(taskPtr);

    if (pump_messages_and_wait_for_event(m_bkgrSyncEvent, 5000))
    {
        vector<TreeNode*>::const_iterator it = task->m_children.begin();
        for (; it != task->m_children.end(); ++it)
        {
            Entry entry;
            TextCtx ctx;
            ctx.m_form = typeid(**it) == typeid(Column) ? etfSHORT : etfLINE; // 2017-12-14 bug fix, table columns are expanded with datatype
            ctx.m_with_package_name = false;
            (*it)->GetTextToCopy(ctx);
            entry.keyword = 
            entry.text    = ctx.m_text;
            entry.name    = (*it)->GetType();
            entry.image   = (*it)->GetImage();

            // TODO: take in consideration multi-line formats
            string::size_type inx = entry.text.find_first_of("=>");
            if (inx != string::npos)
                entry.curPos = inx + 3;
            else
                entry.curPos = entry.text.length();

            entry.minLength = ctx.m_text.length() + 1;

            AppendEntry(entry);
        }
    }
    else
        AfxMessageBox(L"Timeout error while loading object(s).\nPlease try again.", MB_ICONERROR | MB_OK); 
}
