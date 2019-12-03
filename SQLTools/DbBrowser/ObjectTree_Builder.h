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

#pragma once

#include <set>
#include <vector>
#include <boost/noncopyable.hpp>
//#include <arg_shared.h>

namespace ObjectTree {

    #define OT_ATTRIBUTE(T,N) \
        private: \
            T m_##N; \
        public: \
            const T& Get##N () const { return m_##N; }; \
            void  Set##N (const T& val) { m_##N = val; } 

    #define OT_BASE_CLONEABLE(Type) \
        virtual Type* Clone () const = 0;
     
    #define OT_CLONEABLE(Type) \
        virtual Type* Clone () const { return new Type(*this); } 

    #define OT_CLONEABLE_COPYABLE(Type) \
        virtual Type* Clone () const { return new Type(*this); } \
        virtual void Copy (const TreeNode& other) { *this = dynamic_cast<const Type&>(other); }


    enum IDB_SQL_GENERAL_LIST_INDEX
    {
        IDII_FOLDER         =  0,
        IDII_OPEN_FOLDER    =  1,
        IDII_TABLE          =  2,
        IDII_CLUSTER        =  3,
        IDII_DBLINK         =  4,
        IDII_INDEX          =  5,
        IDII_PACKAGE        =  6,
        IDII_PACKAGE_BODY   = 24,
        IDII_SEQUENCE       =  7,
        IDII_SYNONYM        =  8,
        IDII_TRIGGER        =  9,
        IDII_VIEW           = 10,
        IDII_SCHEMA         = 11,
        IDII_FUNCTION       = 12,
        IDII_PROCEDURE      = 13,
        IDII_COLUMN         = 14,
        IDII_CHK_CONSTRAINT = 15,
        IDII_PK_CONSTRAINT  = 16,
        IDII_UK_CONSTRAINT  = 17,
        IDII_FK_CONSTRAINT  = 18,
        IDII_GRANT          = 23,
        IDII_TYPE           = 25,
        IDII_TYPE_BODY      = 26,
        IDII_UNKNOWN        = 28,
        IDII_PARAM_IN       = 29,
        IDII_PARAM_OUT      = 30,
        IDII_PARAM_INOUT    = 31,
        IDII_PARTITION      = 32,
    };

    enum Order
    {
        NA,
        Alphabetical,
        Natural
    };

    class Object;
    struct TreeNode;
    class TreeNodePool;

    struct ObjectDescriptor { std::string owner, name, type; };
    typedef std::vector<ObjectDescriptor> ObjectDescriptorArray;

    enum TextForm { etfDEFAULT, etfSHORT, etfLINE, etfCOLUMN };

    struct TextCtx
    {
        string m_text, m_table_alias;
        TextForm m_form;
        int m_indent_size;
        bool m_with_package_name;
        bool m_shift;

        TextCtx ();
    };

    class TextFormatter
    {
    public:
        TextFormatter (string::size_type max_line_length) 
            : m_max_line_length(max_line_length) {}

        TextFormatter (TextForm);

        void Put (const string& text);
        const string& GetResult ();

    private:
        string m_text, m_buffer;
        string::size_type m_max_line_length;
        void flush ();
    };


    // interface between a tree node and object/attributes
    struct TreeNode : private boost::noncopyable
    {
        TreeNode (TreeNodePool& treeNodePool, TreeNode* parent = 0) 
            : m_treeNodePool(treeNodePool), m_parent(parent), m_complete(false) {}
         virtual ~TreeNode () {};

        TreeNode* GetParent () { return m_parent; }
        const TreeNode* GetParent () const { return m_parent; }

        virtual std::string   GetLabel (TreeNode*) const = 0;
        virtual std::string   GetTooltipText () const    = 0;
        virtual int           GetImage () const          = 0;
        virtual void          GetTextToCopy (TextCtx&) const = 0;
        virtual bool          IsTrueObject () const      = 0;

