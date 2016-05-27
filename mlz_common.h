/*
   mini-LZ library (mlz)
   (c) Martin Sedlak 2016

   Boost Software License - Version 1.0 - August 17th, 2003

   Permission is hereby granted, free of charge, to any person or organization
   obtaining a copy of the software and accompanying documentation covered by
   this license (the "Software") to use, reproduce, display, distribute,
   execute, and transmit the Software, and to prepare derivative works of the
   Software, and to permit third-parties to whom the Software is furnished to
   do so, all subject to the following:

   The copyright notices in the Software and this entire statement, including
   the above license grant, this restriction and the following disclaimer,
   must be included in all copies of the Software, in whole or in part, and
   all derivative works of the Software, unless such copies or derivative
   works are solely in the form of machine-executable object code generated by
   a source language processor.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#ifndef MLZ_COMMON_H
#define MLZ_COMMON_H

/* boilerplate macros */

/* you can override this externally */
#if !defined(MLZ_API)
#	define MLZ_API
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#	define MLZ_INLINE static __inline
#else
#	define MLZ_INLINE static inline
#endif

#if !defined MLZ_CONST
#	define MLZ_CONST  const
#endif

#if !defined(MLZ_DEBUG) && (defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG)
#	define MLZ_DEBUG 1
#endif

#if !defined(MLZ_ASSERT)
#	if defined(MLZ_DEBUG)
#		include <assert.h>
#		define MLZ_ASSERT(expr) assert(expr)
#	else
#		define MLZ_ASSERT(expr)
#	endif
#endif

#if !defined(MLZ_LIKELY)
#	if defined(__clang__) || defined(__GNUC__)
#		define MLZ_LIKELY(x)   __builtin_expect(!!(x), 1)
#		define MLZ_UNLIKELY(x) __builtin_expect(!!(x), 0)
#	else
#		define MLZ_LIKELY(x) x
#		define MLZ_UNLIKELY(x) x
#	endif
#endif

/* types */

#include <stddef.h>

#if defined(__BORLANDC__)
typedef long          intptr_t;
typedef unsigned long uintptr_t;
#endif

/* the following should be compatible with vs2008 and up */
#if (defined(_MSC_VER) && _MSC_VER < 1900) || defined(__BORLANDC__)
	typedef unsigned __int8  mlz_byte;
	typedef signed   __int8  mlz_sbyte;
	typedef unsigned __int16 mlz_ushort;
	typedef signed   __int16 mlz_short;
	typedef unsigned __int32 mlz_uint;
	typedef signed   __int32 mlz_int;
	typedef unsigned __int64 mlz_ulong;
	typedef signed   __int64 mlz_long;
#	if defined(__clang__) || defined(__GNUC__)
#		include <stdint.h>
#	endif
#else
	/* do it the standard way */
#	include <stdint.h>
	typedef uint8_t     mlz_byte;
	typedef int8_t      mlz_sbyte;
	typedef uint16_t    mlz_ushort;
	typedef int16_t     mlz_short;
	typedef uint32_t    mlz_uint;
	typedef int32_t     mlz_int;
	typedef uint64_t    mlz_ulong;
	typedef int64_t     mlz_long;
#endif

typedef intptr_t   mlz_intptr;
typedef uintptr_t  mlz_uintptr;
typedef size_t     mlz_size;
typedef char       mlz_char;
typedef mlz_int    mlz_bool;

/* constants */

#ifndef NULL
#	define MLZ_NULL  0
#else
#	define MLZ_NULL NULL
#endif
#define MLZ_TRUE  ((mlz_bool)1)
#define MLZ_FALSE ((mlz_bool)0)

/* helper macros */

#define MLZ_RET_FALSE(expr) if (!(expr)) return 0

/* compression-specific constants */

typedef enum {
	MLZ_MAX_DIST     = 65535,
	/* min match len of 3 tested better than 2 */
	MLZ_MIN_MATCH    = 3,
	MLZ_MAX_MATCH    = 65535 + MLZ_MIN_MATCH,
	MLZ_MIN_LIT_RUN  = 23,
	MLZ_ACCUM_BITS   = 24,
	MLZ_ACCUM_BYTES  = MLZ_ACCUM_BITS/8,
	/* internal streaming buffer alignment because of multi-threaded mode        */
	/* usually cache line is 64 bytes (or less), but we want a safe reserve here */
	MLZ_CACHELINE_ALIGN = 512
} mlz_constants;

#endif
