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
#include <vector>
#include <deque>
#include <string>

namespace Common
{

template<class T>
class nvector : public std::vector<T>
{
public:
    nvector (const char* name) : m_name(name) {}
    nvector (const char* name, size_type count) : std::vector<T>(count), m_name(name) {}
    nvector (const nvector& other) : std::vector<T>(other), m_name(other.m_name) {}

    nvector& operator = (const nvector& other)
        {
            std::vector<T>::operator = (other);
            m_name = other.m_name;
            return *this;
        }

	const_reference at(size_type _Pos) const
		{	// subscript nonmutable sequence with checking
		if (size() <= _Pos)
			_Xran();
		return (*(begin() + _Pos));
		}

	reference at(size_type _Pos)
		{	// subscript mutable sequence with checking
		if (size() <= _Pos)
			_Xran();
		return (*(begin() + _Pos));
		}

private:
    const char* m_name;

	void _Xran() const
		{	// report an out_of_range error
            _THROW(std::out_of_range, std::string("invalid vector<T> subscript, class::object = ") + m_name);
		}
};

template<class T>
class ndeque : public std::deque<T>
{
public:
    ndeque (const char* name) : m_name(name) {}
    ndeque (const char* name, size_type count) : std::deque<T>(count), m_name(name) {}
    ndeque (const ndeque& other) : std::deque<T>(other), m_name(other.m_name) {}

    ndeque& operator = (const ndeque& other)
        {
            std::deque<T>::operator = (other);
            m_name = other.m_name;
            return *this;
        }

	const_reference at(size_type _Pos) const
		{	// subscript nonmutable sequence with checking
		if (size() <= _Pos)
			_Xran();
		return (*(begin() + _Pos));
		}

	reference at(size_type _Pos)
		{	// subscript mutable sequence with checking
		if (size() <= _Pos)
			_Xran();
		return (*(begin() + _Pos));
		}

private:
    const char* m_name;

	void _Xran() const
		{	// report an out_of_range error
            _THROW(std::out_of_range, std::string("invalid deque<T> subscript, class::object = ") + m_name);
		}
};

}; // Common
