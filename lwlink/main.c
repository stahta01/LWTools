/*
main.c
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


Implements the program startup code

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_cmdline.h>

#include <version.h>

#include "lwlink.h"

#ifdef _MSC_VER
#include <lw_win.h>	// windows build
#else
#include <unistd.h>
#endif

char *program_name;

// command line option handling
#define PROGVER "lwlink from " PACKAGE_STRING

static int parse_opts(int key, char *arg, void *state)
{
	switch (key)
	{
	case 'o':
		// output
		outfile = arg;
		break;
	
	case 's':
		// script file
		scriptfile = arg;
		break;

	case 'd':
		// debug
		debug_level++;
		break;

	case 'e':
		// entry symbol
		entrysym = arg;
		break;
	
	case 'b':
		// decb output
		outformat = OUTPUT_DECB;
		break;
	
	case 'r':
		// raw binary output
		outformat = OUTPUT_RAW;
		break;
	
	case 'f':
		// output format
		if (!strcasecmp(arg, "decb"))
			outformat = OUTPUT_DECB;
		else if (!strcasecmp(arg, "raw"))
			outformat = OUTPUT_RAW;
		else if (!strcasecmp(arg, "raw2"))
			outformat = OUTPUT_RAW2;
		else if (!strcasecmp(arg, "lwex0") || !strcasecmp(arg, "lwex"))
			outformat = OUTPUT_LWEX0;
		else if (!strcasecmp(arg, "os9"))
			outformat = OUTPUT_OS9;
		else if (!strcasecmp(arg, "srec"))
			outformat = OUTPUT_SREC;
		else if (!strcasecmp(arg, "ihex"))
			outformat = OUTPUT_IHEX;
		else
		{
			fprintf(stderr, "Invalid output format: %s\n", arg);
			exit(1);
		}
		break;
	case lw_cmdline_key_end:
		// done; sanity check
		if (!outfile)
			outfile = "a.out";
		break;
	
	case 'l':
		add_input_library(arg);
		break;
	
	case 'L':
		add_library_search(arg);
		break;
	
	case 0x100:
		add_section_base(arg);
		break;
	
	case 0x101:
		sysroot = arg;
		break;
	
	case 'm':
		map_file = arg;
		break;
	
	case lw_cmdline_key_arg:
		add_input_file(arg);
		break;
		
	default:
		return lw_cmdline_err_unknown;
	}
	return 0;
}

static struct lw_cmdline_options options[] =
{
	{ "output",		'o',	"FILE",	0,
				"Output to FILE"},
	{ "debug",		'd',	0,		0,
				"Set debug mode"},
	{ "format",		'f',	"TYPE",	0,
				"Select output format: decb, raw, lwex, os9, srec, ihex"},
	{ "decb",		'b',	0,		0,
				"Generate DECB .bin format output, equivalent of --format=decb"},
	{ "raw",		'r',	0,		0,
				"Generate raw binary format output, equivalent of --format=raw"},
	{ "script",		's',	"FILE",		0,
				"Specify the linking script (overrides the built in defaults)"},
	{ "library",	'l',	"LIBSPEC",	0,
				"Read library libLIBSPEC.a from the search path" },
	{ "library-path", 'L',	"DIR",		0,
				"Add DIR to the library search path" },
	{ "section-base", 0x100,	"SECT=BASE",	0,
				"Load section SECT at BASE" },
	{ "entry", 		'e',		"SYM",			0,
				"Specify SYM as program entry point" },
	{ "sysroot", 0x101,	"DIR",	0,
				"Specify the path to replace an initial = with in library paths" },
	{ "map",		'm',	"FILE",		0,
				"Output informaiton about the link" },
	{ 0 }
};

static struct lw_cmdline_parser cmdline_parser =
{
	options,
	parse_opts,
	"INPUTFILE ...",
	"lwlink, a HD6309 and MC6809 cross-linker",
	PROGVER
};

extern void read_files(void);
extern void setup_script(void);
extern void resolve_files(void);
extern void resolve_sections(void);
extern void generate_symbols(void);
extern void resolve_references(void);
extern void resolve_padding(void);
extern void do_output(void);
extern void display_map(void);

// main function; parse command line, set up assembler state, and run the
// assembler on the first file
int main(int argc, char **argv)
{
	program_name = argv[0];

	if (lw_cmdline_parse(&cmdline_parser, argc, argv, 0, 0, NULL) != 0)
	{
		// bail if parsing failed
		exit(1);
	}
	if (ninputfiles == 0)
	{
		fprintf(stderr, "No input files\n");
		exit(1);
	}

	unlink(outfile);

	// handle the linker script
	setup_script();

	// read the input files
	read_files();

	// trace unresolved references and determine which non-forced
	// objects must be included
	resolve_files();
	
	// resolve section bases and section order
	resolve_sections();

	// generate symbols
	generate_symbols();
	
	// resolve incomplete references
	resolve_references();

	// resolve section padding bits
	resolve_padding();
	
	// do the actual output
	do_output();

	// display/output the link map
	if (map_file)
		display_map();

	exit(0);
}
