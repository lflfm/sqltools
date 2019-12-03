/* 
    Copyright (C) 2005 Aleksey Kochetov

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

namespace Common
{
    template <class T, int N>
    class FixedArray : private noncopyable
    {
    public:
        // type definitions
        typedef T              value_type;
        typedef T*             iterator;
        typedef const T*       const_iterator;
        typedef T&             reference;
        typedef const T&       const_reference;
        typedef std::size_t    size_type;

        FixedArray () : m_cursize(0) {}
        ~FixedArray () { clear(); }

        void clear () {
            for (size_type i = 0; i < m_cursize; i++)
                operator[](i).~T();
            m_cursize = 0;
        }

        // operator[]
        reference operator[] (size_type i)              { ASSERT( i < N && "out of range" ); return elems()[i]; }
        const_reference operator[] (size_type i) const  { ASSERT( i < N && "out of range" ); return elems()[i]; }

        // at() with range check
        reference at (size_type i)                      { rangecheck(i); return elems()[i]; }
        const_reference at (size_type i) const          { rangecheck(i); return elems()[i]; }
    
        // front() and back()
        reference front ()                              { ASSERT( m_cursize > 0  && "out of range" ); return elems()[0]; }
        const_reference front () const                  { ASSERT( m_cursize > 0  && "out of range" ); return elems()[0]; }
        reference back ()                               { ASSERT( m_cursize > 0  && "out of range" ); return elems()[m_cursize-1]; }
        const_reference back () const                   { ASSERT( m_cursize > 0  && "out of range" ); return elems()[m_cursize-1]; }

        const_iterator begin () const                   { return &elems()[0]; }
        iterator begin ()                               { return &elems()[0]; }
        const_iterator end () const                     { return &elems()[m_cursize]; }
        iterator end ()                                 { return &elems()[m_cursize]; }

        size_type size() const                          { return m_cursize; }
        bool empty() const                              { return m_cursize == 0; }
        static size_type max_size()                     { return N; }

        T& push_back (const_reference elem)             { limitcheck(); return *(new (&elems()[m_cursize++])T(elem)); }
        T& append ()                                    { limitcheck(); return *(new (&elems()[m_cursize++])T()); }

        void erase (size_type pos) {
            rangecheck(pos);

            for (size_type i = pos+1; i < m_cursize; i++)
                operator[](i-1) = operator[](i);
            
            operator[](m_cursize-1).~T();
            m_cursize--;
        }
        void erase (iterator it)                        { erase(it - begin()); }
        void erase ()                                   { clear(); }

    private:
        T*       elems ()                               { return (T*)m_buffer; }
        const T* elems () const                         { return (const T*)m_buffer; }
        void rangecheck (size_type i) const             { if (i >= size()) throw std::range_error("FixedArray<>: index out of range"); }
        void limitcheck () const                        { if (max_size() == size()) throw std::range_error("FixedArray<>: size limit"); }

        size_type m_cursize;
        unsigned char m_buffer[sizeof(T) * N];
    };

}; // namespace