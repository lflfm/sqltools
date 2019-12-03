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
#include "ObjectTree_Builder.h"
#include "SQLTools.h"
#include "COMMON\StrHelpers.h"
#include <OCI8/BCursor.h>
#include "SQLUtilities.h"
#include <OpenEditor/OELanguage.h> // for SQL keywords
#include "OpenEditor/OEDocument.h" // for OpenEditor::SettingsManager

using namespace std;
using namespace Common;
using namespace OpenEditor;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
TODO:
    2) cache Find result as well
    5) set current schema at the beginning and object schema with it
    6) do not mark "complete" invalid objects
    10) state image to show invalid objects
    13) add help
    15) try oci for describe packages

  100) Implement extended info intergface to display in property window, can be stored in different pool/cache.
    1) add "Pin Object", add a object to collection \root\pinned OR to combo box
*/

namespace ObjectTree {

    using SQLUtilities::GetSafeDatabaseObjectName;

    const int MAX_COMMENTS_LENGTH = 80;

    static
    string escape_lt_gt (const string& text)
    {
        string buff;

        string::const_iterator it = text.begin();
        for (; it != text.end(); ++it)
            switch (*it)
            {
            case '<': buff += "&lt;"; break;
            case '>': buff += "&gt;"; break;
            default:  buff += *it;    break;
            }

        return buff;
    }

    static
    string tuncate_to (const string& text, int max_length = MAX_COMMENTS_LENGTH)
    {
        if ((int)text.size() > max_length)
        {
            string buff = text.substr(0, max(0, max_length-3));
            buff += "...";
        }
        return text;
    }

    const string TreeNode::m_null;

Object* TreeNodePool::CreateObject (const ObjectDescriptor& desc)
{
    CSingleLock lk(&m_criticalSection, TRUE); 

    // check if object is already loaded
    std::set<TreeNode*>::iterator it = m_pool.begin();
    for (; it != m_pool.end(); ++it)
    {
        if(Object* object = dynamic_cast<Object*>(*it))
            if (object->IsTrueObject() && *object == desc)
                return object;
    }

    Object* object = 0;

    // create a new one
    if (desc.type == "SYNONYM")
        object = new Synonym(*this);
    else if (desc.type == "TABLE")
        object = new Table(*this);
    else if (desc.type == "VIEW")
        object = new View(*this);
    else if (desc.type == "SEQUENCE")
        object = new Sequence(*this);
    else if (desc.type == "TRIGGER")
        object = new Trigger(*this);
    else if (desc.type == "PROCEDURE")
        object = new Procedure(*this);
    else if (desc.type == "FUNCTION")
        object = new Function(*this);
    else if (desc.type == "PACKAGE")
        object = new Package(*this);
    else if (desc.type == "PACKAGE BODY")
        object = new PackageBody(*this);
    else if (desc.type == "TYPE")
        object = new Type(*this);
    else if (desc.type == "TYPE BODY")
        object = new TypeBody(*this);
    else
        object = new Unknown(*this);

    if (object)
    {
        object->SetSchema(desc.owner);
        object->SetName(desc.name);
        object->SetType(desc.type);
        m_pool.insert(object);
    }

    return object;
}

TreeNode* TreeNodePool::ValidatePtr (TreeNode* node) const                  
{ 
    std::set<TreeNode*>::const_iterator it = m_pool.find(node);
    
    if (it != m_pool.end())
        return *it;
        
    if (node)
        TRACE("ValidatePtr: invalid pointer = %p\n", (void*)node);

    return 0;
}


void TreeNodePool::DestroyAll ()
{
    CSingleLock lk(&m_criticalSection, TRUE); 

    std::set<TreeNode*> tmp;
    swap(m_pool, tmp);
    std::set<TreeNode*>::iterator it = tmp.begin();
    for (; it != tmp.end(); ++it)
        delete *it;
}

void Object::SetSchemaNameType (const string& schema, const string& name, const string& type)
{
    SetSchema(schema);
    SetName(name);
    SetType(type);
}

string Object::GetQualifiedName (TreeNode* parent) const
{
    string parentSchema;
    
    if (parent)
        if (Object* obj_parent = dynamic_cast<Object*>(parent))
            parentSchema = obj_parent->GetSchema();

    return (GetSchema() != parentSchema) ? GetSchema() + '.' + GetName() : GetName();
}

std::string Synonym::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent); 
}

