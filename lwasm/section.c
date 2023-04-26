/*
section.c

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

#include <string.h>
#include <ctype.h>

#include <lw_string.h>
#include <lw_alloc.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(pseudo_parse_section)
{
	char *p2;
	char *sn;
	char *opts = NULL;
	sectiontab_t *s;

	if (as -> output_format != OUTPUT_OBJ && as -> output_format != OUTPUT_LWMOD)
	{
		lwasm_register_error(as, l, E_SECTION_TARGET);
		return;
	}
	
	if (!**p)
	{
		lwasm_register_error(as, l, E_SECTION_NAME);
		return;
	}

	l -> len = 0;

	if (as -> csect)
	{
		lw_expr_destroy(as -> csect -> offset);
		as -> csect -> offset = lw_expr_copy(l -> addr);
		as -> csect = NULL;
	}
	
	for (p2 = *p; *p2 && *p2 != ',' && !isspace(*p2); p2++)
		/* do nothing */ ;
	
	sn = lw_strndup(*p, p2 - *p);
	*p = p2;
	
	if (**p == ',' && as -> output_format != OUTPUT_LWMOD)
	{
		// have opts
		(*p)++;
		
		for (p2 = *p; *p2 && !isspace(*p2); p2++)
			/* do nothing */ ;
		
		opts = lw_strndup(*p, p2 - *p);
		*p = p2;
	}

	if (as -> output_format == OUTPUT_LWMOD)
	{
		for (p2 = sn; *p2; p2++)
			*p2 = tolower(*p2);
		
		if (strcmp(sn, "bss") && strcmp(sn, "main") && strcmp(sn, "init") && strcmp(sn, "calls") && strcmp(sn, "modname"))
		{
			lwasm_register_error(as, l, E_SECTION_NAME);
			lw_free(sn);
			lw_free(opts);
			return;
		}
	}

	for (s = as -> sections; s; s = s -> next)
	{
		if (!strcmp(s -> name, sn))
			break;
	}
	if (s && opts)
	{
		lwasm_register_error(as, l, W_DUPLICATE_SECTION);
	}
	if (!s)
	{
		// create section data structure
		s = lw_alloc(sizeof(sectiontab_t));
		s -> oblen = 0;
		s -> obsize = 0;
		s -> obytes = NULL;
		s -> tbase = -1;
		s -> name = lw_strdup(sn);
		s -> offset = lw_expr_build(lw_expr_type_special, lwasm_expr_secbase, s);
		s -> flags = section_flag_none;
		s -> reloctab = NULL;
		if (!strcasecmp(sn, "bss") || !strcasecmp(sn, ".bss"))
		{
			s -> flags |= section_flag_bss;
		}
		if (!strcasecmp(sn, "_constant") || !strcasecmp(sn, "_constants"))
		{
			s -> flags |= section_flag_constant;
		}
		
		// parse options
		if (opts)
		{
			// only one option ("bss" or "!bss")
			if (!strcasecmp(opts, "bss"))
			{
				s -> flags |= section_flag_bss;
			}
			else if (!strcasecmp(opts, "!bss"))
			{
				s -> flags &= ~section_flag_bss;
			}
			else if (!strcasecmp(opts, "constant"))
			{
				s -> flags |= section_flag_constant;
			}
			else if (!strcasecmp(opts, "!constant"))
			{
				s -> flags |= section_flag_constant;
			}
			else
			{
				lwasm_register_error(as, l, E_SECTION_FLAG);
				lw_free(sn);
				lw_free(opts);
				lw_free(s -> name);
				lw_expr_destroy(s -> offset);
				lw_free(s);
				return;
			}
		}
		s -> next = as -> sections;
		as -> sections = s;
	}
	
	// cause all instances of "constant" sections to start at 0
	if (s -> flags & section_flag_constant)
		s -> offset = lw_expr_build(lw_expr_type_int, 0);
	lw_expr_destroy(l -> addr);
	l -> addr = lw_expr_copy(s -> offset);
	lw_expr_destroy(l -> daddr);
	l -> daddr = lw_expr_copy(s -> offset);
	as -> csect = s;
	as -> context = lwasm_next_context(as);

	l -> len = 0;
	
	lw_free(opts);
	lw_free(sn);
}

PARSEFUNC(pseudo_parse_endsection)
{
	if (as -> output_format != OUTPUT_OBJ && as -> output_format != OUTPUT_LWMOD)
	{
		lwasm_register_error(as, l, E_SECTION_TARGET);
		return;
	}

	l -> len = 0;

	if (!(as -> csect))
	{
		lwasm_register_error(as, l, E_SECTION_END);
		return;
	}

	// save offset in case another instance of the section appears	
	lw_expr_destroy(as -> csect -> offset);
	as -> csect -> offset = l -> addr;
	
	// reset address to 0
	l -> addr = lw_expr_build(lw_expr_type_int, 0);
	as -> csect = NULL;
	
	// end of section is a context break
	as -> context = lwasm_next_context(as);

	skip_operand(p);
}

