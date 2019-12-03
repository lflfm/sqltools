/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2016 Aleksey Kochetov

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

namespace ThreadCommunication 
{ 
    template <class T, int instance>
    class Singleton : boost::noncopyable
    {
    public:
        static T& Get ()
        {
            CCriticalSection cs;
            CSingleLock lk(&cs, TRUE);

            if (!m_instance)
                m_instance = new T;

            return *m_instance; 
        }

        static void Destroy ()
        {
            delete m_instance;
            m_instance = 0;
        }
    private:
        static T* m_instance;
    };

    template <class T, int instance>
    T* Singleton<T, instance>::m_instance = 0;

};// namespace ThreadCommunication 
