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

#include "mlz_dec.h"

#define MLZ_DEC_GUARD_MASK (1u << MLZ_ACCUM_BITS)
#define MLZ_DEC_0BIT_MASK  ~1u
#define MLZ_DEC_2BIT_MASK  ~7u
#define MLZ_DEC_3BIT_MASK  ~15u
#define MLZ_DEC_6BIT_MASK  ~127u

#define MLZ_LOAD_ACCUM() \
	{ \
		int i; \
		accum  = MLZ_DEC_GUARD_MASK | *sb++; \
		for (i=1; i<MLZ_ACCUM_BYTES; i++) \
			accum += (mlz_uint)(*sb++) << (8*i); \
	}

#define MLZ_GET_BIT_FAST_NOACCUM(res) \
	MLZ_ASSERT(accum & MLZ_DEC_0BIT_MASK); \
	res = (int)(accum & 1); \
	accum >>= 1;

#define MLZ_GET_BIT_CHECK(res, stmt) \
	MLZ_GET_BIT_FAST_NOACCUM(res) \
	if (MLZ_UNLIKELY(!(accum & MLZ_DEC_0BIT_MASK))) { \
		stmt \
		MLZ_LOAD_ACCUM() \
	}

#define MLZ_GET_BIT(res) MLZ_GET_BIT_CHECK(res, if (sb + MLZ_ACCUM_BYTES > se) return 0;)
#define MLZ_GET_BIT_FAST(res) MLZ_GET_BIT_CHECK(res,;)

#define MLZ_GET_TYPE_FAST_NOACCUM(res) \
	res = (int)(accum & 3); \
	accum >>= 2;

#define MLZ_GET_TYPE_COMMON(res, getbit) \
	if (MLZ_LIKELY(accum & MLZ_DEC_2BIT_MASK)) { \
		MLZ_GET_TYPE_FAST_NOACCUM(res) \
	} else { \
		int tmp; \
		getbit(res); \
		getbit(tmp); \
		res += 2*tmp; \
	}

#define MLZ_GET_TYPE(res)      MLZ_GET_TYPE_COMMON(res, MLZ_GET_BIT)
#define MLZ_GET_TYPE_FAST(res) MLZ_GET_TYPE_COMMON(res, MLZ_GET_BIT_FAST)

#define MLZ_GET_SHORT_LEN_FAST_NOACCUM(res) \
	res = (int)(accum & 7); \
	accum >>= 3;

#define MLZ_GET_SHORT_LEN_COMMON(res, getbit) \
	if (MLZ_LIKELY(accum & MLZ_DEC_3BIT_MASK)) { \
		MLZ_GET_SHORT_LEN_FAST_NOACCUM(res) \
	} else { \
		int tmp; \
		getbit(res) \
		getbit(tmp) \
		res += 2*tmp; \
		getbit(tmp) \
		res += 4*tmp; \
	}

#define MLZ_GET_SHORT_LEN(res)      MLZ_GET_SHORT_LEN_COMMON(res, MLZ_GET_BIT)
#define MLZ_GET_SHORT_LEN_FAST(res) MLZ_GET_SHORT_LEN_COMMON(res, MLZ_GET_BIT_FAST)

#define MLZ_COPY_MATCH_UNSAFE() \
	chlen = len >> 2; \
	len &= 3; \
	dist = -dist; \
	\
	while (chlen-- > 0) { \
		*db = db[dist]; db++; \
		*db = db[dist]; db++; \
		*db = db[dist]; db++; \
		*db = db[dist]; db++; \
	} \
	\
	while (len-- > 0) { \
		*db = db[dist]; db++; \
	}

#define MLZ_COPY_MATCH() \
	MLZ_RET_FALSE(db - dist >= odblimit && db + len <= de); \
 \
	MLZ_COPY_MATCH_UNSAFE()

#define MLZ_LITCOPY(db, sb, run) \
	{ \
		mlz_int chrun = run >> 2; \
		run &= 3; \
		while (chrun-- > 0) { \
			*db++ = *sb++; \
			*db++ = *sb++; \
			*db++ = *sb++; \
			*db++ = *sb++; \
		} \
 \
		while (run-- > 0) \
			*db++ = *sb++; \
	}

