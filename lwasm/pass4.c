/*
pass4.c

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
Resolve2 Pass

Force resolution of instruction sizes.

*/
void do_pass4_aux(asmstate_t *as, int force)
{
	int rc;
	int cnt;
	line_t *cl, *sl;
	struct line_expr_s *le;
	int trycount = 0;

	// first, count the number of unresolved instructions
	for (cnt = 0, cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> len == -1)
			cnt++;
	}

	sl = as -> line_head;
	while (cnt > 0)
	{
		trycount = cnt;
		debug_message(as, 60, "%d unresolved instructions", cnt);

		// find an unresolved instruction
		for ( ; sl && sl -> len != -1; sl = sl -> next)
		{
			debug_message(as, 200, "Search line %p", sl);
			as -> cl = sl;
			lwasm_reduce_expr(as, sl -> addr);
			lwasm_reduce_expr(as, sl -> daddr);
	
			// simplify each expression
			for (le = sl -> exprs; le; le = le -> next)
				lwasm_reduce_expr(as, le -> expr);
		}
		
		debug_message(as, 200, "Found line %p", sl);
		// simplify address
		as -> cl = sl;
		lwasm_reduce_expr(as, sl -> addr);
		lwasm_reduce_expr(as, sl -> daddr);
			
		// simplify each expression
		for (le = sl -> exprs; le; le = le -> next)
			lwasm_reduce_expr(as, le -> expr);


		if (sl -> len == -1 && sl -> insn >= 0 && instab[sl -> insn].resolve)
		{
			(instab[sl -> insn].resolve)(as, sl, 1);
			debug_message(as, 200, "Try resolve = %d/%d", sl -> len, sl -> dlen);
			if (force && sl -> len == -1 && sl -> dlen == -1)
			{
				lwasm_register_error(as, sl, E_INSTRUCTION_FAILED);
				return;
			}
		}
		if (sl -> len != -1 && sl -> dlen != -1)
		{
			cnt--;
			if (cnt == 0)
				return;
			
			// this one resolved - try looking for the next one instead
			// of wasting time running through the rest of the lines
			continue;
		}

		do
		{
			debug_message(as, 200, "Flatten after...");
			rc = 0;
			for (cl = sl; cl; cl = cl -> next)
			{
				debug_message(as, 200, "Flatten line %p", cl);
				as -> cl = cl;
			
				// simplify address
				lwasm_reduce_expr(as, cl -> addr);
				lwasm_reduce_expr(as, cl -> daddr);
				// simplify each expression
				for (le = cl -> exprs; le; le = le -> next)
					lwasm_reduce_expr(as, le -> expr);
			
				if (cl -> len == -1)
				{
					// try resolving the instruction length
					// but don't force resolution
					if (cl -> insn >= 0 && instab[cl -> insn].resolve)
					{
						(instab[cl -> insn].resolve)(as, cl, 0);
						if ((cl -> inmod == 0) && cl -> len >= 0 && cl -> dlen >= 0)
						{
							if (cl -> len == 0)
								cl -> len = cl -> dlen;
							else
								cl -> dlen = cl -> len;
						}
						debug_message(as, 200, "Flatten resolve returns %d", cl -> len);
						if (cl -> len != -1 && cl -> dlen != -1)
						{
							rc++;
							cnt--;
							if (cnt == 0)
								return;
						}
					}
				}
			}
			if (as -> errorcount > 0)
				return;
		} while (rc > 0);
		if (trycount == cnt)
			break;
	}
}

void do_pass4(asmstate_t *as)
{
	do_pass4_aux(as, 1);
}
