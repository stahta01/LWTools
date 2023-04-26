/*
symbol.c

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
#include <lw_expr.h>
#include <lw_string.h>

#include "lwasm.h"

#if 0
struct symtabe *symbol_findprev(asmstate_t *as, struct symtabe *se)
{
	struct symtabe *se1, *se2;
	int i;
	
	for (se2 = NULL, se1 = as -> symtab.head; se1; se1 = se1 -> next)
	{
		debug_message(as, 200, "Sorting; looking at symbol %s (%p) for %s", se1 -> symbol, se1, se -> symbol);
		/* compare se with se1 */
		i = strcasecmp(se -> symbol, se1 -> symbol);
		
		/* if the symbol sorts before se1, we just need to return */
		if (i < 0)
			return se2;
		
		if (i == 0)
		{
			/* symbol name matches; compare other things */
			
			/*if next version is greater than this one, return */
			if (se -> version > se1 -> version)
				return se2;
			/* if next context is great than this one, return */ 
			if (se -> context > se1 -> context)
				return se2;
			
			/* if section name is greater, return */
			/* if se has no section but se1 does, we go first */
			if (se -> section == NULL && se1 -> section != NULL)
				return se2;
			if (se -> section != NULL && se1 -> section != NULL)
			{
				/* compare section names and if se < se1, return */
				i = strcasecmp(se -> section -> name, se1 -> section -> name);
				if (i < 0)
					return se2;
			}
		}
		
		se2 = se1;
	}
	return se2;
}
#endif
struct symtabe *register_symbol(asmstate_t *as, line_t *cl, char *sym, lw_expr_t val, int flags)
{
	struct symtabe *se, *nse;
	struct symtabe *sprev;
	int islocal = 0;
	int context = -1;
	int version = -1;
	char *cp;
	int cdir;
	
	debug_message(as, 200, "Register symbol %s (%02X), %s", sym, flags, lw_expr_print(val));

	if (!(flags & symbol_flag_nocheck))
	{
		if (!sym || !*sym)
		{
			lwasm_register_error2(as, cl, E_SYMBOL_BAD, "(%s)", sym);
			return NULL;
		}
		if (*(unsigned char *)sym < 0x80 && (!strchr(SSYMCHARS, *sym) && !strchr(sym + 1, '$') && !strchr(sym + 1, '@') && !strchr(sym + 1, '?')))
		{
			lwasm_register_error2(as, cl, E_SYMBOL_BAD, "(%s)", sym);
			return NULL;
		}

		if ((*sym == '$' || *sym == '@') && (sym[1] >= '0' && sym[1] <= '9'))
		{
			lwasm_register_error2(as, cl, E_SYMBOL_BAD, "(%s)", sym);
			return NULL;
		}
	}

	for (cp = sym; *cp; cp++)
	{
		if (*cp == '@' || *cp == '?')
			islocal = 1;
		if (*cp == '$' && !(CURPRAGMA(cl, PRAGMA_DOLLARNOTLOCAL)))
			islocal = 1;
		
		// bad symbol
		if (!(flags & symbol_flag_nocheck) && *(unsigned char *)cp < 0x80 && !strchr(SYMCHARS, *cp))
		{
			lwasm_register_error2(as, cl, E_SYMBOL_BAD, "(%s)", sym);
			return NULL;
		}
	}

	if (islocal)
		context = cl -> context;
	
	// first, look up symbol to see if it is already defined
	cdir = 0;
	for (se = as -> symtab.head, sprev = NULL; se; )
	{
		int ndir;
		debug_message(as, 300, "Symbol add lookup: %p", se);
		ndir = strcasecmp(sym, se -> symbol);
//		if (!ndir && !CURPRAGMA(cl, PRAGMA_SYMBOLNOCASE) && !(se -> flags & symbol_flag_set))
		if (!ndir && !(se -> flags & symbol_flag_set))
		{
			if (strcmp(sym, se -> symbol))
			{
				if (!CURPRAGMA(cl, PRAGMA_SYMBOLNOCASE) && !(se -> flags & symbol_flag_nocase))
					ndir = 1;
			}
		}
		if (!ndir && se -> context != context)
		{
			ndir = (context < se -> context) ? -1 : 1;
		}
		if (!ndir)
		{
			if ((flags & symbol_flag_set) && (se -> flags & symbol_flag_set))
			{
				version = se -> version;
			}
			break;
		}
		cdir = ndir;
		sprev = se;
		if (cdir < 0)
			se = se -> left;
		else
			se = se -> right;
	}