std::string Column::GetLabel (TreeNode*) const        
{ 
    string type, val = GetName() + " \t";
    
    to_lower_str(GetType().c_str(), type);
    val += type;
    
    if (!GetNullable()) 
        val += " not null";

    if (m_Virtual)
        val += " as (" + GetDefault() + ")";
    else if (!GetDefault().empty()) 
        val += " default " + GetDefault();

    if (!GetComments().empty()
    && GetSQLToolsSettings().GetObjectViewerShowComments())
        val += " \t /*" + tuncate_to(GetComments()) + " */";

    return val; 
}

std::string ProcedureParameter::GetLabel (TreeNode*) const        
{ 
    string val = GetSafeDatabaseObjectName(GetName()) + " \t" + GetType();
    return val; 
}

std::string DependentObjects::GetLabel (TreeNode*) const        
{ 
    return "Dependent Objects"; 
}

std::string DependentOn::GetLabel (TreeNode*) const        
{ 
    return "Dependent On"; 
}

std::string GrantedPriveleges::GetLabel (TreeNode*) const        
{ 
    return "Granted Priveleges"; 
}

std::string GrantedPrivelege::GetLabel (TreeNode*) const        
{ 
    return GetName(); 
}

std::string References::GetLabel (TreeNode*) const        
{ 
    return "References"; 
}

std::string ReferencedBy::GetLabel (TreeNode*) const        
{ 
    return "Referenced By"; 
}

std::string Constraints::GetLabel (TreeNode*) const        
{ 
    return "Constraints"; 
}

std::string Constraint::GetLabel (TreeNode*) const        
{ 
    return GetName() + " \t" + GetInfo(); 
}

std::string Indexes::GetLabel (TreeNode*) const        
{ 
    return "Indexes"; 
}

std::string Index::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent) + " \t" + GetInfo(); 
}

std::string Triggers::GetLabel (TreeNode*) const        
{ 
    return "Triggers"; 
}

std::string Trigger::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent); 
}

std::string TableBase::GetLabel (TreeNode* parent) const 
{ 
    string val = GetQualifiedName(parent) + " \t" + GetInfo();

    if (GetSQLToolsSettings().GetObjectViewerShowComments()
    && !GetComments().empty())
        val += " \t/*" + tuncate_to(GetComments()) + " */";

    return val; 
}

std::string Sequence::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent) + " \t" + GetInfo(); 
}

std::string Procedure::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent) + '\t' + GetInfo(); 
}

std::string Package::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent); 
}

std::string PackageBody::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent); 
}

std::string PackageProcedure::GetLabel (TreeNode*) const        
{ 
    return GetSafeDatabaseObjectName(GetName()) + " \t" + GetArgumentList(); 
}

std::string Type::GetLabel (TreeNode* parent) const 
{ 
    return Package::GetLabel(parent) + " \t" + GetInfo(); 
}

std::string Partitioning::GetLabel (TreeNode*) const        
{ 
    return "Partitioning"; 
}

std::string PartitionedBy::GetLabel (TreeNode*) const        
{ 
    return "Partitioned By\t" + GetInfo(); 
}

std::string SubpartitionedBy::GetLabel (TreeNode*) const        
{ 
    return "Subpartitioned By\t" + GetInfo(); 
}

std::string SubpartitionTemplate::GetLabel (TreeNode*) const        
{ 
    return "Subpartition Template"; 
}

std::string Partitions::GetLabel (TreeNode*) const        
{ 
    return "Partitions"; 
}

std::string Partition::GetLabel (TreeNode*) const
{
   return GetName() + " \t" + GetInfo(); 
}

