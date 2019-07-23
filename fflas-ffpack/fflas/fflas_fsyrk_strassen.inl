/*
 * Copyright (C) 2019 the FFLAS-FFPACK group
 *
 * Written by Clement Pernet <Clement.Pernet@imag.fr>
 *
 * ========LICENCE========
 * This file is part of the library FFLAS-FFPACK.
 *
 * FFLAS-FFPACK is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 *.
 */

#ifndef __FFLASFFPACK_fflas_fsyrk_strassen_INL
#define __FFLASFFPACK_fflas_fsyrk_strassen_INL

namespace FFLAS {

    // template<class Field>
    // inline void
    // S1S2quadrant (const Field& F,
    //               const size_t N,
    //               const size_t K,
    //               const typename Field::Element x,
    //               typename Field::ConstElement_ptr A, const size_t lda,
    //               typename Field::Element_ptr S, const size_t lds,
    //               typename Field::Element_ptr T, const size_t ldt){

    //     typename Field::ConstElement_ptr A11=A;
    //     typename Field::ConstElement_ptr A21=A+N*lda;
    //     typename Field::ConstElement_ptr A22=A21+K;

    //     size_t N2 = N>>1;
    //     size_t K2 = K>>1;
    //     for (size_t i=0; i<N2; i++, A11+=lda, A21+=lda, A22+=lda, S+=lds, T+=ldt){
    //             // @todo: vectorize this inner loop
    //         for (size_t j=0; j<K2; j++){
    //             typename Field::Element t;
    //             F.init(t);
    //             F.sub(t,A11[j],A21[j]);
    //             F.mul(S[j],t,x);
    //             F.maxpy(T[j], A21[j], x, A22[j]);
    //         }
    //     }
    // }

    // template<class Field>
    // inline void
    // computeS1S2 (const Field& F,
    //              const size_t N,
    //              const size_t K,
    //              const typename Field::Element x,
    //              const typename Field::Element y,
    //              typename Field::ConstElement_ptr A, const size_t lda,
    //              typename Field::Element_ptr S, const size_t lds,
    //              typename Field::Element_ptr T, const size_t ldt){
    //         // S1 = (A11-A21) x Y^T in S
    //         // S2 = A22 - A21 x Y^T in T
    //         // where Y = [ x.I  y.I]
    //         //           [ -y.I x.I]
    //     size_t N2 = N>>1;
    //     typename Field::Element negy;
    //     F.init(negy);
    //     F.neg(negy, y);
    //     S1S2quadrant (F, N, K, x, A, lda, S, lds, T, ldt);
    //     S1S2quadrant (F, N, K, negy, A+N2, lda, S+N2, lds, T+N2, ldt);
    //     S1S2quadrant (F, N, K, y, A+N2*lda, lda, S+N2*lds, lds, T+N2*ldt, ldt);
    //     S1S2quadrant (F, N, K, x, A+N2*lda+N2, lda, S+N2*lds+N2, lds, T+N2*ldt+N2, ldt);
    // }
    //     ///////////
    
    template<class Field>
    inline void
    computeS1S2 (const Field& F,
                 const size_t N,
                 const size_t K,
                 const typename Field::Element x,
                 const typename Field::Element y,
                 typename Field::ConstElement_ptr A, const size_t lda,
                 typename Field::Element_ptr S, const size_t lds,
                 typename Field::Element_ptr T, const size_t ldt){
            // S1 = (A11-A21) x Y^T in S
            // S2 = A22 - A21 x Y^T in T
            // where Y = [ x.I  y.I]
            //           [ -y.I x.I]
        size_t N2 = N>>1;
        size_t K2 = K>>1;
        size_t K4 = K2>>1;

        typename Field::ConstElement_ptr A11 = A;
        typename Field::ConstElement_ptr A21 = A + N2*lda;
        typename Field::ConstElement_ptr A22 = A21 + K2;
        typename Field::ConstElement_ptr A11r = A11 + K4;
        typename Field::ConstElement_ptr A21r = A21 + K4;
        typename Field::ConstElement_ptr A22r = A22 + K4;
        typename Field::Element_ptr Sr = S + K4;
        typename Field::Element_ptr Tr = T + K4;
        typename Field::Element negy;
        F.init(negy);
        F.neg(negy, y);

        for (size_t i=0; i<N2; i++, A11+=lda, A21+=lda, A22+=lda, A11r+=lda, A21r+=lda, A22r+=lda, S+=lds, Sr+=lds, T+=ldt, Tr+=ldt){
                // @todo: vectorize this inner loop
            for (size_t j=0; j<K4; j++){
                typename Field::Element t, tr;
                F.init(t);
                F.init(tr);
                F.sub(t,A11[j],A21[j]);
                F.sub(tr,A11r[j],A21r[j]);
                F.mul(S[j],t,x);
                F.mul(Sr[j],t,negy);
                F.axpyin(S[j],tr,y);
                F.axpyin(Sr[j],tr,x);

                F.maxpy(T[j], A21[j], x, A22[j]);
                F.maxpy(Tr[j], A21[j], negy, A22r[j]);

                F.maxpyin(T[j], A21r[j], y);
                F.maxpyin(Tr[j], A21r[j], x);
            }
        }

    }
    
