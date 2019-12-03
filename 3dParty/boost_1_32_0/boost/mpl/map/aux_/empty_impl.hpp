
#ifndef BOOST_MPL_MAP_AUX_EMPTY_IMPL_HPP_INCLUDED
#define BOOST_MPL_MAP_AUX_EMPTY_IMPL_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Source: /cvsroot/oedt/oedt/3dParty/boost_1_32_0/boost/mpl/map/aux_/empty_impl.hpp,v $
// $Date: 2005/10/11 02:00:25 $
// $Revision: 1.1.1.1 $

#include <boost/mpl/empty_fwd.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/map/aux_/tag.hpp>

namespace boost { namespace mpl {

template<>
struct empty_impl< aux::map_tag >
{
    template< typename Map > struct apply
        : not_< typename Map::size >
    {
    };
};

}}

#endif // BOOST_MPL_MAP_AUX_EMPTY_IMPL_HPP_INCLUDED
