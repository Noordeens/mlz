/*
   mini-LZ library (mlz)
   Martin Sedlak 2016

   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/

/* define this in your project to build this tool */
#ifdef MLZ_COMMANDLINE_TOOL

#ifdef _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS
#endif

/* very simple command line tool, demonstrates streaming API */
/* FIXME: clean up                                           */

#include "mlz_stream_enc.h"
#include "mlz_stream_dec.h"
#include "mlz_enc.h"
#include "mlz_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MLZ_CONST char *in_file  = MLZ_NULL;
static MLZ_CONST char *out_file = MLZ_NULL;
static int level                = MLZ_LEVEL_MAX;
static mlz_bool compress        = MLZ_TRUE;
static mlz_bool force           = MLZ_FALSE;
/* test archive */
static mlz_bool test            = MLZ_FALSE;
/* use independent blocks */
static mlz_bool independent     = MLZ_FALSE;
/* use compressed block checksum */
static mlz_bool block_checksum  = MLZ_FALSE;
static mlz_bool show_ver        = MLZ_FALSE;
static mlz_int  block_size      = 65536;

static char buffer[65536];

static int parse_args(int argc, char **argv)
{
	int i;
	for (i=1; i<argc; i++) {
		if (argv[i][0] != '-') {
			if (!in_file)
				in_file = argv[i];
			else if (!out_file)
				out_file = argv[i];
			else {
				fprintf(stderr, "too many files\n");
				return 1;
			}
			continue;
		}
		/* assume arg */
		if (argv[i][1] >= '0' && argv[i][1] <= '9') {
			long alevel = strtol(argv[i]+1, MLZ_NULL, 10);
			level = alevel < 0 ? 0 : (alevel > MLZ_LEVEL_MAX ? MLZ_LEVEL_MAX : (mlz_int)alevel);
		} else if (strcmp(argv[i], "--best") == 0 || strcmp(argv[i], "--max") == 0) {
			level = MLZ_LEVEL_MAX;
		} else if (strcmp(argv[i], "--fastest") == 0) {
			level = MLZ_LEVEL_MAX;
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
			show_ver = MLZ_TRUE;
		} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compress") == 0) {
			compress = MLZ_TRUE;
		} else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--decompress") == 0) {
			compress = MLZ_FALSE;
		} else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
			force = MLZ_TRUE;
		} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
			test = MLZ_TRUE;
		} else if (strcmp(argv[i], "-bc") == 0 || strcmp(argv[i], "--block-checksum") == 0) {
			block_checksum = MLZ_TRUE;
		} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--independent") == 0) {
			independent = MLZ_TRUE;
		} else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--block") == 0) {
			long ablock_size;
			if (i+1 >= argc) {
				fprintf(stderr, "block size expects argument\n");
				return 2;
			}
			ablock_size = strtol(argv[++i], MLZ_NULL, 10);
			ablock_size *= 1024;
			if (ablock_size < MLZ_MIN_BLOCK_SIZE || ablock_size > MLZ_MAX_BLOCK_SIZE) {
				fprintf(stderr, "invalid block size: %ld\n", ablock_size);
				return 2;
			}
			block_size = (mlz_int)ablock_size;
		} else {
			fprintf(stderr, "invalid argument: `%s'\n", argv[i]);
			return 2;
		}
	}
	if (!in_file && test) {
		fprintf(stderr, "please specify input file\n");
		return 3;
	}
	if (!out_file && !test) {
		fprintf(stderr, "please specify both input and output file\n");
		return 3;
	}
	if (test)
		compress = MLZ_FALSE;
	return 0;
}

static void help(void)
{
	printf("usage: mlzc [args] <infile> <outfile>\n");
	printf("       -c or --compress   compress   in->out\n");
	printf("       -d or --decompress decompress in->out\n");
	printf("       -0 to -10         select compression level\n");
	printf("       --best or --max   maximum compression (default)\n");
	printf("       --fastest         fastest compression (hurts ratio a lot)\n");
	printf("       -f or --force     force to overwrite outfile\n");
	printf("       -t or --test      test compressed infile\n");
	printf("       -b or --block <n> set block size in kb, default is 64\n");
	printf("       -bc or --block-checksum include compressed block checksum\n");
	printf("       -v or --version   show library version\n");
	printf("       -i or --independent use independent blocks\n");
	printf("           when using independent blocks, it's recommended\n");
	printf("           to use block size of 128k or more\n");
}