        virtual unsigned int  GetChildrenCount () const           { return 0; }
        virtual TreeNode*     GetChild (unsigned int)             { return 0; }
        virtual const TreeNode* GetChild (unsigned int) const     { return 0; }

        virtual bool          IsComplete () const                 { return m_complete; }
        virtual void          SetComplete (bool complete = true)  { m_complete = complete; }
        virtual void          Load (OciConnect&)                  {}
        virtual void          Clear ()                            {}
        virtual bool          IsRefreshable () const              { return true; }
        virtual bool          CanSortChildren () const            { return false; }
        virtual Order         GetSortOrder () const               { return NA; }
        virtual void          SortChildren (Order)                {}
        //getContextMenu

        virtual const string& GetName () const                    { return m_null; }
        virtual const string& GetType () const                    { return m_null; }

        virtual TreeNode* Clone () const                          = 0;
        virtual void Copy (const TreeNode& other)                 = 0;
    protected:
        TreeNode (const TreeNode& other)
            : m_complete(other.m_complete),
            m_parent(other.m_parent),
            m_treeNodePool(other.m_treeNodePool) {}

        TreeNode& operator = (const TreeNode& other)
        {
            if (this != &other)
            {
                m_complete = other.m_complete;
                m_parent   = other.m_parent;
            }
            return *this;
        }

        bool m_complete;
        TreeNode* m_parent;
        TreeNodePool& m_treeNodePool;
        static const string m_null;
        friend Object;
    };


    // thread safe class
    class TreeNodePool : private boost::noncopyable
    {
    public:
        ~TreeNodePool () { DestroyAll(); }

        Object*   CreateObject (const ObjectDescriptor&);
        void      DestroyAll ();
        TreeNode* ValidatePtr (TreeNode* node) const;

        template <class T, class P> T* Create(P* p)             { T* ptr = new T(*this, p); m_pool.insert(ptr); return ptr; }

    protected:
        std::set<TreeNode*> m_pool;
        mutable CCriticalSection m_criticalSection;
    };

    class Object : public TreeNode
    {
    public:
        Object (TreeNodePool& treeNodePool, Object* parent = 0) 
            : TreeNode(treeNodePool, parent), m_ChildFirstTabPos(0) {}

        OT_ATTRIBUTE(std::string, Schema)
        OT_ATTRIBUTE(std::string, Name)
        OT_ATTRIBUTE(std::string, Type)
        OT_ATTRIBUTE(std::string, Info)
        OT_ATTRIBUTE(int, ChildFirstTabPos)

        void SetSchemaNameType(const string&, const string&, const string&);

        int operator == (const ObjectDescriptor&) const;

        virtual std::string GetTooltipText () const;
        virtual unsigned int GetChildrenCount () const { return m_children.size(); }
        virtual TreeNode* GetChild (unsigned int inx)  { return m_children.at(inx); }
        virtual const TreeNode* GetChild (unsigned int inx) const { return m_children.at(inx); }
        virtual string GetQualifiedName (TreeNode* parent) const;

        virtual void Clear ()
        {     
            m_children.clear();
            SetComplete(false); 
        }

    protected:
        std::vector<TreeNode*> m_children;

        Object& operator = (const Object& other)
        {
            if (this != &other)
            {
                *(TreeNode*)this = other;

                m_Schema           = other.m_Schema          ;
                m_Name             = other.m_Name            ;
                m_Type             = other.m_Type            ;
                m_Info             = other.m_Info            ;
                m_ChildFirstTabPos = other.m_ChildFirstTabPos;
                m_children         = other.m_children        ;

                std::vector<TreeNode*>::iterator it = m_children.begin();
                for (; it != m_children.end(); ++it)
                    (*it)->m_parent = this;
            }
            return *this;
        }
    };

    class Synonym : public Object
    {
    public:
        Synonym (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        OT_ATTRIBUTE(std::string, RefOwner)
        OT_ATTRIBUTE(std::string, RefName)
        OT_ATTRIBUTE(std::string, RefType)
        OT_ATTRIBUTE(std::string, RefDBLink)

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_SYNONYM; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Synonym)
    };