PARSEFUNC(pseudo_parse_export)
{
	int after = 0;
	char *sym = NULL;
	exportlist_t *e;
	
	if (as -> output_format != OUTPUT_OBJ)
	{
		lwasm_register_error2(as, l, E_OBJTARGET_ONLY, "(%s)", "EXPORT");
		return;
	}

	l -> len = 0;
	
	if (l -> sym)
	{
		sym = lw_strdup(l -> sym);
		l -> symset = 1;
	}
	
	if (l -> sym)
	{
		skip_operand(p);
	}

again:
	if (after || !sym)
	{
		char *p2;
		
		after = 1;
		for (p2 = *p; *p2 && *p2 != ',' && !isspace(*p2); p2++)
			/* do nothing */ ;
		
		sym = lw_strndup(*p, p2 - *p);
		*p = p2;
	}
	if (!sym)
	{
		lwasm_register_error2(as, l, E_SYMBOL_MISSING, "for %s", "EXPORT");
		return;
	}
	
	// add the symbol to the "export" list (which will be resolved
	// after the parse pass is complete
	e = lw_alloc(sizeof(exportlist_t));
	e -> next = as -> exportlist;
	e -> symbol = lw_strdup(sym);
	e -> line = l;
	e -> se = NULL;
	as -> exportlist = e;
	lw_free(sym);
	
	if (after && **p == ',')
	{
		(*p)++;
		for (; **p && isspace(**p); (*p)++)
			/* do nothing */ ;
		goto again;
	}
}

PARSEFUNC(pseudo_parse_extern)
{
	int after = 0;
	char *sym = NULL;
	importlist_t *e;
	
	if (as -> output_format != OUTPUT_OBJ)
	{
		lwasm_register_error2(as, l, E_OBJTARGET_ONLY, "(%s)", "IMPORT");
		return;
	}
	
	l -> len = 0;
	
	if (l -> sym)
	{
		sym = lw_strdup(l -> sym);
		l -> symset = 1;
	}
	
	if (l -> sym)
	{
		skip_operand(p);
	}

again:
	if (after || !sym)
	{
		char *p2;
		
		after = 1;
		for (p2 = *p; *p2 && *p2 != ',' && !isspace(*p2); p2++)
			/* do nothing */ ;
		
		sym = lw_strndup(*p, p2 - *p);
		*p = p2;
	}
	if (!sym)
	{
		lwasm_register_error2(as, l, E_SYMBOL_MISSING, "for %s", "IMPORT");
		return;
	}
	
	// add the symbol to the "export" list (which will be resolved
	// after the parse pass is complete
	e = lw_alloc(sizeof(importlist_t));
	e -> next = as -> importlist;
	e -> symbol = lw_strdup(sym);
	as -> importlist = e;
	lw_free(sym);
	
	if (after && **p == ',')
	{
		(*p)++;
		for (; **p && isspace(**p); (*p)++)
			/* do nothing */ ;
		goto again;
	}
}

PARSEFUNC(pseudo_parse_extdep)
{
	int after = 0;
	char *sym = NULL;
//	importlist_t *e;
	
	if (as -> output_format != OUTPUT_OBJ)
	{
		lwasm_register_error2(as, l, E_OBJTARGET_ONLY, "(%s)", "EXTDEP");
		return;
	}
	
	if (!as -> csect)
	{
		lwasm_register_error(as, l, E_SECTION_EXTDEP);
		return;
	}
	
	l -> len = 0;
	
	if (l -> sym)
		sym = lw_strdup(l -> sym);
	
	if (l -> sym)
	{
		skip_operand(p);
	}

again:
	if (after || !sym)
	{
		char *p2;
		
		after = 1;
		for (p2 = *p; *p2 && *p2 != ',' && !isspace(*p2); p2++)
			/* do nothing */ ;
		
		sym = lw_strndup(*p, p2 - *p);
	}
	if (!sym)
	{
		lwasm_register_error2(as, l, E_SYMBOL_MISSING, "for %s", "EXTDEP");
		return;
	}
	
	// create a zero-width dependency
	{
		lw_expr_t e;
		e = lw_expr_build(lw_expr_type_int, 0);
		lwasm_emitexpr(l, e, 0);
		lw_expr_destroy(e);
	}
	
	if (after && **p == ',')
	{
		(*p)++;
		for (; **p && isspace(**p); (*p)++)
			/* do nothing */ ;
		goto again;
	}
}
