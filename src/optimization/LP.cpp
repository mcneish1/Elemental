/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename Real>
void LP
( const Matrix<Real>& A, 
  const Matrix<Real>& b, const Matrix<Real>& c, 
        Matrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))
    if( ctrl.approach == LP_ADMM )
    {
        lp::primal::ADMM( A, b, c, x, ctrl.admmCtrl );
        return;
    }

    // TODO: Use the initialization suggested by Vandenberghe
    Matrix<Real> y, z;
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );

    if( ctrl.approach == LP_IPF )
        lp::primal::IPF( A, b, c, x, y, z, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::primal::Mehrotra( A, b, c, x, y, z, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const Matrix<Real>& A, const Matrix<Real>& G,
  const Matrix<Real>& b, const Matrix<Real>& c, 
  const Matrix<Real>& h,
        Matrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // Use the initialization procedure suggested by Vandenberghe
    Matrix<Real> y, z, s;
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );
    s = h;
    Gemv( NORMAL, Real(-1), G, x, Real(1), s );

    if( ctrl.approach == LP_IPF )
        lp::dual::IPF( A, G, b, c, h, x, y, z, s, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::dual::Mehrotra( A, G, b, c, h, x, y, z, s, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const AbstractDistMatrix<Real>& A, 
  const AbstractDistMatrix<Real>& b, const AbstractDistMatrix<Real>& c,
        AbstractDistMatrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))
    if( ctrl.approach == LP_ADMM )
    {
        lp::primal::ADMM( A, b, c, x, ctrl.admmCtrl );
        return;
    }

    // TODO: Use the initialization suggested by Vandenberghe
    DistMatrix<Real> y(A.Grid()), z(A.Grid());
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );

    if( ctrl.approach == LP_IPF )
        lp::primal::IPF( A, b, c, x, y, z, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::primal::Mehrotra( A, b, c, x, y, z, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& G,
  const AbstractDistMatrix<Real>& b, const AbstractDistMatrix<Real>& c,
  const AbstractDistMatrix<Real>& h,
        AbstractDistMatrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // TODO: Use the initialization suggested by Vandenberghe
    DistMatrix<Real> y(A.Grid()), z(A.Grid()), s(A.Grid());
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );
    s = h;
    Gemv( NORMAL, Real(-1), G, x, Real(1), s );

    if( ctrl.approach == LP_IPF )
        lp::dual::IPF( A, G, b, c, h, x, y, z, s, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::dual::Mehrotra( A, G, b, c, h, x, y, z, s, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const SparseMatrix<Real>& A, 
  const Matrix<Real>& b, const Matrix<Real>& c, 
        Matrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // TODO: Use the initialization suggested by Vandenberghe
    Matrix<Real> y, z;
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );

    if( ctrl.approach == LP_IPF )
        lp::primal::IPF( A, b, c, x, y, z, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::primal::Mehrotra( A, b, c, x, y, z, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const SparseMatrix<Real>& A, const SparseMatrix<Real>& G,
  const Matrix<Real>& b, const Matrix<Real>& c, 
  const Matrix<Real>& h,
        Matrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // TODO: Use the initialization suggested by Vandenberghe
    Matrix<Real> y, z, s;
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );
    s = h;
    Multiply( NORMAL, Real(-1), G, x, Real(1), s );

    if( ctrl.approach == LP_IPF )
        lp::dual::IPF( A, G, b, c, h, x, y, z, s, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::dual::Mehrotra( A, G, b, c, h, x, y, z, s, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const DistSparseMatrix<Real>& A, 
  const DistMultiVec<Real>& b, const DistMultiVec<Real>& c, 
        DistMultiVec<Real>& x, const lp::primal::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // TODO: Use the initialization suggested by Vandenberghe
    DistMultiVec<Real> y(A.Comm()), z(A.Comm());
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );

    if( ctrl.approach == LP_IPF )
        lp::primal::IPF( A, b, c, x, y, z, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::primal::Mehrotra( A, b, c, x, y, z, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

template<typename Real>
void LP
( const DistSparseMatrix<Real>& A, const DistSparseMatrix<Real>& G,
  const DistMultiVec<Real>& b, const DistMultiVec<Real>& c, 
  const DistMultiVec<Real>& h,
        DistMultiVec<Real>& x, const lp::dual::Ctrl<Real>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LP"))

    // TODO: Use the initialization suggested by Vandenberghe
    DistMultiVec<Real> y(A.Comm()), z(A.Comm()), s(A.Comm());
    Zeros( y, A.Height(), 1 );
    Uniform( z, A.Width(), 1, Real(0.5), Real(0.49) );
    s = h;
    Multiply( NORMAL, Real(-1), G, x, Real(1), s );

    if( ctrl.approach == LP_IPF )
        lp::dual::IPF( A, G, b, c, h, x, y, z, s, ctrl.ipfCtrl );
    else if( ctrl.approach == LP_MEHROTRA )
        lp::dual::Mehrotra( A, G, b, c, h, x, y, z, s, ctrl.mehrotraCtrl );
    else
        LogicError("Unsupported solver");
}

#define PROTO(Real) \
  template void LP \
  ( const Matrix<Real>& A, \
    const Matrix<Real>& b, const Matrix<Real>& c, \
          Matrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const Matrix<Real>& A, const Matrix<Real>& G, \
    const Matrix<Real>& b, const Matrix<Real>& c, \
    const Matrix<Real>& h, \
          Matrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const AbstractDistMatrix<Real>& A, \
    const AbstractDistMatrix<Real>& b, const AbstractDistMatrix<Real>& c, \
          AbstractDistMatrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& G, \
    const AbstractDistMatrix<Real>& b, const AbstractDistMatrix<Real>& c, \
    const AbstractDistMatrix<Real>& h, \
          AbstractDistMatrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const SparseMatrix<Real>& A, \
    const Matrix<Real>& b, const Matrix<Real>& c, \
          Matrix<Real>& x, const lp::primal::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const SparseMatrix<Real>& A, const SparseMatrix<Real>& G, \
    const Matrix<Real>& b, const Matrix<Real>& c, \
    const Matrix<Real>& h, \
          Matrix<Real>& x, const lp::dual::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const DistSparseMatrix<Real>& A, \
    const DistMultiVec<Real>& b, const DistMultiVec<Real>& c, \
          DistMultiVec<Real>& x, const lp::primal::Ctrl<Real>& ctrl ); \
  template void LP \
  ( const DistSparseMatrix<Real>& A, const DistSparseMatrix<Real>& G, \
    const DistMultiVec<Real>& b, const DistMultiVec<Real>& c, \
    const DistMultiVec<Real>& h, \
          DistMultiVec<Real>& x, const lp::dual::Ctrl<Real>& ctrl );

#define EL_NO_INT_PROTO
#define EL_NO_COMPLEX_PROTO
#include "El/macros/Instantiate.h"

} // namespace El