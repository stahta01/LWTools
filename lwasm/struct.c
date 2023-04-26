/*
struct.c
Copyright © 2010 William Astle

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

Contains stuff associated with structure processing
*/

#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(pseudo_parse_struct)
{
	structtab_t *s;
	
	if (as -> instruct)
	{
		lwasm_register_error(as, l, E_STRUCT_RECURSE);
		return;
	}
	
	if (l -> sym == NULL)
	{
		lwasm_register_error(as, l, E_STRUCT_NOSYMBOL);
		return;
	}
	
	for (s = as -> structs; s; s = s -> next)
	{
		if (!strcmp(s -> name, l -> sym))	
			break;
	}
	
	if (s)
	{
		lwasm_register_error(as, l, E_STRUCT_DUPE);
		return;
	}
	
	as -> instruct = 1;
	
	s = lw_alloc(sizeof(structtab_t));
	s -> name = lw_strdup(l -> sym);
	s -> next = as -> structs;
	s -> fields = NULL;
	s -> size = 0;
	s -> definedat = l;
	as -> structs = s;
	as -> cstruct = s;
	
	skip_operand(p);
	
	l -> len = 0;
	l -> symset = 1;
	
	// save current assembly address and initialize to 0
	as -> savedaddr = l -> addr;
	l -> addr = lw_expr_build(lw_expr_type_int, 0);
}

static void instantiate_struct(asmstate_t *as, line_t *l, structtab_t *s, char *pref, lw_expr_t baseaddr)
{
	int len;
	char *t;
	int plen;
	int addr = 0;
	lw_expr_t te, te2;
	structtab_field_t *e;

	if (baseaddr == NULL)
		baseaddr = l -> addr;
	
	plen = strlen(pref);
	for (e = s -> fields; e; e = e -> next)
	{
		if (e -> name)
		{
			len = plen + strlen(e -> name) + 1;
			t = lw_alloc(len + 1);
			sprintf(t, "%s.%s", pref, e -> name);
		}
		else
		{
			len = plen + 30;
			t = lw_alloc(len + 1);
			sprintf(t, "%s.____%d", pref, addr);
		}

		te = lw_expr_build(lw_expr_type_int, addr);
		te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, te, baseaddr);
		register_symbol(as, l, t, te2, symbol_flag_nocheck);
		lw_expr_destroy(te);
		
		if (e -> substruct)
		{
			instantiate_struct(as, l, e -> substruct, t, te2);
		}
		
		lw_expr_destroy(te2);
		
		lw_free(t);
		addr += e -> size;
	}
	
	/* register the "sizeof" symbol */
	len = plen + 8;
	t = lw_alloc(len + 1);
	sprintf(t, "sizeof{%s}", pref);
	te = lw_expr_build(lw_expr_type_int, s -> size);
	register_symbol(as, l, t, te, symbol_flag_nocheck);
	lw_expr_destroy(te);
	lw_free(t);
}


PARSEFUNC(pseudo_parse_endstruct)
{
	lw_expr_t te;
		
	if (as -> instruct == 0)
	{
		lwasm_register_error(as, l, W_ENDSTRUCT_WITHOUT);
		skip_operand(p);
		return;
	}

	te = lw_expr_build(lw_expr_type_int, 0);
	
	// make all the relevant generic struct symbols
	instantiate_struct(as, l, as -> cstruct, as -> cstruct -> name, te);
	lw_expr_destroy(te);
	
	l -> soff = as -> cstruct -> size;
	as -> instruct = 0;
	
	skip_operand(p);
	
	lw_expr_destroy(l -> addr);
	l -> addr = as -> savedaddr;
	as -> savedaddr = NULL;
	
	l -> len = 0;
}

void register_struct_entry(asmstate_t *as, line_t *l, int size, structtab_t *ss)
{
	structtab_field_t *e, *e2;
	
	l -> soff = as -> cstruct -> size;
	e = lw_alloc(sizeof(structtab_field_t));
	e -> next = NULL;
	e -> size = size;
	if (l -> sym)
		e -> name = lw_strdup(l -> sym);
	else
		e -> name = NULL;
	e -> substruct = ss;
	if (as -> cstruct -> fields)
	{
		for (e2 = as -> cstruct -> fields; e2 -> next; e2 = e2 -> next)
			/* do nothing */ ;
		e2 -> next = e;
	}
	else
	{
		as -> cstruct -> fields = e;
	}
	as -> cstruct -> size += size;
}

int expand_struct(asmstate_t *as, line_t *l, char **p, char *opc)
{
	structtab_t *s;
	
	debug_message(as, 200, "Checking for structure expansion: %s", opc);

	for (s = as -> structs; s; s = s -> next)
	{
		if (!strcmp(opc, s -> name))
			break;
	}
	
	if (!s)
		return -1;
	
	debug_message(as, 10, "Expanding structure: %s", opc);
	
	if (!(l -> sym))
	{
		lwasm_register_error(as, l, E_STRUCT_NONAME);
		return -1;
	}

	l -> len = s -> size;

	if (as -> instruct)
	{
		/* register substruct here */
		register_struct_entry(as, l, s -> size, s);
		/* mark the symbol consumed */
		l -> symset = 1;
		l -> len = 0;
		return 0;
	}
	else
	{
		/* instantiate the struct here */
		instantiate_struct(as, l, s, l -> sym, NULL);
		return 0;
	}
	
	return 0;
}