static int process(void)
{
	FILE *fin, *fout = MLZ_NULL;

	if (!test && !force) {
		FILE *ftest = fopen(out_file, "rb");
		if (ftest) {
			char buf[16];
			fclose(ftest);
			printf("output file `%s' already exists.\noverwrite? (y/n)\n", out_file);
			fgets(buf, sizeof(buf), stdin);
			if (buf[0] != 'y') {
				printf("aborted\n");
				return 0;
			}
		}
	}

	fin = fopen(in_file, "rb");
	if (!fin) {
		fprintf(stderr, "cannot open infile: `%s'\n", in_file);
		return 4;
	}

	if (!test) {
		fout = fopen(out_file, "wb");
		if (!fout) {
			fclose(fin);
			fprintf(stderr, "cannot create outfile: `%s'\n", out_file);
			return 5;
		}
	}

	if (compress) {
		mlz_out_stream   *outs;
		mlz_stream_params par  = mlz_default_stream_params;
		par.handle             = fout;
		par.independent_blocks = independent;
		par.block_size         = block_size;
		if (block_checksum)
			par.block_checksum = mlz_adler32_simple;

		outs = mlz_out_stream_open(&par, level);
		if (!outs) {
			fclose(fin);
			if (fout)
				fclose(fout);
			fprintf(stderr, "couldn't create out stream\n");
			return 6;
		}
		for (;;) {
			size_t nread = fread(buffer, 1, sizeof(buffer), fin);
			if (!nread)
				break;
			if (mlz_stream_write(outs, buffer, nread) != (mlz_intptr)nread) {
				mlz_out_stream_close(outs);
				fclose(fin);
				if (fout)
					fclose(fout);
				fprintf(stderr, "failed to write out stream\n");
				return 7;
			}
		}
		if (!mlz_out_stream_close(outs)) {
			fclose(fin);
			if (fout)
				fclose(fout);
			fprintf(stderr, "failed to close out stream\n");
			return 8;
		}
	} else {
		mlz_in_stream    *ins;
		mlz_stream_params par  = mlz_default_stream_params;
		par.handle             = fin;
		par.independent_blocks = independent;
		par.block_size         = block_size;
		if (block_checksum)
			par.block_checksum = mlz_adler32_simple;

		ins = mlz_in_stream_open(&par);
		if (!ins) {
			fclose(fin);
			if (fout)
				fclose(fout);
			fprintf(stderr, "couldn't create in stream\n");
			return 9;
		}
		for (;;) {
			mlz_intptr nread = mlz_stream_read(ins, buffer, sizeof(buffer));
			if (nread < 0) {
				mlz_in_stream_close(ins);
				fclose(fin);
				if (fout)
					fclose(fout);
				fprintf(stderr, "failed to read in stream\n");
				return 10;
			}
			if (!nread)
				break;
			if (!test && fout && (mlz_intptr)fwrite(buffer, 1, nread, fout) != nread) {
				mlz_in_stream_close(ins);
				fclose(fin);
				if (fout)
					fclose(fout);
				fprintf(stderr, "failed to write out file\n");
				return 11;
			}
		}
		if (!mlz_in_stream_close(ins)) {
			fclose(fin);
			if (fout)
				fclose(fout);
			fprintf(stderr, "failed to close in stream\n");
			return 12;
		}
	}
	if (fout)
		fclose(fout);
	fclose(fin);
	return 0;
}

int main(int argc, char **argv)
{
	int err = parse_args(argc, argv);
	if (show_ver)
		printf("mlz v" MLZ_VERSION "\n");
	if (err) {
		help();
		return err;
	}
	return process();
}

#endif
