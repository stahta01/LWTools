/*
insn_rlist.c
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
for handling inherent mode instructions
*/

#include <ctype.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(insn_parse_rlist)
{
	int rb = 0;
	int rn;
	static const char *regs = "CCA B DPX Y U PCD S ";

	while (**p && !isspace(**p) && **p != ';' && **p != '*')
	{
		rn = lwasm_lookupreg2(regs, p);
		if (rn < 0)
		{
			lwasm_register_error2(as, l, E_REGISTER_BAD, "'%s'", *p);
			return;
		}
		lwasm_skip_to_next_token(l, p);
		if (**p && **p != ',' && !isspace(**p) && **p != ';' && **p != '*')
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
		}
		if (**p == ',')
		{
			(*p)++;
			lwasm_skip_to_next_token(l, p);
		}
		if ((instab[l -> insn].ops[0]) & 2)
		{
			// pshu/pulu
			if (rn == 6)
			{
				lwasm_register_error2(as, l, E_REGISTER_BAD, "'%s'", "u");
				return;
			}
		}
		else
		{
			if (rn == 9)
			{
				lwasm_register_error2(as, l, E_REGISTER_BAD, "'%s'", "s");
				return;
			}
		}
		if (rn == 8)
			rn = 6;
		else if (rn == 9)
			rn = 0x40;
		else
			rn = 1 << rn;
		rb |= rn;
	}
	if (rb == 0)
		lwasm_register_error(as, l, E_OPERAND_BAD);
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
	l -> pb = rb;
}

EMITFUNC(insn_emit_rlist)
{
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emit(l, l -> pb);

	l -> cycle_adj = lwasm_cycle_calc_rlist(l);
}
