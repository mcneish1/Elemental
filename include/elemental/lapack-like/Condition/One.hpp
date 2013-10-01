/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_LAPACK_CONDITION_ONE_HPP
#define ELEM_LAPACK_CONDITION_ONE_HPP

#include "elemental/lapack-like/Inverse.hpp"
#include "elemental/lapack-like/Norm/One.hpp"

namespace elem {

template<typename F> 
inline Base<F>
OneCondition( const Matrix<F>& A )
{
#ifndef RELEASE
    CallStackEntry entry("OneCondition");
#endif
    typedef Base<F> Real;
    Matrix<F> B( A );
    const Real oneNorm = OneNorm( B );
    try { Inverse( B ); }
    catch( SingularMatrixException& e ) 
    { return std::numeric_limits<Real>::infinity(); }
    const Real oneNormInv = OneNorm( B );
    return oneNorm*oneNormInv;
}

template<typename F,Distribution U,Distribution V> 
inline Base<F>
OneCondition( const DistMatrix<F,U,V>& A )
{
#ifndef RELEASE
    CallStackEntry entry("OneCondition");
#endif
    typedef Base<F> Real;
    DistMatrix<F> B( A );
    const Real oneNorm = OneNorm( B );
    try { Inverse( B ); }
    catch( SingularMatrixException& e ) 
    { return std::numeric_limits<Real>::infinity(); }
    const Real oneNormInv = OneNorm( B );
    return oneNorm*oneNormInv;
}

} // namespace elem

#endif // ifndef ELEM_LAPACK_CONDITION_ONE_HPP