std::string Unknown::GetLabel (TreeNode* parent) const 
{ 
    return GetQualifiedName(parent); 
}

    static void open_table (string& buff)
    {
        buff += "<table border=1>";
    }
    static void add_row (string& buff, const char* name, const string& value)
    {
        buff += "<tr><td bgcolor=buttonface>";
        buff += name;
        buff += "</td><td bgcolor=window>" + value + "</td></tr>";
    }
    static void close_table (string& buff)
    {
        buff += "</table>";
    }
    static void reopen_table (string& buff)
    {
        string::size_type pos = buff.find("</table>");
        buff.resize(pos);
    }

std::string Object::GetTooltipText () const
{
    std::string text;

    //text = "Schema:\t<b>" + GetSchema() + "</b>";
    //text += "\r\nName:\t<b>" + GetName() + "</b>";
    //text += "\r\nType:\t<b>" + GetType() + "</b>";

    open_table(text);
    if (!GetSchema().empty())
        add_row(text, "Schema", GetSchema());
    add_row(text, "Name", GetName());
    add_row(text, "Type", GetType());
    close_table(text);

    return text;
}

std::string Synonym::GetTooltipText () const
{
    std::string text = Object::GetTooltipText();

    reopen_table(text);
    add_row(text, "Ref Owner", GetRefOwner ());
    add_row(text, "Ref Name", GetRefName  ());
    if (!GetRefDBLink().empty())
        add_row(text, "Ref DBLink", GetRefDBLink());
    close_table(text);

    return text;
}

std::string Folder::GetTooltipText () const
{
    std::string text;

    //text = GetLabel(0);
    //text += "\r\n<hr size=\"1\"/>";
    //text += "\r\n<font size=\"2\">Drag with SHIFT [+ CTRL] to get the content</font>";

    open_table(text);
    add_row(text, "Name", GetLabel(0));
    add_row(text, "<i>Hint</i>", "<i>Drag with SHIFT [+ CTRL] to get the content</i>");
    close_table(text);

    return text;
}

std::string Column::GetTooltipText () const
{
    std::string text;

    //text += "Name:\t<b>" + GetName() + "</b>";
    //text += "\r\nType:\t<b>" + GetType();
    //if (!GetNullable()) text += " NOT NULL";
    //text += "</b>";
    //if (!GetDefault().empty()) text += "\r\nDefault:\t<b>" + GetDefault() + "</b>";

    //if (!GetComments().empty())
    //    text += "\r\nComments:\t<b>" + tuncate_to(GetComments()) + "</b>";

    open_table(text);
    add_row(text, "Name", GetName());
    add_row(text, "Type", GetNullable() ? GetType() : GetType() + " NOT NULL");
    if (GetVirtual())
        add_row(text, "Virtual", tuncate_to(GetDefault()));
    else if (!GetDefault().empty()) 
        add_row(text, "Default", tuncate_to(escape_lt_gt(GetDefault())));
    if (!GetComments().empty()) 
        add_row(text, "Comments", tuncate_to(escape_lt_gt(GetComments())));
    close_table(text);

    return text;
}

std::string ProcedureParameter::GetTooltipText () const
{
    std::string text;

    //text += "Name:\t<b>" + GetName() + "</b>";
    //text += "\r\nType:\t<b>" + GetType() + "</b>";
    open_table(text);
    add_row(text, "Name", GetName());
    add_row(text, "Type", GetType());
    close_table(text);

    return text;
}

std::string GrantedPrivelege::GetTooltipText () const
{
    return string();
}

