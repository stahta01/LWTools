/*
pass2.c

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
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

/*
pass 2: deal with undefined symbols and do a simplification pass
on all the expressions. Handle PRAGMA_IMPORTUNDEFEXPORT

*/
void do_pass2(asmstate_t *as)
{
	line_t *cl;
	exportlist_t *ex;
	struct symtabe *s;
	importlist_t *im;
	struct line_expr_s *le;

	// verify the export list
	if (as -> output_format == OUTPUT_OBJ)
	{	
		for (ex = as -> exportlist; ex; ex = ex -> next)
		{
			s = lookup_symbol(as, NULL, ex -> symbol);
			if (!s)
			{
				if (CURPRAGMA(ex -> line, PRAGMA_IMPORTUNDEFEXPORT))
				{
					for (im = as -> importlist; im; im = im -> next)
					{
						if (!strcmp(ex -> symbol, im -> symbol))
							break;
					}
					if (!im)
					{
						im = lw_alloc(sizeof(importlist_t));
						im -> symbol = lw_strdup(ex -> symbol);
						im -> next = as -> importlist;
						as -> importlist = im;
					}
				}
				else
				{
					// undefined export - register error
					lwasm_register_error(as, ex->line, E_SYMBOL_UNDEFINED_EXPORT);
				}
			}
			ex -> se = s;
		}
		if (as -> errorcount > 0)
			return;
	}

	// we want to throw errors on undefined symbols here
	as -> badsymerr = 1;
	
	// now do some reductions on expressions
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		as -> cl = cl;
		
		// simplify address
		lwasm_reduce_expr(as, cl -> addr);

		// simplify data address
		lwasm_reduce_expr(as, cl -> daddr);
		
		// simplify each expression
		for (le = cl -> exprs; le; le = le -> next)
			lwasm_reduce_expr(as, le -> expr);
	}	
}
