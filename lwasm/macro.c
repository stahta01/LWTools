/*
macro.c
Copyright © 2008 William Astle

This file is part of LWASM.

LWASM is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

Contains stuff associated with macro processing
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "input.h"
#include "instab.h"

PARSEFUNC(pseudo_parse_macro)
{
	macrotab_t *m;
	char *t;
	char tc;
		
	l -> len = 0;
	l -> hideline = 1;
	if (as -> skipcond)
	{
		as -> skipmacro = 1;
		skip_operand(p);
		return;
	}
	
	if (as -> inmacro)
	{
		lwasm_register_error(as, l, E_MACRO_RECURSE);
		return;
	}
	
	if (!(l -> sym))
	{
		lwasm_register_error(as, l, E_MACRO_NONAME);
		return;
	}

	for (m = as -> macros; m; m = m -> next)
	{
		if (!strcasecmp(m -> name, l -> sym))
			break;
	}
	if (m)
	{
		lwasm_register_error(as, l, E_MACRO_DUPE);
		return;
	}
	
	m = lw_alloc(sizeof(macrotab_t));
	m -> name = lw_strdup(l -> sym);
	m -> next = as -> macros;
	m -> lines = NULL;
	m -> numlines = 0;
	m -> flags = 0;
	m -> definedat = l;
	as -> macros = m;

	t = *p;
	while (**p && !isspace(**p))
		(*p)++;
	tc = **p;
	/* ignore unknown flags */
	if (strcasecmp(t, "noexpand") == 0)
		m -> flags |= macro_noexpand;
	**p = tc;
	as -> inmacro = 1;
}

PARSEFUNC(pseudo_parse_endm)
{
	l -> hideline = 1;
	l -> len = 0;

	if (as -> skipcond)
	{
		as -> skipmacro = 0;
		return;
	}
	
	if (!as -> inmacro)
	{
		lwasm_register_error(as, l, E_MACRO_ENDM);
		return;
	}
	
	as -> inmacro = 0;
	
	// a macro definition counts as a context break for local symbols
	as -> context = lwasm_next_context(as);
}

// the current macro will ALWAYS be the first one in the table
int add_macro_line(asmstate_t *as, char *optr)
{
	if (!as -> inmacro)
		return 0;
	
	as -> macros -> lines = lw_realloc(as -> macros -> lines, sizeof(char *) * (as -> macros -> numlines + 1));
	as -> macros -> lines[as -> macros -> numlines] = lw_strdup(optr);
	as -> macros -> numlines += 1;
	return 1;
}

void macro_add_to_buff(char **buff, int *loc, int *len, char c)
{
	if (*loc == *len)
	{
		*buff = lw_realloc(*buff, *len + 32);
		*len += 32;
	}
	(*buff)[(*loc)++] = c;
}

