/*
insn_logicmem.c
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

Contains code for handling logic/mem instructions
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

void insn_parse_gen_aux(asmstate_t *as, line_t *l, char **optr, int elen);
void insn_resolve_gen_aux(asmstate_t *as, line_t *l, int force, int elen);
void insn_emit_gen_aux(asmstate_t *as, line_t *l, int extra);

// for aim, oim, eim, tim
PARSEFUNC(insn_parse_logicmem)
{
//	const char *p2;
	lw_expr_t s;
	
	if (**p == '#')
		(*p)++;
	
	s = lwasm_parse_expr(as, p);
	if (!s)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	
	lwasm_save_expr(l, 100, s);
	lwasm_skip_to_next_token(l, p);
	if (**p != ',' && **p != ';')
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	
	(*p)++;
	lwasm_skip_to_next_token(l, p);
	// now we have a general addressing mode - call for it
	insn_parse_gen_aux(as, l, p, 1);
}

RESOLVEFUNC(insn_resolve_logicmem)
{
	if (l -> len != -1)
		return;
	
	insn_resolve_gen_aux(as, l, force, 1);
}

EMITFUNC(insn_emit_logicmem)
{
	lw_expr_t e;
	int v;
	
	e = lwasm_fetch_expr(l, 100);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		lwasm_register_error(as, l, E_IMMEDIATE_UNRESOLVED);
		return;
	}
	
	v = lw_expr_intval(e);
/*	if (v < -128 || v > 255)
	{
		fprintf(stderr, "BYTE: %d\n", v);
		lwasm_register_error(as, l, E_BYTE_OVERFLOW);
		return;
	}
*/	
	insn_emit_gen_aux(as, l, v & 0xff);
}
