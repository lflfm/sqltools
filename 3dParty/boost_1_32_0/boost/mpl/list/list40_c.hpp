
#ifndef BOOST_MPL_LIST_LIST40_C_HPP_INCLUDED
#define BOOST_MPL_LIST_LIST40_C_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Source: /cvsroot/oedt/oedt/3dParty/boost_1_32_0/boost/mpl/list/list40_c.hpp,v $
// $Date: 2005/10/11 02:00:25 $
// $Revision: 1.1.1.1 $

#if !defined(BOOST_MPL_PREPROCESSING_MODE)
#   include <boost/mpl/list/list30_c.hpp>
#endif

#include <boost/mpl/aux_/config/use_preprocessed.hpp>

#if !defined(BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS) \
 && !defined(BOOST_MPL_PREPROCESSING_MODE)

#   define BOOST_MPL_PREPROCESSED_HEADER list40_c.hpp
#   include <boost/mpl/list/aux_/include_preprocessed.hpp>

#else

#   include <boost/preprocessor/iterate.hpp>

namespace boost { namespace mpl {

#   define BOOST_PP_ITERATION_PARAMS_1 \
    (3,(31, 40, <boost/mpl/list/aux_/numbered_c.hpp>))
#   include BOOST_PP_ITERATE()

}}

#endif // BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS

#endif // BOOST_MPL_LIST_LIST40_C_HPP_INCLUDED