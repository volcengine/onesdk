/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

/* For more information on how the RH hash works and in particular how we do
 * deletions, see:
 * http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/
 */

#include "hash_table.h"
#include "math.h"
#include "private/hash_table_impl.h"
#include "string.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Include lookup3.c so we can (potentially) inline it and make use of the mix()
 * macro. */


/*
 * The following public domain code has been modified as follows:
 * # All functions have been made static.
 * # The self test harness has been turned off.
 * # stdint.h include removed for C89 compatibility.
 *
 * The original code was retrieved from http://burtleburtle.net/bob/c/lookup3.c
 */

/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.

These are functions for producing 32-bit hashes for hash table lookup.
hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
are externally useful functions.  Routines to test the hash are included
if SELF_TEST is defined.  You can use this free for any purpose.  It's in
the public domain.  It has no warranty.

You probably want to use hashlittle().  hashlittle() and hashbig()
hash byte arrays.  hashlittle() is is faster than hashbig() on
little-endian machines.  Intel and AMD are little-endian machines.
On second thought, you probably want hashlittle2(), which is identical to
hashlittle() except it returns two 32-bit hashes for the price of one.
You could implement hashbig2() if you wanted but I haven't bothered here.

If you want to find a hash of, say, exactly 7 integers, do
  a = i1;  b = i2;  c = i3;
  mix(a,b,c);
  a += i4; b += i5; c += i6;
  mix(a,b,c);
  a += i7;
  final(a,b,c);
then use c as the hash value.  If you have a variable length array of
4-byte integers to hash, use hashword().  If you have a byte array (like
a character string), use hashlittle().  If you have several byte arrays, or
a mix of things, see the comments above hashlittle().

Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
then mix those integers.  This is fast (you can do a lot more thorough
mixing with 12*3 instructions on 3 integers than you can with 3 instructions
on 1 byte), but shoehorning those bytes into integers efficiently is messy.
-------------------------------------------------------------------------------
*/
// #define SELF_TEST 1

#include <stdio.h>      /* defines printf for tests */
#include <time.h>       /* defines time_t for timings in the test */
#ifndef _MSC_VER
#include <sys/param.h>  /* attempt to define endianness */
#endif
#ifdef linux
# include <endian.h>    /* attempt to define endianness */
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) /*Disable "conditional expression is constant" */
#endif /* _MSC_VER */

#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif /* CBMC */

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
 #if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
      __BYTE_ORDER == __LITTLE_ENDIAN) || \
     (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
      __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
     (defined(i386) || defined(__i386__) || defined(__i486__) || \
      defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL) || \
      defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(_M_PPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
--------------------------------------------------------------------
 This works on all machines.  To be useful, it requires
 -- that the key be an array of uint32_t's, and
 -- that the length be the number of uint32_t's in the key

 The function hashword() is identical to hashlittle() on little-endian
 machines, and identical to hashbig() on big-endian machines,
 except that the length has to be measured in uint32_ts rather than in
 bytes.  hashlittle() is more complicated than hashword() only because
 hashlittle() has to dance around fitting the key bytes into registers.
--------------------------------------------------------------------
*/
static uint32_t hashword(
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t        initval)         /* the previous hash, or an arbitrary value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  {
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}


/*
--------------------------------------------------------------------
hashword2() -- same as hashword(), but take two seeds and return two
32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
both be initialized with seeds.  If you pass in (*pb)==0, the output
(*pc) will be the same as the return value from hashword().
--------------------------------------------------------------------
*/
static void hashword2 (
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t       *pc,                      /* IN: seed OUT: primary hash value */
uint32_t       *pb)               /* IN: more seed OUT: secondary hash value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)(length<<2)) + *pc;
  c += *pb;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  {
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  *pc=c; *pb=b;
}


/*
-------------------------------------------------------------------------------
hashlittle() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  length  : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

static uint32_t hashlittle( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string. Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this. But VALGRIND and CBMC
     * will still catch it and complain. CBMC will ignore this type of error
     * in the code block between the pragmas "CPROVER check push" and
     * "CPROVER check pop". The masking trick does make the hash noticably
     * faster for short strings (like English words).
     */
#ifndef VALGRIND
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "pointer"
#endif
    // changed in aws-c-common: fix unused variable warning

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }
#ifdef CBMC
#    pragma CPROVER check pop
#endif
#else /* make valgrind happy */

    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : return c;
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}


/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
/* AddressSanitizer hates this implementation, even though it's innocuous */
AWS_SUPPRESS_ASAN static void hashlittle2(
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */

{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string. Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this. But VALGRIND and CBMC
     * will still catch it and complain. CBMC will ignore this type of error
     * in the code block between the pragmas "CPROVER check push" and
     * "CPROVER check pop". The masking trick does make the hash noticeably
     * faster for short strings (like English words).
     */
#ifndef VALGRIND
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "pointer"
#endif
    // changed in aws-c-common: fix unused variable warning

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#ifdef CBMC
#    pragma CPROVER check pop
#endif
#else /* make valgrind happy */

    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}



/*
 * hashbig():
 * This is the same as hashword() on big-endian machines.  It is different
 * from hashlittle() on all machines.  hashbig() takes advantage of
 * big-endian byte ordering.
 */
static uint32_t hashbig( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;
  union { const void *ptr; size_t i; } u; /* to cast key to (size_t) happily */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_BIG_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /*
     * "k[2]<<8" actually reads beyond the end of the string, but
     * then shifts out the part it's not allowed to read.  Because the
     * string is aligned, the illegal read is in the same word as the
     * rest of the string. Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this. But VALGRIND and CBMC
     * will still catch it and complain. CBMC will ignore this type of error
     * in the code block between the pragmas "CPROVER check push" and
     * "CPROVER check pop". The masking trick does make the hash noticably
     * faster for short strings (like English words).
     */
#ifndef VALGRIND
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "pointer"
#endif
    // changed in aws-c-common: fix unused variable warning

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff00; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff0000; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff000000; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff00; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff0000; a+=k[0]; break;
    case 5 : b+=k[1]&0xff000000; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff00; break;
    case 2 : a+=k[0]&0xffff0000; break;
    case 1 : a+=k[0]&0xff000000; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }
#ifdef CBMC
#    pragma CPROVER check pop
#endif
#else  /* make valgrind happy */

    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<8;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<16;  /* fall through */
    case 9 : c+=((uint32_t)k8[8])<<24;  /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<8;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<16;  /* fall through */
    case 5 : b+=((uint32_t)k8[4])<<24;  /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<8;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<16;  /* fall through */
    case 1 : a+=((uint32_t)k8[0])<<24; break;
    case 0 : return c;
    }