	if (se && version == -1)
	{
		// multiply defined symbol
		lwasm_register_error2(as, cl, E_SYMBOL_DUPE, "(%s)", sym);
		return NULL;
	}

	if (flags & symbol_flag_set)
	{
		version++;
	}
	
	// symplify the symbol expression - replaces "SET" symbols with
	// symbol table entries
	lwasm_reduce_expr(as, val);

	nse = lw_alloc(sizeof(struct symtabe));
	nse -> context = context;
	nse -> version = version;
	nse -> flags = flags;
	if (CURPRAGMA(cl, PRAGMA_NOLIST))
	{
		nse -> flags |= symbol_flag_nolist;
	}
	if (CURPRAGMA(cl, PRAGMA_SYMBOLNOCASE))
	{
		nse -> flags |= symbol_flag_nocase;
	}
	if (!cl && (as -> pragmas & PRAGMA_SYMBOLNOCASE))
	{
		nse -> flags |= symbol_flag_nocase;
	}
	nse -> value = lw_expr_copy(val);
	nse -> symbol = lw_strdup(sym);
	nse -> right = NULL;
	nse -> left = NULL;
	nse -> nextver = NULL;
	if (se)
	{
		nse -> nextver = se;
		nse -> left = se -> left;
		nse -> right = se -> right;
		se -> left = NULL;
		se -> right = NULL;
	}
	if (cl)
		nse -> section = cl -> csect;
	else
		nse -> section = NULL;
	if (!sprev)
	{
		debug_message(as, 200, "Adding symbol at head of symbol table");
		as -> symtab.head = nse;
	}
	else
	{
		debug_message(as, 200, "Adding symbol in middle of symbol table");
		if (cdir < 0)
			sprev -> left = nse;
		else
			sprev -> right = nse;
	}
	if (CURPRAGMA(cl, PRAGMA_EXPORT) && cl -> csect && !islocal)
	{
		exportlist_t *e;
		
		/* export symbol if not already exported */
		e = lw_alloc(sizeof(exportlist_t));
		e -> next = as -> exportlist;
		e -> symbol = lw_strdup(sym);
		e -> line = cl;
		e -> se = nse;
		as -> exportlist = e;
	}
	return nse;
}

// for "SET" symbols, always returns the LAST definition of the
// symbol. This works because the lwasm_reduce_expr() call in 
// register_symbol will ensure there are no lingering "var" references
// to the set symbol anywhere in the symbol table; they will all be
// converted to direct references
// NOTE: this means that for a forward reference to a SET symbol,
// the LAST definition will be the one used.
// This arrangement also ensures that any reference to the symbol
// itself inside a "set" definition will refer to the previous version
// of the symbol.
struct symtabe * lookup_symbol(asmstate_t *as, line_t *cl, char *sym)
{
	int local = 0;
	struct symtabe *s;
	int cdir;

	debug_message(as, 100, "Look up symbol %s", sym);
	
	// check if this is a local symbol
	if (strchr(sym, '@') || strchr(sym, '?'))
		local = 1;
	
	if (cl && !CURPRAGMA(cl, PRAGMA_DOLLARNOTLOCAL) && strchr(sym, '$'))
		local = 1;
	if (!cl && !(as -> pragmas & PRAGMA_DOLLARNOTLOCAL) && strchr(sym, '$'))
		local = 1;
	
	// cannot look up local symbol in global context!!!!!
	if (!cl && local)
		return NULL;
	
	for (s = as -> symtab.head; s; )
	{
		cdir = strcasecmp(sym, s -> symbol);
		if (!cdir && !(s->flags & symbol_flag_nocase))
		{
			if (strcmp(sym, s -> symbol))
				cdir = 1;
		}
		if (!cdir)
		{
			if (local && s -> context != cl -> context)
			{
				cdir = (cl -> context < s -> context) ? -1 : 1;
			}	
		}
		
		if (!cdir)
		{
			debug_message(as, 100, "Found symbol %s: %s, %s", sym, s -> symbol, lw_expr_print(s -> value));
			return s;
		}
		
		if (cdir < 0)
			s = s -> left;
		else
			s = s -> right;
	}
	debug_message(as, 100, "Symbol not found %s", sym);
	return NULL;
}

