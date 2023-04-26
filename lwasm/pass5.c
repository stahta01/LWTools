/*
pass5.c

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
AssignAddresses Pass

Force resolution of all line addresses

*/

static int exprok_aux(lw_expr_t e, void *priv)
{
	asmstate_t *as = priv;
	
	if (lw_expr_istype(e, lw_expr_type_int))
		return 0;
	if (lw_expr_istype(e, lw_expr_type_oper))
		return 0;
	if (lw_expr_istype(e, lw_expr_type_special) && (as -> output_format == OUTPUT_OBJ || as -> output_format == OUTPUT_LWMOD))
	{
		int t;
		t = lw_expr_specint(e);
		if (t == lwasm_expr_secbase)
			return 0;
	}
	
	return 1;
}

static int exprok(asmstate_t *as, lw_expr_t e)
{
	if (lw_expr_testterms(e, exprok_aux, as))
		return 0;
	return 1;
}

void do_pass5(asmstate_t *as)
{
//	int rc;
	int cnt;
	int ocnt;
	line_t *cl, *sl;
//	struct line_expr_s *le;

	// first, count the number of non-constant addresses; do
	// a reduction first on each one
	for (cnt = 0, cl = as -> line_head; cl; cl = cl -> next)
	{
		as -> cl = cl;
		lwasm_reduce_expr(as, cl -> addr);
		if (!exprok(as, cl -> addr))
			cnt++;
		lwasm_reduce_expr(as, cl -> daddr);
		if (!exprok(as, cl -> daddr))
			cnt++;
	}

	sl = as -> line_head;
	while (cnt > 0)
	{
		ocnt = cnt;
		
		// find an unresolved address
		for ( ; sl && exprok(as, sl -> addr) && exprok(as, sl -> daddr); sl = sl -> next)
			/* do nothing */ ;

		// simplify address
		for (cl = sl; cl; cl = cl -> next)
		{
			as -> cl = sl;
			lwasm_reduce_expr(as, sl -> addr);
		
			if (exprok(as, cl -> addr))
			{
				if (0 == --cnt)
					return;
			}
			lwasm_reduce_expr(as, sl -> daddr);
			if (exprok(as, cl -> addr))
			{
				if (0 == --cnt)
					return;
			}
		}
		
		if (cnt == ocnt)
			break;
	}
	
	if (cnt)
	{
		// we have non-resolved line addresses here
		for (cl = sl; cl; cl = cl -> next)
		{
			if (!exprok(as, cl -> addr))
			{
				lwasm_register_error(as, cl, E_LINE_ADDRESS);
			}
			if (!exprok(as, cl -> daddr))
			{
				lwasm_register_error(as, cl, E_LINED_ADDRESS);
			}
		}
	}
}
