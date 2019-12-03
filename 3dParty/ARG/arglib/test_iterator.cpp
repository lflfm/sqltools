//  File. . . . : test_iterator.cpp
//  Description : a test harness
//  Rev . . . . :
//
//  Classes . . : 
//
//  Modification History
//
//	31-Mar-00 : Original version                                Alan Griffiths
//
	/*-------------------------------------------------------------------**
	** arglib: A collection of utilities (e.g. smart pointers).          **
	** Copyright (C) 1999, 2000 Alan Griffiths.                          **
	**                                                                   **
	**  This code is part of the 'arglib' library. The latest version of **
	**  this library is available from:                                  **
	**																	 **
	**      http://www.octopull.demon.co.uk/arglib/                      **
	**																	 **
	**  This code may be used for any purpose provided that:             **
	**                                                                   **
	**    1. This complete notice is retained unchanged in any version   **
	**       of this code.                                               **
	**                                                                   **
	**    2. If this code is incorporated into any work that displays    **
	**       copyright notices then the copyright for the 'arglib'       **
	**       library must be included.                                   **
	**                                                                   **
	**  This library is distributed in the hope that it will be useful,  **
    **  but WITHOUT ANY WARRANTY; without even the implied warranty of   **
    **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.             **
	**                                                                   **
	**  You may contact the author at: alan@octopull.demon.co.uk         **
	**-------------------------------------------------------------------*/

#include "arg_iterator.h"

#include "arg_test.h"

#include <set>
#include <vector>
#include <list>

#ifdef _MSC_VER        
#include <algorithm>
#include <iterator>
#endif

// Make sure all the methods can instantiate...
#ifdef _MSC_VER        
#pragma warning(disable: 4660)
#endif

#if (_MSC_VER == 1100)
// MSVC5 has problem with templates instantiated with both
// const and non-const parameter types.
template struct arg::forward_iterator<const int>::implementation<const int*>;
template struct arg::bidirectional_iterator<const int>::implementation<const int*>;
#endif

template class arg::forward_iterator<int>;
template class arg::forward_iterator<const int>;

template class arg::bidirectional_iterator<int>;
template class arg::bidirectional_iterator<const int>;

namespace
{
	const int test_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	const int* const test_data_begin = test_data;

	const int* const test_data_end = 
		test_data + sizeof test_data / sizeof *test_data;
}

namespace test_forward_iterator
{
	using arg::forward_iterator;

	/// Count elements using opaque iterators
	int test_count(
		forward_iterator<const int> begin, 
		forward_iterator<const int> end)
	{
		int count = 0;

	    for (; begin != end; ++begin)
	    {
			++count;
	    }

		return count;
	}

	/// Sum elements using opaque iterators
	int test_sum(
		forward_iterator<const int> begin, 
		forward_iterator<const int> end)
	{
		int sum = 0;

	    for (; begin != end; ++begin)
	    {
			sum += *begin;
	    }

		return sum;
	}

	/// Sum elements using opaque iterators
	void increment(
		forward_iterator<int> begin, 
		forward_iterator<int> end)
	{
	    for (; begin != end; ++begin)
	    {
#if (_MSC_VER == 1100)
// MSVC5 has problem with templates instantiated with both
// const and non-const parameter types.
			++const_cast<int&>(*begin);
#else
			++*begin;
#endif
	    }
	}

	int run()
	{
		std::vector<int> vector(test_data_begin, test_data_end);
#ifndef _MSC_VER        
		std::list<int>   list(test_data_begin, test_data_end);
#else        
		std::list<int>   list;
        std::copy(test_data_begin, test_data_end, std::back_inserter(list));
#endif        
		std::set<int>    set(test_data_begin, test_data_end);

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(test_data_begin, test_data_end));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(vector.begin(), vector.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(list.begin(), list.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(set.begin(), set.end()));

		ARG_TEST(45 == test_sum(test_data_begin, test_data_end));

		ARG_TEST(45 == test_sum(vector.begin(), vector.end()));

		ARG_TEST(45 == test_sum(list.begin(), list.end()));

		ARG_TEST(45 == test_sum(set.begin(), set.end()));

		increment(vector.begin(), vector.end());

		increment(list.begin(), list.end());

		ARG_TEST(54 == test_sum(vector.rbegin(), vector.rend()));

		ARG_TEST(54 == test_sum(list.rbegin(), list.rend()));

		return 10;
	}	
}

namespace test_bidirectional_iterator
{
	using arg::bidirectional_iterator;

	/// Count elements using opaque iterators
	int test_count(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		int count = 0;

	    for (; begin != end; ++begin)
	    {
			++count;
	    }

		return count;
	}

	/// Sum elements using opaque iterators
	int test_sum(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		int sum = 0;

	    for (; begin != end; ++begin)
	    {
			sum += *begin;
	    }

		return sum;
	}

	/// Sum elements using opaque iterators
	void increment(
		bidirectional_iterator<int> begin, 
		bidirectional_iterator<int> end)
	{
	    for (; begin != end; ++begin)
	    {
#if (_MSC_VER == 1100)
// MSVC5 has problem with templates instantiated with both
// const and non-const parameter types.
			++const_cast<int&>(*begin);
#else
			++*begin;
#endif
	    }
	}