#endif /* !VALGRIND */

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((uint32_t)k[0])<<24;
      a += ((uint32_t)k[1])<<16;
      a += ((uint32_t)k[2])<<8;
      a += ((uint32_t)k[3]);
      b += ((uint32_t)k[4])<<24;
      b += ((uint32_t)k[5])<<16;
      b += ((uint32_t)k[6])<<8;
      b += ((uint32_t)k[7]);
      c += ((uint32_t)k[8])<<24;
      c += ((uint32_t)k[9])<<16;
      c += ((uint32_t)k[10])<<8;
      c += ((uint32_t)k[11]);
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[11];
    case 11: c+=((uint32_t)k[10])<<8;
    case 10: c+=((uint32_t)k[9])<<16;
    case 9 : c+=((uint32_t)k[8])<<24;
    case 8 : b+=k[7];
    case 7 : b+=((uint32_t)k[6])<<8;
    case 6 : b+=((uint32_t)k[5])<<16;
    case 5 : b+=((uint32_t)k[4])<<24;
    case 4 : a+=k[3];
    case 3 : a+=((uint32_t)k[2])<<8;
    case 2 : a+=((uint32_t)k[1])<<16;
    case 1 : a+=((uint32_t)k[0])<<24;
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}


#ifdef SELF_TEST

/* used for timings */
void driver1()
{
  uint8_t buf[256];
  uint32_t i;
  uint32_t h=0;
  time_t a,z;

  time(&a);
  for (i=0; i<256; ++i) buf[i] = 'x';
  for (i=0; i<1; ++i)
  {
    h = hashlittle(&buf[0],1,h);
  }
  time(&z);
  if (z-a > 0) printf("time %d %.8x\n", z-a, h);
}

/* check that every input bit changes every output bit half the time */
#define HASHSTATE 1
#define HASHLEN   1
#define MAXPAIR 60
#define MAXLEN  70
void driver2()
{
  uint8_t qa[MAXLEN+1], qb[MAXLEN+2], *a = &qa[0], *b = &qb[1];
  uint32_t c[HASHSTATE], d[HASHSTATE], i=0, j=0, k, l, m=0, z;
  uint32_t e[HASHSTATE],f[HASHSTATE],g[HASHSTATE],h[HASHSTATE];
  uint32_t x[HASHSTATE],y[HASHSTATE];
  uint32_t hlen;

  printf("No more than %d trials should ever be needed \n",MAXPAIR/2);
  for (hlen=0; hlen < MAXLEN; ++hlen)
  {
    z=0;
    for (i=0; i<hlen; ++i)  /*----------------------- for each input byte, */
    {
      for (j=0; j<8; ++j)   /*------------------------ for each input bit, */
      {
	for (m=1; m<8; ++m) /*------------ for serveral possible initvals, */
	{
	  for (l=0; l<HASHSTATE; ++l)
	    e[l]=f[l]=g[l]=h[l]=x[l]=y[l]=~((uint32_t)0);

      	  /*---- check that every output bit is affected by that input bit */
	  for (k=0; k<MAXPAIR; k+=2)
	  {
	    uint32_t finished=1;
	    /* keys have one bit different */
	    for (l=0; l<hlen+1; ++l) {a[l] = b[l] = (uint8_t)0;}
	    /* have a and b be two keys differing in only one bit */
	    a[i] ^= (k<<j);
	    a[i] ^= (k>>(8-j));
	     c[0] = hashlittle(a, hlen, m);
	    b[i] ^= ((k+1)<<j);
	    b[i] ^= ((k+1)>>(8-j));
	     d[0] = hashlittle(b, hlen, m);
	    /* check every bit is 1, 0, set, and not set at least once */
	    for (l=0; l<HASHSTATE; ++l)
	    {
	      e[l] &= (c[l]^d[l]);
	      f[l] &= ~(c[l]^d[l]);
	      g[l] &= c[l];
	      h[l] &= ~c[l];
	      x[l] &= d[l];
	      y[l] &= ~d[l];
	      if (e[l]|f[l]|g[l]|h[l]|x[l]|y[l]) finished=0;
	    }
	    if (finished) break;
	  }
	  if (k>z) z=k;
	  if (k==MAXPAIR)
	  {
	     printf("Some bit didn't change: ");
	     printf("%.8x %.8x %.8x %.8x %.8x %.8x  ",
	            e[0],f[0],g[0],h[0],x[0],y[0]);
	     printf("i %d j %d m %d len %d\n", i, j, m, hlen);
	  }
	  if (z==MAXPAIR) goto done;
	}
      }
    }
   done:
    if (z < MAXPAIR)
    {
      printf("Mix success  %2d bytes  %2d initvals  ",i,m);
      printf("required  %d  trials\n", z/2);
    }
  }
  printf("\n");
}

/* Check for reading beyond the end of the buffer and alignment problems */
void driver3()
{
  uint8_t buf[MAXLEN+20], *b;
  uint32_t len;
  uint8_t q[] = "This is the time for all good men to come to the aid of their country...";
  uint32_t h;
  uint8_t qq[] = "xThis is the time for all good men to come to the aid of their country...";
  uint32_t i;
  uint8_t qqq[] = "xxThis is the time for all good men to come to the aid of their country...";
  uint32_t j;
  uint8_t qqqq[] = "xxxThis is the time for all good men to come to the aid of their country...";
  uint32_t ref,x,y;
  uint8_t *p;

  printf("Endianness.  These lines should all be the same (for values filled in):\n");
  printf("%.8x                            %.8x                            %.8x\n",
         hashword((const uint32_t *)q, (sizeof(q)-1)/4, 13),
         hashword((const uint32_t *)q, (sizeof(q)-5)/4, 13),
         hashword((const uint32_t *)q, (sizeof(q)-9)/4, 13));
  p = q;
  printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
         hashlittle(p, sizeof(q)-1, 13), hashlittle(p, sizeof(q)-2, 13),
         hashlittle(p, sizeof(q)-3, 13), hashlittle(p, sizeof(q)-4, 13),
         hashlittle(p, sizeof(q)-5, 13), hashlittle(p, sizeof(q)-6, 13),
         hashlittle(p, sizeof(q)-7, 13), hashlittle(p, sizeof(q)-8, 13),
         hashlittle(p, sizeof(q)-9, 13), hashlittle(p, sizeof(q)-10, 13),
         hashlittle(p, sizeof(q)-11, 13), hashlittle(p, sizeof(q)-12, 13));
  p = &qq[1];
  printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
         hashlittle(p, sizeof(q)-1, 13), hashlittle(p, sizeof(q)-2, 13),
         hashlittle(p, sizeof(q)-3, 13), hashlittle(p, sizeof(q)-4, 13),
         hashlittle(p, sizeof(q)-5, 13), hashlittle(p, sizeof(q)-6, 13),
         hashlittle(p, sizeof(q)-7, 13), hashlittle(p, sizeof(q)-8, 13),
         hashlittle(p, sizeof(q)-9, 13), hashlittle(p, sizeof(q)-10, 13),
         hashlittle(p, sizeof(q)-11, 13), hashlittle(p, sizeof(q)-12, 13));
  p = &qqq[2];
  printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
         hashlittle(p, sizeof(q)-1, 13), hashlittle(p, sizeof(q)-2, 13),
         hashlittle(p, sizeof(q)-3, 13), hashlittle(p, sizeof(q)-4, 13),
         hashlittle(p, sizeof(q)-5, 13), hashlittle(p, sizeof(q)-6, 13),
         hashlittle(p, sizeof(q)-7, 13), hashlittle(p, sizeof(q)-8, 13),
         hashlittle(p, sizeof(q)-9, 13), hashlittle(p, sizeof(q)-10, 13),
         hashlittle(p, sizeof(q)-11, 13), hashlittle(p, sizeof(q)-12, 13));
  p = &qqqq[3];
  printf("%.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n",
         hashlittle(p, sizeof(q)-1, 13), hashlittle(p, sizeof(q)-2, 13),
         hashlittle(p, sizeof(q)-3, 13), hashlittle(p, sizeof(q)-4, 13),
         hashlittle(p, sizeof(q)-5, 13), hashlittle(p, sizeof(q)-6, 13),
         hashlittle(p, sizeof(q)-7, 13), hashlittle(p, sizeof(q)-8, 13),
         hashlittle(p, sizeof(q)-9, 13), hashlittle(p, sizeof(q)-10, 13),
         hashlittle(p, sizeof(q)-11, 13), hashlittle(p, sizeof(q)-12, 13));
  printf("\n");

  /* check that hashlittle2 and hashlittle produce the same results */
  i=47; j=0;
  hashlittle2(q, sizeof(q), &i, &j);
  if (hashlittle(q, sizeof(q), 47) != i)
    printf("hashlittle2 and hashlittle mismatch\n");

  /* check that hashword2 and hashword produce the same results */
  len = 0xdeadbeef;
  i=47, j=0;
  hashword2(&len, 1, &i, &j);
  if (hashword(&len, 1, 47) != i)
    printf("hashword2 and hashword mismatch %x %x\n",
	   i, hashword(&len, 1, 47));

  /* check hashlittle doesn't read before or after the ends of the string */
  for (h=0, b=buf+1; h<8; ++h, ++b)
  {
    for (i=0; i<MAXLEN; ++i)
    {
      len = i;
      for (j=0; j<i; ++j) *(b+j)=0;

      /* these should all be equal */
      ref = hashlittle(b, len, (uint32_t)1);
      *(b+i)=(uint8_t)~0;
      *(b-1)=(uint8_t)~0;
      x = hashlittle(b, len, (uint32_t)1);
      y = hashlittle(b, len, (uint32_t)1);
      if ((ref != x) || (ref != y))
      {
	printf("alignment error: %.8x %.8x %.8x %d %d\n",ref,x,y,
               h, i);
      }
    }
  }
}

