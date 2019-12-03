/*
    Copyright (C) 2004 Aleksey Kochetov

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
#ifndef __HANDLER_H__
#define __HANDLER_H__

namespace Common
{
    template <class O>
    struct Handler
    {
        struct Thunk
        {
            virtual ~Thunk() {}
            virtual bool Handle (O&) = 0;
        }
        *m_pThunk;

        Handler ()  { m_pThunk = 0; }
        ~Handler () { delete m_pThunk; }

        bool Handle (O& obj, bool defVal = true)
            {
                if (m_pThunk)
                    return m_pThunk->Handle(obj);
                else
                    return defVal;
            }

        void operator = (Thunk* thunk)
            {
                delete m_pThunk;
                m_pThunk = thunk;
            }
    };

    template <class T, class O>
    class AnyHandler : public Handler<O>::Thunk
    {
        T& m_Owner;
        bool (T::*m_fnHandler)(O&);
        bool Handle (O& obj) { return (m_Owner.*m_fnHandler)(obj); }
    public:
        AnyHandler (T& owner, bool (T::*handler)(O&))
            : m_Owner(owner), m_fnHandler(handler) {}
    };

    template <class T, class O>
    void SetHandler (Handler<O>& thunk, T& owner, bool (T::*handler)(O&))
        {
            thunk = new AnyHandler<T,O>(owner, handler);
        }

    template <class O, class P>
    struct Handler1
    {
        struct Thunk
        {
            virtual ~Thunk() {}
            virtual bool Handle (O&, P& param) = 0;
        }
        *m_pThunk;

        Handler1 ()  { m_pThunk = 0; }
        ~Handler1 () { delete m_pThunk; }

        bool Handle (O& obj, P& param, bool defVal)
            {
                if (m_pThunk)
                    return m_pThunk->Handle(obj, param);
                else
                    return defVal;
            }

        void operator = (Thunk* thunk)
            {
                delete m_pThunk;
                m_pThunk = thunk;
            }
    };

    template <class T, class O, class P>
    class AnyHandler1 : public Handler1<O,P>::Thunk
    {
        T& m_Owner;
        bool (T::*m_fnHandler)(O&, P&);
        bool Handle (O& obj, P& param) { return (m_Owner.*m_fnHandler)(obj, param); }
    public:
        AnyHandler1 (T& owner, bool (T::*handler)(O&, P&))
            : m_Owner(owner), m_fnHandler(handler) {}
    };

    template <class T, class O, class P>
    void SetHandler1 (Handler1<O,P>& thunk, T& owner, bool (T::*handler)(O&, P&))
        {
            thunk = new AnyHandler1<T,O,P>(owner, handler);
        }


    template <class O, class P, class P2>
    struct Handler2
    {
        struct Thunk
        {
            virtual ~Thunk() {}
            virtual bool Handle (O&, P& param, P2& param2) = 0;
        }
        *m_pThunk;

        Handler2 ()  { m_pThunk = 0; }
        ~Handler2 () { delete m_pThunk; }

        bool Handle (O& obj, P& param, P2& param2, bool defVal)
            {
                if (m_pThunk)
                    return m_pThunk->Handle(obj, param, param2);
                else
                    return defVal;
            }

        void operator = (Thunk* thunk)
            {
                delete m_pThunk;
                m_pThunk = thunk;
            }
    };

    template <class T, class O, class P, class P2>
    class AnyHandler2 : public Handler2<O,P,P2>::Thunk
    {
        T& m_Owner;
        bool (T::*m_fnHandler)(O&, P&, P2&);
        bool Handle (O& obj, P& param, P2& param2) { return (m_Owner.*m_fnHandler)(obj, param, param2); }
    public:
        AnyHandler2 (T& owner, bool (T::*handler)(O&, P&, P2&))
            : m_Owner(owner), m_fnHandler(handler) {}
    };

    template <class T, class O, class P, class P2>
    void SetHandler2 (Handler2<O,P,P2>& thunk, T& owner, bool (T::*handler)(O&, P&, P2&))
        {
            thunk = new AnyHandler2<T,O,P,P2>(owner, handler);
        }

    template <class O, class P, class P2, class P3>
    struct Handler3
    {
        struct Thunk
        {
            virtual ~Thunk() {}
            virtual bool Handle (O&, P& param, P2& param2, P3& param3) = 0;
        }
        *m_pThunk;

        Handler3 ()  { m_pThunk = 0; }
        ~Handler3 () { delete m_pThunk; }

        bool Handle (O& obj, P& param, P2& param2, P3& param3, bool defVal)
            {
                if (m_pThunk)
                    return m_pThunk->Handle(obj, param, param2, param3);
                else
                    return defVal;
            }

        void operator = (Thunk* thunk)
            {
                delete m_pThunk;
                m_pThunk = thunk;
            }
    };

    template <class T, class O, class P, class P2, class P3>
    class AnyHandler3 : public Handler3<O,P,P2,P3>::Thunk
    {
        T& m_Owner;
        bool (T::*m_fnHandler)(O&, P&, P2&, P3&);
        bool Handle (O& obj, P& param, P2& param2, P3& param3) { return (m_Owner.*m_fnHandler)(obj, param, param2, param3); }
    public:
        AnyHandler3 (T& owner, bool (T::*handler)(O&, P&, P2&, P3&))
            : m_Owner(owner), m_fnHandler(handler) {}
    };

    template <class T, class O, class P, class P2, class P3>
    void SetHandler3 (Handler3<O,P,P2,P3>& thunk, T& owner, bool (T::*handler)(O&, P&, P2&, P3&))
        {
            thunk = new AnyHandler3<T,O,P,P2,P3>(owner, handler);
        }

}//namespace Common

#endif//__HANDLER_H__

