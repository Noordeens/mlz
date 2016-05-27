mini-LZ library (mlz) v0.2b
(c) Martin Sedlak 2016

a simple, portable LZ-only codec written in plain C
distributed under Boost software license
(this means you don't have to include the license with binary distributions)

performance on Silesia corpus (single ~207MB block) on my i7:
compression (max level): 4-5MB/sec (thus very slow)
decompression: ~400-500MB/sec (heavily depends on compiler optimizer) (reasonably fast)
compressed size: ~71MB (worse than zlib but still relatively good)

(note: assuming MB = 1024^2 bytes)

interface:
checked or unchecked decompression (unchecked means doesn't check for buffer bounds overflow)
single-file unchecked decompression (mlz_dec_mini.h)
simple interface for streaming codec

simple example streaming commandline tool in mlzc.c (just define MLZ_COMMANDLINE_TOOL)

streaming compression can be multithreaded now (define MLZ_THREADS),
but speedup is lousy (~2.5x with 4 cores)

for basic block codec, the following files will do:
mlz_common.h
mlz_enc.c, mlz_enc.h for compression
mlz_dec.c, mlz_dec.h for decompression

see headers for detailed description

basic block interface:

mlz_compress_simple(
	destination_buffer,
	destination_buffer_size, /* =limit */
	source_buffer,
	source_buffer_size,
	compression_level /* use MLZ_COMPRESS_MAX or 10 for maximum compression) */
)

returns 0 on failure or size of compressed block

mlz_decompress_simple(
	destination_buffer,
	destionation_buffer_size, /* = limit */
	source_buffer,
	source_buffer_size
)

returns 0 on failure or size of decompressed block

streaming interface:
see headers and mlzc.c for detailed description

algorithm: plain lz77 with deep lazy matching
64kb "sliding dictionary", handling extreme cases
(long literal runs and extremely well compressed data)
matcher is simple hash-list (hash-chain)
data format is described in source files (using 24-bit bit accumulator)

have fun
