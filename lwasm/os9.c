/*
os9.c
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


This file implements the various pseudo operations related to OS9 target
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"


// OS9 syscall
PARSEFUNC(pseudo_parse_os9)
{
	lw_expr_t e;
	
	if (as -> output_format != OUTPUT_OS9 && as -> output_format != OUTPUT_OBJ)
	{
		lwasm_register_error2(as, l, E_DIRECTIVE_OS9_ONLY, "%s", "os9");
		return;
	}
	
	// fetch immediate value
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 0, e);
	l -> len = 3;
}

EMITFUNC(pseudo_emit_os9)
{
	lw_expr_t e;
	
	e = lwasm_fetch_expr(l, 0);

	lwasm_emitop(l, 0x103f);
	lwasm_emitexpr(l, e, 1);
}

PARSEFUNC(pseudo_parse_mod)
{
	lw_expr_t e;
	int i;
	
	if (as -> output_format != OUTPUT_OS9)
	{
		lwasm_register_error2(as, l, E_DIRECTIVE_OS9_ONLY, "%s", "mod");
		skip_operand(p);
		return;
	}
	
	if (as -> inmod)
	{
		lwasm_register_error(as, l, E_MODULE_IN);
		skip_operand(p);
		return;
	}

	// parse 6 expressions...
	for (i = 0; i < 5; i++)
	{
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}

		lwasm_save_expr(l, i, e);

		if (**p != ',')
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		(*p)++;
	}

	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 5, e);

	l -> inmod = 1;
	
	// we have an implicit ORG 0 with "mod"
	lw_expr_destroy(l -> addr);
	l -> addr = lw_expr_build(lw_expr_type_int, 0);

	// likewise for data pointer
	lw_expr_destroy(l -> daddr);
	l -> daddr = lw_expr_build(lw_expr_type_int, 0);

	// init crc
	as -> inmod = 1;
	
	l -> len = 13;
}

EMITFUNC(pseudo_emit_mod)
{
	lw_expr_t e1, e2, e3, e4;
	int csum;
	
	as -> crc[0] = 0xff;
	as -> crc[1] = 0xff;
	as -> crc[2] = 0xff;

	// sync bytes
	lwasm_emit(l, 0x87);
	lwasm_emit(l, 0xcd);
	
	// mod length
	lwasm_emitexpr(l, e1 = lwasm_fetch_expr(l, 0), 2);
	
	// name offset
	lwasm_emitexpr(l, e2 = lwasm_fetch_expr(l, 1), 2);
	
	// type
	lwasm_emitexpr(l, e3 = lwasm_fetch_expr(l, 2), 1);
	
	// flags/rev
	lwasm_emitexpr(l, e4 = lwasm_fetch_expr(l, 3), 1);
	
	// header check
	csum = ~(0x87 ^ 0xCD ^(lw_expr_intval(e1) >> 8) ^ (lw_expr_intval(e1) & 0xff)
		^ (lw_expr_intval(e2) >> 8) ^ (lw_expr_intval(e2) & 0xff)
		^ lw_expr_intval(e3) ^ lw_expr_intval(e4));
	lwasm_emit(l, csum);

	// module type specific output
	// note that these are handled the same for all so
	// there need not be any special casing
	
	// exec offset or fmgr name offset
	lwasm_emitexpr(l, lwasm_fetch_expr(l, 4), 2);
	
	// data size or drvr name offset
	lwasm_emitexpr(l, lwasm_fetch_expr(l, 5), 2);
}

PARSEFUNC(pseudo_parse_emod)
{
	skip_operand(p);
	if (as -> output_format != OUTPUT_OS9)
	{
		lwasm_register_error2(as, l, E_DIRECTIVE_OS9_ONLY, "%s", "emod");
		return;
	}
	
	if (!(as -> inmod))
	{
		lwasm_register_error(as, l, E_MODULE_NOTIN);
		return;
	}
	
	as -> inmod = 0;
	l -> len = 3;
}

EMITFUNC(pseudo_emit_emod)
{
	unsigned char tcrc[3];
	
	// don't mess with CRC!
	tcrc[0] = as -> crc[0] ^ 0xff;
	tcrc[1] = as -> crc[1] ^ 0xff;
	tcrc[2] = as -> crc[2] ^ 0xff;
	lwasm_emit(l, tcrc[0]);
	lwasm_emit(l, tcrc[1]);
	lwasm_emit(l, tcrc[2]);
}
