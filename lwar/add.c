/*
add.c
Copyright © 2009 William Astle

This file is part of LWAR.

LWAR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Implements the program startup code

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwar.h"

void do_add(void)
{
	FILE *f;
	unsigned char buf[8];
	long l;
	int c;
	FILE *f2;
	int i;
	
	f = fopen(archive_file, "rb+");
	if (!f)
	{
		if (errno == ENOENT)
		{
			f = fopen(archive_file, "wb");
			if (f)
			{
				fputs("LWAR1V", f);
				goto doadd;
			}
		}
		perror("Cannot open archive file");
	}
	
	(void)(fread(buf, 1, 6, f) && 1);
	if (memcmp("LWAR1V", buf, 6))
	{
		fprintf(stderr, "%s is not a valid archive file.\n", archive_file);
		exit(1);
	}

	for (;;)
	{
		c = fgetc(f);
		if (c == EOF && ferror(f))
		{
			perror("Reading archive file");
			exit(1);
		}
		if (c == EOF)
			goto doadd;
		
		if (!c)
		{
			fseek(f, -1, SEEK_CUR);
			goto doadd;
		}
		
		// find the end of the file name
		while (c)
		{
			c = fgetc(f);
			if (c == EOF || ferror(f))
			{
				fprintf(stderr, "Bad archive file\n");
				exit(1);
			}
		}
		
		// get length of archive member
		l = 0;
		c = fgetc(f);
		l = c << 24;
		c = fgetc(f);
		l |= c << 16;
		c = fgetc(f);
		l |= c << 8;
		c = fgetc(f);
		l |= c;
		
		fseek(f, l, SEEK_CUR);
	}
	// back up to the NUL byte at the end of the file
	fseek(f, -1, SEEK_CUR);
doadd:
	for (i = 0; i < nfiles; i++)
	{
		f2 = fopen(files[i], "rb");
		if (!f2)
		{
			fprintf(stderr, "Cannot open file %s:", files[i]);
			perror("");
			exit(1);
		}
		(void)(fread(buf, 1, 6, f2) && 1);
		if (mergeflag && !memcmp("LWAR1V", buf, 6))
		{
			// add archive contents...
			for (;;)
			{
				c = fgetc(f2);
				if (c == EOF || ferror(f2))
				{
					perror("Reading input archive file");
					exit(1);
				}
				if (c == EOF)
					break;
		
				if (!c)
				{
					break;
				}
		
				// find the end of the file name
				while (c)
				{
					fputc(c, f);
					c = fgetc(f2);
					if (c == EOF || ferror(f))
					{
						fprintf(stderr, "Bad input archive file\n");
						exit(1);
					}
				}
				fputc(0, f);
				
				// get length of archive member
				l = 0;
				c = fgetc(f2);
				fputc(c, f);
				l = c << 24;
				c = fgetc(f2);
				fputc(c, f);
				l |= c << 16;
				c = fgetc(f2);
				fputc(c, f);
				l |= c << 8;
				c = fgetc(f2);
				fputc(c, f);
				l |= c;
		
				while (l)
				{
					c = fgetc(f2);
					fputc(c, f);
					l--;
				}
			}
			
			fclose(f2);
			continue;
		}
		fseek(f2, 0, SEEK_END);
		l = ftell(f2);
		fseek(f2, 0, SEEK_SET);
		fputs(get_file_name(files[i]), f);
		fputc(0, f);
		fputc(l >> 24, f);
		fputc((l >> 16) & 0xff, f);
		fputc((l >> 8) & 0xff, f);
		fputc(l & 0xff, f);
		while (l)
		{
			c = fgetc(f2);
			fputc(c, f);
			l--;
		}
	}
	
	// flag end of file
	fputc(0, f);	
}
