/*
pass3.c

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
Resolve1 Pass

repeatedly resolve instruction sizes and line addresses
until nothing more reduces

*/
void do_pass3(asmstate_t *as)
{
	int rc;
	line_t *cl;
	struct line_expr_s *le;
	
	do
	{
		rc = 0;
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
			
			if (cl -> len == -1 || cl -> dlen == -1)
			{
				// try resolving the instruction length
				// but don't force resolution
				if (cl -> insn >= 0 && instab[cl -> insn].resolve)
				{
					(instab[cl -> insn].resolve)(as, cl, 0);
					debug_message(as, 100, "len = %d, dlen = %d", cl -> len, cl -> dlen);
					if ((cl -> inmod == 0) && cl -> len >= 0 && cl -> dlen >= 0)
					{
						if (cl -> len == 0)
							cl -> len = cl -> dlen;
						else
							cl -> dlen = cl -> len;
					}
					if (cl -> len != -1 && cl -> dlen != -1)
						rc++;
				}
			}
		}
		if (as -> errorcount > 0)
			return;
	} while (rc > 0);
}
