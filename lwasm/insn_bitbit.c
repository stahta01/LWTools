/*
insn_bitbit.c
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

#include <stdlib.h>
#include <ctype.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

// these instructions cannot tolerate external references
PARSEFUNC(insn_parse_bitbit)
{
	int r;
	lw_expr_t e;
//	int v1;
//	int tv;

	r = toupper(*(*p)++);
	if (r == 'A')
		r = 1;
	else if (r == 'B')
		r = 2;
	else if (r == 'C' && toupper(**p) == 'C')
	{
		r = 0;
		(*p)++;
	}
	else
	{
		lwasm_register_error(as, l, E_REGISTER_BAD);
		return;
	}
	lwasm_skip_to_next_token(l, p);
	if (*(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 0, e);
	if (*(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}

	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 1, e);

	if (*(*p)++ != ',')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_skip_to_next_token(l, p);
	// ignore base page address modifier
	if (**p == '<')
		(*p)++;
			
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 2, e);

	l -> lint = r;
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 2;
}

EMITFUNC(insn_emit_bitbit)
{
	int v1, v2;
	lw_expr_t e;
	
	e = lwasm_fetch_expr(l, 0);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		lwasm_register_error(as, l, E_BITNUMBER_UNRESOLVED);
		return;
	}
	v1 = lw_expr_intval(e);
	if (v1 < 0 || v1 > 7)
	{
		lwasm_register_error(as, l, E_BITNUMBER_INVALID);
		v1 = 0;
	}

	e = lwasm_fetch_expr(l, 1);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		lwasm_register_error(as, l, E_BITNUMBER_UNRESOLVED);
		return;
	}
	v2 = lw_expr_intval(e);
	if (v2 < 0 || v2 > 7)
	{
		lwasm_register_error(as, l, E_BITNUMBER_INVALID);
		v2 = 0;
	}
	l -> pb = (l -> lint << 6) | (v1 << 3) | v2;
	
	e = lwasm_fetch_expr(l, 2);
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		v1 = lw_expr_intval(e) & 0xFFFF;
		v2 = v1 - ((l -> dpval) << 8);
		if (v2 > 0xFF || v2 < 0)
		{
			lwasm_register_error(as, l, E_BYTE_OVERFLOW);
			return;
		}
	}
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emit(l, l -> pb);
	lwasm_emitexpr(l, e, 1);
}
