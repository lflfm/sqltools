/*
    Copyright (C) 2002 Aleksey Kochetov

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
#ifndef __FASTMAP_H__
#define __FASTMAP_H__

#include <map>

namespace Common
{
#pragma warning (push)
#pragma warning (disable : 4309)
    template<class T> // T should be char, bool, int...
    class Fastmap
    {
    public:
        Fastmap ();
        Fastmap (T defVal);
        Fastmap (const Fastmap&);
        Fastmap& operator = (const Fastmap&);
        void erase ();
        // actuall class was created for operator [] only!
            // Safe converion from char to unsigned char!
            // it's impotant for the second part of ascii table
        T  operator [] (unsigned int i) const     { return _data[(unsigned char)i]; }
        T& operator [] (unsigned int i)           { return _data[(unsigned char)i]; }
        unsigned int _size () const               { return sizeof(_data)/sizeof(_data[0]); }

    private:
        T _data[256];
    };

    // The original Fastmap was quite small and efficient
    // unfortunately the same approach did not work for whchar_t
    // so new FastmapW is combination of Fastmap and std:map
    // created with assumption that the map will be used extremely rare
    template<class T>
    class FastmapW 
    {
    public:
        FastmapW (T defaultVal);
        FastmapW (const FastmapW&);
        FastmapW& operator = (const FastmapW&);
        void erase ();
        T  operator [] (unsigned int i) const;
        void assign (unsigned int i, T value);
        void bitwiseOR (unsigned int i, T value);
        void indexes (std::vector<unsigned int>&);

    private:
        Fastmap<T> _fastmap;
        std::map<unsigned int, T> _map; 
        T _default;
    };
#pragma warning (pop)

    template<class T>
    Fastmap<T>::Fastmap<T> ()
    {
        erase();
    }

    template<class T>
    Fastmap<T>::Fastmap<T> (T defVal)
    {
        for (unsigned int i = 0; i < _size(); ++i)
            (*this)[i] = defVal;
    }

    template<class T>
    Fastmap<T>::Fastmap<T> (const Fastmap<T>& other)
    {
        memcpy(this, &other, sizeof(*this));
    }

    template<class T>
    Fastmap<T>& Fastmap<T>::operator = (const Fastmap<T>& other)
    {
        memcpy(this, &other, sizeof(*this));
        return *this;
    }

    template<class T>
    void Fastmap<T>::erase ()
    {
        memset(this, 0, sizeof(*this));
    }


    template<class T>
    FastmapW<T>::FastmapW (T defVal)
    {
        _default = defVal;
    }

    template<class T>
    FastmapW<T>::FastmapW<T> (const FastmapW<T>& other)
        : _fastmap(other._fastmap)
    {
        _default = other._default;
        _map = other._map;
    }

    template<class T>
    FastmapW<T>& FastmapW<T>::operator = (const FastmapW<T>& other)
    {
        _fastmap = other._fastmap;
        _default = other._default;
        _map = other._map;
        return *this;
    }

    template<class T>
    T  FastmapW<T>::operator [] (unsigned int i) const     
    { 
        if (i < 256)
            return _fastmap[i]; 

        if (!_map.empty())
        {
            auto it = _map.find(i);
            if (it != _map.end())
                return it->second;
        }

        return _default;
    }

    template<class T>
    void FastmapW<T>::assign (unsigned int i, T value)
    {
        if (i < 256)
            _fastmap[i] = value;
        else         
            _map[i] = value; 
    }

    template<class T>
    void FastmapW<T>::bitwiseOR (unsigned int i, T value)
    {
        if (i < 256)
            _fastmap[i] |= value;
        else         
        {
            auto it = _map.find(i);
            if (it != _map.end())
                _map[i] |= value; 
            else
                _map[i] = value; 
        }
    }

    template<class T>
    void FastmapW<T>::indexes (std::vector<unsigned int>& buff)
    {
        buff.clear();

        for (int i = 0; i < 256; ++i)
            if (_fastmap[i] != _default)
                buff.push_back(i);

        for (auto it = _map.begin(); it != _map.end(); ++it)
            if (it->second != _default)
                buff.push_back(it->first);
    }

    template<class T>
    void FastmapW<T>::erase ()
    {
        _fastmap.erase();
        _map.clear();
    }
} // END namespace Common

#endif//__FASTMAP_H__