    struct Property
    {
        const char* m_name;
        std::string m_value;

        Property () : m_name(0) {} 
        Property (const char* name, const std::string& value) : m_name(name), m_value(value) {} 
    };

    class Column : public TreeNode
    {
    public:
        Column (TreeNodePool& treeNodePool, Object* parent) 
            : TreeNode(treeNodePool, parent), m_Position(0), m_Nullable(true), m_Virtual(false) {}

        OT_ATTRIBUTE(std::string, Name)
        OT_ATTRIBUTE(std::string, Type)
        OT_ATTRIBUTE(std::string, Comments)
        OT_ATTRIBUTE(std::string, Default)
        OT_ATTRIBUTE(int, Position)
        OT_ATTRIBUTE(bool, Nullable)
        OT_ATTRIBUTE(bool, Virtual)

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_COLUMN; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }

        OT_CLONEABLE_COPYABLE(Column)
    };

    class ProcedureParameter : public TreeNode
    {
    public:
        ProcedureParameter (TreeNodePool& treeNodePool, Object* parent) 
            : TreeNode(treeNodePool, parent) {}

        OT_ATTRIBUTE(std::string, Name)
        OT_ATTRIBUTE(std::string, Type)

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        std::string   GetTooltipText () const;
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }
        virtual std::string   GetInOut () const = 0;
    };

    class ProcedureParameterIn : public ProcedureParameter
    {
    public:
        ProcedureParameterIn (TreeNodePool& treeNodePool, Object* parent) 
            : ProcedureParameter(treeNodePool, parent) {}

        int           GetImage () const                 { return IDII_PARAM_IN; }
        std::string   GetInOut () const                 { return "IN"; }

        OT_CLONEABLE_COPYABLE(ProcedureParameterIn)
    };

    class ProcedureParameterOut : public ProcedureParameter
    {
    public:
        ProcedureParameterOut (TreeNodePool& treeNodePool, Object* parent) 
            : ProcedureParameter(treeNodePool, parent) {}

        int           GetImage () const                 { return IDII_PARAM_OUT; }
        std::string   GetInOut () const                 { return "OUT"; }

        OT_CLONEABLE_COPYABLE(ProcedureParameterOut)
    };

    class ProcedureParameterInOut : public ProcedureParameter
    {
    public:
        ProcedureParameterInOut (TreeNodePool& treeNodePool, Object* parent) 
            : ProcedureParameter(treeNodePool, parent) {}

        int           GetImage () const                 { return IDII_PARAM_INOUT; }
        std::string   GetInOut () const                 { return "IN/OUT"; }

        OT_CLONEABLE_COPYABLE(ProcedureParameterInOut)
    };

    class Folder : public Object
    {
    public:
        Folder (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_FOLDER; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
    };

    class DependentObjects : public Folder
    {
    public:
        DependentObjects (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(DependentObjects)
    };

    class DependentOn : public Folder
    {
    public:
        DependentOn (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(DependentOn)
    };

    class GrantedPriveleges : public Folder
    {
    public:
        GrantedPriveleges (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(GrantedPriveleges)
    };

    class GrantedPrivelege : public Object
    {
    public:
        GrantedPrivelege (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_GRANT; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }

        OT_CLONEABLE_COPYABLE(GrantedPrivelege)
    };

    class References : public Folder
    {
    public:
        References (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(References)
    };

    class ReferencedBy : public Folder
    {
    public:
        ReferencedBy (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(ReferencedBy)
    };

    class Constraints : public Folder
    {
    public:
        Constraints (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Constraints)
    };

    class Constraint : public Object
    {
    public:
        Constraint (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        std::string   GetTooltipText () const;
        int           GetImage () const;
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }

        OT_ATTRIBUTE(std::vector<std::string>, Columns);
        friend Constraints;

        OT_CLONEABLE_COPYABLE(Constraint)
    };

	inline
    int Constraint::GetImage () const      
	{
		const string& type = GetType();
		if (!type.empty())
		{
			switch (type.at(0))
			{
			case 'C': return IDII_CHK_CONSTRAINT;
			case 'P': return IDII_PK_CONSTRAINT;
			case 'U': return IDII_UK_CONSTRAINT;
			case 'R': return IDII_FK_CONSTRAINT;
			}
		}
		return IDII_UNKNOWN;
	}

    class FkConstraint : public Constraint
    {
    public:
        FkConstraint (TreeNodePool& treeNodePool, Object* parent) 
            : Constraint(treeNodePool, parent) {}

        OT_ATTRIBUTE(std::string, RefSchema)
        OT_ATTRIBUTE(std::string, RefConstraintName)

        // TreeNode Objects methods
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(FkConstraint)
    };

    class Indexes : public Folder
    {
    public:
        Indexes (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Indexes)
    };

    class Index : public Object
    {
    public:
        Index (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent) {}

        OT_ATTRIBUTE(std::string, Subtype)

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_INDEX; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

// TODO: remove because coluns have been added as children nodes
        OT_ATTRIBUTE(std::vector<std::string>, Columns);
        friend Indexes;

        OT_CLONEABLE_COPYABLE(Index)
    };

    class Triggers : public Folder
    {
    public:
        Triggers (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Triggers)
    };

    class Trigger : public Object
    {
    public:
        Trigger (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_TRIGGER; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Trigger)
    };

        template <class T, class C> void sort_childern (T*, Order);

    class TableBase : public Object
    {
    protected:
        TableBase (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent), m_order(NA) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        std::string   GetTooltipText () const;
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        bool          CanSortChildren () const          { return true; }
        Order         GetSortOrder () const             { return m_order; }
        void          SortChildren (Order);

        Order m_order;
        OT_ATTRIBUTE(std::string, Comments)
        OT_ATTRIBUTE(std::vector<Property>, Properties)
        friend void sort_childern<TableBase, Column> (TableBase*, Order);
    };

    class Table : public TableBase
    {
    public:
        Table (TreeNodePool& treeNodePool, Object* parent = 0) 
            : TableBase(treeNodePool, parent), m_Partitioned(false), m_Subpartitioned(false) {}

        OT_ATTRIBUTE(bool, Partitioned)
        OT_ATTRIBUTE(bool, Subpartitioned)

        // TreeNode Objects methods
        int           GetImage () const                 { return IDII_TABLE; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Table)
    };

    class View : public TableBase
    {
    public:
        View (TreeNodePool& treeNodePool, Object* parent = 0) 
            : TableBase(treeNodePool, parent) {}

        // TreeNode Objects methods
        int           GetImage () const                 { return IDII_VIEW; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(View)
    };

    class Sequence : public Object
    {
    public:
        Sequence (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_SEQUENCE; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        OT_ATTRIBUTE(std::vector<Property>, Properties)

        OT_CLONEABLE_COPYABLE(Sequence)
    };

    struct Argument
    {
        string m_name, m_type, m_in_out;
        bool m_defaulted;

        Argument () : m_defaulted(false) {}
        Argument (const string& name, const string& type, const string& in_out, bool defaulted) 
            : m_name(name), m_type(type), m_in_out(in_out), m_defaulted(defaulted) {}
    };

    class Procedure : public Object
    {
    public:
        Procedure (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_PROCEDURE; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Procedure)
    };

    class Function : public Procedure
    {
    public:
        Function (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Procedure(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_FUNCTION; }

        OT_ATTRIBUTE(std::string, Return);

        OT_CLONEABLE_COPYABLE(Function)
    };

        class PackageProcedure;

    class Package : public Object
    {
    public:
        Package (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent), m_order(NA) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_PACKAGE; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        bool          CanSortChildren () const            { return true; }
        Order         GetSortOrder () const               { return m_order; }
        void          SortChildren (Order);
    
    protected:
        Order m_order;
        void addProcedure (int subprogram_position, const string& subprogram_name,  const string& arg_list, const string& return_type, const std::vector<Argument>&);
        friend void sort_childern<Package, PackageProcedure> (Package*, Order);

        OT_CLONEABLE_COPYABLE(Package)
    };

    class PackageBody : public Object
    {
    public:
        PackageBody (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_PACKAGE_BODY; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(PackageBody)
    };

    class PackageProcedure : public Object
    {
    public:
        PackageProcedure (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent), m_Position(0) { SetType("PROCEDURE"); }

        OT_ATTRIBUTE(int, Position)
        OT_ATTRIBUTE(std::string, ArgumentList)

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_PROCEDURE; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }

        friend class Package;

        OT_CLONEABLE_COPYABLE(PackageProcedure)
    };

    class PackageFunction : public PackageProcedure
    {
    public:
        PackageFunction (TreeNodePool& treeNodePool, Object* parent) 
            : PackageProcedure(treeNodePool, parent) { SetType("FUNCTION"); }

        // TreeNode Objects methods
        std::string   GetTooltipText () const;
        int           GetImage () const                 { return IDII_FUNCTION; }

        OT_ATTRIBUTE(std::string, Return);

        OT_CLONEABLE_COPYABLE(PackageFunction)
    };

    class Type : public Package
    {
    public:
        Type (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Package(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_TYPE; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Type)
    };

    class TypeBody : public PackageBody
    {
    public:
        TypeBody (TreeNodePool& treeNodePool, Object* parent = 0) 
            : PackageBody(treeNodePool, parent) {}

        // TreeNode Objects methods
        int           GetImage () const                 { return IDII_TYPE_BODY; }

        OT_CLONEABLE_COPYABLE(TypeBody)
    };

    class Unknown : public Object
    {
    public:
        Unknown (TreeNodePool& treeNodePool, Object* parent = 0) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_UNKNOWN; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return true; }
        bool          IsRefreshable () const            { return false; }

        OT_CLONEABLE_COPYABLE(Unknown)
    };

    class FindObjectException : public Common::AppException {
        public: FindObjectException (const std::string& what) : Common::AppException (what) {}
    };

    void FindObjects (OciConnect&, const std::string& name, std::vector<ObjectDescriptor>&);

    class Partitioning : public Folder
    {
    public:
        Partitioning (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_ATTRIBUTE(std::string, PartitioningType);
        OT_ATTRIBUTE(std::string, SubpartitioningType);

        OT_CLONEABLE_COPYABLE(Partitioning)
    };

    class PartitionedBy : public Folder
    {
    public:
        PartitionedBy (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        bool          IsRefreshable () const            { return false; }

        OT_CLONEABLE_COPYABLE(PartitionedBy)
    };

    class SubpartitionedBy : public Folder
    {
    public:
        SubpartitionedBy (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        bool          IsRefreshable () const            { return false; }

        OT_CLONEABLE_COPYABLE(SubpartitionedBy)
    };

    class SubpartitionTemplate : public Folder
    {
    public:
        SubpartitionTemplate (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_ATTRIBUTE(std::string, Info)

        OT_CLONEABLE_COPYABLE(SubpartitionTemplate)
    };

    class Partitions : public Folder
    {
    public:
        Partitions (TreeNodePool& treeNodePool, Object* parent) 
            : Folder(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode*) const;
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Partitions)
    };

    class Partition : public Object
    {
    public:
        Partition (TreeNodePool& treeNodePool, Object* parent) 
            : Object(treeNodePool, parent) {}

        // TreeNode Objects methods
        std::string   GetLabel (TreeNode* parent) const;
        int           GetImage () const                 { return IDII_PARTITION; }
        void          GetTextToCopy (TextCtx&) const;
        bool          IsTrueObject () const             { return false; }
        bool          IsRefreshable () const            { return false; }
        void          Load (OciConnect&);

        OT_CLONEABLE_COPYABLE(Partition)
    };


};//namespace ObjectTree {
