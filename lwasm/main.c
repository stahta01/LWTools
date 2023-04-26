/*
main.c

Copyright © 2010 William Astle

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_stringlist.h>
#include <lw_expr.h>
#include <lw_cmdline.h>

#include "lwasm.h"
#include "input.h"

void lwasm_do_unicorns(asmstate_t *as);

int parse_pragma_string(asmstate_t *as, char *str, int ignoreerr);

/* command line option handling */
#define PROGVER "lwasm from " PACKAGE_STRING
char *program_name;

static struct lw_cmdline_options options[] =
{
	{ "output",		'o',	"FILE",		0,							"Output to FILE"},
	{ "debug",		'd',	"LEVEL",	lw_cmdline_opt_optional,	"Set debug mode"},
	{ "format",		'f',	"TYPE",		0,							"Select output format: decb, basic, raw, obj, os9, ihex, srec"},
	{ "list",		'l',	"FILE",		lw_cmdline_opt_optional,	"Generate list [to FILE]"},
	{ "list-nofiles", 0x104, 0,			0,							"Omit file names in list output"},
	{ "symbols",	's',	0,			lw_cmdline_opt_optional,	"Generate symbol list in listing, no effect without --list"},
	{ "symbols-nolocals", 0x103,	0,	lw_cmdline_opt_optional,	"Same as --symbols but with local labels ignored"},
	{ "symbol-dump", 0x106, "FILE",		lw_cmdline_opt_optional,	"Dump global symbol table in assembly format" },
	{ "tabs",		't',	"WIDTH",	0,							"Set tab spacing in listing (0=don't expand tabs)" },
	{ "map",		'm',	"FILE",		lw_cmdline_opt_optional,	"Generate map [to FILE]"},
	{ "decb",		'b',	0,			0,							"Generate DECB .bin format output, equivalent of --format=decb"},
	{ "raw",		'r',	0,			0,							"Generate raw binary format output, equivalent of --format=raw"},
	{ "obj",		0x100,	0,			0,							"Generate proprietary object file format for later linking, equivalent of --format=obj" },
	{ "depend",		0x101,	0,			0,							"Output a dependency list to stdout; do not do any actual output though assembly is completed as usual" },
	{ "dependnoerr", 0x102,	0,			0,							"Output a dependency list to stdout; do not do any actual output though assembly is completed as usual; don't bail on missing include files" },
	{ "pragma",		'p',	"PRAGMA",	0,							"Set an assembler pragma to any value understood by the \"pragma\" pseudo op"},
	{ "6809",		'9',	0,			0,							"Set assembler to 6809 only mode" },
	{ "6309",		'3',	0,			0,							"Set assembler to 6309 mode (default)" },
	{ "includedir",	'I',	"PATH",		0,							"Add entry to include path" },
	{ "define",		'D',	"SYM[=VAL]",0,							"Automatically define SYM to be VAL (or 1)"},
	{ "preprocess",	'P',	0,			0,							"Preprocess macros and conditionals and output revised source to stdout" },
	{ "unicorns",	0x142,	0,			0,							"Add sooper sekrit sauce"},
	{ "6800compat",	0x200,	0,			0,							"Enable 6800 compatibility instructions, equivalent to --pragma=6800compat" },
	{ "no-output",  0x105,  0,          0,                          "Inhibit creation of output file" },
	{ 0 }
};


