/*
lwcc/cpp-main.c

Copyright © 2013 William Astle

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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_stringlist.h>
#include <lw_cmdline.h>
#include <lw_string.h>

#include <version.h>

#include "cpp.h"
#include "tree.h"

node_t *process_file(const char *);
static void do_error(const char *f, ...);
extern node_t *parse_program(struct preproc_info *pp);
extern void generate_code(node_t *n, FILE *of);

node_t *program_tree = NULL;

/* command line option handling */
#define PROGVER "lwcc-cc from " PACKAGE_STRING
char *program_name;

/* input files */
lw_stringlist_t input_files;
lw_stringlist_t includedirs;
lw_stringlist_t sysincludedirs;
lw_stringlist_t macrolist;

/* various flags */
int trigraphs = 0;
char *output_file = NULL;
FILE *output_fp = NULL;

static struct lw_cmdline_options options[] =
{
	{ "output",		'o',	"FILE",		0,							"Output to FILE"},
	{ "include",	'i',	"FILE",		0,							"Pre-include FILE" },
	{ "includedir",	'I',	"PATH",		0,							"Add entry to the user include path" },
	{ "sincludedir", 'S',	"PATH",		0,							"Add entry to the system include path" },
	{ "define", 	'D',	"SYM[=VAL]",0, 							"Automatically define SYM to be VAL (or 1)"},
	{ "trigraphs",	0x100,	NULL,		0,							"Enable interpretation of trigraphs" },
	{ 0 }
};

static int parse_opts(int key, char *arg, void *state)
{
	switch (key)
	{
	case 'o':
		if (output_file)
			do_error("Output file specified more than once.");
		output_file = arg;
		break;
		
	case 0x100:
		trigraphs = 1;
		break;

	case 'I':
		lw_stringlist_addstring(includedirs, arg);
		break;
	
	case 'S':
		lw_stringlist_addstring(sysincludedirs, arg);
		break;

	case 'D':
		lw_stringlist_addstring(macrolist, arg);
		break;
		
	case lw_cmdline_key_end:
		break;
	
	case lw_cmdline_key_arg:
		lw_stringlist_addstring(input_files, arg);
		break;
		
	default:
		return lw_cmdline_err_unknown;
	}
	return 0;
}

static struct lw_cmdline_parser cmdline_parser =
{
	options,
	parse_opts,
	"INPUTFILE",
	"lwcc-cc - C compiler for lwcc",
	PROGVER
};

int main(int argc, char **argv)
{
	program_name = argv[0];
	int retval = 0;
	node_t *n;
	
	input_files = lw_stringlist_create();
	includedirs = lw_stringlist_create();
	sysincludedirs = lw_stringlist_create();
	macrolist = lw_stringlist_create();
	
	/* parse command line arguments */	
	lw_cmdline_parse(&cmdline_parser, argc, argv, 0, 0, NULL);

	/* set up output file */
	if (output_file == NULL || strcmp(output_file, "-") == 0)
	{
		output_fp = stdout;
	}
	else
	{
		output_fp = fopen(output_file, "wb");
		if (output_fp == NULL)
		{
			do_error("Failed to create output file %s: %s", output_file, strerror(errno)); 
		}
	}
	
	program_tree = node_create(NODE_PROGRAM);	
	
	if (lw_stringlist_nstrings(input_files) == 0)
	{
		/* if no input files, work on stdin */
		n = process_file("-");
		if (!n)
			retval = 1;
		else
			node_addchild(program_tree, n);
	}
	else
	{
		char *s;
		lw_stringlist_reset(input_files);
		for (s = lw_stringlist_current(input_files); s; s = lw_stringlist_next(input_files))
		{
			n = process_file(s);
			if (!n)
				retval = 1;
			if (retval != 0)
				break;
			node_addchild(program_tree, n);
		}
	}
	lw_stringlist_destroy(input_files);
	lw_stringlist_destroy(includedirs);
	lw_stringlist_destroy(sysincludedirs);
	lw_stringlist_destroy(macrolist);
	
	node_display(program_tree, stdout);
	
	// generate output
	generate_code(program_tree, output_fp);
	
	node_destroy(program_tree);
	exit(retval);
}

node_t *process_file(const char *fn)
{
	struct preproc_info *pp;
	char *tstr;
	node_t *n;
			
	pp = preproc_init(fn);
	if (!pp)
		return NULL;

	/* set up the include paths */
	lw_stringlist_reset(includedirs);
	for (tstr = lw_stringlist_current(includedirs); tstr; tstr = lw_stringlist_next(includedirs))
	{
		preproc_add_include(pp, tstr, 0);
	}

	lw_stringlist_reset(sysincludedirs);
	for (tstr = lw_stringlist_current(sysincludedirs); tstr; tstr = lw_stringlist_next(sysincludedirs))
	{
		preproc_add_include(pp, tstr, 1);
	}

	/* set up pre-defined macros */
	lw_stringlist_reset(macrolist);
	for (tstr = lw_stringlist_current(macrolist); tstr; tstr = lw_stringlist_next(macrolist))
	{
		preproc_add_macro(pp, tstr);
	}

	n = parse_program(pp);
	preproc_finish(pp);
	
	return n;
}

static void do_error(const char *f, ...)
{
	va_list args;
	va_start(args, f);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, f, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
