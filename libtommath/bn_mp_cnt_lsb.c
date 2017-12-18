#include <tommath_private.h>
#ifdef BN_MP_CNT_LSB_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tstdenis82@gmail.com, http://libtom.org
 */

static const int lnz[16] = {
   4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/* Counts the number of lsbs which are zero before the first zero bit */
int mp_cnt_lsb(const mp_int *a)
{
   int x;
   mp_digit q, qq;

   /* easy out */
   if (mp_iszero(a) == MP_YES) {
      return 0;
   }

   /* scan lower digits until non-zero */
   for (x = 0; (x < a->used) && (a->dp[x] == 0u); x++) {}
   q = a->dp[x];
   x *= DIGIT_BIT;

   /* now scan this digit until a 1 is found */
   if ((q & 1u) == 0u) {
      do {
         qq  = q & 15u;
         x  += lnz[qq];
         q >>= 4;
      } while (qq == 0u);
   }
   return x;
}

#endif

/* ref:         HEAD -> develop */
/* git commit:  99057f6759576514f8d72e31854d0814a091d050 */
/* commit time: 2017-12-06 21:46:11 +0100 */
