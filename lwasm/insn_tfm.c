/*
insn_tfm.c
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

#include <ctype.h>
#include <string.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(insn_parse_tfm)
{
	static const char *reglist = "DXYUS   AB  00EF";
	int r0, r1;
	char *c;
	int tfm = 0;
			
	c = strchr(reglist, toupper(*(*p)++));
	if (!c)
	{
		lwasm_register_error(as, l, E_REGISTER_BAD);
		return;
	}
	r0 = c - reglist;
	if (**p == '+')
	{
		(*p)++;
		tfm = 1;
	}
	else if (**p == '-')
	{
		(*p)++;
		tfm = 2;
	}
	lwasm_skip_to_next_token(l, p);
	if (*(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_UNKNOWN_OPERATION);
		return;
	}
	lwasm_skip_to_next_token(l, p);
	c = strchr(reglist, toupper(*(*p)++));
	if (!c)
	{
		lwasm_register_error(as, l, E_REGISTER_BAD);
		return;
	}
	r1 = c - reglist;

	if (**p == '+')
	{
		(*p)++;
		tfm |= 4;
	}
	else if (**p == '-')
	{
		(*p)++;
		tfm |= 8;
	}
	
	if (**p && !isspace(**p))
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	/* only D, X, Y, U, S are valid tfm registers */
	if (r0 > 4 || r1 > 4)
	{
		if (r0 < r1) r0 = r1;
		lwasm_register_error2(as, l, E_REGISTER_BAD, "'%c'", reglist[r0]);
	}

	// valid values of tfm here are:
	// 1: r0+,r1 (2)
	// 4: r0,r1+ (3)
	// 5: r0+,r1+ (0)
	// 10: r0-,r1- (1)
	switch (tfm)
	{
	case 5: //r0+,r1+
		l -> lint =  instab[l -> insn].ops[0];
		break;

	case 10: //r0-,r1-
		l -> lint = instab[l -> insn].ops[1];
		break;

	case 1: // r0+,r1
		l -> lint = instab[l -> insn].ops[2];
		break;

	case 4: // r0,r1+
		l -> lint = instab[l -> insn].ops[3];
		break;

	default:
		lwasm_register_error(as, l, E_UNKNOWN_OPERATION);
		return;
	}
	l -> pb = (r0 << 4) | r1;
	l -> len = OPLEN(l -> lint) + 1;
}

EMITFUNC(insn_emit_tfm)
{
	lwasm_emitop(l, l -> lint);
	lwasm_emit(l, l -> pb);
}

PARSEFUNC(insn_parse_tfmrtor)
{
	int r0, r1;
	static const char *regs = "D X Y U S       A B     0 0 E F ";
	
	// register to register (r0,r1)
	// registers are in order:
	// D,X,Y,U,S,PC,W,V
	// A,B,CC,DP,0,0,E,F
	r0 = lwasm_lookupreg2(regs, p);
	lwasm_skip_to_next_token(l, p);
	if (r0 < 0 || *(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		r0 = r1 = 0;
	}
	else
	{
		lwasm_skip_to_next_token(l, p);
		r1 = lwasm_lookupreg2(regs, p);
		if (r1 < 0)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			r0 = r1 = 0;
		}
	}
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
	l -> pb = (r0 << 4) | r1;
}

EMITFUNC(insn_emit_tfmrtor)
{
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emit(l, l -> pb);
}