static int parse_opts(int key, char *arg, void *state)
{
	asmstate_t *as = state;

	switch (key)
	{
	case 'I':
		lw_stringlist_addstring(as -> include_list, arg);
		break;
	
	case 'D':
	{
		char *offs;
		int val = 1;
		lw_expr_t te;
		
		if ((offs = strchr(arg, '=')))
		{
			*offs = '\0';
			val = strtol(offs + 1, NULL, 0);
		}
		
		/* register global symbol */
		te = lw_expr_build(lw_expr_type_int, val);
		register_symbol(as, NULL, arg, te, symbol_flag_nocheck | symbol_flag_set);
		lw_expr_destroy(te);
		
		if (offs)
			*offs = '=';
		break;
	}
	case 'o':
		if (as -> output_file)
			lw_free(as -> output_file);
		as -> output_file = lw_strdup(arg);
		as -> flags &= ~FLAG_NOOUT;
		break;

	case 0x105:
		as -> flags |= FLAG_NOOUT;
		break;

	case 0x106:
		if (as -> symbol_dump_file)
			lw_free(as -> symbol_dump_file);
		if (!arg)
			as -> symbol_dump_file = lw_strdup("-");
		else
			as -> symbol_dump_file = lw_strdup(arg);
		as -> flags |= FLAG_SYMDUMP;
		break;

	case 'd':
#ifdef LWASM_NODEBUG
		fprintf(stderr, "This binary has been built without debugging message support\n");
#else
		if (!arg)
			as -> debug_level = 50;
		else
			as -> debug_level = atoi(arg);
#endif
		break;

	case 't':
		if (arg)
			as -> tabwidth = atoi(arg);
		break;

	case 'l':
		if (as -> list_file)
			lw_free(as -> list_file);
		if (!arg)
			as -> list_file = lw_strdup("-");
		else
			as -> list_file = lw_strdup(arg);
		as -> flags |= FLAG_LIST;
		break;

	case 'm':
		if (as -> map_file)
			lw_free(as -> map_file);
		if (!arg)
			as -> map_file = lw_strdup("-");
		else
			as -> map_file = lw_strdup(arg);
		as -> flags |= FLAG_MAP;
		break;

	case 's':
		as -> flags |= FLAG_SYMBOLS;
		break;

	case 0x103:
		as -> flags |= FLAG_SYMBOLS | FLAG_SYMBOLS_NOLOCALS;
		break;

	case 0x104:
		as -> listnofile = 1;
		break;

	case 'b':
		as -> output_format = OUTPUT_DECB;
		break;

	case 'r':
		as -> output_format = OUTPUT_RAW;
		break;

	case 0x100:
		as -> output_format = OUTPUT_OBJ;
		break;

	case 0x101:
		as -> flags |= FLAG_DEPEND;
		break;

	case 0x102:
		as -> flags |= FLAG_DEPEND | FLAG_DEPENDNOERR;
		break;
	
	case 0x142:
		as -> flags |= FLAG_UNICORNS;
		break;

	case 'f':
		if (!strcasecmp(arg, "decb"))
			as -> output_format = OUTPUT_DECB;
		else if (!strcasecmp(arg, "basic"))
			as -> output_format = OUTPUT_BASIC;
		else if (!strcasecmp(arg, "raw"))
			as -> output_format = OUTPUT_RAW;
		else if (!strcasecmp(arg, "rawrel"))
			as -> output_format = OUTPUT_RAWREL;
		else if (!strcasecmp(arg, "obj"))
			as -> output_format = OUTPUT_OBJ;
		else if (!strcasecmp(arg, "srec"))
			as -> output_format = OUTPUT_SREC;
		else if (!strcasecmp(arg, "ihex"))
			as -> output_format = OUTPUT_IHEX;
		else if (!strcasecmp(arg, "hex"))
			as -> output_format = OUTPUT_HEX;
		else if (!strcasecmp(arg, "os9"))
		{
			as -> pragmas |= PRAGMA_DOLLARNOTLOCAL;
			as -> output_format = OUTPUT_OS9;
		}
		else if (!strcasecmp(arg, "lwmod"))
		{
			as -> output_format = OUTPUT_LWMOD;
		}
		else
		{
			fprintf(stderr, "Invalid output format: %s\n", arg);
			exit(1);
		}
		break;
		
	case 'p':
		if (parse_pragma_string(as, arg, 0) == 0)
		{
			fprintf(stderr, "Unrecognized pragma string: %s\n", arg);
			exit(1);
		}
		break;

	case '9':
		as -> pragmas |= PRAGMA_6809;
		break;

	case '3':
		as -> pragmas &= ~PRAGMA_6809;
		break;

	case 'P':
		as -> preprocess = 1;
		break;
	
	case 0x200:
		as -> pragmas |= PRAGMA_6800COMPAT;
		break;
		
	case lw_cmdline_key_end:
		break;
	
	case lw_cmdline_key_arg:
		lw_stringlist_addstring(as -> input_files, arg);
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
	"lwasm, a HD6309 and MC6809 cross-assembler\vPlease report bugs to lost@l-w.ca.",
	PROGVER
};

/*
main function; parse command line, set up assembler state, and run the 
assembler on the first file
*/
void do_pass1(asmstate_t *as);
void do_pass2(asmstate_t *as);
void do_pass3(asmstate_t *as);
void do_pass4(asmstate_t *as);
void do_pass5(asmstate_t *as);
void do_pass6(asmstate_t *as);
void do_pass7(asmstate_t *as);
void do_output(asmstate_t *as);
void do_symdump(asmstate_t *as);
void do_list(asmstate_t *as);
void do_map(asmstate_t *as);
lw_expr_t lwasm_evaluate_special(int t, void *ptr, void *priv);
lw_expr_t lwasm_evaluate_var(char *var, void *priv);
lw_expr_t lwasm_parse_term(char **p, void *priv);
void lwasm_dividezero(void *priv);

struct passlist_s
{
	char *passname;
	void (*fn)(asmstate_t *as);
	int fordep;
} passlist[] = {
	{ "parse", do_pass1, 1 },
	{ "symcheck", do_pass2 },
	{ "resolve1", do_pass3 },
	{ "resolve2", do_pass4 },
	{ "addressresolve", do_pass5 },
	{ "finalize", do_pass6 },
	{ "emit", do_pass7 },
	{ NULL, NULL }
};


int main(int argc, char **argv)
{
	int passnum;

	/* assembler state */
	asmstate_t asmstate = { 0 };
	program_name = argv[0];

	lw_expr_set_special_handler(lwasm_evaluate_special);
	lw_expr_set_var_handler(lwasm_evaluate_var);
	lw_expr_set_term_parser(lwasm_parse_term);
	lw_expr_setdivzero(lwasm_dividezero);

	/* initialize assembler state */
	asmstate.include_list = lw_stringlist_create();
	asmstate.input_files = lw_stringlist_create();
	asmstate.nextcontext = 1;
	asmstate.exprwidth = 16;
	asmstate.tabwidth = 8;

	// enable the "forward reference maximum size" pragma; old available
	// can be obtained with --pragma=noforwardrefmax
	asmstate.pragmas = PRAGMA_FORWARDREFMAX;
	
	/* parse command line arguments */	
	if (lw_cmdline_parse(&cmdline_parser, argc, argv, 0, 0, &asmstate) != 0)
	{
		exit(1);
	}

	if (!asmstate.output_file)
	{
		asmstate.output_file = lw_strdup("a.out");
	}

	input_init(&asmstate);

	for (passnum = 0; passlist[passnum].fn; passnum++)
	{
		if ((asmstate.flags & FLAG_DEPEND) && passlist[passnum].fordep == 0)
			continue;
		asmstate.passno = passnum;
		debug_message(&asmstate, 50, "Doing pass %d (%s)\n", passnum, passlist[passnum].passname);
		(passlist[passnum].fn)(&asmstate);
		debug_message(&asmstate, 50, "After pass %d (%s)\n", passnum, passlist[passnum].passname);
		dump_state(&asmstate);

		if (asmstate.preprocess)
		{
			/* we're done if we were preprocessing */
			exit(0);
		}
		if (asmstate.errorcount > 0)
		{
			if (asmstate.flags & FLAG_DEPEND)
			{
				// don't show errors during dependency scanning but
				// stop processing immediately
				break;
			}
			if (asmstate.flags & FLAG_UNICORNS)
				lwasm_do_unicorns(&asmstate);
			else
				lwasm_show_errors(&asmstate);
			exit(1);
		}
	}

	if (asmstate.flags & FLAG_DEPEND)
	{
		// output dependencies (other than "includebin")
		char *n;
		
		while ((n = lw_stack_pop(asmstate.includelist)))
		{
			fprintf(stdout, "%s\n", n);
			lw_free(n);
		}
	}	
	else if ((asmstate.flags & FLAG_NOOUT) == 0)
	{
		debug_message(&asmstate, 50, "Doing output");
		do_output(&asmstate);
	}
	
	debug_message(&asmstate, 50, "Done assembly");

	if (asmstate.flags & FLAG_UNICORNS)
	{	
		debug_message(&asmstate, 50, "Invoking unicorns");
		lwasm_do_unicorns(&asmstate);
	}
	do_symdump(&asmstate);
	do_list(&asmstate);
	do_map(&asmstate);

	if (asmstate.testmode_errorcount > 0) exit(1);

	exit(0);
}