/* check for problems with nulls */
 void driver4()
{
  uint8_t buf[1];
  uint32_t h,i,state[HASHSTATE];


  buf[0] = ~0;
  for (i=0; i<HASHSTATE; ++i) state[i] = 1;
  printf("These should all be different\n");
  for (i=0, h=0; i<8; ++i)
  {
    h = hashlittle(buf, 0, h);
    printf("%2ld  0-byte strings, hash is  %.8x\n", i, h);
  }
}

void driver5()
{
  uint32_t b,c;
  b=0, c=0, hashlittle2("", 0, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* deadbeef deadbeef */
  b=0xdeadbeef, c=0, hashlittle2("", 0, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* bd5b7dde deadbeef */
  b=0xdeadbeef, c=0xdeadbeef, hashlittle2("", 0, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* 9c093ccd bd5b7dde */
  b=0, c=0, hashlittle2("Four score and seven years ago", 30, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* 17770551 ce7226e6 */
  b=1, c=0, hashlittle2("Four score and seven years ago", 30, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* e3607cae bd371de4 */
  b=0, c=1, hashlittle2("Four score and seven years ago", 30, &c, &b);
  printf("hash is %.8lx %.8lx\n", c, b);   /* cd628161 6cbea4b3 */
  c = hashlittle("Four score and seven years ago", 30, 0);
  printf("hash is %.8lx\n", c);   /* 17770551 */
  c = hashlittle("Four score and seven years ago", 30, 1);
  printf("hash is %.8lx\n", c);   /* cd628161 */
}


int main()
{
  driver1();   /* test that the key is hashed: used for timings */
  driver2();   /* test that whole key is hashed thoroughly */
  driver3();   /* test that nothing but the key is hashed */
  driver4();   /* test hashing multiple buffers (all buffers are null) */
  driver5();   /* test the hash against known vectors */
  return 1;
}

#endif  /* SELF_TEST */




static void s_suppress_unused_lookup3_func_warnings(void) {
    /* We avoid making changes to lookup3 if we can avoid it, but since it has functions
     * we're not using, reference them somewhere to suppress the unused function warning.
     */
    (void)hashword;
    (void)hashword2;
    (void)hashlittle;
    (void)hashbig;
}

/**
 * Calculate the hash for the given key.
 * Ensures a reasonable semantics for null keys.
 * Ensures that no object ever hashes to 0, which is the sentinal value for an empty hash element.
 */
static uint64_t s_hash_for(struct hash_table_state *state, const void *key) {
    AWS_PRECONDITION(hash_table_state_is_valid(state));
    s_suppress_unused_lookup3_func_warnings();

    if (key == NULL) {
        /* The best answer */
        return 42;
    }

    uint64_t hash_code = state->hash_fn(key);
    if (!hash_code) {
        hash_code = 1;
    }
    AWS_RETURN_WITH_POSTCONDITION(hash_code, hash_code != 0);
}

/**
 * Check equality of two objects, with a reasonable semantics for null.
 */
static bool s_safe_eq_check(aws_hash_callback_eq_fn *equals_fn, const void *a, const void *b) {
    /* Short circuit if the pointers are the same */
    if (a == b) {
        return true;
    }
    /* If one but not both are null, the objects are not equal */
    if (a == NULL || b == NULL) {
        return false;
    }
    /* If both are non-null, call the underlying equals fn */
    return equals_fn(a, b);
}

/**
 * Check equality of two hash keys, with a reasonable semantics for null keys.
 */
static bool s_hash_keys_eq(struct hash_table_state *state, const void *a, const void *b) {
    AWS_PRECONDITION(hash_table_state_is_valid(state));
    bool rval = s_safe_eq_check(state->equals_fn, a, b);
    AWS_RETURN_WITH_POSTCONDITION(rval, hash_table_state_is_valid(state));
}

static size_t s_index_for(struct hash_table_state *map, struct hash_table_entry *entry) {
    AWS_PRECONDITION(hash_table_state_is_valid(map));
    size_t index = entry - map->slots;
    AWS_RETURN_WITH_POSTCONDITION(index, index < map->size && hash_table_state_is_valid(map));
}

#if 0
/* Useful debugging code for anyone working on this in the future */
static uint64_t s_distance(struct hash_table_state *state, int index) {
    return (index - state->slots[index].hash_code) & state->mask;
}

void hash_dump(struct aws_hash_table *tbl) {
    struct hash_table_state *state = tbl->p_impl;

    printf("Dumping hash table contents:\n");

    for (int i = 0; i < state->size; i++) {
        printf("%7d: ", i);
        struct hash_table_entry *e = &state->slots[i];
        if (!e->hash_code) {
            printf("EMPTY\n");
        } else {
            printf("k: %p v: %p hash_code: %lld displacement: %lld\n",
                e->element.key, e->element.value, e->hash_code,
                (i - e->hash_code) & state->mask);
        }
    }
}
#endif

#if 0
/* Not currently exposed as an API. Should we have something like this? Useful for benchmarks */
AWS_COMMON_API
void aws_hash_table_print_stats(struct aws_hash_table *table) {
    struct hash_table_state *state = table->p_impl;
    uint64_t total_disp = 0;
    uint64_t max_disp = 0;

    printf("\n=== Hash table statistics ===\n");
    printf("Table size: %zu/%zu (max load %zu, remaining %zu)\n", state->entry_count, state->size, state->max_load, state->max_load - state->entry_count);
    printf("Load factor: %02.2lf%% (max %02.2lf%%)\n",
        100.0 * ((double)state->entry_count / (double)state->size),
        state->max_load_factor);

    for (size_t i = 0; i < state->size; i++) {
        if (state->slots[i].hash_code) {
            int displacement = distance(state, i);
            total_disp += displacement;
            if (displacement > max_disp) {
                max_disp = displacement;
            }
        }
    }

    size_t *disp_counts = calloc(sizeof(*disp_counts), max_disp + 1);

    for (size_t i = 0; i < state->size; i++) {
        if (state->slots[i].hash_code) {
            disp_counts[distance(state, i)]++;
        }
    }

    uint64_t median = 0;
    uint64_t passed = 0;
    for (uint64_t i = 0; i <= max_disp && passed < total_disp / 2; i++) {
        median = i;
        passed += disp_counts[i];
    }

    printf("Displacement statistics: Avg %02.2lf max %llu median %llu\n", (double)total_disp / (double)state->entry_count, max_disp, median);
    for (uint64_t i = 0; i <= max_disp; i++) {
        printf("Displacement %2lld: %zu entries\n", i, disp_counts[i]);
    }
    free(disp_counts);
    printf("\n");
}
#endif

size_t aws_hash_table_get_entry_count(const struct aws_hash_table *map) {
    struct hash_table_state *state = map->p_impl;
    return state->entry_count;
}

/* Given a header template, allocates space for a hash table of the appropriate
 * size, and copies the state header into this allocated memory, which is
 * returned.
 */
static struct hash_table_state *s_alloc_state(const struct hash_table_state *template) {
    size_t required_bytes;
    if (hash_table_state_required_bytes(template->size, &required_bytes)) {
        return NULL;
    }

    /* An empty slot has hashcode 0. So this marks all slots as empty */
    struct hash_table_state *state = aws_mem_calloc(template->alloc, 1, required_bytes);

    if (state == NULL) {
        return state;
    }

    *state = *template;
    return state;
}

/* Computes the correct size and max_load based on a requested size. */
static int s_update_template_size(struct hash_table_state *template, size_t expected_elements) {
    size_t min_size = expected_elements;

    if (min_size < 2) {
        min_size = 2;
    }

    /* size is always a power of 2 */
    size_t size;
    if (aws_round_up_to_power_of_two(min_size, &size)) {
        return AWS_OP_ERR;
    }

    /* Update the template once we've calculated everything successfully */
    template->size = size;
    template->max_load = (size_t)(template->max_load_factor * (double)template->size);
    /* Ensure that there is always at least one empty slot in the hash table */
    if (template->max_load >= size) {
        template->max_load = size - 1;
    }

    /* Since size is a power of 2: (index & (size - 1)) == (index % size) */
    template->mask = size - 1;

    return AWS_OP_SUCCESS;
}

int aws_hash_table_init(
    struct aws_hash_table *map,
    struct aws_allocator *alloc,
    size_t size,
    aws_hash_fn *hash_fn,
    aws_hash_callback_eq_fn *equals_fn,
    aws_hash_callback_destroy_fn *destroy_key_fn,
    aws_hash_callback_destroy_fn *destroy_value_fn) {
    AWS_PRECONDITION(map != NULL);
    AWS_PRECONDITION(alloc != NULL);
    AWS_PRECONDITION(hash_fn != NULL);
    AWS_PRECONDITION(equals_fn != NULL);
    struct hash_table_state template;
    template.hash_fn = hash_fn;
    template.equals_fn = equals_fn;
    template.destroy_key_fn = destroy_key_fn;
    template.destroy_value_fn = destroy_value_fn;
    template.alloc = alloc;

    template.entry_count = 0;
    template.max_load_factor = 0.95; /* TODO - make configurable? */

    if (s_update_template_size(&template, size)) {
        return AWS_OP_ERR;
    }
    map->p_impl = s_alloc_state(&template);

    if (!map->p_impl) {
        return AWS_OP_ERR;
    }

    AWS_SUCCEED_WITH_POSTCONDITION(aws_hash_table_is_valid(map));
}

void aws_hash_table_clean_up(struct aws_hash_table *map) {
    AWS_PRECONDITION(map != NULL);
    AWS_PRECONDITION(
        map->p_impl == NULL || aws_hash_table_is_valid(map),
        "Input aws_hash_table [map] must be valid or hash_table_state pointer [map->p_impl] must be NULL, in case "
        "aws_hash_table_clean_up was called twice.");
    struct hash_table_state *state = map->p_impl;

    /* Ensure that we're idempotent */
    if (!state) {
        return;
    }

    aws_hash_table_clear(map);
    aws_mem_release(map->p_impl->alloc, map->p_impl);

    map->p_impl = NULL;
    AWS_POSTCONDITION(map->p_impl == NULL);
}

void aws_hash_table_swap(struct aws_hash_table *AWS_RESTRICT a, struct aws_hash_table *AWS_RESTRICT b) {
    AWS_PRECONDITION(a != b);
    struct aws_hash_table tmp = *a;
    *a = *b;
    *b = tmp;
}

void aws_hash_table_move(struct aws_hash_table *AWS_RESTRICT to, struct aws_hash_table *AWS_RESTRICT from) {
    AWS_PRECONDITION(to != NULL);
    AWS_PRECONDITION(from != NULL);
    AWS_PRECONDITION(to != from);
    AWS_PRECONDITION(aws_hash_table_is_valid(from));

    *to = *from;
    AWS_ZERO_STRUCT(*from);
    AWS_POSTCONDITION(aws_hash_table_is_valid(to));
}

/* Tries to find where the requested key is or where it should go if put.
 * Returns AWS_ERROR_SUCCESS if the item existed (leaving it in *entry),
 * or AWS_ERROR_HASHTBL_ITEM_NOT_FOUND if it did not (putting its destination
 * in *entry). Note that this does not take care of displacing whatever was in
 * that entry before.
 *
 * probe_idx is set to the probe index of the entry found.
 */

static int s_find_entry1(
    struct hash_table_state *state,
    uint64_t hash_code,
    const void *key,
    struct hash_table_entry **p_entry,
    size_t *p_probe_idx);

/* Inlined fast path: Check the first slot, only. */
/* TODO: Force inlining? */
static int inline s_find_entry(
    struct hash_table_state *state,
    uint64_t hash_code,
    const void *key,
    struct hash_table_entry **p_entry,
    size_t *p_probe_idx) {
    struct hash_table_entry *entry = &state->slots[hash_code & state->mask];

    if (entry->hash_code == 0) {
        if (p_probe_idx) {
            *p_probe_idx = 0;
        }
        *p_entry = entry;
        return AWS_ERROR_HASHTBL_ITEM_NOT_FOUND;
    }

    if (entry->hash_code == hash_code && s_hash_keys_eq(state, key, entry->element.key)) {
        if (p_probe_idx) {
            *p_probe_idx = 0;
        }
        *p_entry = entry;
        return AWS_OP_SUCCESS;
    }

    return s_find_entry1(state, hash_code, key, p_entry, p_probe_idx);
}

static int s_find_entry1(
    struct hash_table_state *state,
    uint64_t hash_code,
    const void *key,
    struct hash_table_entry **p_entry,
    size_t *p_probe_idx) {
    size_t probe_idx = 1;
    /* If we find a deleted entry, we record that index and return it as our probe index (i.e. we'll keep searching to
     * see if it already exists, but if not we'll overwrite the deleted entry).
     */

    int rv;
    struct hash_table_entry *entry;
    /* This loop is guaranteed to terminate because entry_probe is bounded above by state->mask (i.e. state->size - 1).
     * Since probe_idx increments every loop iteration, it will become larger than entry_probe after at most state->size
     * transitions and the loop will exit (if it hasn't already)
     */
    while (1) {
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
        uint64_t index = (hash_code + probe_idx) & state->mask;
#ifdef CBMC
#    pragma CPROVER check pop
#endif
        entry = &state->slots[index];
        if (!entry->hash_code) {
            rv = AWS_ERROR_HASHTBL_ITEM_NOT_FOUND;
            break;
        }

        if (entry->hash_code == hash_code && s_hash_keys_eq(state, key, entry->element.key)) {
            rv = AWS_ERROR_SUCCESS;
            break;
        }

#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
        uint64_t entry_probe = (index - entry->hash_code) & state->mask;
#ifdef CBMC
#    pragma CPROVER check pop
#endif

        if (entry_probe < probe_idx) {
            /* We now know that our target entry cannot exist; if it did exist,
             * it would be at the current location as it has a higher probe
             * length than the entry we are examining and thus would have
             * preempted that item
             */
            rv = AWS_ERROR_HASHTBL_ITEM_NOT_FOUND;
            break;
        }

        probe_idx++;
    }

    *p_entry = entry;
    if (p_probe_idx) {
        *p_probe_idx = probe_idx;
    }

    return rv;
}

int aws_hash_table_find(const struct aws_hash_table *map, const void *key, struct aws_hash_element **p_elem) {
    AWS_PRECONDITION(aws_hash_table_is_valid(map));
    AWS_PRECONDITION(AWS_OBJECT_PTR_IS_WRITABLE(p_elem), "Input aws_hash_element pointer [p_elem] must be writable.");

    struct hash_table_state *state = map->p_impl;
    uint64_t hash_code = s_hash_for(state, key);
    struct hash_table_entry *entry;

    int rv = s_find_entry(state, hash_code, key, &entry, NULL);

    if (rv == AWS_ERROR_SUCCESS) {
        *p_elem = &entry->element;
    } else {
        *p_elem = NULL;
    }
    AWS_SUCCEED_WITH_POSTCONDITION(aws_hash_table_is_valid(map));
}

/**
 * Attempts to find a home for the given entry.
 * If the entry was empty (i.e. hash-code of 0), then the function does nothing and returns NULL
 * Otherwise, it emplaces the item, and returns a pointer to the newly emplaced entry.
 * This function is only called after the hash-table has been expanded to fit the new element,
 * so it should never fail.
 */
static struct hash_table_entry *s_emplace_item(
    struct hash_table_state *state,
    struct hash_table_entry entry,
    size_t probe_idx) {
    AWS_PRECONDITION(hash_table_state_is_valid(state));

    if (entry.hash_code == 0) {
        AWS_RETURN_WITH_POSTCONDITION(NULL, hash_table_state_is_valid(state));
    }

    struct hash_table_entry *rval = NULL;

    /* Since a valid hash_table has at least one empty element, this loop will always terminate in at most linear time
     */
    while (entry.hash_code != 0) {
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
        size_t index = (size_t)(entry.hash_code + probe_idx) & state->mask;
#ifdef CBMC
#    pragma CPROVER check pop
#endif
        struct hash_table_entry *victim = &state->slots[index];

#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
        size_t victim_probe_idx = (size_t)(index - victim->hash_code) & state->mask;
#ifdef CBMC
#    pragma CPROVER check pop
#endif

        if (!victim->hash_code || victim_probe_idx < probe_idx) {
            /* The first thing we emplace is the entry itself. A pointer to its location becomes the rval */
            if (!rval) {
                rval = victim;
            }

            struct hash_table_entry tmp = *victim;
            *victim = entry;
            entry = tmp;

            probe_idx = victim_probe_idx + 1;
        } else {
            probe_idx++;
        }
    }

    AWS_RETURN_WITH_POSTCONDITION(
        rval,
        hash_table_state_is_valid(state) && rval >= &state->slots[0] && rval < &state->slots[state->size],
        "Output hash_table_entry pointer [rval] must point in the slots of [state].");
}

static int s_expand_table(struct aws_hash_table *map) {
    struct hash_table_state *old_state = map->p_impl;
    struct hash_table_state template = *old_state;

    size_t new_size;
    if (aws_mul_size_checked(template.size, 2, &new_size)) {
        return AWS_OP_ERR;
    }

    if (s_update_template_size(&template, new_size)) {
        return AWS_OP_ERR;
    }

    struct hash_table_state *new_state = s_alloc_state(&template);
    if (!new_state) {
        return AWS_OP_ERR;
    }

    for (size_t i = 0; i < old_state->size; i++) {
        struct hash_table_entry entry = old_state->slots[i];
        if (entry.hash_code) {
            /* We can directly emplace since we know we won't put the same item twice */
            s_emplace_item(new_state, entry, 0);
        }
    }

    map->p_impl = new_state;
    aws_mem_release(new_state->alloc, old_state);

    return AWS_OP_SUCCESS;
}

int aws_hash_table_create(
    struct aws_hash_table *map,
    const void *key,
    struct aws_hash_element **p_elem,
    int *was_created) {

    struct hash_table_state *state = map->p_impl;
    uint64_t hash_code = s_hash_for(state, key);
    struct hash_table_entry *entry;
    size_t probe_idx;
    int ignored;
    if (!was_created) {
        was_created = &ignored;
    }

    int rv = s_find_entry(state, hash_code, key, &entry, &probe_idx);

    if (rv == AWS_ERROR_SUCCESS) {
        if (p_elem) {
            *p_elem = &entry->element;
        }
        *was_created = 0;
        return AWS_OP_SUCCESS;
    }

    /* Okay, we need to add an entry. Check the load factor first. */
    size_t incr_entry_count;
    if (aws_add_size_checked(state->entry_count, 1, &incr_entry_count)) {
        return AWS_OP_ERR;
    }
    if (incr_entry_count > state->max_load) {
        rv = s_expand_table(map);
        if (rv != AWS_OP_SUCCESS) {
            /* Any error was already raised in expand_table */
            return rv;
        }
        state = map->p_impl;
        /* If we expanded the table, we need to discard the probe index returned from find_entry,
         * as it's likely that we can find a more desirable slot. If we don't, then later gets will
         * terminate before reaching our probe index.

         * n.b. currently we ignore this probe_idx subsequently, but leaving
         this here so we don't
         * forget when we optimize later. */
        probe_idx = 0;
    }

    state->entry_count++;
    struct hash_table_entry new_entry;
    new_entry.element.key = key;
    new_entry.element.value = NULL;
    new_entry.hash_code = hash_code;

    entry = s_emplace_item(state, new_entry, probe_idx);

    if (p_elem) {
        *p_elem = &entry->element;
    }

    *was_created = 1;

    return AWS_OP_SUCCESS;
}

AWS_COMMON_API
int aws_hash_table_put(struct aws_hash_table *map, const void *key, void *value, int *was_created) {
    struct aws_hash_element *p_elem;
    int was_created_fallback;

    if (!was_created) {
        was_created = &was_created_fallback;
    }

    if (aws_hash_table_create(map, key, &p_elem, was_created)) {
        return AWS_OP_ERR;
    }

    /*
     * aws_hash_table_create might resize the table, which results in map->p_impl changing.
     * It is therefore important to wait to read p_impl until after we return.
     */
    struct hash_table_state *state = map->p_impl;

    if (!*was_created) {
        if (p_elem->key != key && state->destroy_key_fn) {
            state->destroy_key_fn((void *)p_elem->key);
        }

        if (state->destroy_value_fn) {
            state->destroy_value_fn((void *)p_elem->value);
        }
    }

    p_elem->key = key;
    p_elem->value = value;

    return AWS_OP_SUCCESS;
}

/* Clears an entry. Does _not_ invoke destructor callbacks.
 * Returns the last slot touched (note that if we wrap, we'll report an index
 * lower than the original entry's index)
 */
static size_t s_remove_entry(struct hash_table_state *state, struct hash_table_entry *entry) {
    AWS_PRECONDITION(hash_table_state_is_valid(state));
    AWS_PRECONDITION(state->entry_count > 0);
    AWS_PRECONDITION(
        entry >= &state->slots[0] && entry < &state->slots[state->size],
        "Input hash_table_entry [entry] pointer must point in the available slots.");
    state->entry_count--;

    /* Shift subsequent entries back until we find an entry that belongs at its
     * current position. This is important to ensure that subsequent searches
     * don't terminate at the removed element.
     */
    size_t index = s_index_for(state, entry);
    /* There is always at least one empty slot in the hash table, so this loop always terminates */
    while (1) {
        size_t next_index = (index + 1) & state->mask;

        /* If we hit an empty slot, stop */
        if (!state->slots[next_index].hash_code) {
            break;
        }
        /* If the next slot is at the start of the probe sequence, stop.
         * We know that nothing with an earlier home slot is after this;
         * otherwise this index-zero entry would have been evicted from its
         * home.
         */
        if ((state->slots[next_index].hash_code & state->mask) == next_index) {
            break;
        }

        /* Okay, shift this one back */
        state->slots[index] = state->slots[next_index];
        index = next_index;
    }

    /* Clear the entry we shifted out of */
    AWS_ZERO_STRUCT(state->slots[index]);
    AWS_RETURN_WITH_POSTCONDITION(index, hash_table_state_is_valid(state) && index <= state->size);
}

int aws_hash_table_remove(
    struct aws_hash_table *map,
    const void *key,
    struct aws_hash_element *p_value,
    int *was_present) {
    AWS_PRECONDITION(aws_hash_table_is_valid(map));
    AWS_PRECONDITION(
        p_value == NULL || AWS_OBJECT_PTR_IS_WRITABLE(p_value), "Input pointer [p_value] must be NULL or writable.");
    AWS_PRECONDITION(
        was_present == NULL || AWS_OBJECT_PTR_IS_WRITABLE(was_present),
        "Input pointer [was_present] must be NULL or writable.");

    struct hash_table_state *state = map->p_impl;
    uint64_t hash_code = s_hash_for(state, key);
    struct hash_table_entry *entry;
    int ignored;

    if (!was_present) {
        was_present = &ignored;
    }

    int rv = s_find_entry(state, hash_code, key, &entry, NULL);

    if (rv != AWS_ERROR_SUCCESS) {
        *was_present = 0;
        AWS_SUCCEED_WITH_POSTCONDITION(aws_hash_table_is_valid(map));
    }

    *was_present = 1;

    if (p_value) {
        *p_value = entry->element;
    } else {
        if (state->destroy_key_fn) {
            state->destroy_key_fn((void *)entry->element.key);
        }
        if (state->destroy_value_fn) {
            state->destroy_value_fn(entry->element.value);
        }
    }
    s_remove_entry(state, entry);

    AWS_SUCCEED_WITH_POSTCONDITION(aws_hash_table_is_valid(map));
}

int aws_hash_table_remove_element(struct aws_hash_table *map, struct aws_hash_element *p_value) {
    AWS_PRECONDITION(aws_hash_table_is_valid(map));
    AWS_PRECONDITION(p_value != NULL);

    struct hash_table_state *state = map->p_impl;
    struct hash_table_entry *entry = AWS_CONTAINER_OF(p_value, struct hash_table_entry, element);

    s_remove_entry(state, entry);

    AWS_SUCCEED_WITH_POSTCONDITION(aws_hash_table_is_valid(map));
}

int aws_hash_table_foreach(
    struct aws_hash_table *map,
    int (*callback)(void *context, struct aws_hash_element *pElement),
    void *context) {

    for (struct aws_hash_iter iter = aws_hash_iter_begin(map); !aws_hash_iter_done(&iter); aws_hash_iter_next(&iter)) {
        int rv = callback(context, &iter.element);
        if (rv & AWS_COMMON_HASH_TABLE_ITER_ERROR) {
            int error = aws_last_error();
            if (error == AWS_ERROR_SUCCESS) {
                aws_raise_error(AWS_ERROR_UNKNOWN);
            }
            return AWS_OP_ERR;
        }

        if (rv & AWS_COMMON_HASH_TABLE_ITER_DELETE) {
            aws_hash_iter_delete(&iter, false);
        }

        if (!(rv & AWS_COMMON_HASH_TABLE_ITER_CONTINUE)) {
            break;
        }
    }

    return AWS_OP_SUCCESS;
}

bool aws_hash_table_eq(
    const struct aws_hash_table *a,
    const struct aws_hash_table *b,
    aws_hash_callback_eq_fn *value_eq) {
    AWS_PRECONDITION(aws_hash_table_is_valid(a));
    AWS_PRECONDITION(aws_hash_table_is_valid(b));
    AWS_PRECONDITION(value_eq != NULL);

    if (aws_hash_table_get_entry_count(a) != aws_hash_table_get_entry_count(b)) {
        AWS_RETURN_WITH_POSTCONDITION(false, aws_hash_table_is_valid(a) && aws_hash_table_is_valid(b));
    }

    /*
     * Now that we have established that the two tables have the same number of
     * entries, we can simply iterate one and compare against the same key in
     * the other.
     */
    for (size_t i = 0; i < a->p_impl->size; ++i) {
        const struct hash_table_entry *const a_entry = &a->p_impl->slots[i];
        if (a_entry->hash_code == 0) {
            continue;
        }

        struct aws_hash_element *b_element = NULL;

        aws_hash_table_find(b, a_entry->element.key, &b_element);

        if (!b_element) {
            /* Key is present in A only */
            AWS_RETURN_WITH_POSTCONDITION(false, aws_hash_table_is_valid(a) && aws_hash_table_is_valid(b));
        }

        if (!s_safe_eq_check(value_eq, a_entry->element.value, b_element->value)) {
            AWS_RETURN_WITH_POSTCONDITION(false, aws_hash_table_is_valid(a) && aws_hash_table_is_valid(b));
        }
    }
    AWS_RETURN_WITH_POSTCONDITION(true, aws_hash_table_is_valid(a) && aws_hash_table_is_valid(b));
}

/**
 * Given an iterator, and a start slot, find the next available filled slot if it exists
 * Otherwise, return an iter that will return true for aws_hash_iter_done().
 * Note that aws_hash_iter_is_valid() need not hold on entry to the function, since
 * it can be called on a partially constructed iter from aws_hash_iter_begin().
 *
 * Note that calling this on an iterator which is "done" is idempotent: it will return another
 * iterator which is "done".
 */
static inline void s_get_next_element(struct aws_hash_iter *iter, size_t start_slot) {
    AWS_PRECONDITION(iter != NULL);
    AWS_PRECONDITION(aws_hash_table_is_valid(iter->map));
    struct hash_table_state *state = iter->map->p_impl;
    size_t limit = iter->limit;

    for (size_t i = start_slot; i < limit; i++) {
        struct hash_table_entry *entry = &state->slots[i];

        if (entry->hash_code) {
            iter->element = entry->element;
            iter->slot = i;
            iter->status = AWS_HASH_ITER_STATUS_READY_FOR_USE;
            return;
        }
    }
    iter->element.key = NULL;
    iter->element.value = NULL;
    iter->slot = iter->limit;
    iter->status = AWS_HASH_ITER_STATUS_DONE;
    AWS_POSTCONDITION(aws_hash_iter_is_valid(iter));
}

struct aws_hash_iter aws_hash_iter_begin(const struct aws_hash_table *map) {
    AWS_PRECONDITION(aws_hash_table_is_valid(map));
    struct hash_table_state *state = map->p_impl;
    struct aws_hash_iter iter;
    AWS_ZERO_STRUCT(iter);
    iter.map = map;
    iter.limit = state->size;
    s_get_next_element(&iter, 0);
    AWS_RETURN_WITH_POSTCONDITION(
        iter,
        aws_hash_iter_is_valid(&iter) &&
            (iter.status == AWS_HASH_ITER_STATUS_DONE || iter.status == AWS_HASH_ITER_STATUS_READY_FOR_USE),
        "The status of output aws_hash_iter [iter] must either be DONE or READY_FOR_USE.");
}

bool aws_hash_iter_done(const struct aws_hash_iter *iter) {
    AWS_PRECONDITION(aws_hash_iter_is_valid(iter));
    AWS_PRECONDITION(
        iter->status == AWS_HASH_ITER_STATUS_DONE || iter->status == AWS_HASH_ITER_STATUS_READY_FOR_USE,
        "Input aws_hash_iter [iter] must either be done, or ready to use.");
    /*
     * SIZE_MAX is a valid (non-terminal) value for iter->slot in the event that
     * we delete slot 0. See comments in aws_hash_iter_delete.
     *
     * As such we must use == rather than >= here.
     */
    bool rval = (iter->slot == iter->limit);
    AWS_POSTCONDITION(
        iter->status == AWS_HASH_ITER_STATUS_DONE || iter->status == AWS_HASH_ITER_STATUS_READY_FOR_USE,
        "The status of output aws_hash_iter [iter] must either be DONE or READY_FOR_USE.");
    AWS_POSTCONDITION(
        rval == (iter->status == AWS_HASH_ITER_STATUS_DONE),
        "Output bool [rval] must be true if and only if the status of [iter] is DONE.");
    AWS_POSTCONDITION(aws_hash_iter_is_valid(iter));
    return rval;
}

void aws_hash_iter_next(struct aws_hash_iter *iter) {
    AWS_PRECONDITION(aws_hash_iter_is_valid(iter));
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
    s_get_next_element(iter, iter->slot + 1);
#ifdef CBMC
#    pragma CPROVER check pop
#endif
    AWS_POSTCONDITION(
        iter->status == AWS_HASH_ITER_STATUS_DONE || iter->status == AWS_HASH_ITER_STATUS_READY_FOR_USE,
        "The status of output aws_hash_iter [iter] must either be DONE or READY_FOR_USE.");
    AWS_POSTCONDITION(aws_hash_iter_is_valid(iter));
}

void aws_hash_iter_delete(struct aws_hash_iter *iter, bool destroy_contents) {
    AWS_PRECONDITION(
        iter->status == AWS_HASH_ITER_STATUS_READY_FOR_USE, "Input aws_hash_iter [iter] must be ready for use.");
    AWS_PRECONDITION(aws_hash_iter_is_valid(iter));
    AWS_PRECONDITION(
        iter->map->p_impl->entry_count > 0,
        "The hash_table_state pointed by input [iter] must contain at least one entry.");

    struct hash_table_state *state = iter->map->p_impl;
    if (destroy_contents) {
        if (state->destroy_key_fn) {
            state->destroy_key_fn((void *)iter->element.key);
        }
        if (state->destroy_value_fn) {
            state->destroy_value_fn(iter->element.value);
        }
    }

    size_t last_index = s_remove_entry(state, &state->slots[iter->slot]);

    /* If we shifted elements that are not part of the window we intend to iterate
     * over, it means we shifted an element that we already visited into the
     * iter->limit - 1 position. To avoid double iteration, we'll now reduce the
     * limit to compensate.
     *
     * Note that last_index cannot equal iter->slot, because slots[iter->slot]
     * is empty before we start walking the table.
     */
    if (last_index < iter->slot || last_index >= iter->limit) {
        iter->limit--;
    }

    /*
     * After removing this entry, the next entry might be in the same slot, or
     * in some later slot, or we might have no further entries.
     *
     * We also expect that the caller will call aws_hash_iter_done and aws_hash_iter_next
     * after this delete call. This gets a bit tricky if we just deleted the value
     * in slot 0, and a new value has shifted in.
     *
     * To deal with this, we'll just step back one slot, and let _next start iteration
     * at our current slot. Note that if we just deleted slot 0, this will result in
     * underflowing to SIZE_MAX; we have to take care in aws_hash_iter_done to avoid
     * treating this as an end-of-iteration condition.
     */
#ifdef CBMC
#    pragma CPROVER check push
#    pragma CPROVER check disable "unsigned-overflow"
#endif
    iter->slot--;
#ifdef CBMC
#    pragma CPROVER check pop
#endif
    iter->status = AWS_HASH_ITER_STATUS_DELETE_CALLED;
    AWS_POSTCONDITION(
        iter->status == AWS_HASH_ITER_STATUS_DELETE_CALLED,
        "The status of output aws_hash_iter [iter] must be DELETE_CALLED.");
    AWS_POSTCONDITION(aws_hash_iter_is_valid(iter));
}

void aws_hash_table_clear(struct aws_hash_table *map) {
    AWS_PRECONDITION(aws_hash_table_is_valid(map));
    struct hash_table_state *state = map->p_impl;

    /* Check that we have at least one destructor before iterating over the table */
    if (state->destroy_key_fn || state->destroy_value_fn) {
        for (size_t i = 0; i < state->size; ++i) {
            struct hash_table_entry *entry = &state->slots[i];
            if (!entry->hash_code) {
                continue;
            }
            if (state->destroy_key_fn) {
                state->destroy_key_fn((void *)entry->element.key);
            }
            if (state->destroy_value_fn) {
                state->destroy_value_fn(entry->element.value);
            }
        }
    }
    /* Since hash code 0 represents an empty slot we can just zero out the
     * entire table. */
    memset(state->slots, 0, sizeof(*state->slots) * state->size);

    state->entry_count = 0;
    AWS_POSTCONDITION(aws_hash_table_is_valid(map));
}

uint64_t aws_hash_c_string(const void *item) {
    AWS_PRECONDITION(aws_c_string_is_valid(item));
    const char *str = item;

    /* first digits of pi in hex */
    uint32_t b = 0x3243F6A8, c = 0x885A308D;
    hashlittle2(str, strlen(str), &c, &b);

    return ((uint64_t)b << 32) | c;
}

uint64_t aws_hash_string(const void *item) {
    AWS_PRECONDITION(aws_string_is_valid(item));
    const struct aws_string *str = item;

    /* first digits of pi in hex */
    uint32_t b = 0x3243F6A8, c = 0x885A308D;
    hashlittle2(aws_string_bytes(str), str->len, &c, &b);
    AWS_RETURN_WITH_POSTCONDITION(((uint64_t)b << 32) | c, aws_string_is_valid(str));
}

uint64_t aws_hash_byte_cursor_ptr(const void *item) {
    AWS_PRECONDITION(aws_byte_cursor_is_valid(item));
    const struct aws_byte_cursor *cur = item;

    /* first digits of pi in hex */
    uint32_t b = 0x3243F6A8, c = 0x885A308D;
    hashlittle2(cur->ptr, cur->len, &c, &b);
    AWS_RETURN_WITH_POSTCONDITION(((uint64_t)b << 32) | c, aws_byte_cursor_is_valid(cur)); /* NOLINT */
}

uint64_t aws_hash_ptr(const void *item) {
    /* Since the numeric value of the pointer is considered, not the memory behind it, 0 is an acceptable value */
    /* first digits of e in hex
     * 2.b7e 1516 28ae d2a6 */
    uint32_t b = 0x2b7e1516, c = 0x28aed2a6;

    hashlittle2(&item, sizeof(item), &c, &b);

    return ((uint64_t)b << 32) | c;
}

uint64_t aws_hash_combine(uint64_t item1, uint64_t item2) {
    uint32_t b = item2 & 0xFFFFFFFF; /* LSB */
    uint32_t c = item2 >> 32;        /* MSB */

    hashlittle2(&item1, sizeof(item1), &c, &b);
    return ((uint64_t)b << 32) | c;
}

bool aws_hash_callback_c_str_eq(const void *a, const void *b) {
    AWS_PRECONDITION(aws_c_string_is_valid(a));
    AWS_PRECONDITION(aws_c_string_is_valid(b));
    bool rval = !strcmp(a, b);
    AWS_RETURN_WITH_POSTCONDITION(rval, aws_c_string_is_valid(a) && aws_c_string_is_valid(b));
}

bool aws_hash_callback_string_eq(const void *a, const void *b) {
    AWS_PRECONDITION(aws_string_is_valid(a));
    AWS_PRECONDITION(aws_string_is_valid(b));
    bool rval = aws_string_eq(a, b);
    AWS_RETURN_WITH_POSTCONDITION(rval, aws_string_is_valid(a) && aws_string_is_valid(b));
}

void aws_hash_callback_string_destroy(void *a) {
    AWS_PRECONDITION(aws_string_is_valid(a));
    aws_string_destroy(a);
}

bool aws_ptr_eq(const void *a, const void *b) {
    return a == b;
}

/**
 * Best-effort check of hash_table_state data-structure invariants
 * Some invariants, such as that the number of entries is actually the
 * same as the entry_count field, would require a loop to check
 */
bool aws_hash_table_is_valid(const struct aws_hash_table *map) {
    return map && map->p_impl && hash_table_state_is_valid(map->p_impl);
}

/**
 * Best-effort check of hash_table_state data-structure invariants
 * Some invariants, such as that the number of entries is actually the
 * same as the entry_count field, would require a loop to check
 */
bool hash_table_state_is_valid(const struct hash_table_state *map) {
    if (!map) {
        return false;
    }
    bool hash_fn_nonnull = (map->hash_fn != NULL);
    bool equals_fn_nonnull = (map->equals_fn != NULL);
    /*destroy_key_fn and destroy_value_fn are both allowed to be NULL*/
    bool alloc_nonnull = (map->alloc != NULL);
    bool size_at_least_two = (map->size >= 2);
    bool size_is_power_of_two = aws_is_power_of_two(map->size);
    bool entry_count = (map->entry_count <= map->max_load);
    bool max_load = (map->max_load < map->size);
    bool mask_is_correct = (map->mask == (map->size - 1));
    bool max_load_factor_bounded = map->max_load_factor == 0.95; //(map->max_load_factor < 1.0);
    bool slots_allocated = AWS_MEM_IS_WRITABLE(&map->slots[0], sizeof(map->slots[0]) * map->size);

    return hash_fn_nonnull && equals_fn_nonnull && alloc_nonnull && size_at_least_two && size_is_power_of_two &&
           entry_count && max_load && mask_is_correct && max_load_factor_bounded && slots_allocated;
}

/**
 * Given a pointer to a hash_iter, checks that it is well-formed, with all data-structure invariants.
 */
bool aws_hash_iter_is_valid(const struct aws_hash_iter *iter) {
    if (!iter) {
        return false;
    }
    if (!iter->map) {
        return false;
    }
    if (!aws_hash_table_is_valid(iter->map)) {
        return false;
    }
    if (iter->limit > iter->map->p_impl->size) {
        return false;
    }

    switch (iter->status) {
        case AWS_HASH_ITER_STATUS_DONE:
            /* Done iff slot == limit */
            return iter->slot == iter->limit;
        case AWS_HASH_ITER_STATUS_DELETE_CALLED:
            /* iter->slot can underflow to SIZE_MAX after a delete
             * see the comments for aws_hash_iter_delete() */
            return iter->slot <= iter->limit || iter->slot == SIZE_MAX;
        case AWS_HASH_ITER_STATUS_READY_FOR_USE:
            /* A slot must point to a valid location (i.e. hash_code != 0) */
            return iter->slot < iter->limit && iter->map->p_impl->slots[iter->slot].hash_code != 0;
    }
    /* Invalid status code */
    return false;
}

/**
 * Determine the total number of bytes needed for a hash-table with
 * "size" slots. If the result would overflow a size_t, return
 * AWS_OP_ERR; otherwise, return AWS_OP_SUCCESS with the result in
 * "required_bytes".
 */
int hash_table_state_required_bytes(size_t size, size_t *required_bytes) {

    size_t elemsize;
    if (aws_mul_size_checked(size, sizeof(struct hash_table_entry), &elemsize)) {
        return AWS_OP_ERR;
    }

    if (aws_add_size_checked(elemsize, sizeof(struct hash_table_state), required_bytes)) {
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

uint64_t aws_hash_uint64_t_by_identity(const void *item) {
    return *(uint64_t *)item;
}

bool aws_hash_compare_uint64_t_eq(const void *a, const void *b) {
    return *(uint64_t *)a == *(uint64_t *)b;
}