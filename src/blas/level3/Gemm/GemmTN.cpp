/*
   Copyright (C) 2009-2010 Jack Poulson <jack.poulson@gmail.com>

   This file is part of Elemental.

   Elemental is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Elemental is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Elemental.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "elemental/blas_internal.hpp"
using namespace std;
using namespace elemental;

template<typename T>
void
elemental::blas::internal::GemmTN
( Orientation orientationOfA,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& B,
  T beta,        DistMatrix<T,MC,MR>& C )
{
#ifndef RELEASE
    PushCallStack("blas::internal::GemmTN");
    if( A.GetGrid() != B.GetGrid() || B.GetGrid() != C.GetGrid() )
        throw logic_error( "{A,B,C} must be distributed over the same grid." );
    if( orientationOfA == Normal )
        throw logic_error( "GemmTN assumes A is (Conjugate)Transposed." );
#endif
    const int m = C.Height();
    const int n = C.Width();
    const int k = A.Height();
    const float weightTowardsC = 2.0;

    if( m <= n && weightTowardsC*m <= k )
    {
        blas::internal::GemmTNB( orientationOfA, alpha, A, B, beta, C );
    }
    else if( n <= m && weightTowardsC*n <= k )
    {
        blas::internal::GemmTNA( orientationOfA, alpha, A, B, beta, C );
    }
    else
    {
        blas::internal::GemmTNC( orientationOfA, alpha, A, B, beta, C );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

// Transpose Normal Gemm that avoids communicating the matrix A.
template<typename T> 
void
elemental::blas::internal::GemmTNA
( Orientation orientationOfA,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& B,
  T beta,        DistMatrix<T,MC,MR>& C )
{
#ifndef RELEASE
    PushCallStack("blas::internal::GemmTNA");    
    if( A.GetGrid() != B.GetGrid() || B.GetGrid() != C.GetGrid() )
        throw logic_error( "{A,B,C} must be distributed over the same grid." );
    if( orientationOfA == Normal )
        throw logic_error( "GemmTNA assumes A is (Conjugate)Transposed." );
    if( A.Width()  != C.Height() ||
        B.Width()  != C.Width()  ||
        A.Height() != B.Height()   )
    {
        ostringstream msg;
        msg << "Nonconformal GemmTNA: " << endl
            << "  A ~ " << A.Height() << " x " << A.Width() << endl
            << "  B ~ " << B.Height() << " x " << B.Width() << endl
            << "  C ~ " << C.Height() << " x " << C.Width() << endl;
        throw logic_error( msg.str() );
    }
#endif
    const Grid& g = A.GetGrid();

    // Matrix views
    DistMatrix<T,MC,MR> BL(g), BR(g),
                        B0(g), B1(g), B2(g);

    DistMatrix<T,MC,MR> CL(g), CR(g),
                        C0(g), C1(g), C2(g);

    // Temporary distributions
    DistMatrix<T,MC,Star> B1_MC_Star(g);
    DistMatrix<T,MR,Star> D1_MR_Star(g);
    DistMatrix<T,MR,MC  > D1_MR_MC(g);
    DistMatrix<T,MC,MR  > D1(g);

    // Start the algorithm
    blas::Scal( beta, C );
    LockedPartitionRight( B, BL, BR, 0 );
    PartitionRight( C, CL, CR, 0 );
    while( BR.Width() > 0 )
    {
        LockedRepartitionRight
        ( BL, /**/     BR,
          B0, /**/ B1, B2 );
 
        RepartitionRight
        ( CL, /**/     CR,
          C0, /**/ C1, C2 );

        B1_MC_Star.AlignWith( A );
        D1_MR_Star.AlignWith( A );
        D1_MR_Star.ResizeTo( C1.Height(), C1.Width() );
        D1.AlignWith( C1 );
        //--------------------------------------------------------------------//
        B1_MC_Star = B1; // B1[MC,*] <- B1[MC,MR]

        // D1[MR,*] := alpha (A1[MC,MR])^T B1[MC,*]
        //           = alpha (A1^T)[MR,MC] B1[MC,*]
        blas::internal::LocalGemm
        ( orientationOfA, Normal, alpha, A, B1_MC_Star, (T)0, D1_MR_Star );

        // C1[MC,MR] += scattered & transposed D1[MR,*] summed over grid cols
        D1_MR_MC.SumScatterFrom( D1_MR_Star );
        D1 = D1_MR_MC; 
        blas::Axpy( (T)1, D1, C1 );
        //--------------------------------------------------------------------//
        B1_MC_Star.FreeAlignments();
        D1_MR_Star.FreeAlignments();
        D1.FreeAlignments();

        SlideLockedPartitionRight
        ( BL,     /**/ BR,
          B0, B1, /**/ B2 );

        SlidePartitionRight
        ( CL,     /**/ CR,
          C0, C1, /**/ C2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

// Transpose Normal Gemm that avoids communicating the matrix B.
template<typename T> 
void
elemental::blas::internal::GemmTNB
( Orientation orientationOfA,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& B,
  T beta,        DistMatrix<T,MC,MR>& C )
{
#ifndef RELEASE
    PushCallStack("blas::internal::GemmTNB");
    if( A.GetGrid() != B.GetGrid() || B.GetGrid() != C.GetGrid() )
        throw logic_error( "{A,B,C} must be distributed over the same grid." );
    if( orientationOfA == Normal )
        throw logic_error( "GemmTNB assumes A is (Conjugate)Transposed." );
    if( A.Width()  != C.Height() ||
        B.Width()  != C.Width()  ||
        A.Height() != B.Height()   )
    {
        ostringstream msg;
        msg << "Nonconformal GemmTNB: " << endl
            << "  A ~ " << A.Height() << " x " << A.Width() << endl
            << "  B ~ " << B.Height() << " x " << B.Width() << endl
            << "  C ~ " << C.Height() << " x " << C.Width() << endl;
        throw logic_error( msg.str() );
    }
#endif
    const Grid& g = A.GetGrid();

    // Matrix views
    DistMatrix<T,MC,MR> AL(g), AR(g),
                        A0(g), A1(g), A2(g);

    DistMatrix<T,MC,MR> CT(g),  C0(g),
                        CB(g),  C1(g),
                                C2(g);

    // Temporary distributions
    DistMatrix<T,MC,Star> A1_MC_Star(g);
    DistMatrix<T,Star,MR> D1_Star_MR(g);

    // Start the algorithm
    blas::Scal( beta, C );
    LockedPartitionRight( A, AL, AR, 0 );
    PartitionDown
    ( C, CT,
         CB, 0 );
    while( AR.Width() > 0 )
    {
        LockedRepartitionRight
        ( AL, /**/     AR,
          A0, /**/ A1, A2 );

        RepartitionDown
        ( CT,  C0,
         /**/ /**/
               C1,
          CB,  C2 );

        A1_MC_Star.AlignWith( B );
        D1_Star_MR.AlignWith( B );
        D1_Star_MR.ResizeTo( C1.Height(), C1.Width() );
        //--------------------------------------------------------------------//
        A1_MC_Star = A1; // A1[MC,*] <- A1[MC,MR]

        // D1[*,MR] := alpha (A1[MC,*])^T B[MC,MR]
        //           = alpha (A1^T)[*,MC] B[MC,MR]
        blas::internal::LocalGemm
        ( orientationOfA, Normal, alpha, A1_MC_Star, B, (T)0, D1_Star_MR );

        // C1[MC,MR] += scattered result of D1[*,MR] summed over grid cols
        C1.SumScatterUpdate( (T)1, D1_Star_MR );
        //--------------------------------------------------------------------//
        A1_MC_Star.FreeAlignments();
        D1_Star_MR.FreeAlignments();

        SlideLockedPartitionRight
        ( AL,     /**/ AR,
          A0, A1, /**/ A2 );

        SlidePartitionDown
        ( CT,  C0,
               C1,
         /**/ /**/
          CB,  C2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

// Transpose Normal Gemm that avoids communicating the matrix C.
template<typename T> 
void
elemental::blas::internal::GemmTNC
( Orientation orientationOfA,
  T alpha, const DistMatrix<T,MC,MR>& A,
           const DistMatrix<T,MC,MR>& B,
  T beta,        DistMatrix<T,MC,MR>& C )
{
#ifndef RELEASE
    PushCallStack("blas::internal::GemmTNC");
    if( A.GetGrid() != B.GetGrid() || B.GetGrid() != C.GetGrid() )
        throw logic_error( "{A,B,C} must be distributed over the same grid." );
    if( orientationOfA == Normal )
        throw logic_error( "GemmTNC assumes A is (Conjugate)Transposed." );
    if( A.Width()  != C.Height() ||
        B.Width()  != C.Width()  ||
        A.Height() != B.Height()   )
    {
        ostringstream msg;
        msg << "Nonconformal GemmTNC: " << endl
            << "  A ~ " << A.Height() << " x " << A.Width() << endl
            << "  B ~ " << B.Height() << " x " << B.Width() << endl
            << "  C ~ " << C.Height() << " x " << C.Width() << endl;
        throw logic_error( msg.str() );
    }
#endif
    const Grid& g = A.GetGrid();

    // Matrix views
    DistMatrix<T,MC,MR> AT(g),  A0(g),
                        AB(g),  A1(g),
                                A2(g);

    DistMatrix<T,MC,MR> BT(g),  B0(g),
                        BB(g),  B1(g),
                                B2(g);

    // Temporary distributions
    DistMatrix<T,Star,MC> A1_Star_MC(g);
    DistMatrix<T,Star,MR> B1_Star_MR(g);

    // Start the algorithm
    blas::Scal( beta, C );
    LockedPartitionDown
    ( A, AT,
         AB, 0 );
    LockedPartitionDown
    ( B, BT,
         BB, 0 );
    while( AB.Height() > 0 )
    {
        LockedRepartitionDown
        ( AT,  A0,
         /**/ /**/
               A1,
          AB,  A2 );

        LockedRepartitionDown
        ( BT,  B0,
         /**/ /**/
               B1,
          BB,  B2 );

        A1_Star_MC.AlignWith( C );
        B1_Star_MR.AlignWith( C );
        //--------------------------------------------------------------------//
        A1_Star_MC = A1; // A1[*,MC] <- A1[MC,MR]
        B1_Star_MR = B1; // B1[*,MR] <- B1[MC,MR]

        // C[MC,MR] += alpha (A1[*,MC])^T B1[*,MR]
        //           = alpha (A1^T)[MC,*] B1[*,MR]
        blas::internal::LocalGemm
        ( orientationOfA, Normal, alpha, A1_Star_MC, B1_Star_MR, (T)1, C );
        //--------------------------------------------------------------------//
        A1_Star_MC.FreeAlignments();
        B1_Star_MR.FreeAlignments();

        SlideLockedPartitionDown
        ( AT,  A0,
               A1,
         /**/ /**/
          AB,  A2 );

        SlideLockedPartitionDown
        ( BT,  B0,
               B1,
         /**/ /**/
          BB,  B2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template void elemental::blas::internal::GemmTN
( Orientation orientationOfA,
  float alpha, const DistMatrix<float,MC,MR>& A,      
               const DistMatrix<float,MC,MR>& B,
  float beta,        DistMatrix<float,MC,MR>& C );

template void elemental::blas::internal::GemmTNA
( Orientation orientationOfA,
  float alpha, const DistMatrix<float,MC,MR>& A,      
               const DistMatrix<float,MC,MR>& B,
  float beta,        DistMatrix<float,MC,MR>& C );

template void elemental::blas::internal::GemmTNB
( Orientation orientationOfA,
  float alpha, const DistMatrix<float,MC,MR>& A,      
               const DistMatrix<float,MC,MR>& B,
  float beta,        DistMatrix<float,MC,MR>& C );

template void elemental::blas::internal::GemmTNC
( Orientation orientationOfA,
  float alpha, const DistMatrix<float,MC,MR>& A,      
               const DistMatrix<float,MC,MR>& B,
  float beta,        DistMatrix<float,MC,MR>& C );

template void elemental::blas::internal::GemmTN
( Orientation orientationOfA,
  double alpha, const DistMatrix<double,MC,MR>& A,         
                const DistMatrix<double,MC,MR>& B,
  double beta,        DistMatrix<double,MC,MR>& C );

template void elemental::blas::internal::GemmTNA
( Orientation orientationOfA,
  double alpha, const DistMatrix<double,MC,MR>& A,         
                const DistMatrix<double,MC,MR>& B,
  double beta,        DistMatrix<double,MC,MR>& C );

template void elemental::blas::internal::GemmTNB
( Orientation orientationOfA,
  double alpha, const DistMatrix<double,MC,MR>& A,         
                const DistMatrix<double,MC,MR>& B,
  double beta,        DistMatrix<double,MC,MR>& C );

template void elemental::blas::internal::GemmTNC
( Orientation orientationOfA,
  double alpha, const DistMatrix<double,MC,MR>& A,         
                const DistMatrix<double,MC,MR>& B,
  double beta,        DistMatrix<double,MC,MR>& C );

#ifndef WITHOUT_COMPLEX
template void elemental::blas::internal::GemmTN
( Orientation orientationOfA,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A, 
                  const DistMatrix<scomplex,MC,MR>& B,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNA
( Orientation orientationOfA,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A, 
                  const DistMatrix<scomplex,MC,MR>& B,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNB
( Orientation orientationOfA,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A, 
                  const DistMatrix<scomplex,MC,MR>& B,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNC
( Orientation orientationOfA,
  scomplex alpha, const DistMatrix<scomplex,MC,MR>& A, 
                  const DistMatrix<scomplex,MC,MR>& B,
  scomplex beta,        DistMatrix<scomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTN
( Orientation orientationOfA,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A, 
                  const DistMatrix<dcomplex,MC,MR>& B,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNA
( Orientation orientationOfA,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A, 
                  const DistMatrix<dcomplex,MC,MR>& B,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNB
( Orientation orientationOfA,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A, 
                  const DistMatrix<dcomplex,MC,MR>& B,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& C );

template void elemental::blas::internal::GemmTNC
( Orientation orientationOfA,
  dcomplex alpha, const DistMatrix<dcomplex,MC,MR>& A, 
                  const DistMatrix<dcomplex,MC,MR>& B,
  dcomplex beta,        DistMatrix<dcomplex,MC,MR>& C );
#endif