std::string Constraint::GetTooltipText () const
{
    std::string text;

    //text = "Schema:\t<b>" + GetSchema() + "</b>";
    //text += "\r\nName:\t<b>" + GetName() + "</b>";
    //
    //if (!GetType().empty())
    //{
    //    switch (GetType().at(0))
    //    {
    //    case 'P': text += "\r\nType:\t<b>PRIMARY KEY</b>"; break;
    //    case 'U': text += "\r\nType:\t<b>UNIQUE</b>"; break;
    //    case 'R': text += "\r\nType:\t<b>FOREIGN KEY</b>"; break;
    //    case 'C': text += "\r\nType:\t<b>CHECK</b>"; break;
    //    }
    //}
    string type;
    if (!GetType().empty())
    {
        switch (GetType().at(0))
        {
        case 'P': type = "PRIMARY KEY"; break;
        case 'U': type = "UNIQUE"; break;
        case 'R': type = "FOREIGN KEY"; break;
        case 'C': type = "CHECK"; break;
        }
    }
    open_table(text);
    add_row(text, "Schema", GetSchema());
    add_row(text, "Name", GetName());
    add_row(text, "Type", type);
    close_table(text);

    return text;
}

std::string TableBase::GetTooltipText () const
{
    std::string text = Object::GetTooltipText();

    reopen_table(text);
    if (!GetComments().empty())
    {
        //text += "\r\nComments:\t<b>" + tuncate_to(GetComments()) + "</b>";
        add_row(text, "Comments", tuncate_to(escape_lt_gt(GetComments())));
    }

    vector<Property>::const_iterator it = GetProperties().begin();
    for (; it != GetProperties().end(); ++it)
        add_row(text, it->m_name, it->m_value);

    add_row(text, "<i>Hint</i>", "<i>Drag with SHIFT [+ CTRL] to get the list of column</i>");
    close_table(text);

    //text += "\r\n<hr size=\"1\"/>";
    //text += "\r\n<font size=\"2\">Drag with SHIFT [+ CTRL] to get the list of columns</font>";

    return text;
}

std::string Sequence::GetTooltipText () const
{
    std::string text = Object::GetTooltipText();

    reopen_table(text);
    vector<Property>::const_iterator it = GetProperties().begin();
    for (; it != GetProperties().end(); ++it)
    {
        //text += "\r\n";
        //text += it->m_name;
        //text += ":\t<b>" + it->m_value + "</b>";
        add_row(text, it->m_name, it->m_value);
    }
    close_table(text);

    return text;
}

std::string Procedure::GetTooltipText () const
{
    std::string text = Object::GetTooltipText();

    if (unsigned int count = GetChildrenCount())
    {
        //text += "\r\n<hr size=\"1\"/>";

        reopen_table(text);

        text += "</table><tr><td bgcolor=buttonface>Parameters:</td></tr><table border=1>";

        for (unsigned int i = 0; i < count; ++i)
            if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                //text += "\r\n" + param->GetName() + '\t' + param->GetInOut() + '\t' + param->GetType();
                add_row(text, param->GetName().c_str(), param->GetType());
        close_table(text);
    }

    return text;
}

std::string Function::GetTooltipText () const
{
    std::string text = Procedure::GetTooltipText();

    //text += "\r\n<hr size=\"1\"/>";
    //text += "\r\nReturn:\t<b>" + GetReturn() + "</b>";

    reopen_table(text);
    add_row(text, "Return", GetReturn());
    close_table(text);

    return text;
}

std::string PackageProcedure::GetTooltipText () const
{
    std::string text;

    //text += "Name:\t<b>" + GetName() + "</b>";
    //text += "\r\nType:\t<b>PROCEDURE</b>";
    //
    //if (unsigned int count = GetChildrenCount())
    //{
    //    text += "\r\n<hr size=\"1\"/>";

    //    for (unsigned int i = 0; i < count; ++i)
    //        if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
    //            text += "\r\n" + param->GetName() + '\t' + param->GetInOut() + '\t' + param->GetType();
    //}

    open_table(text);
    add_row(text, "Name", GetName());
    add_row(text, "Type", GetType());

    text += "</table><tr><td bgcolor=buttonface>Parameters:</td></tr><table border=1>";

    if (unsigned int count = GetChildrenCount())
    {
        for (unsigned int i = 0; i < count; ++i)
            if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                add_row(text, param->GetName().c_str(), param->GetType());
    }

    close_table(text);


    return text;
}