#define MLZ_LITERAL_RUN() \
	{ \
		mlz_int run = sb[0] + (sb[1] << 8); \
		MLZ_RET_FALSE(run >= MLZ_MIN_LIT_RUN); \
		sb += 2; \
		MLZ_RET_FALSE(sb + run <= se && db + run <= de); \
		MLZ_LITCOPY(db, sb, run); \
	}

#define MLZ_LITERAL_RUN_UNSAFE() \
	{ \
		mlz_int run = sb[0] + (sb[1] << 8); \
		sb += 2; \
		MLZ_LITCOPY(db, sb, run); \
	}

#define MLZ_LITERAL_RUN_SAFE() \
	MLZ_RET_FALSE(sb+1 < se); \
	MLZ_LITERAL_RUN()

#define MLZ_TINY_MATCH() \
	len += MLZ_MIN_MATCH; \
	dist = *sb++ + 1;

#define MLZ_TINY_MATCH_SAFE() \
	MLZ_RET_FALSE(sb < se); \
	MLZ_TINY_MATCH()

#define MLZ_SHORT_MATCH() \
	dist = sb[0] + (sb[1] << 8); \
	sb += 2; \
	len = dist >> 13; \
	len += MLZ_MIN_MATCH; \
	dist &= (1 << 13) - 1;

#define MLZ_SHORT_MATCH_SAFE() \
	MLZ_RET_FALSE(sb+1 < se); \
	MLZ_SHORT_MATCH()

#define MLZ_SHORT2_MATCH() \
	len += MLZ_MIN_MATCH; \
	dist = sb[0] + (sb[1] << 8); \
	sb += 2;

#define MLZ_SHORT2_MATCH_SAFE() \
	MLZ_RET_FALSE(sb+1 < se); \
	MLZ_SHORT2_MATCH()

#define MLZ_FULL_MATCH() \
	len = sb[0]; \
	if (len == 255) { \
		len = sb[1] + (sb[2] << 8); \
		sb += 2; \
	} \
	len += MLZ_MIN_MATCH; \
	dist = sb[1] + (sb[2] << 8); \
	sb += 3;

#define MLZ_FULL_MATCH_SAFE() \
	MLZ_RET_FALSE(sb+2 < se); \
	len = sb[0]; \
	if (len == 255) { \
		len = sb[1] + (sb[2] << 8); \
		sb += 2; \
		MLZ_RET_FALSE(sb+2 < se); \
	} \
	len += MLZ_MIN_MATCH; \
	dist = sb[1] + (sb[2] << 8); \
	sb += 3;

#define MLZ_LITERAL_UNSAFE() \
	if (!bit0) { \
		*db++ = *sb++; \
		continue; \
	}

#define MLZ_LITERAL() \
	if (!bit0) { \
		MLZ_RET_FALSE(sb < se && db < de); \
		*db++ = *sb++; \
		continue; \
	}

#define MLZ_LITERAL_FAST() \
	if (!bit0) { \
		MLZ_RET_FALSE(db < de); \
		*db++ = *sb++; \
		continue; \
	}

#define MLZ_INIT_DECOMPRESS() \
	mlz_uint accum; \
	mlz_int chlen; \
	int bit0, type; \
 \
	MLZ_CONST mlz_byte *sb = (MLZ_CONST mlz_byte *)(src); \
	MLZ_CONST mlz_byte *se = sb + src_size; \
	mlz_byte *db = (mlz_byte *)dst; \
	MLZ_CONST mlz_byte *odb = db;

