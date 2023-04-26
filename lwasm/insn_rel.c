/*
insn_rel.c
Copyright © 2009 William Astle

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
*/

/*
for handling relative mode instructions
*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

/*
For generic relative, the first "opcode" is the natural opcode for the
mneumonic. The second "opcode" is the natural size of the relative offset.
These will be used when pragma autobranchlength is NOT in effect.

The third "opcode" is the short (8 bit) version of the branch. The final one
is the long (16 bit) version of the branch. These will be used when pragma
autobranchlength is in effect.

When autobranchlength is in effect, the branch target can be prefixed with
either < or > to force a short or long branch. Note that in this mode,
a > or < on its own still specifies a branch point.

*/
PARSEFUNC(insn_parse_relgen)
{
	lw_expr_t t = NULL, e1, e2;
	
	l -> lint = -1;
	l -> maxlen = OPLEN(instab[l -> insn].ops[3]) + 2;
	l -> minlen = OPLEN(instab[l -> insn].ops[2]) + 1;
	if (CURPRAGMA(l, PRAGMA_AUTOBRANCHLENGTH) == 0)
	{
		l -> lint = instab[l -> insn].ops[1];
	}
	else
	{
		if (**p == '>' && (((*p)[1]) && !isspace((*p)[1])))
		{
			(*p)++;
			l -> lint = 16;
		}
		else if (**p == '<' && (((*p)[1]) && !isspace((*p)[1])))
		{
			(*p)++;
			l -> lint = 8;
		}
	}
	
	/* forced sizes handled */
	
	// sometimes there is a "#", ignore if there
	if (**p == '#')
		(*p)++;

	if (CURPRAGMA(l, PRAGMA_QRTS))
	{
		// handle ?RTS conditional return
		if (**p == '?')
		{
			if (strncasecmp(*p, "?RTS", 4) == 0)
			{
				(*p) += 4;

				line_t *cl = l;
				for (cl = cl->prev; cl; cl = cl->prev)
				{
					if (cl->insn == -1)
						continue;

					if (l->addr->value - cl->addr->value > 128)
					{
						cl = NULL;
						break;
					}

					if (cl->conditional_return)
						break;

					if (instab[cl->insn].ops[0] == 0x39)
						break;
				}

				if (cl)
				{
					l->lint = -1;
					if (cl->conditional_return)
					{
						e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_lineaddr, cl);
						e1 = lw_expr_build(lw_expr_type_int, 2);
						t = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, e1, e2);
					}
					else
					{
						t = lw_expr_build(lw_expr_type_special, lwasm_expr_lineaddr, cl);
					}
				}
				else
				{
					l->conditional_return = 1;

					// t = * + 1

					e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_lineaddr, l);
					e1 = lw_expr_build(lw_expr_type_int, 1);
					t = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, e1, e2);

					lw_expr_destroy(e1);
					lw_expr_destroy(e2);
				}
			}
		}
	}
	
	if (!t)
	{
		t = lwasm_parse_expr(as, p);
	}

	if (!t)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}

	// if we know the length of the instruction, set it now
	if (l -> lint == 8)
	{
		l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
		if (l->conditional_return) l->len++;
	}
	else if (l -> lint == 16)
	{
		l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
	}

	// the offset calculation here depends on the length of this line!
	// how to calculate requirements?
	// this is the same problem faced by ,pcr indexing
	e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
	e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, t, e2);
	lw_expr_destroy(e2);
	e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e1, l -> addr);
	lw_expr_destroy(e1);
	lwasm_save_expr(l, 0, e2);
	lw_expr_destroy(t);

	if (l -> len == -1)
	{
		e1 = lw_expr_copy(e2);
		l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
		lwasm_reduce_expr(as, e1);
		l -> len = -1;
		if (lw_expr_istype(e1, lw_expr_type_int))
		{
			int v;
			v = lw_expr_intval(e1);
			if (v >= -128 && v <= 127)
			{
				l -> lint = 8;
				l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
			}
			else
			{
				l -> lint = 16;
				l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
			}
		}
		lw_expr_destroy(e1);
	}
}

RESOLVEFUNC(insn_resolve_relgen)
{
	lw_expr_t e, e2;
	int offs;
	
	if (l -> lint == -1)
	{
		e = lwasm_fetch_expr(l, 0);
		if (!lw_expr_istype(e, lw_expr_type_int))
		{
			// temporarily set the instruction length to see if we get a
			// constant for our expression; if so, we can select an instruction
			// size
			e2 = lw_expr_copy(e);
			// size of 8-bit opcode + 8 bit offset
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
			lwasm_reduce_expr(as, e2);
			l -> len = -1;
			if (lw_expr_istype(e2, lw_expr_type_int))
			{
				// it reduced to an integer; is it in 8 bit range?
				offs = lw_expr_intval(e2);
				if (offs >= -128 && offs <= 127)
				{
					// fits in 8 bits
					l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
					l -> lint = 8;
				}
				else
				{
					// requires 16 bits
					l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
					l -> lint = 16;
				}
			}
			// size of 8-bit opcode + 8 bit offset
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
			as -> pretendmax = 1;
			lwasm_reduce_expr(as, e2);
			as -> pretendmax = 0;
			l -> len = -1;
			if (lw_expr_istype(e2, lw_expr_type_int))
			{
				// it reduced to an integer; is it in 8 bit range?
				offs = lw_expr_intval(e2);
				if (offs >= -128 && offs <= 127)
				{
					// fits in 8 bits with a worst case scenario
					l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
					l -> lint = 8;
				}
			}
			lw_expr_destroy(e2);
		}
		if (lw_expr_istype(e, lw_expr_type_int))
		{
			// it reduced to an integer; is it in 8 bit range?
			offs = lw_expr_intval(e);
			if (offs >= -128 && offs <= 127)
			{
				// fits in 8 bits
				l -> len = OPLEN(instab[l -> insn].ops[2]) + 1;
				l -> lint = 8;
			}
			else
			{
				// requires 16 bits
				l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
				l -> lint = 16;
			}
		}
	}
	if (!force)
		return;
		
	if (l -> len == -1)
	{
		l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
		l -> lint = 16;
	}
}

EMITFUNC(insn_emit_relgen)
{
	lw_expr_t e;
	int offs;
	
	e = lwasm_fetch_expr(l, 0);
	if (l -> lint == 8)
	{
		if (!lw_expr_istype(e, lw_expr_type_int))
		{
			lwasm_register_error(as, l, E_EXPRESSION_NOT_CONST);
			return;
		}
	
		offs = lw_expr_intval(e);
		if (l -> lint == 8 && (offs < -128 || offs > 127))
		{
			lwasm_register_error(as, l, E_BYTE_OVERFLOW);
			return;
		}

		if (l->conditional_return)
		{
			lwasm_emitop(l, instab[l->insn].ops[2] ^ 1);	/* flip branch, add RTS */
			lwasm_emit(l, 1);
			lwasm_emit(l, 0x39);
			l->cycle_adj = 3;
		}
		else
		{
			lwasm_emitop(l, instab[l->insn].ops[2]);
			lwasm_emit(l, offs);
		}
	}
	else
	{
		lwasm_emitop(l, instab[l -> insn].ops[3]);
		lwasm_emitexpr(l, e, 2);
		if (CURPRAGMA(l, PRAGMA_OPERANDSIZE) && lw_expr_istype(e, lw_expr_type_int))
		{
			offs = lw_expr_intval(e);
			if (offs >= -128 && offs <= 127)
			{
				lwasm_register_error(as, l, W_OPERAND_SIZE);
			}
		}
	}
}
