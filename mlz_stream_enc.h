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

#ifndef MLZ_STREAM_ENC_H
#define MLZ_STREAM_ENC_H

#include "mlz_stream_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mlz_matcher;

typedef struct
{
	/* for each parallel thread */
	struct mlz_matcher  *matchers[MLZ_MAX_THREADS];
	/* original unaligned buffer ptr */
	mlz_byte            *buffer_unaligned;
	/* 64k previous context, nk block size, nk output buffer; 1k aligned */
	mlz_byte            *buffer;
	/* points into buffer */
	mlz_byte            *out_buffer;
	mlz_stream_params    params;
	/* temporary output lengths in multi-threaded mode */
	size_t               out_lens[MLZ_MAX_THREADS];
	mlz_uint             checksum;
	mlz_int              ptr;
	mlz_int              block_size;
	mlz_int              context_size;
	mlz_int              level;
	mlz_int              num_threads;
	mlz_bool             first_block;
} mlz_out_stream;

/* level = compression level, 0 = fastest, 10 = best */
/* returns new stream or MLZ_NULL on failure */
MLZ_API mlz_out_stream *
mlz_out_stream_open(
	MLZ_CONST mlz_stream_params *params,
	mlz_int                      level
);

/* returns -1 on error, otherwise number of bytes read */
MLZ_API mlz_intptr
mlz_stream_write(
	mlz_out_stream *stream,
	MLZ_CONST void *buf,
	mlz_intptr      size
);

/* returns MLZ_TRUE on success */
MLZ_API mlz_bool
mlz_out_stream_close(
	mlz_out_stream *stream
);

#ifdef __cplusplus
}
#endif

#endif