	int reverse_test_count(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		int count = 0;

	    for (; begin != end; --end)
	    {
			++count;
	    }

		return count;
	}

	/// Sum elements using opaque iterators
	int reverse_test_sum(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		int sum = 0;

	    while (begin != end)
	    {
			sum += *--end;
	    }

		return sum;
	}

	/// Sum elements using opaque iterators
	void reverse_decrement(
		bidirectional_iterator<int> begin, 
		bidirectional_iterator<int> end)
	{
	    while (begin != end)
	    {
#if (_MSC_VER == 1100)
// MSVC5 has problem with templates instantiated with both
// const and non-const parameter types.
			--const_cast<int&>(*--end);
#else
			--*--end;
#endif
	    }
	}

	/// Count elements using opaque iterators
	int forward_test_count(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		return test_forward_iterator::test_count(begin, end);
	}

	/// Sum elements using opaque iterators
	int forward_test_sum(
		bidirectional_iterator<const int> begin, 
		bidirectional_iterator<const int> end)
	{
		return test_forward_iterator::test_sum(begin, end);
	}

	/// Sum elements using opaque iterators
	void forward_increment(
		bidirectional_iterator<int> begin, 
		bidirectional_iterator<int> end)
	{
		test_forward_iterator::increment(begin, end);
	}

	int run()
	{
		std::vector<int> vector(test_data_begin, test_data_end);
#ifndef _MSC_VER        
		std::list<int>   list(test_data_begin, test_data_end);
#else        
		std::list<int>   list;
        std::copy(test_data_begin, test_data_end, std::back_inserter(list));
#endif        
		std::set<int>    set(test_data_begin, test_data_end);

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(test_data_begin, test_data_end));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(vector.begin(), vector.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(list.begin(), list.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			test_count(set.begin(), set.end()));

		ARG_TEST(45 == test_sum(test_data_begin, test_data_end));

		ARG_TEST(45 == test_sum(vector.begin(), vector.end()));

		ARG_TEST(45 == test_sum(list.begin(), list.end()));

		ARG_TEST(45 == test_sum(set.begin(), set.end()));

		increment(vector.begin(), vector.end());

		increment(list.begin(), list.end());

		ARG_TEST(54 == test_sum(vector.rbegin(), vector.rend()));

		ARG_TEST(54 == test_sum(list.rbegin(), list.rend()));

		// Now do it all backwards...

		ARG_TEST(54 == reverse_test_sum(vector.rbegin(), vector.rend()));

		ARG_TEST(54 == reverse_test_sum(list.rbegin(), list.rend()));

		reverse_decrement(vector.begin(), vector.end());

		reverse_decrement(list.begin(), list.end());

		ARG_TEST(test_data_end-test_data_begin == 
			reverse_test_count(test_data_begin, test_data_end));

		ARG_TEST(test_data_end-test_data_begin == 
			reverse_test_count(vector.begin(), vector.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			reverse_test_count(list.begin(), list.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			reverse_test_count(set.begin(), set.end()));

		ARG_TEST(45 == reverse_test_sum(test_data_begin, test_data_end));

		ARG_TEST(45 == reverse_test_sum(vector.begin(), vector.end()));

		ARG_TEST(45 == reverse_test_sum(list.begin(), list.end()));

		ARG_TEST(45 == reverse_test_sum(set.begin(), set.end()));

		// And again with conversions to forward_iterators

		ARG_TEST(test_data_end-test_data_begin == 
			forward_test_count(test_data_begin, test_data_end));

		ARG_TEST(test_data_end-test_data_begin == 
			forward_test_count(vector.begin(), vector.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			forward_test_count(list.begin(), list.end()));

		ARG_TEST(test_data_end-test_data_begin == 
			forward_test_count(set.begin(), set.end()));

		ARG_TEST(45 == forward_test_sum(test_data_begin, test_data_end));

		ARG_TEST(45 == forward_test_sum(vector.begin(), vector.end()));

		ARG_TEST(45 == forward_test_sum(list.begin(), list.end()));

		ARG_TEST(45 == forward_test_sum(set.begin(), set.end()));

		forward_increment(vector.begin(), vector.end());

		forward_increment(list.begin(), list.end());

		ARG_TEST(54 == forward_test_sum(vector.rbegin(), vector.rend()));

		ARG_TEST(54 == forward_test_sum(list.rbegin(), list.rend()));

		return 30;
	}	
}

int main()
{
	int tests = test_forward_iterator::run();
	tests += test_bidirectional_iterator::run();

	// THIS SHOULD BE AN ERROR - as a set iterator returns
	// a _const_ reference.  But it compiles in gcc and bcc.
	//{
	//	std::set<int>    set(test_data_begin, test_data_end);
	//	test_forward_iterator::increment(set.begin(), set.end());
	//}

	// THIS SHOULD BE AN ERROR - and fails to compile in gcc and bcc.
	//increment(test_data_begin, test_data_end);

	return ARG_END_TESTS(tests);
}	