std::string PackageFunction::GetTooltipText () const
{
    std::string text = PackageProcedure::GetTooltipText();

    //text += "Name:\t<b>" + GetName() + "</b>";
    //text += "\r\nType:\t<b>FUNCTION</b>";

    //if (unsigned int count = GetChildrenCount())
    //{
    //    text += "\r\n<hr size=\"1\"/>";

    //    for (unsigned int i = 0; i < count; ++i)
    //        if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
    //            text += "\r\n" + param->GetName() + '\t' + param->GetInOut() + '\t' + param->GetType();
    //}

    //text += "\r\n<hr size=\"1\"/>";
    //text += "\r\nReturn:\t<b>" + GetReturn() + "</b>";

    reopen_table(text);
    add_row(text, "Return", GetReturn());
    close_table(text);

    return text;
}


    // TODO: make least_column & least_position safer, check pointers
    template <class C> int least_column (const TreeNode* c1, const TreeNode* c2)
    {
        return 
            dynamic_cast<const C*>(c1)->GetName()
            < dynamic_cast<const C*>(c2)->GetName();
    }

    template <class C> int least_position (const TreeNode* c1, const TreeNode* c2)
    {
        return 
            dynamic_cast<const C*>(c1)->GetPosition()
            < dynamic_cast<const C*>(c2)->GetPosition();
    }

//void TableBase::SortChildren (Order order)
//{
//    if (m_order != order)
//    {
//        int ncount = 0;
//        vector<TreeNode*>::iterator it = m_children.begin();
//        for (; it != m_children.end(); ++it)
//            if (dynamic_cast<const Column*>(*it))
//                ncount++;
//            else
//                break;
//
//        if (ncount)
//        {
//            switch (order)
//            {
//            case Alphabetical:
//                sort(m_children.begin(), m_children.begin() + ncount, least_column);
//                break;
//            case Natural:
//                sort(m_children.begin(), m_children.begin() + ncount, least_position);
//                break;
//            }
//        }
//
//        m_order = order;
//    }
//}
//
template <class T, class C> void sort_childern (T* This, Order order)
{
    if (This->m_order != order)
    {
        int ncount = 0;
        vector<TreeNode*>::iterator it = This->m_children.begin();
        for (; it != This->m_children.end(); ++it)
            if (dynamic_cast<const C*>(*it))
                ncount++;
            else
                break;

        if (ncount)
        {
            switch (order)
            {
            case Alphabetical:
                sort(This->m_children.begin(), This->m_children.begin() + ncount, least_column<C>);
                break;
            case Natural:
                sort(This->m_children.begin(), This->m_children.begin() + ncount, least_position<C>);
                break;
            }
        }

        This->m_order = order;
    }
}

void TableBase::SortChildren (Order order)
{
    sort_childern<TableBase, Column>(this, order);
}

void Package::SortChildren (Order order)
{
    sort_childern<Package, PackageProcedure>(this, order);
}


///////////////////////////////////////////////////////////////////////////////

    inline 
    string get_safe_name (const string& name)
    {
        return SQLUtilities::GetSafeDatabaseObjectName(name);
    }

    static
    std::string get_full_name (const string& schema, const string& name)
    {
        return SQLUtilities::GetSafeDatabaseObjectFullName(schema, name);
    }

TextCtx::TextCtx () 
    : m_form(etfSHORT), m_indent_size(2), 
    m_with_package_name(true), m_shift(false) 
{
    const OpenEditor::SettingsManager& settingsManager = COEDocument::GetSettingsManager();
    ClassSettingsPtr plSql = settingsManager.FindByName("PL/SQL");
    m_indent_size = (plSql.get()) ? plSql->GetIndentSpacing() : 2;
}

TextFormatter::TextFormatter (TextForm form)
    : m_max_line_length(0)
{
    if (form != etfCOLUMN)
        m_max_line_length = GetSQLToolsSettings().GetObjectViewerMaxLineLength();
}