size_t
mlz_decompress(
	void           *dst,
	size_t          dst_size,
	MLZ_CONST void *src,
	size_t          src_size,
	size_t          bytes_before_dst
)
{
	MLZ_INIT_DECOMPRESS()
	MLZ_CONST mlz_byte *de = db + dst_size;
	MLZ_CONST mlz_byte *odblimit = odb - bytes_before_dst;
	mlz_int dist = 0, len = 0;
	(void)dist;
	(void)len;

	/*
	bit 0: byte literal
	match:
	100: tiny match + 3 bits len-min_match + byte dist-1
	101: short match + word dist (3 msbits decoded as short length)
	110: short match + 3 bits len-min match + word dist
	111: full match + byte len (255 => word len follows) + word dist
	dist = 0 => literal run (then word follows: number of literals, value < 36 (MIN_LIT_RUN) is illegal)
	*/

	MLZ_RET_FALSE(sb + MLZ_ACCUM_BYTES <= se);

	MLZ_LOAD_ACCUM()

	/* fast path if we know we don't have to check for anything */
	/* max data to read: 5 bytes + 2 accum reserve */

	while (sb < se - (5 + 2*MLZ_ACCUM_BYTES)) {
		if ((accum & MLZ_DEC_6BIT_MASK)) {
			MLZ_GET_BIT_FAST_NOACCUM(bit0)
			MLZ_LITERAL_FAST()

			/* match... */
			MLZ_GET_TYPE_FAST_NOACCUM(type)
			if (type == 0) {
				/* tiny match */
				MLZ_GET_SHORT_LEN_FAST_NOACCUM(len)
				MLZ_TINY_MATCH()
			} else if (type == 2) {
				/* short match */
				MLZ_SHORT_MATCH()
			} else if (type == 1) {
				/* short2 match */
				MLZ_GET_SHORT_LEN_FAST_NOACCUM(len)
				MLZ_SHORT2_MATCH()
			} else {
				/* full match */
				MLZ_FULL_MATCH()
			}
			if (dist == 0) {
				/* literal run */
				MLZ_LITERAL_RUN()
				continue;
			}
			/* copy match */
			MLZ_COPY_MATCH()
			continue;
		}

		MLZ_GET_BIT_FAST(bit0)
		MLZ_LITERAL_FAST()

		/* match... */
		MLZ_GET_TYPE_FAST(type)
		if (type == 0) {
			/* tiny match */
			MLZ_GET_SHORT_LEN_FAST(len)
			MLZ_TINY_MATCH()
		} else if (type == 2) {
			/* short match */
			MLZ_SHORT_MATCH()
		} else if (type == 1) {
			/* short2 match */
			MLZ_GET_SHORT_LEN_FAST(len)
			MLZ_SHORT2_MATCH()
		} else {
			/* full match */
			MLZ_FULL_MATCH()
		}
		if (dist == 0) {
			/* literal run */
			MLZ_LITERAL_RUN()
			continue;
		}
		/* copy match */
		MLZ_COPY_MATCH()
	}

	while (sb < se) {
		MLZ_GET_BIT(bit0)
		MLZ_LITERAL()

		/* match... */
		MLZ_GET_TYPE(type)
		if (type == 0) {
			/* tiny match */
			MLZ_GET_SHORT_LEN(len)
			MLZ_TINY_MATCH_SAFE()
		} else if (type == 2) {
			/* short match */
			MLZ_SHORT_MATCH_SAFE()
		} else if (type == 1) {
			/* short2 match */
			MLZ_GET_SHORT_LEN(len)
			MLZ_SHORT2_MATCH_SAFE()
		} else {
			/* full match */
			MLZ_FULL_MATCH_SAFE()
		}
		if (dist == 0) {
			/* literal run */
			MLZ_LITERAL_RUN_SAFE()
			continue;
		}
		/* copy match */
		MLZ_COPY_MATCH()
	}

	/* using strict condition (full source buffer decoded) */
	return sb == se ? (size_t)(db - odb) : 0;
}

size_t
mlz_decompress_simple(
	void           *dst,
	size_t          dst_size,
	MLZ_CONST void *src,
	size_t          src_size
)
{
	return mlz_decompress(dst, dst_size, src, src_size, 0);
}