// this is just like a regular operation function
/*
macro args are referenced by "\n" where 1 <= n <= 9
or by "\{n}"; a \ can be included by writing \\
a comma separates argument but one can be included with "\,"
whitespace ends argument list but can be included with "\ " or the like

*/
int expand_macro(asmstate_t *as, line_t *l, char **p, char *opc)
{
	int lc;
	line_t *cl; //, *nl;
	int oldcontext;
	macrotab_t *m;

	char **args = NULL;		// macro arguments
	int nargs = 0;			// number of arguments

	char *p2, *p3;
	
	int bloc, blen;
	char *linebuff;

	for (m = as -> macros; m; m = m -> next)
	{
		if (!strcasecmp(opc, m -> name))
			break;
	}
	// signal no macro expansion
	if (!m)
		return -1;
	
	// save current symbol context for after macro expansion
	oldcontext = as -> context;

	cl = l;

	as -> context = lwasm_next_context(as);

	while (**p && !isspace(**p) && **p)
	{
		p2 = *p;
		while (*p2 && !isspace(*p2) && *p2 != ',')
		{
			if (*p2 == '\\')
			{
				if (p2[1])
					p2++;
			}
			p2++;
		}

		// have arg here
		args = lw_realloc(args, sizeof(char *) * (nargs + 1));
		args[nargs] = lw_alloc(p2 - *p + 1);
		args[nargs][p2 - *p] = '\0';
		memcpy(args[nargs], *p, p2 - *p);
		*p = p2;

		// now collapse out "\" characters
		for (p3 = p2 = args[nargs]; *p2; p2++, p3++)
		{
			if (*p2 == '\\' && p2[1])
			{
				p2++;
			}
			*p3 = *p2;
		}
		*p3 = '\0';
		
		nargs++;
		if (**p == ',')
			(*p)++;
	}
	

	// now create a string for the macro
	// and push it into the front of the input stack
	bloc = blen = 0;
	linebuff = NULL;

	if (m -> flags & macro_noexpand)
	{
		char ctcbuf[100];
		char *p;
		snprintf(ctcbuf, 100, "\001\001SETNOEXPANDSTART\n");
		for (p = ctcbuf; *p; p++)
			macro_add_to_buff(&linebuff, &bloc, &blen, *p);
	}

	
	for (lc = 0; lc < m -> numlines; lc++)
	{
		for (p2 = m -> lines[lc]; *p2; p2++)
		{
			if (*p2 == '\\' && p2[1] == '*')
			{
				int n;
				/* all arguments */
				for (n = 0; n < nargs; n++)
				{
					for (p3 = args[n]; *p3; p3++)
					{
						macro_add_to_buff(&linebuff, &bloc, &blen, *p3);
					}
					if (n != (nargs -1))
					{
						macro_add_to_buff(&linebuff, &bloc, &blen, ',');
					}
				}
				p2++;
			}
			else if (*p2 == '\\' && isdigit(p2[1]))
			{
				int n;
					
				p2++;
				n = *p2 - '0';
				if (n == 0)
				{
					for (p3 = m -> name; *p3; p3++)
						macro_add_to_buff(&linebuff, &bloc, &blen, *p3);
					continue;
				}
				if (n < 1 || n > nargs)
					continue;
				for (p3 = args[n - 1]; *p3; p3++)
					macro_add_to_buff(&linebuff, &bloc, &blen, *p3);
				continue;
			}
			else if (*p2 == '{')
			{
				int n = 0, n2;
				p2++;
				while (*p2 && isdigit(*p2))
				{
					n2 = *p2 - '0';
					if (n2 < 0 || n2 > 9)
						n2 = 0;
					n = n * 10 + n2;
					p2++;
				}
				if (*p2 == '}')
					p2++;
				 
				if (n == 0)
				{
					for (p3 = m -> name; *p3; p3++)
						macro_add_to_buff(&linebuff, &bloc, &blen, *p3);
					continue;
				}
				if (n < 1 || n > nargs)
					continue;
				for (p3 = args[n - 1]; *p3; p3++)
					macro_add_to_buff(&linebuff, &bloc, &blen, *p3);
				continue;
			}
			else
			{
				macro_add_to_buff(&linebuff, &bloc, &blen, *p2);
			}
		}

		macro_add_to_buff(&linebuff, &bloc, &blen, '\n');
		
	}

	if (m -> flags & macro_noexpand)
	{
		char ctcbuf[100];
		char *p;
		snprintf(ctcbuf, 100, "\001\001SETNOEXPANDEND\n");
		for (p = ctcbuf; *p; p++)
			macro_add_to_buff(&linebuff, &bloc, &blen, *p);
	}

	{
		char ctcbuf[100];
		char *p;
		snprintf(ctcbuf, 100, "\001\001SETCONTEXT %d\n\001\001SETLINENO %d\n", oldcontext, cl -> lineno + 1);
		for (p = ctcbuf; *p; p++)
			macro_add_to_buff(&linebuff, &bloc, &blen, *p);
	}
	macro_add_to_buff(&linebuff, &bloc, &blen, 0);
	
	// push the macro into the front of the stream
	input_openstring(as, opc, linebuff);
	lw_free(linebuff);

	// clean up
	if (args)
	{
		while (nargs)
		{
			lw_free(args[--nargs]);
		}
		lw_free(args);
	}

	// indicate a macro was expanded
	l -> hideline = 1;
	return 0;	
}
