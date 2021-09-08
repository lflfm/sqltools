/*
    Copyright (C) 2002,2011 Aleksey Kochetov

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
#ifndef __OEString_h__
#define __OEString_h__

#include <COMMON/ExceptionHelper.h>

namespace Common
{

template <typename T = char>
class OEString
{
public:
    typedef typename size_t size_type;

    static const size_type maxlen = (INT_MAX-1);
    static const size_type BYTES_PER_PARA = 16;

    OEString ();
    OEString (const OEString& other);
    OEString (const T* str, size_type len = maxlen);
    ~OEString ();

    T at (size_type pos) const;
    const T* data () const;          // it is NOT a NULL terminated string
    T first () const;
    T last () const;

    size_type size () const;
    size_type length () const;
    size_type capacity () const;         // it returns 0 for externally allocated string
    size_type max_size () const;
    bool is_static_str () const;

    void cleanup ();
    void operator = (const OEString& other);
    void assign (const OEString& other);
    void assign (const T* str, size_type len = maxlen, bool exact = false);
    void assign_static (const T* str, size_type len);
    void append (T ch);
    void append (const T* str, size_type len);
    void append (const OEString& other);
    void insert (size_type pos, const T* str, size_type len = maxlen);
    void replace (size_type pos, T ch);
    void erase (size_type from, size_type count = maxlen);
    void truncate ();

    T* reserve (size_type len);
    void set_length (size_type len);

    static void _Xlen ();	                // report a length_error
    static void _Xran (const char* where);	// report an out_of_range error

private:
    void _reallocate (size_type length, bool = false);
    void reallocate (size_type length1, size_type length2, bool = false);
    T&   _ref_counter (const T* ptr);
    size_type _string_length (const T*);

private:
    size_type _length, _capacity;
    T* _buffer;

    void _set_length (size_type length)     { _length  = length; }
    void _set_capacity (size_type capacity) { _capacity = capacity; }

}; // OEString


    template <typename T>
    inline
    OEString<T>::OEString ()
    {
        memset(this, 0, sizeof(OEString));
    }

    template <typename T>
    inline
    OEString<T>::OEString (const OEString<T>& other)
    {
        memset(this, 0, sizeof(OEString));
        *this = other;
    }

    template <typename T>
    inline
    OEString<T>::OEString (const T* str, size_type len)
    {
        memset(this, 0, sizeof(OEString));
        assign(str, len);
    }

    template <typename T>
    inline
    OEString<T>::~OEString ()
    {
        try { EXCEPTION_FRAME;

            cleanup();
        }
        _DESTRUCTOR_HANDLER_;
    }

    template <typename T>
    inline
    T OEString<T>::at (size_type pos) const
    {
        if (pos >= length()) _Xran("OEString<T>::at");

        return _buffer[pos];
    }

    // it is NOT a NULL terminated string
    template <typename T>
    inline
    const T* OEString<T>::data () const
    {
        return _buffer;
    }

    template <typename T>
    inline
    T OEString<T>::first () const
    {
        return at(0);
    }

    template <typename T>
    inline
    T OEString<T>::last () const
    {
        return at(length()-1);
    }

    template <typename T>
    inline
    typename OEString<T>::size_type OEString<T>::size () const
    {
        return _length;
    }

    template <typename T>
    inline
    typename OEString<T>::size_type OEString<T>::length () const
    {
        return _length;
    }

    // it returns 0 for externally allocated string
    template <typename T>
    inline
    typename OEString<T>::size_type OEString<T>::capacity () const
    {
        return _capacity;
    }

    template <typename T>
    inline
    typename OEString<T>::size_type OEString<T>::max_size () const
    {
        return maxlen;
    }

    template <typename T>
    inline
    bool OEString<T>::is_static_str () const
    {
        return !capacity() && _buffer;
    }

    template <typename T>
    inline
    void OEString<T>::cleanup ()
    {
        if (_buffer)
        {
            if (capacity() && !(--_ref_counter(_buffer)))
                delete[] (_buffer-1);
            _buffer = 0;
        }
        _set_length(0);
        _set_capacity(0);
    }

    template <typename T>
    inline
    void OEString<T>::operator = (const OEString& other)
    {
        assign(other);
    }

    template <typename T>
    void OEString<T>::assign (const OEString& other)
    {
        if (this != &other)
        {
            cleanup();

            if (other._buffer)
            {
                if (!other.capacity()) // other has an external buffer!
                {
                    _buffer = other._buffer;
                }
                else if (_ref_counter(other._buffer) < 127) // typically
                {
                    _buffer = other._buffer;
                    ++_ref_counter(_buffer);
                }
                else // ref counter overflow!
                {
                    _buffer   = new T[other.capacity() + 1] + 1;
                    _ref_counter(_buffer) = 1;
                    memcpy(_buffer, other._buffer, other.length() * sizeof(T));
                }
            }
            _set_length(other.length());
            _set_capacity(other.capacity());
        }
    }

    template <typename T>
    void OEString<T>::assign (const T* str, size_type len, bool exact)
    {
        if (len > 0)
        {
            if (len == max_size())
            {
                size_type size = _string_length(str);
                if (size > max_size()) OEString<T>::_Xlen();
                len = size;
            }

            if (!capacity() || len > capacity() || _ref_counter(_buffer) > 1)
                reallocate(len, capacity(), exact);

            memcpy(_buffer, str, len * sizeof(T));
            _set_length(len);
        }
        else
            cleanup();
    }

    template <typename T>
    void OEString<T>::assign_static (const T* str, size_type len)
    {
        if (len > max_size()) OEString<T>::_Xlen();

        if (capacity()) cleanup();
        _buffer = const_cast<T*>(str);
        _set_length(len);
    }

    template <typename T>
    void OEString<T>::append (T ch)
    {
        if (!capacity() || length() + 1 > capacity() || _ref_counter(_buffer) > 1)
            reallocate(length() + 1, capacity());

        _buffer[_length++] = ch;
    }

    template <typename T>
    void OEString<T>::append (const T* str, size_type len)
    {
        if (len > 0)
        {
            if (!capacity() || length() + len > capacity() || _ref_counter(_buffer) > 1)
                reallocate(length() + len, capacity());

            memcpy(_buffer + length(), str, len * sizeof(T));
            _set_length(length() + len);
        }
    }

    template <typename T>
    void OEString<T>::append (const OEString& other)
    {
        if (other.length() > 0)
        {
            if (!capacity() || length() + other.length() > capacity() || _ref_counter(_buffer) > 1)
                reallocate(length() + other.length(), capacity());

            memcpy(_buffer + length(), other._buffer, other.length() * sizeof(T));
            _set_length(length() + other.length());
        }
    }

    template <typename T>
    void OEString<T>::insert (size_type pos, const T* str, size_type len)
    {
        if (len > 0)
        {
            if (pos > length()) _Xran("OEString<T>::insert");

            if (len == max_size())
                len = _string_length(str);

            if (!capacity() || length() + len > capacity() || _ref_counter(_buffer) > 1)
                reallocate(length() + len, capacity());

            memmove(_buffer + pos + len, _buffer + pos, (length() - pos) * sizeof(T));
            memcpy(_buffer + pos, str, len * sizeof(T));
            _set_length(length() + len);
        }
    }

    template <typename T>
    void OEString<T>::replace (size_type pos, T ch)
    {
        if (pos >= length()) _Xran("OEString<T>::replace");

        if (!capacity() || _ref_counter(_buffer) > 1)
            reallocate(length(), capacity());

        _buffer[pos] = ch;
    }

    template <typename T>
    void OEString<T>::erase (size_type from, size_type count)
    {
        if (from > length()) _Xran("OEString<T>::erase");

        if (from < length())
        {
            if (count >= length() - from)
            {
                if (from)
                {
                    _set_length(from);

                    if (capacity() && _ref_counter(_buffer))
                        _reallocate(length()); // split
                }
                else  // erase from 0 pos
                    cleanup();

            }
            else
            {
                if (!capacity() && !from)
                {
                    _buffer += count;
                }
                else
                {
                    if (!capacity() || _ref_counter(_buffer) > 1)
                        reallocate(length(), capacity());

                    if (from + count > length())
                        count = length() - from;

                    memmove(_buffer + from, _buffer + from + count, (length() - (from + count)) * sizeof(T));
                }
                _set_length(length() - count);
            }
        }
    }

    template <>
    inline
    void OEString<char>::truncate ()
    {
        if (_buffer)
            for (; _length > 0 && isspace(_buffer[_length-1]); _length--)
                ;
    }

    template <>
    inline
    void OEString<wchar_t>::truncate ()
    {
        if (_buffer)
            for (; _length > 0 && iswspace(_buffer[_length-1]); _length--)
                ;
    }

    template <typename T>
    inline
    T& OEString<T>::_ref_counter (const T* ptr)
    {
        return ((T*)ptr)[-1];
    }

    template <>
    inline
    typename OEString<char>::size_type OEString<char>::_string_length (const char* str)
    {
        return strlen(str);
    }

    template <>
    inline
    typename OEString<wchar_t>::size_type OEString<wchar_t>::_string_length (const wchar_t* str)
    {
        return wcslen(str);
    }

    template <typename T>
    T* OEString<T>::reserve (size_type len)
    {
        _reallocate(len, false);
        return _buffer;
    }

    template <typename T>
    inline
    void OEString<T>::set_length (size_type len)
    {
        if (len > capacity()) OEString<T>::_Xlen();
        _set_length(len);
    }

    template <typename T>
    void OEString<T>::reallocate (size_type length1, size_type length2, bool exact)
    {
        _reallocate(max(length1, length2), exact);
    }

    template <typename T>
    void OEString<T>::_reallocate (size_type len, bool exact)
    {
        if (len < length())
            return;

        if (!len) 
            cleanup();

        size_type cap = exact ? len : (len + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);

        if (cap + 1 > max_size()) _Xlen();

        T* buffer = new T[cap + 1];
        _ref_counter(buffer + 1) = 1;
        memcpy(buffer + 1, _buffer, length() * sizeof(T));

        if (capacity() && !(--_ref_counter(_buffer)))
        {
            delete[] (_buffer-1);
        }

        _buffer = buffer + 1;
        _set_capacity(cap);
    }

    template <typename T>
    void OEString<T>::_Xlen() // report a length_error
    {
        THROW_STD_EXCEPTION("OEString: data is too long.");	
    }

    template <typename T>
    void OEString<T>::_Xran(const char* where) // report an out_of_range error
    {
        _THROW2(std::out_of_range, std::string(where) + ": invalid subscript.");
    }

    typedef OEString<char> OEStringA;
    typedef OEString<wchar_t> OEStringW;


}; // Common

#endif//__OEString_h__
