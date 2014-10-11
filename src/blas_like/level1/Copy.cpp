/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename T>
void Copy( const Matrix<T>& A, Matrix<T>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    B = A; 
}

template<typename S,typename T>
void Copy( const Matrix<S>& A, Matrix<T>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    auto convert = []( const S alpha ) { return T(alpha); };
    EntrywiseMap( A, B, std::function<T(S)>(convert) );
}

template<typename T,Dist U,Dist V>
inline void Copy( const AbstractDistMatrix<T>& A, DistMatrix<T,U,V>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    B = A;
}

// Datatype conversions should not be very common, and so it is likely best to
// avoid explicitly instantiating every combination
template<typename S,typename T,Dist U,Dist V>
inline void Copy( const AbstractDistMatrix<S>& A, DistMatrix<T,U,V>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    if( A.Grid() == B.Grid() && A.ColDist() == U && A.RowDist() == V )
    {
        if( !B.RootConstrained() )
            B.SetRoot( A.Root() );
        if( !B.ColConstrained() )
            B.AlignCols( A.ColAlign() );
        if( !B.RowConstrained() )
            B.AlignRows( A.RowAlign() );
        if( A.Root() == B.Root() && 
            A.ColAlign() == B.ColAlign() && A.RowAlign() == B.RowAlign() )
        {
            B.Resize( A.Height(), A.Width() );
            Copy( A.LockedMatrix(), B.Matrix() );
            return;
        }
    }
    DistMatrix<S,U,V> BOrig(A.Grid());
    BOrig.AlignWith( B );
    BOrig = A;
    B.Resize( A.Height(), A.Width() );
    Copy( BOrig.LockedMatrix(), B.Matrix() );
}

template<typename T,Dist U,Dist V>
inline void Copy
( const AbstractBlockDistMatrix<T>& A, BlockDistMatrix<T,U,V>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    B = A;
}

// Datatype conversions should not be very common, and so it is likely best to
// avoid explicitly instantiating every combination
template<typename S,typename T,Dist U,Dist V>
inline void Copy
( const AbstractBlockDistMatrix<S>& A, BlockDistMatrix<T,U,V>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    if( A.Grid() == B.Grid() && A.ColDist() == U && A.RowDist() == V )
    {
        if( !B.RootConstrained() )
            B.SetRoot( A.Root() );
        if( !B.ColConstrained() )
            B.AlignColsWith( A.DistData() );
        if( !B.RowConstrained() )
            B.AlignRowsWith( A.DistData() );
        if( A.Root() == B.Root() && 
            A.ColAlign() == B.ColAlign() && 
            A.RowAlign() == B.RowAlign() && 
            A.ColCut() == B.ColCut() &&
            A.RowCut() == B.RowCut() )
        {
            B.Resize( A.Height(), A.Width() );
            Copy( A.LockedMatrix(), B.Matrix() );
            return;
        }
    }
    BlockDistMatrix<S,U,V> BOrig(A.Grid());
    BOrig.AlignWith( B );
    BOrig = A;
    B.Resize( A.Height(), A.Width() );
    Copy( BOrig.LockedMatrix(), B.Matrix() );
}

template<typename S,typename T>
void Copy( const AbstractDistMatrix<S>& A, AbstractDistMatrix<T>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    #define GUARD(CDIST,RDIST) B.ColDist() == CDIST && B.RowDist() == RDIST
    #define PAYLOAD(CDIST,RDIST) \
        auto& BCast = dynamic_cast<DistMatrix<T,CDIST,RDIST>&>(B); \
        Copy( A, BCast );
    #include "El/macros/GuardAndPayload.h"
}

template<typename S,typename T>
void Copy( const AbstractBlockDistMatrix<S>& A, AbstractBlockDistMatrix<T>& B )
{
    DEBUG_ONLY(CallStackEntry cse("Copy"))
    #define GUARD(CDIST,RDIST) B.ColDist() == CDIST && B.RowDist() == RDIST
    #define PAYLOAD(CDIST,RDIST) \
        auto& BCast = dynamic_cast<BlockDistMatrix<T,CDIST,RDIST>&>(B); \
        Copy( A, BCast );
    #include "El/macros/GuardAndPayload.h"
}

// TODO: include guards so that certain datatypes can be properly disabled 

#define CONVERT(S,T) \
  template void Copy( const Matrix<S>& A, Matrix<T>& B ); \
  template void Copy \
  ( const AbstractDistMatrix<S>& A, AbstractDistMatrix<T>& B ); \
  template void Copy \
  ( const AbstractBlockDistMatrix<S>& A, AbstractBlockDistMatrix<T>& B );

#define PROTO_INT(T) CONVERT(T,T)

#define PROTO_REAL(Real) \
  CONVERT(Real,Real) \
  /* Promotions up to Real */ \
  CONVERT(Int,Real) \
  /* Promotions up from Real */ \
  CONVERT(Real,Complex<Real>)

#define PROTO_COMPLEX(C) \
  CONVERT(C,C) \
  /* Promotions up to C */ \
  CONVERT(Int,C)

#define PROTO_FLOAT \
  PROTO_REAL(float) \
  /* Promotions up from float */ \
  CONVERT(float,double) \
  CONVERT(float,Complex<double>)

#define PROTO_DOUBLE \
  PROTO_REAL(double) \
  /* Promotions down to float */ \
  CONVERT(double,float) \
  /* Mixed conversion */ \
  CONVERT(double,Complex<float>)

#define PROTO_COMPLEX_FLOAT \
  PROTO_COMPLEX(Complex<float>) \
  /* Promotions up from Complex<float> */ \
  CONVERT(Complex<float>,Complex<double>)

#define PROTO_COMPLEX_DOUBLE \
  PROTO_COMPLEX(Complex<double>) \
  /* Promotions down from Complex<double> */ \
  CONVERT(Complex<double>,Complex<float>)

#include "El/macros/Instantiate.h"

} // namespace El