    template<class Field>
    inline typename Field::Element_ptr
    fsyrk_strassen (const Field& F,
                    const FFLAS_UPLO uplo,
                    const FFLAS_TRANSPOSE trans,
                    const size_t N,
                    const size_t K,
                    const typename Field::Element y1,
                    const typename Field::Element y2,
                    const typename Field::Element alpha,
                    typename Field::ConstElement_ptr A, const size_t lda,
                    const typename Field::Element beta,
                    typename Field::Element_ptr C, const size_t ldc,
                    size_t reclevel /*todo: use helper instead*/
                    ) {
        
            // written for NoTrans, Lower
        if (reclevel == 0){
            WriteMatrix(std::cerr<<"Base Case:"<<std::endl<<" A = "<<std::endl, F, N, K, A, lda);

            fsyrk (F, uplo, trans, N, K, alpha, A, lda, beta, C, ldc);
            WriteMatrix(std::cerr<<" C <- "<<std::endl, F, N, N, C, ldc);
            return C;

        }
        size_t N2 = N>>1;
        size_t K2 = K>>1;
        typename Field::ConstElement_ptr A11 = A;
        typename Field::ConstElement_ptr A12 = A + K2;
        typename Field::ConstElement_ptr A21 = A + N2*lda;
        typename Field::ConstElement_ptr A22 = A21 + N2;
        typename Field::Element_ptr C11 = C;
        typename Field::Element_ptr C12 = C + N2;
        typename Field::Element_ptr C21 = C + N2*ldc;
        typename Field::Element_ptr C22 = C21 + N2;

        FFLAS_TRANSPOSE OppTrans = (trans == FflasNoTrans)? FflasTrans : FflasNoTrans;

        typename Field::Element negalpha;
        F.init(negalpha);
        F.neg(negalpha,alpha);

        if (F.isZero(beta)){ // no accumulation, schedule without extra temp

                // S1 = (A11-A21) x Y^T in C21
                // S2 = A22 - A21 x Y^T in C12
            computeS1S2 (F, N2, K2, y1, y2, A,lda, C21, ldc, C12, ldc);

            WriteMatrix(std::cerr<<"S1 = "<<std::endl, F, N2, K2, C21, ldc);
            WriteMatrix(std::cerr<<"S2 = "<<std::endl, F, N2, K2, C12, ldc);

                // - P4^T = - S2 x S1^T in  C22
            fgemm (F, trans, OppTrans, N2, N2, K2, alpha, C12, ldc, C21, ldc, F.zero, C22, ldc);

                // S3 = S1 + A22 in C21
            faddin (F, N2, K2, A22, lda, C21, ldc);

                // P5 = S3 x S3^T in C12
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, C21, ldc, F.zero, C12, ldc, reclevel-1);

                // S4 = S3 - A12 in C11
            fsub (F, N2, K2, C21, ldc, A12, lda, C11, ldc);

                // - P3 = - A22 x S4^T in C21
            fgemm (F, trans, OppTrans, N2, N2, K2, negalpha, A22, lda, C11, ldc, F.zero, C21, ldc);

                // P1 = A11 x A11^T in C11
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, A11, lda, F.zero, C11, ldc, reclevel-1);

                // U1 = P1 + P5 in C12
            faddin (F, uplo, N2, C11, ldc, C12, ldc); // TODO triangular addin (to be implemented)

                // make U1 explicit: Up(U1)=Low(U1)^T
            for (size_t i=0; i<N2; i++)
                fassign(F, i, C12+i*ldc, 1, C12+i, ldc);

                // U2 = U1 - P4 in C12
            fsubin (F,  N2, N2, C22, ldc, C12, ldc);

                // U4 = U2 - P3 in C21
            faddin (F, N2, N2, C12, ldc, C21, ldc);

                // U5 = U2 - P4^T in C22
            faddin (F, uplo, N2, C12, ldc, C22, ldc);

