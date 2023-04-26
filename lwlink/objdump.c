/*
objdump.c
Copyright © 2009 William Astle

This file is part of LWLINK

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


A standalone program to dump an object file in a text form to stdout

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void read_lwobj16v0(unsigned char *filedata, long filesize);
char *program_name;

char *string_cleanup(char *sym)
{
	static char symbuf[4096];
	int optr = 0;
	while (*sym)
	{
		if (*sym < 33 || *sym > 126)
		{
			int c;
			symbuf[optr++] = '\\';
			c = *sym >> 4;
			c+= 48;
			if (c > 57)
				c += 7;
			symbuf[optr++] = c;
			c = *sym & 15;
			c += 48;
			if (c > 57)
				c += 7;
			symbuf[optr++] = c;
		}
		else if (*sym == '\\')
		{
			symbuf[optr++] = '\\';
			symbuf[optr++] = '\\';
		}
		else
		{
			symbuf[optr++] = *sym;
		}
		sym++;
	}
	symbuf[optr] = '\0';
	return symbuf;
}

/*
The logic of reading the entire file into memory is simple. All the symbol
names in the file are NUL terminated strings and can be used directly without
making additional copies.
*/
int main(int argc, char **argv)
{
	long size;
	FILE *f;
	long bread;
	unsigned char *filedata;

	program_name = argv[0];	
	if (argc != 2)
	{
		fprintf(stderr, "Must specify exactly one input file.\n");
		exit(1);
	}

	f = fopen(argv[1], "rb");
	if (!f)
	{
		fprintf(stderr, "Can't open file %s:", argv[1]);
		perror("");
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);
		
	filedata = lw_alloc(size);
		
	bread = fread(filedata, 1, size, f);
	if (bread < size)
	{
		fprintf(stderr, "Short read on file %s (%ld/%ld):", argv[1], bread, size);
		perror("");
		exit(1);
	}
			
	fclose(f);
		
	if (!memcmp(filedata, "LWOBJ16", 8))
	{
		// read v0 LWOBJ16 file
		read_lwobj16v0(filedata, size);
	}
	else
	{
		fprintf(stderr, "%s: unknown file format\n", argv[1]);
		exit(1);
	}
	exit(0);
}

// this macro is used to bail out if we run off the end of the file data
// while parsing - it keeps the code below cleaner
#define NEXTBYTE()	do { cc++; if (cc > filesize) { fprintf(stderr, "***invalid file format\n"); exit(1); } } while (0)
// this macro is used to refer to the current byte in the stream
#define CURBYTE()	(filedata[cc < filesize ? cc : filesize - 1])
// this one will leave the input pointer past the trailing NUL
#define CURSTR()	read_lwobj16v0_str(&cc, &filedata, filesize)
unsigned char *read_lwobj16v0_str(long *cc1, unsigned char **filedata1, long filesize)
{
	int cc = *cc1;
	unsigned char *filedata = *filedata1;
	unsigned char *fp;
	fp = &CURBYTE();
	while (CURBYTE())
		NEXTBYTE();
	NEXTBYTE();
	*cc1 = cc;
	*filedata1 = filedata;
	return fp;
}
// the function below can be switched to dealing with data coming from a
// source other than an in-memory byte pool by adjusting the input data
// in "fn" and the above two macros
void read_lwobj16v0(unsigned char *filedata, long filesize)
{
	unsigned char *fp;
	long cc;
	int val;
	int bss;
//	int constant;

	static char *opernames[] = {
		"?",
		"PLUS",
		"MINUS",
		"TIMES",
		"DIVIDE",
		"MOD",
		"INTDIV",
		"BWAND",
		"BWOR",
		"BWXOR",
		"AND",
		"OR",
		"NEG",
		"COM"
	};
	static const int numopers = 13;
		
	// start reading *after* the magic number
	cc = 8;
	
	while (1)
	{
		bss = 0;
//		constant = 0;
		
		// bail out if no more sections
		if (!(CURBYTE()))
			break;
		
		fp = CURSTR();
		
		printf("SECTION %s\n", fp);
		
		// read flags
		while (CURBYTE())
		{
			switch (CURBYTE())
			{
			case 0x01:
				printf("    FLAG: BSS\n");
				bss = 1;
				break;
			case 0x02:
				printf("    FLAG: CONSTANT\n");
//				constant = 1;
				break;
				
			default:
				printf("    FLAG: %02X (unknown)\n", CURBYTE());
				break;
			}
			NEXTBYTE();
		}
		// skip NUL terminating flags
		NEXTBYTE();
		
		printf("    Local symbols:\n");
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
			
			printf("        %s=%04X\n", string_cleanup((char *)fp), val);
			
		}
		// skip terminating NUL
		NEXTBYTE();

		printf("    Exported symbols\n");
				
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
			
			printf("        %s=%04X\n", string_cleanup((char *)fp), val);
		}
		// skip terminating NUL
		NEXTBYTE();
		
		// now parse the incomplete references and make a list of
		// external references that need resolution
		printf("    Incomplete references\n");
		while (CURBYTE())
		{
			printf("        (");
			// parse the expression
			while (CURBYTE())
			{
				int tt = CURBYTE();
				NEXTBYTE();
				switch (tt)
				{
				case 0x01:
					// 16 bit integer
					tt = CURBYTE() << 8;
					NEXTBYTE();
					tt |= CURBYTE();
					NEXTBYTE();
					// normalize for negatives...
					if (tt > 0x7fff)
						tt -= 0x10000;
					printf(" I16=%d", tt);
					break;
				
				case 0x02:
					// external symbol reference
					printf(" ES=%s", string_cleanup((char *)CURSTR()));
					break;
					
				case 0x03:
					// internal symbol reference
					printf(" IS=%s", string_cleanup((char *)CURSTR()));
					break;
				
				case 0x04:
					// operator
					if (CURBYTE() > 0 && CURBYTE() <= numopers)
						printf(" OP=%s", opernames[CURBYTE()]);
					else
						printf(" OP=?");
					NEXTBYTE();
					break;

				case 0x05:
					// section base reference (NULL internal reference is
					// the section base address
					printf(" SB");
					break;
				
				case 0xFF:
					// section flags
					printf(" FLAGS=%02X", CURBYTE());
					NEXTBYTE();
					break;
					
				default:
					printf(" ERR");
				}
			}
			// skip the NUL
			NEXTBYTE();
			
			// fetch the offset
			val = CURBYTE() << 8;
			NEXTBYTE();
			val |= CURBYTE() & 0xff;
			NEXTBYTE();
			printf(" ) @ %04X\n", val);
		}
		// skip the NUL terminating the relocations
		NEXTBYTE();
				
		// now set code location and size and verify that the file
		// contains data going to the end of the code (if !SECTION_BSS)
		val = CURBYTE() << 8;
		NEXTBYTE();
		val |= CURBYTE();
		NEXTBYTE();
		
		printf("    CODE %04X bytes", val);
		
		// skip the code if we're not in a BSS section
		if (!bss)
		{
			int i;
			for (i = 0; i < val; i++)
			{
				if (! (i % 16))
				{
					printf("\n    %04X ", i);
				}
				printf("%02X", CURBYTE());
				NEXTBYTE();
			}
		}
		printf("\n");
	}
}
