/*
unicorns.c

Copyright © 2012 William Astle

This file is part of LWTOOLS.

LWTOOLS is free software: you can redistribute it and/or modify it under the
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

/*
This adds output to lwasm that is suitable for IDEs and other tools
that are interesting in the doings of the assembler.
*/

#include <stdio.h>
#include <string.h>

#include "input.h"
#include "lwasm.h"
#include "lw_alloc.h"

void do_list(asmstate_t *as);

static void print_urlencoding(FILE *stream, const char *string)
{
	for ( ; *string; string++)
	{
		if (*string < 33 || *string > 126 || strchr("$&+,/:;=?@\"<>#%{}|\\^~[]`", *string))
		{
			fprintf(stream, "%%%02X", *string);
		}
		else
		{
			fputc(*string, stream);
		}
	}
}

static void show_unicorn_error(FILE *fp, line_t *l, lwasm_error_t *ee, const char *tag)
{
	fprintf(fp, "%s: lineno=%d,filename=", tag, l -> lineno);
	print_urlencoding(fp, l -> linespec);
	fputs(",message=", fp);
	print_urlencoding(fp, ee -> mess);
	if (ee -> charpos > 0)
		fprintf(fp, ",col=%d", ee -> charpos);
	fputc('\n', fp);
}

void lwasm_do_unicorns(asmstate_t *as)
{
	struct ifl *ifl;
	macrotab_t *me;
	structtab_t *se;
	int i;
	line_t *l;
	lwasm_error_t *ee;
			
	/* output file list */	
	for (ifl = ifl_head; ifl; ifl = ifl -> next)
	{
		fputs("RESOURCE: type=file,filename=", stdout);
		print_urlencoding(stdout, ifl -> fn);
		fputc('\n', stdout);
	}
	
	/* output macro list */
	for (me = as -> macros; me; me = me -> next)
	{
		fprintf(stdout, "RESOURCE: type=macro,name=%s,lineno=%d,filename=", me -> name, me -> definedat -> lineno);
		print_urlencoding(stdout, me -> definedat -> linespec);
		fputs(",flags=", stdout);
		if (me -> flags & macro_noexpand)
			fputs("noexpand", stdout);
		fputs(",def=", stdout);
		for (i = 0; i < me -> numlines; i++)
		{
			if (i)
				fputc(';', stdout);
			print_urlencoding(stdout, me -> lines[i]);
		}
		fputc('\n', stdout);
	}
	
	/* output structure list */
	for (se = as -> structs; se; se = se -> next)
	{
		fprintf(stdout, "RESOURCE: type=struct,name=%s,lineno=%d,filename=", se -> name, se -> definedat -> lineno);
		print_urlencoding(stdout, se -> definedat -> linespec);
		fputc('\n', stdout);
	}

	/* output error and warning lists */
	for (l = as -> line_head; l; l = l -> next)
	{
		if (l -> err)
		{
			for (ee = l -> err; ee; ee = ee -> next)
			{
				show_unicorn_error(stdout, l, ee, "ERROR");
			}
		}
		
		if (l -> warn)
		{
			for (ee = l -> warn; ee; ee = ee -> next)
			{
				show_unicorn_error(stdout, l, ee, "WARNING");
			}
		}
	}
	
	fprintf(stdout, "UNICORNSAWAY:\n");
}