void TextFormatter::Put (const string& text)
{
    if (m_buffer.length() + 2 + text.length() > m_max_line_length)
        flush();

    if (!m_buffer.empty())
        m_buffer += ", ";
        
    m_buffer += text;
}

const string& TextFormatter::GetResult ()
{
    flush();
    return m_text;
}

void TextFormatter::flush ()
{
    if (!m_buffer.empty())
    {
        if (!m_text.empty())
            m_text += ",\n";
            
        m_text += m_buffer;
        m_buffer.clear();
    }
}

///////////////////////////////////////////////////////////////////////////////

void Synonym::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName()); 
}

/*
it would be nice to have an option to copy with table alias
*/
void Column::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text.clear();

    if (!ctx.m_table_alias.empty())
        ctx.m_text = ctx.m_table_alias + '.';
    
    ctx.m_text += get_safe_name(GetName());

    //if (ctx.m_shift
    //&& (ctx.m_form == etfSHORT || ctx.m_form == etfLINE || && ctx.m_form == etfCOLUMN))
    if (ctx.m_form == etfLINE)
    {
        ctx.m_text += ' ';
        ctx.m_text += GetType();
    }
}

void ProcedureParameter::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text.clear();

    if (!ctx.m_table_alias.empty())
        ctx.m_text = ctx.m_table_alias + '.';
    
    ctx.m_text += get_safe_name(GetName()); 
}

void Folder::GetTextToCopy (TextCtx& ctx) const 
{ 
    switch (ctx.m_form)
    {
    case etfDEFAULT:
    case etfSHORT:
        ctx.m_text = GetLabel(0);
        break;
    case etfLINE:
    case etfCOLUMN:
        {
            TextFormatter out(ctx.m_form);

            std::vector<TreeNode*>::const_iterator it = m_children.begin();
            for (; it != m_children.end(); ++it)
            {
                TextCtx ctx2(ctx);
                ctx2.m_shift = false;
                ctx2.m_form = etfSHORT;
            
                (**it).GetTextToCopy(ctx2);
                out.Put(ctx2.m_text);
            }

            ctx.m_text = out.GetResult();
        }
    }
}

void GrantedPrivelege::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = GetLabel(0); 
}

void Constraint::GetTextToCopy (TextCtx& ctx) const 
{ 
    switch (ctx.m_form)
    {
    case etfDEFAULT:
    case etfSHORT:
        ctx.m_text = get_safe_name(GetName()); 
        break;
    case etfLINE:
    case etfCOLUMN:
        {
            TextFormatter out(ctx.m_form);

            std::vector<std::string>::const_iterator it = m_Columns.begin();
            for (; it != m_Columns.end(); ++it)
                out.Put(get_safe_name(*it)); 

            ctx.m_text = out.GetResult();
        }
    }
}

void Index::GetTextToCopy (TextCtx& ctx) const 
{
    switch (ctx.m_form)
    {
    case etfDEFAULT:
    case etfSHORT:
        ctx.m_text = get_safe_name(GetName()); 
        break;
    case etfLINE:
    case etfCOLUMN:
        {
            TextFormatter out(ctx.m_form);

            std::vector<std::string>::const_iterator it = m_Columns.begin();
            for (; it != m_Columns.end(); ++it)
                out.Put(get_safe_name(*it)); 

            ctx.m_text = out.GetResult();
        }
    }
}

void Trigger::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName());
}

void TableBase::GetTextToCopy (TextCtx& ctx) const 
{ 
    switch (ctx.m_form)
    {
    case etfDEFAULT:
    case etfSHORT:
        ctx.m_text = get_full_name(GetSchema(), GetName());
        break;
    case etfLINE:
    case etfCOLUMN:
        {
            TextFormatter out(ctx.m_form);

            std::vector<TreeNode*>::const_iterator it = m_children.begin();
            for (; it != m_children.end() && typeid(**it) == typeid(Column); ++it)
            {
                TextCtx ctx2(ctx);
                ctx2.m_shift = false;
                ctx2.m_form = etfSHORT;
                (**it).GetTextToCopy(ctx2);
                out.Put(ctx2.m_text);
            }

            ctx.m_text = out.GetResult();
        }
    }
}

