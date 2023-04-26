/*
lwlink.c
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



*/

#define __lwlink_c_seen__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwlink.h"

int debug_level = 0;
int outformat = OUTPUT_DECB;
char *outfile = NULL;
char *scriptfile = NULL;
int symerr = 0;
char *map_file = NULL;

fileinfo_t **inputfiles = NULL;
int ninputfiles = 0;

int nlibdirs = 0;
char **libdirs = NULL;

int nscriptls = 0;
char **scriptls = NULL;

char *sysroot = "/";

char *entrysym = NULL;

void add_input_file(char *fn)
{
	inputfiles = lw_realloc(inputfiles, sizeof(fileinfo_t *) * (ninputfiles + 1));
	inputfiles[ninputfiles] = lw_alloc(sizeof(fileinfo_t));
	memset(inputfiles[ninputfiles], 0, sizeof(fileinfo_t));
	inputfiles[ninputfiles] -> forced = 1;
	inputfiles[ninputfiles++] -> filename = lw_strdup(fn);
}

void add_input_library(char *libname)
{
	inputfiles = lw_realloc(inputfiles, sizeof(fileinfo_t *) * (ninputfiles + 1));
	inputfiles[ninputfiles] = lw_alloc(sizeof(fileinfo_t));
	memset(inputfiles[ninputfiles], 0, sizeof(fileinfo_t));
	inputfiles[ninputfiles] -> islib = 1;
	inputfiles[ninputfiles] -> forced = 0;
	inputfiles[ninputfiles++] -> filename = lw_strdup(libname);	
}

void add_library_search(char *libdir)
{
	libdirs = lw_realloc(libdirs, sizeof(char*) * (nlibdirs + 1));
	libdirs[nlibdirs] = lw_strdup(libdir);
	nlibdirs++;
}

void add_section_base(char *sectspec)
{
	char *base;
	int baseaddr;
	char *t;
	int l;
	
	base = strchr(sectspec, '=');
	if (!base)
	{
		l = strlen(sectspec);
		baseaddr = 0;
	}
	else
	{
		baseaddr = strtol(base + 1, NULL, 16);
		l = base - sectspec;
		*base = '\0';
	}
	baseaddr = baseaddr & 0xffff;
	
	t = lw_alloc(l + 25);
	sprintf(t, "section %s load %04X", sectspec, baseaddr);
	if (base)
		*base = '=';
	
	scriptls = lw_realloc(scriptls, sizeof(char *) * (nscriptls + 1));
	scriptls[nscriptls++] = t;
}

char *sanitize_symbol(char *symbol)
{
	static char symbuf[2048];
	char *sym = symbol;
	char *tp = symbuf;
	
	for (; *sym; sym++)
	{
		int c1 = *sym;
		if (c1 == '\\')
		{
			*tp++ = '\\';
			*tp++ = '\\';
			continue;
		}
		if (c1 < 32 || c1 > 126)
		{
			int c;
			*tp++ = '\\';
			c = c1 >> 4;
			c += 48;
			if (c > 57)
				c += 7;
			*tp++ = c;
			c = c1 & 15;
			c += 48;
			if (c > 57)
				c += 7;
			*tp++ = c;
			continue;
		}
		*tp++ = c1;
	}
	*tp = 0;
	return symbuf;
}
