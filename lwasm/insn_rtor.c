/*
insn_rtor.c
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

*/

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(insn_parse_rtor)
{
	int r0, r1;

	static const char *regs = "D X Y U S PCW V A B CCDP0 0 E F ";
	static const char *regs9 = "D X Y U S PC    A B CCDP        ";
		
	// register to register (r0,r1)
	// registers are in order:
	// D,X,Y,U,S,PC,W,V
	// A,B,CC,DP,0,0,E,F

	r0 = lwasm_lookupreg2(!CURPRAGMA(l, PRAGMA_6809) ? regs : regs9, p);
	lwasm_skip_to_next_token(l, p);
	if (r0 < 0 || *(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		r0 = r1 = 0;
	}
	else
	{
		lwasm_skip_to_next_token(l, p);
		r1 = lwasm_lookupreg2(!CURPRAGMA(l, PRAGMA_6809) ? regs : regs9, p);
		if (r1 < 0)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			r0 = r1 = 0;
		}
	}
	l -> pb = (r0 << 4) | r1;
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
}

EMITFUNC(insn_emit_rtor)
{
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emit(l, l -> pb);
}
