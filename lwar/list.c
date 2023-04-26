/*
list.c
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

void do_list(void)
{
	FILE *f;
	char buf[8];
	long l;
	int c;
		
	f = fopen(archive_file, "rb");
	if (!f)
	{
		perror("Opening archive file");
		exit(1);
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
		if (ferror(f))
		{
			perror("Reading archive file");
			exit(1);
		}
		if (c == EOF)
			return;
		
		
		// find the end of the file name
		if (!c)
			return;

		while (c)
		{
			putchar(c);
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
		printf(": %04lx bytes\n", l);
		fseek(f, l, SEEK_CUR);
	}
}
