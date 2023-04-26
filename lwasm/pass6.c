/*
pass6.c

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
Finalize Pass

Reduce all expressions in a final pass.

Observation:

Everything should reduce as far as it is going to in a single pass
because all line addresses are now constant (or section-base offset)
*/

static int exprok_aux(lw_expr_t e, void *priv)
{
	asmstate_t *as = priv;
	
	if (lw_expr_istype(e, lw_expr_type_int))
		return 0;
	
	if (as -> output_format == OUTPUT_OBJ || as -> output_format == OUTPUT_LWMOD)
	{
		if (lw_expr_istype(e, lw_expr_type_oper))
			return 0;
		if (lw_expr_istype(e, lw_expr_type_special) && (as -> output_format == OUTPUT_OBJ || as -> output_format == OUTPUT_LWMOD))
		{
			int t;
			t = lw_expr_specint(e);
			if (t == lwasm_expr_secbase || t == lwasm_expr_syment || t == lwasm_expr_import)
				return 0;
		}
	}
	
	return 1;
}

static int exprok(asmstate_t *as, lw_expr_t e)
{
	if (lw_expr_testterms(e, exprok_aux, as))
		return 0;
	return 1;
}


void do_pass6(asmstate_t *as)
{
	line_t *cl;
	struct line_expr_s *le;

	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		as -> cl = cl;
		for (le = cl -> exprs; le; le = le -> next)
		{
			lwasm_reduce_expr(as, le -> expr);
			if (!exprok(as, le -> expr))
			{
				lwasm_register_error2(as, cl, E_EXPRESSION_BAD, "%s", lw_expr_print(le -> expr));
			}
		}
	}
}