                // P2 = A12 x A12^T in C12
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, A12, lda, F.zero , C12, ldc, reclevel-1);

                // U3 = P1 + P2 in C11
            faddin (F, uplo, N2, C12, ldc, C11, ldc);

        } else { // with accumulation, schedule with 1 temp

            typename Field::Element negbeta;
            F.init (negbeta);
            F.neg (negbeta, beta);
            typename Field::Element_ptr T = fflas_new (F, N2, std::max(N2,K2));
            size_t ldt = std::max(N2,K2);

                // S2 = A22 - A21 x Y^T in C12
                // S1 = (A11-A21) x Y^T in T1
            computeS1S2 (F, N2, K2, y1, y2, A, lda, C12, ldc, T, ldt);

                // Up(C11) = Low(C22) (saving C22)
            for (size_t i=0; i<N2-1; ++i)
                fassign (F, N2-i-1, C22 + (N2-i-1)*ldc, 1, C11 + 1 + i*(ldc+1), 1);

                // - P4^T = - S2 x S1^T in C22
            fgemm (F, trans, OppTrans, N2, N2, K2, negalpha, C12, ldc, T, ldt, F.zero, C22, ldc);


                // S3 = S1 + A22 in T1
            faddin (F, N2, K2, A22, lda, T, ldt);

                // P5 = S3 x S3^T in C12
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, T, ldt, F.zero, C12, ldc, reclevel-1);

                // S4 = S3 - A12 in T1
            fsubin (F, N2, K2, A12, lda, T, ldt);

                // - P3 = - A22 x S4^T -  beta C21 in C21
            fgemm (F, trans, OppTrans, N2, N2, K2, negalpha, A22, lda, T, ldt, negbeta, C21, ldc);

                // P1 = A11 x A11^T in T1
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, A11, lda, F.zero, T, ldt, reclevel-1);

                // P2 = A12 x A12^T + beta C11 in C11
            fsyrk_strassen (F, uplo, trans, N2, K2, y1, y2, alpha, A12, lda, beta, C11, ldc, reclevel-1);

                // U3 = P1 + P2 in C11
            faddin (F, uplo, N2, T, ldt, C11, ldc);

            fflas_delete (T);

                // U1 = P5 + P1  in C12 // Still symmetric
            faddin (F, uplo, N2, T, ldt, C12, ldc);

                // Make U1 explicit (copy the N/2 missing part)
            for (size_t i=0; i<N2; ++i)
                fassign (F, i, C12 + i*ldc, 1, C12 + i, ldc);

                // U2 = U1 - P4 in C12
            for (size_t i=0; i<N2; i++)
                faddin (F, N2, C22 + i*ldc, 1, C12 + i, ldc);

                // U4 = U2 - P3 in C21
            faddin (F, N2, N2, C12, ldc, C21, ldc);

                // U5 = U2 - P4^T + beta Up(C11)^T in C22 (only the lower triang part)
            faddin (F, uplo, N2, C12, ldc, C22, ldc);
            for (size_t i=1; i<N2; i++){ // TODO factorize out in a triple add
                faxpy (F, i, beta, C11 + (N-i)*(ldc+1), 1, C22+i*ldc, 1);
            }
        }
        return C;
    }
//============================
            // version with accumulation and 3 temps
            // S1 = A11 - A21 in C12

            // A22' = A22 Y in T1

            // S2 = A21 + A22' in T2

            // P4 = S1 x S2^T in  T3

            // P1 = A11 x A11^T in C12

            // C11 = P1 + beta.C11 in C11
        
            // U3 = P2 + P1 = A12 x A12^T + P1 in C11

            // S3 = A11 - S2 in T2
        
            // U1 = P1 - P5 = -S3 x S3^T + P1 in C12

            // U2 = U1 - P4 in C12

            // U5 = U2 - P4^T + C22 in C22 (add)

            // S4 = S3  + A12 x Y in T2
            // A12xY on the fly

            // - P3 = - A22 Y x S4^T + C21 in C21

            // U4 = U2 - P3  in C21

////////////////////////////
            // version without accumulation but requires 1 temp


        
            // S1 = A11 - A21 in C12

            // A22' = A22 Y in C11

            // S2 = A21 + A22' in C21

            // P4 = S1 x S2^T in  C22

            // S3 = A11 - S2 in C21

            // -P5 = -S3 x S3^T  in T

            // S4 = S3 x Y + A12 in C12

            // -P3 = -A22' x S4^T in C21

            // P1 = A11 x A11^T in C11

            // U1 = P1 - P5 in T

            // U2 = U1 - P4 in T

            // U4 = U2 - P3 in C21

            // U5 = U2 - P4^T in C22
        
            // P2 = A12 x A12^T in C12

            // U3 = P1 + P2 in C11

//        ======================
            // algo du papier sans acc mais pas en place
            // S1 = A11 - A21

            // S2 = A21 + A22 x Y^T

            // S3 = A11 - S2

            // S4 = S3 x Y + A12

            // P1 = A11 x A11^T in C11

            // P2 = A12 x A12^T

            // P3 = A22 Y x S4^T

            // P4 = S1 x S2^T

            // P5 = S3 x S3^T

            // U1 = P1 - P5

            // U2 = U1 - P4

            // U3 = P1 + P2 in C11

            // U4 = U2 - P3 in C21

            // U5 = U2 - P4^T in C22
        
}


#endif // __FFLASFFPACK_fflas_fsyrk_strassen_INL
/* -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
