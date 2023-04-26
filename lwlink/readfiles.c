/*
readfiles.c
Copyright © 2009 William Astle

This file is part of LWLINK.

LWLINK is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Reads input files

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwlink.h"

void read_lwobj16v0(fileinfo_t *fn);
void read_lwar1v(fileinfo_t *fn);

/*
The logic of reading the entire file into memory is simple. All the symbol
names in the file are NUL terminated strings and can be used directly without
making additional copies.
*/
void read_file(fileinfo_t *fn)
{
	if (!memcmp(fn -> filedata, "LWOBJ16", 8))
		{
			// read v0 LWOBJ16 file
			read_lwobj16v0(fn);
		}
		else if (!memcmp(fn -> filedata, "LWAR1V", 6))
		{
			// archive file
			read_lwar1v(fn);
		}
		else
		{
			fprintf(stderr, "%s: unknown file format\n", fn -> filename);
			exit(1);
		}
}

void read_files(void)
{
	int i;
	long size;
	FILE *f;
	long bread;
	for (i = 0; i < ninputfiles; i++)
	{
		if (inputfiles[i] -> islib)
		{
			char *tf;
			char *sfn;
			int s;
			int j;
			
			f = NULL;
			
			if (inputfiles[i] -> filename[0] == ':')
			{
				// : suppresses the libfoo.a behaviour
				sfn = lw_strdup(inputfiles[i] -> filename + 1);
			}
			else
			{
				sfn = lw_alloc(strlen(inputfiles[i] -> filename) + 6);
				sprintf(sfn, "lib%s.a", inputfiles[i] -> filename);
			}
			
			for (j = 0; j < nlibdirs; j++)
			{
				if (libdirs[j][0] == '=')
				{
					// handle sysroot
					s = strlen(libdirs[j]) + 2 + strlen(sysroot) + strlen(sfn);
					tf = lw_alloc(s + 1);
					sprintf(tf, "%s/%s/%s", sysroot, libdirs[j] + 1, sfn);
				}
				else
				{
					s = strlen(libdirs[j]) + 1 + strlen(sfn);
					tf = lw_alloc(s + 1);
					sprintf(tf, "%s/%s", libdirs[j], sfn);
				}
				f = fopen(tf, "rb");
				if (!f)
				{
					free(tf);
					continue;
				}
				free(tf);
				break;
			}
			free(sfn);
			if (!f)
			{
				fprintf(stderr, "Can't open library: -l%s\n", inputfiles[i] -> filename);
				exit(1);
			}
		}
		else
		{
			f = fopen(inputfiles[i] -> filename, "rb");
			if (!f)
			{
				fprintf(stderr, "Can't open file %s:", inputfiles[i] -> filename);
				perror("");
				exit(1);
			}
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		rewind(f);
		
		inputfiles[i] -> filedata = lw_alloc(size);
		inputfiles[i] -> filesize = size;
		
		bread = fread(inputfiles[i] -> filedata, 1, size, f);
		if (bread < size)
		{
			fprintf(stderr, "Short read on file %s (%ld/%ld):", inputfiles[i] -> filename, bread, size);
			perror("");
			exit(1);
		}
			
		fclose(f);
		
		read_file(inputfiles[i]);
	}
}

// this macro is used to bail out if we run off the end of the file data
// while parsing - it keeps the code below cleaner
#define NEXTBYTE()	do { cc++; if (cc > fn -> filesize) { fprintf(stderr, "%s: invalid file format\n", fn -> filename); exit(1); } } while (0)
// this macro is used to refer to the current byte in the stream
#define CURBYTE()	(fn -> filedata[cc < fn -> filesize ? cc : fn -> filesize - 1])
// this one will leave the input pointer past the trailing NUL
#define CURSTR()	read_lwobj16v0_str(&cc, fn)
unsigned char *read_lwobj16v0_str(long *cc1, fileinfo_t *fn)
{
	int cc = *cc1;
	unsigned char *fp;
	fp = &CURBYTE();
	while (CURBYTE())
		NEXTBYTE();
	NEXTBYTE();
	*cc1 = cc;
	return fp;
}
// the function below can be switched to dealing with data coming from a
// source other than an in-memory byte pool by adjusting the input data
// in "fn" and the above two macros

void read_lwobj16v0(fileinfo_t *fn)
{
	unsigned char *fp;
	long cc;
	section_t *s;
	int val;
	symtab_t *se;
	
	// start reading *after* the magic number
	cc = 8;
	
	// init data
	fn -> sections = NULL;
	fn -> nsections = 0;

	while (1)
	{
//		NEXTBYTE();
		// bail out if no more sections
		if (!(CURBYTE()))
			break;
		
		fp = CURSTR();
		
		// we now have a section name in fp
		// create new section entry
		fn -> sections = lw_realloc(fn -> sections, sizeof(section_t) * (fn -> nsections + 1));
		s = &(fn -> sections[fn -> nsections]);
		fn -> nsections += 1;
		
		s -> localsyms = NULL;
		s -> flags = 0;
		s -> codesize = 0;
		s -> name = fp;
		s -> loadaddress = 0;
		s -> localsyms = NULL;
		s -> exportedsyms = NULL;
		s -> incompletes = NULL;
		s -> processed = 0;
		s -> file = fn;
		s -> afterbytes = NULL;
		s -> aftersize = 0;
		
		// read flags
		while (CURBYTE())
		{
			switch (CURBYTE())
			{
			case 0x01:
				s -> flags |= SECTION_BSS;
				break;

			case 0x02:
				s -> flags |= SECTION_CONST;
				break;
				
			default:
				fprintf(stderr, "%s (%s): unrecognized section flag %02X\n", fn -> filename, s -> name, (int)(CURBYTE()));
				exit(1);
			}
			NEXTBYTE();
		}
		// skip NUL terminating flags
		NEXTBYTE();
		
		// now parse the local symbol table
		while (CURBYTE())
		{
			fp = CURSTR();

			// fp is the symbol name
			val = (CURBYTE()) << 8;
			NEXTBYTE();
			val |= (CURBYTE());
			NEXTBYTE();
			// val is now the symbol value
			
			// create symbol table entry
			se = lw_alloc(sizeof(symtab_t));
			se -> next = s -> localsyms;
			s -> localsyms = se;
			se -> sym = fp;
			se -> offset = val;
		}
		// skip terminating NUL
		NEXTBYTE();
		
		// now parse the exported symbol table
		while (CURBYTE())
		{
			fp = CURSTR();

			// fp is the symbol name
			val = (CURBYTE()) << 8;
			NEXTBYTE();
			val |= (CURBYTE());
			NEXTBYTE();
			// val is now the symbol value
			
			// create symbol table entry
			se = lw_alloc(sizeof(symtab_t));
			se -> next = s -> exportedsyms;
			s -> exportedsyms = se;
			se -> sym = fp;
			se -> offset = val;
		}
		// skip terminating NUL
		NEXTBYTE();
		
		// now parse the incomplete references and make a list of
		// external references that need resolution
		while (CURBYTE())
		{
			reloc_t *rp;
			lw_expr_term_t *term;
			
			// we have a reference
			rp = lw_alloc(sizeof(reloc_t));
			rp -> next = s -> incompletes;
			s -> incompletes = rp;
			rp -> offset = 0;
			rp -> expr = lw_expr_stack_create();
			rp -> flags = RELOC_NORM;
			
			// parse the expression
			while (CURBYTE())
			{
				int tt = CURBYTE();
				NEXTBYTE();
				switch (tt)
				{
				case 0xFF:
					// a flag specifier
					tt = CURBYTE();
					rp -> flags = tt;
					NEXTBYTE();
					term = NULL;
					break;
					
				case 0x01:
					// 16 bit integer
					tt = CURBYTE() << 8;
					NEXTBYTE();
					tt |= CURBYTE();
					NEXTBYTE();
					// normalize for negatives...
					if (tt > 0x7fff)
						tt -= 0x10000;
					term = lw_expr_term_create_int(tt);
					break;
				
				case 0x02:
					// external symbol reference
					term = lw_expr_term_create_sym((char *)CURSTR(), 0);
					break;
					
				case 0x03:
					// internal symbol reference
					term = lw_expr_term_create_sym((char *)CURSTR(), 1);
					break;
				
				case 0x04:
					// operator
					term = lw_expr_term_create_oper(CURBYTE());
					NEXTBYTE();
					break;

				case 0x05:
					// section base reference (NULL internal reference is
					// the section base address
					term = lw_expr_term_create_sym(NULL, 1);
					break;
					
				default:
					fprintf(stderr, "%s (%s): bad relocation expression (%02X)\n", fn -> filename, s -> name, tt);
					exit(1);
				}
				if (term)
				{
					lw_expr_stack_push(rp -> expr, term);
					lw_expr_term_free(term);
				}
			}
			// skip the NUL
			NEXTBYTE();
			
			// fetch the offset
			rp -> offset = CURBYTE() << 8;
			NEXTBYTE();
			rp -> offset |= CURBYTE() & 0xff;
			NEXTBYTE();
		}
		// skip the NUL terminating the relocations
		NEXTBYTE();
				
		// now set code location and size and verify that the file
		// contains data going to the end of the code (if !SECTION_BSS)
		s -> codesize = CURBYTE() << 8;
		NEXTBYTE();
		s -> codesize |= CURBYTE();
		NEXTBYTE();
		
		s -> code = &(CURBYTE());
		
		// skip the code if we're not in a BSS section
		if (!(s -> flags & SECTION_BSS))
		{
			int i;
			for (i = 0; i < s -> codesize; i++)
				NEXTBYTE();
		}
	}
}

/*
Read an archive file - this will create a "sub" record and farm out the
parsing of the sub files to the regular file parsers

The archive file format consists of the 6 byte magic number followed by a
series of records as follows:

- NUL terminated file name
- 32 bit file length in big endian order
- the file data

An empty file name indicates the end of the file.

*/
void read_lwar1v(fileinfo_t *fn)
{
	int cc = 6;
	int flen;
	unsigned long l;
	for (;;)
	{
		if (cc >= fn -> filesize || !(fn -> filedata[cc]))
			return;

		for (l = cc; cc < fn -> filesize && fn -> filedata[cc]; cc++)
			/* do nothing */ ;

		cc++;

		if (cc >= fn -> filesize)
		{
			fprintf(stderr, "Malformed archive file %s.\n", fn -> filename);
			exit(1);
		}

		if (cc + 4 > fn -> filesize)
			return;

		flen = (fn -> filedata[cc++] << 24);
		flen |= (fn -> filedata[cc++] << 16);
		flen |= (fn -> filedata[cc++] << 8);
		flen |= (fn -> filedata[cc++]);

		if (flen == 0)
			return;
		
		if (cc + flen > fn -> filesize)
		{
			fprintf(stderr, "Malformed archive file %s.\n", fn -> filename);
			exit(1);
		}
		
		// add the "sub" input file
		fn -> subs = lw_realloc(fn -> subs, sizeof(fileinfo_t *) * (fn -> nsubs + 1));
		fn -> subs[fn -> nsubs] = lw_alloc(sizeof(fileinfo_t));
		memset(fn -> subs[fn -> nsubs], 0, sizeof(fileinfo_t));
		fn -> subs[fn -> nsubs] -> filedata = fn -> filedata + cc;
		fn -> subs[fn -> nsubs] -> filesize = flen;
		fn -> subs[fn -> nsubs] -> filename = lw_strdup((char *)(fn -> filedata + l));
		fn -> subs[fn -> nsubs] -> parent = fn;
		fn -> subs[fn -> nsubs] -> forced = fn -> forced;		
		read_file(fn -> subs[fn -> nsubs]);
		fn -> nsubs++;
		cc += flen;
	}
}
