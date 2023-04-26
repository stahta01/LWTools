/*
lwar.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <lw_alloc.h>

#define __lwar_c_seen__
#include "lwar.h"

typedef struct
{
	FILE *f;
} arhandle_real;

int debug_level = 0;
int operation = 0;
int nfiles = 0;
char *archive_file = NULL;
int mergeflag = 0;
int filename_flag = 0;

char **files = NULL;

void add_file_name(char *fn)
{
	files = lw_realloc(files, sizeof(char *) * (nfiles + 1));
	files[nfiles] = fn;
	nfiles++;
}

char *get_file_name(char *fn)
{
	char *filename;
	if (filename_flag != 0)
	{
#ifdef _MSC_VER
		filename = strrchr(fn, '\\');
#else
		filename = strrchr(fn, '/');
#endif
		if (filename != NULL)
			return filename + 1;
	}
	return fn;
}