void Sequence::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName());
}

void Procedure::GetTextToCopy (TextCtx& ctx) const 
{ 
    string ret_val = get_full_name(GetSchema(), GetName());

    switch ((ctx.m_form == etfDEFAULT) ? etfLINE : ctx.m_form)
    {
    case etfLINE:
        {
            ret_val += "(";
            unsigned int count = GetChildrenCount();
            for (unsigned int i = 0; i < count; ++i)
                if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                {
                    if (i) ret_val += ", ";
                    ret_val += get_safe_name(param->GetName()) + " => ";
                }
            ret_val += ");";
        }
        break;
    case etfCOLUMN:
        {
            unsigned int max_arg_length = 0;
            unsigned int count = GetChildrenCount();

            for (unsigned int i = 0; i < count; ++i)
                if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                    max_arg_length = max(max_arg_length, param->GetName().size());

            if (max_arg_length)
            {
                string indent(ctx.m_indent_size, ' ');
        
                ret_val += "\n(\n";

                for (unsigned int i = 0; i < count; ++i)
                    if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                    {
                        if (i) ret_val += ",\n";
                        ret_val += indent + get_safe_name(param->GetName()) 
                            + string(max_arg_length - param->GetName().length(), ' ') + " => ";
                    }
                ret_val += "\n);\n";
            }
            else
                ret_val += "();\n";
        }
        break;
    }

    ctx.m_text = ret_val; 
}

void Package::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName());
}

void PackageBody::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName());
}

void PackageProcedure::GetTextToCopy (TextCtx& ctx) const 
{ 
    string ret_val;

    if (ctx.m_with_package_name)
    {
        if (GetSQLToolsSettings().GetObjectViewerCopyProcedureWithPackageName())
            if (const Package* package = dynamic_cast<const Package*>(m_parent))
                ret_val = get_full_name(package->GetSchema(), package->GetName());

        ret_val += '.' + get_safe_name(GetName()); 
    }
    else
        ret_val += get_safe_name(GetName()); 

    switch ((ctx.m_form == etfDEFAULT) ? etfLINE : ctx.m_form)
    {
    case etfLINE:
        {
            ret_val += "(";
            unsigned int count = GetChildrenCount();
            for (unsigned int i = 0; i < count; ++i)
                if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                {
                    if (i) ret_val += ", ";
                    ret_val += get_safe_name(param->GetName()) + " => ";
                }
            ret_val += ");";
        }
        break;
    case etfCOLUMN:
        {
            unsigned int max_arg_length = 0;
            unsigned int count = GetChildrenCount();

            for (unsigned int i = 0; i < count; ++i)
                if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                    max_arg_length = max(max_arg_length, param->GetName().size());

            if (max_arg_length)
            {
                string indent(ctx.m_indent_size, ' ');
        
                ret_val += "\n(\n";

                for (unsigned int i = 0; i < count; ++i)
                    if (const ProcedureParameter* param = dynamic_cast<const ProcedureParameter*>(GetChild(i)))
                    {
                        if (i) ret_val += ",\n";
                        ret_val += indent + get_safe_name(param->GetName()) 
                            + string(max_arg_length - param->GetName().length(), ' ') + " => ";
                    }
                ret_val += "\n);\n";
            }
            else
                ret_val += "();\n";
        }
        break;
    }

    ctx.m_text = ret_val; 
}

void Partition::GetTextToCopy (TextCtx& ctx) const 
{ 
    switch (ctx.m_form)
    {
    case etfDEFAULT:
    case etfSHORT:
        ctx.m_text = GetName();
        break;
    case etfLINE:
    case etfCOLUMN:
        ctx.m_text = GetInfo();
    }
}

void Unknown::GetTextToCopy (TextCtx& ctx) const 
{ 
    ctx.m_text = get_full_name(GetSchema(), GetName());
}

};//namespace ObjectTree {