struct listinfo
{
	sectiontab_t *sect;
	asmstate_t *as;
	int complex;
};

int list_symbols_test(lw_expr_t e, void *p)
{
	struct listinfo *li = p;
	
	if (li -> complex)
		return 0;
	
	if (lw_expr_istype(e, lw_expr_type_special))
	{
		if (lw_expr_specint(e) == lwasm_expr_secbase)
		{
			if (li -> sect)
			{
				li -> complex = 1;
			}
			else
			{
				li -> sect = lw_expr_specptr(e);
			}
		}
	}
	return 0;
}

void list_symbols_aux(asmstate_t *as, FILE *of, struct symtabe *se)
{
	struct symtabe *s;
	lw_expr_t te;
	struct listinfo li;

	li.as = as;
	
	if (!se)
		return;
	
	list_symbols_aux(as, of, se -> left);
	
	for (s = se; s; s = s -> nextver)
	{	
		if (s -> flags & symbol_flag_nolist)
			continue;

		if ((as -> flags & FLAG_SYMBOLS_NOLOCALS) && (s -> context >= 0))
			continue;

		lwasm_reduce_expr(as, s -> value);
		fputc('[', of);
		if (s -> flags & symbol_flag_set)
			fputc('S', of);
		else
			fputc(' ', of);
		if (as -> output_format == OUTPUT_OBJ)
		{
			if (lw_expr_istype(s -> value, lw_expr_type_int))
				fputc('c', of);
			else
				fputc('s', of);
		}
		if (s -> context < 0)
			fputc('G', of);
		else
			fputc('L', of);

		fputc(']', of);
		fputc(' ', of);
		fprintf(of, "%-32s ", s -> symbol);
		
		te = lw_expr_copy(s -> value);
		li.complex = 0;
		li.sect = NULL;
		lw_expr_testterms(te, list_symbols_test, &li);
		if (li.sect)
		{
			as -> exportcheck = 1;
			as -> csect = li.sect;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
		}
		
		if (lw_expr_istype(te, lw_expr_type_int))
		{
			fprintf(of, "%04X", lw_expr_intval(te));
			if (li.sect)
			{
				fprintf(of, " (%s)", li.sect -> name);
			}
			fprintf(of, "\n");
		}
		else
		{
			fprintf(of, "<<incomplete>>\n");
//			fprintf(of, "%s\n", lw_expr_print(s -> value));
		}
		lw_expr_destroy(te);
	}
	
	list_symbols_aux(as, of, se -> right);
}

void list_symbols(asmstate_t *as, FILE *of)
{
	fprintf(of, "\nSymbol Table:\n");
	list_symbols_aux(as, of, as -> symtab.head);
}

void map_symbols(asmstate_t *as, FILE *of, struct symtabe *se)
{
	struct symtabe *s;
	lw_expr_t te;
	struct listinfo li;

	li.as = as;

	if (!se)
		return;

	map_symbols(as, of, se -> left);

	for (s = se; s; s = s -> nextver)
	{
		if (s -> flags & symbol_flag_nolist)
			continue;
		lwasm_reduce_expr(as, s -> value);

		te = lw_expr_copy(s -> value);
		li.complex = 0;
		li.sect = NULL;
		lw_expr_testterms(te, list_symbols_test, &li);
		if (li.sect)
		{
			as -> exportcheck = 1;
			as -> csect = li.sect;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
		}

		if (lw_expr_istype(te, lw_expr_type_int))
		{
			fprintf(of, "Symbol: %s", s -> symbol);
			if (s -> context != -1)
				fprintf(of, "_%04X", lw_expr_intval(te));
			fprintf(of, " (%s) = %04X\n", as -> output_file, lw_expr_intval(te));

		}
		lw_expr_destroy(te);
	}

	map_symbols(as, of, se -> right);
}

void do_map(asmstate_t *as)
{
	FILE *of = NULL;

	if (!(as -> flags & FLAG_MAP))
		return;

	if (as -> map_file)
	{
		if (strcmp(as -> map_file, "-") == 0)
		{
			of = stdout;
		}
		else
			of = fopen(as -> map_file, "w");
	}
	else
		of = stdout;
	if (!of)
	{
		fprintf(stderr, "Cannot open map file '%s' for output\n", as -> map_file);
		return;
	}

	map_symbols(as, of, as -> symtab.head);

	fclose(of);
}
