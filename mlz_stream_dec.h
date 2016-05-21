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

#ifndef MLZ_STREAM_DEC_H
#define MLZ_STREAM_DEC_H

#include "mlz_stream_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO?: come up with multithreaded interface for streaming decompression of independent blocks */
typedef struct
{
	/* 64k previous context, nk block size, nkb unpack reserve */
	mlz_byte            *buffer;
	MLZ_CONST mlz_byte  *ptr;
	MLZ_CONST mlz_byte  *top;
	mlz_stream_params    params;
	mlz_uint             checksum;
	mlz_int              block_size;
	mlz_int              block_reserve;
	mlz_int              context_size;
	mlz_bool             is_eof;
	mlz_bool             first_block;
	/* first block cached? allows fast rewind early */
	mlz_bool             first_cached;
} mlz_in_stream;

/* simple adler32 checksum (Mark Adler's Fletcher variant) */
MLZ_API mlz_uint
mlz_adler32(
	MLZ_CONST void *buf,
	size_t size,
	mlz_uint checksum
);

/* simple block variant of the above */
MLZ_API mlz_uint
mlz_adler32_simple(
	MLZ_CONST void *buf,
	size_t size
);

/* returns new stream or MLZ_NULL on failure */
MLZ_API mlz_in_stream *
mlz_in_stream_open(
	MLZ_CONST mlz_stream_params *params
);

/* returns -1 on error, otherwise number of bytes read */
MLZ_API mlz_intptr
mlz_stream_read(
	mlz_in_stream *stream,
	void          *buf,
	mlz_intptr     size
);

/* returns MLZ_TRUE on success */
MLZ_API mlz_bool
mlz_in_stream_rewind(
	mlz_in_stream *stream
);

/* returns MLZ_TRUE on success */
MLZ_API mlz_bool
mlz_in_stream_close(
	mlz_in_stream *stream
);

#ifdef __cplusplus
}
#endif

#endif