size_t
mlz_decompress_unsafe(
	void           *dst,
	MLZ_CONST void *src,
	size_t          src_size
)
{
	MLZ_INIT_DECOMPRESS()
	mlz_int dist = 0, len = 0;
	(void)dist;
	(void)len;

	MLZ_LOAD_ACCUM()

	while (sb < se) {
		if ((accum & MLZ_DEC_6BIT_MASK)) {
			MLZ_GET_BIT_FAST_NOACCUM(bit0)
			MLZ_LITERAL_UNSAFE()

			/* match... */
			MLZ_GET_TYPE_FAST_NOACCUM(type)
			if (type == 0) {
				/* tiny match */
				MLZ_GET_SHORT_LEN_FAST_NOACCUM(len)
				MLZ_TINY_MATCH()
			} else if (type == 2) {
				/* short match */
				MLZ_SHORT_MATCH()
			} else if (type == 1) {
				/* short2 match */
				MLZ_GET_SHORT_LEN_FAST_NOACCUM(len)
				MLZ_SHORT2_MATCH()
			} else {
				/* full match */
				MLZ_FULL_MATCH()
			}
			if (dist == 0) {
				/* literal run */
				MLZ_LITERAL_RUN_UNSAFE()
				continue;
			}
			/* copy match */
			MLZ_COPY_MATCH_UNSAFE()
			continue;
		}

		MLZ_GET_BIT_FAST(bit0)
		MLZ_LITERAL_UNSAFE()

		/* match... */
		MLZ_GET_TYPE_FAST(type)
		if (type == 0) {
			/* tiny match */
			MLZ_GET_SHORT_LEN_FAST(len)
			MLZ_TINY_MATCH()
		} else if (type == 2) {
			/* short match */
			MLZ_SHORT_MATCH()
		} else if (type == 1) {
			/* short2 match */
			MLZ_GET_SHORT_LEN_FAST(len)
			MLZ_SHORT2_MATCH()
		} else {
			/* full match */
			MLZ_FULL_MATCH()
		}
		if (dist == 0) {
			/* literal run  */
			MLZ_LITERAL_RUN_UNSAFE()
			continue;
		}
		/* copy match */
		MLZ_COPY_MATCH_UNSAFE()
	}
	return (size_t)(db - odb);
}

#undef MLZ_DEC_GUARD_MASK
#undef MLZ_DEC_0BIT_MASK
#undef MLZ_DEC_2BIT_MASK
#undef MLZ_DEC_3BIT_MASK
#undef MLZ_DEC_6BIT_MASK
#undef MLZ_LOAD_ACCUM
#undef MLZ_GET_BIT_FAST_NOACCUM
#undef MLZ_GET_BIT_CHECK
#undef MLZ_GET_BIT
#undef MLZ_GET_BIT_FAST
#undef MLZ_GET_TYPE_FAST_NOACCUM
#undef MLZ_GET_TYPE_COMMON
#undef MLZ_GET_TYPE
#undef MLZ_GET_TYPE_FAST
#undef MLZ_GET_SHORT_LEN_FAST_NOACCUM
#undef MLZ_GET_SHORT_LEN_COMMON
#undef MLZ_GET_SHORT_LEN
#undef MLZ_GET_SHORT_LEN_FAST
#undef MLZ_COPY_MATCH_UNSAFE
#undef MLZ_COPY_MATCH
#undef MLZ_LITCOPY
#undef MLZ_LITERAL_RUN
#undef MLZ_LITERAL_RUN_UNSAFE
#undef MLZ_LITERAL_RUN_SAFE
#undef MLZ_TINY_MATCH
#undef MLZ_TINY_MATCH_SAFE
#undef MLZ_SHORT_MATCH
#undef MLZ_SHORT_MATCH_SAFE
#undef MLZ_SHORT2_MATCH
#undef MLZ_SHORT2_MATCH_SAFE
#undef MLZ_FULL_MATCH
#undef MLZ_FULL_MATCH_SAFE
#undef MLZ_LITERAL_UNSAFE
#undef MLZ_LITERAL
#undef MLZ_LITERAL_FAST
#undef MLZ_INIT_DECOMPRESS
