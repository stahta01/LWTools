/*
insn_gen.c, Copyright © 2009 William Astle

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

Contains code for parsing general addressing modes (IMM+DIR+EXT+IND)
*/

#include <ctype.h>
#include <stdlib.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

void insn_indexed_parse_aux(asmstate_t *as, line_t *l, char **p);
void insn_indexed_resolve_aux(asmstate_t *as, line_t *l, int force, int elen);
void insn_indexed_emit_aux(asmstate_t *as, line_t *l);

void insn_parse_indexed_aux(asmstate_t *as, line_t *l, char **p);
void insn_resolve_indexed_aux(asmstate_t *as, line_t *l, int force, int elen);

// "extra" is required due to the way OIM, EIM, TIM, and AIM work
void insn_parse_gen_aux(asmstate_t *as, line_t *l, char **p, int elen)
{
	char *optr2;
	int v1, tv;
	lw_expr_t s;
	
	if (!**p)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}

	/* this is the easy case - start it [ or , means indexed */
	if (**p == ',' || **p == '[')
	{
indexed:
		l -> lint = -1;
		l -> lint2  = 1;
		insn_parse_indexed_aux(as, l, p);
		l -> minlen = OPLEN(instab[l -> insn].ops[1]) + 1 + elen;
		l -> maxlen = OPLEN(instab[l -> insn].ops[1]) + 3 + elen;
		goto out;
	}

	/* we have to parse the first expression to find if we have a comma after it */
	optr2 = *p;
	if (**p == '<')
	{
		(*p)++;
		l -> lint2 = 0;
		if (**p == '<')
		{
			*p = optr2;
			goto indexed;
		}
	}
	// for compatibility with asxxxx
	// * followed by a digit, alpha, or _, or ., or ?, or another * is "f8"
	else if (**p == '*')
	{
		tv = *(*p + 1);
		if (isdigit(tv) || isalpha(tv) || tv == '_' || tv == '.' || tv == '?' || tv == '@' || tv == '*' || tv == '+' || tv == '-')
		{
			l -> lint2 = 0;
			(*p)++;
		}
	}
	else if (**p == '>')
	{
		(*p)++;
		l -> lint2 = 2;
	}
	else
	{
		l -> lint2 = -1;
	}
	lwasm_skip_to_next_token(l, p);
	
	s = lwasm_parse_expr(as, p);
	
	if (**p == ',')
	{
		/* we have an indexed mode here - reset and transfer control to indexing mode */
		lw_expr_destroy(s);
		*p = optr2;
		goto indexed;
	}
	if (!s)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	
	lwasm_save_expr(l, 0, s);

	l -> minlen = OPLEN(instab[l -> insn].ops[0]) + 1 + elen;
	l -> maxlen = OPLEN(instab[l -> insn].ops[2]) + 2 + elen;
	if (as -> output_format == OUTPUT_OBJ && l -> lint2 == -1)
	{
		l -> lint2 = 2;
		goto out;
	}

	if (l -> lint2 != -1)
		goto out;

	// if we have a constant now, figure out dp vs nondp
	if (lw_expr_istype(s, lw_expr_type_int))
	{
		if (s -> value > 0xffff) lwasm_register_error(as, l, E_BYTE_OVERFLOW);

		v1 = lw_expr_intval(s);
		if (((v1 >> 8) & 0xff) == (l -> dpval & 0xff))
		{
			l -> lint2 = 0;
			goto out;
		}
		l -> lint2 = 2;
	}
	else
	{
		int min;
		int max;
		
		if (lwasm_calculate_range(as, s, &min, &max) == 0)
		{
//			fprintf(stderr, "range (P) %d...%d for %s\n", min, max, lw_expr_print(s));
			if (min > max)
			{
				// we don't know what to do in this case so don't do anything
				goto out;
			}
			min = (min >> 8) & 0xff;
			max = (max >> 8) & 0xff;
			if ((l -> dpval & 0xff) < min || (l -> dpval & 0xff) > max)
			{
				l -> lint2 = 2;
				goto out;
			}
			if (min == max && (l -> dpval & 0xff) == min)
			{
				l -> lint2 = 0;
				goto out;
			}
			// if here, we don't know if the value is in the DP or not
			{
				l -> lint2 = -1;
				goto out;
			}
		}
	}

out:
	if (l -> lint2 != -1)
	{
		if (l -> lint2 == 0)
		{
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1 + elen;
		}
		else if (l -> lint2 == 2)
		{
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 2 + elen;
		}
		else if (l -> lint2 == 1 && l -> lint != -1)
		{
			if (l -> lint == 3)
				l -> len = OPLEN(instab[l -> insn].ops[1]) + 1 + elen;
			else
				l -> len = OPLEN(instab[l -> insn].ops[1]) + l -> lint + 1 + elen;
		}
	}
}

void insn_resolve_gen_aux(asmstate_t *as, line_t *l, int force, int elen)
{
	lw_expr_t e;
	
	if (l -> lint2 == 1)
	{
		// indexed
		insn_resolve_indexed_aux(as, l, force, elen);
		goto out;
	}
	
	if (l -> lint2 != -1)
		return;
	
	e = lwasm_fetch_expr(l, 0);
	lwasm_reduce_expr(as, e);
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		int v;
		
		v = lw_expr_intval(e);

		if (((v >> 8) & 0xff) == (l -> dpval & 0xff))
		{
			l -> lint2 = 0;
			goto out;
		}
		l -> lint2 = 2;
		goto out;
	}
	else
	{
		int min;
		int max;
		
		if (lwasm_calculate_range(as, e, &min, &max) == 0)
		{
//			fprintf(stderr, "range (R) %d...%d for %s\n", min, max, lw_expr_print(e));
			if (min > max)
			{
				// we don't know what to do in this case so don't do anything
				goto out;
			}
			min = (min >> 8) & 0xff;
			max = (max >> 8) & 0xff;
			if ((l -> dpval & 0xff) < min || (l -> dpval & 0xff) > max)
			{
				l -> lint2 = 2;
				goto out;
			}
			if (min == max && (l -> dpval & 0xff) == min)
			{
				l -> lint2 = 0;
				goto out;
			}
			// if here, we don't know if the value is in the DP or not
			{
				l -> lint2 = -1;
				goto out;
			}
		}
	}

	if (force)
	{
		l -> lint2 = 2;
	}

out:
	if (l -> lint2 != -1)
	{
		if (l -> lint2 == 0)
		{
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1 + elen;
		}
		else if (l -> lint2 == 2)
		{
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 2 + elen;
		}
		else if (l -> lint2 == 1 && l -> lint != -1)
		{
			if (l -> lint == 3)
				l -> len = OPLEN(instab[l -> insn].ops[1]) + 1 + elen;
			else
				l -> len = OPLEN(instab[l -> insn].ops[1]) + l -> lint + 1 + elen;
		}
	}
}

void insn_emit_gen_aux(asmstate_t *as, line_t *l, int extra)
{
	lw_expr_t e;
	
	e = lwasm_fetch_expr(l, 0);
	lwasm_emitop(l, instab[l -> insn].ops[l -> lint2]);
	
	if (extra != -1)
		lwasm_emit(l, extra);
	
	if (l -> lint2 == 1)
	{
		if (l -> lint == 3)
		{
			int offs;
			if (lw_expr_istype(e, lw_expr_type_int))
			{
				offs = lw_expr_intval(e);
				if ((offs >= -16 && offs <= 15) || offs >= 0xFFF0)
				{
					l -> pb |= offs & 0x1f;
					l -> lint = 0;
				}
				else
				{
					lwasm_register_error(as, l, E_BYTE_OVERFLOW);
				}
			}
			else
			{
				lwasm_register_error(as, l, E_EXPRESSION_NOT_RESOLVED);
			}
		}
		lwasm_emit(l, l -> pb);
		if (l -> lint > 0)
		{
			int i;
			i = lw_expr_intval(e);
			if (l -> lint == 1)
			{
				if (i < -128 || i > 127)
					lwasm_register_error(as, l, E_BYTE_OVERFLOW);
			}
			else if (l -> lint == 2 && lw_expr_istype(e, lw_expr_type_int) && CURPRAGMA(l, PRAGMA_OPERANDSIZE))
			{
				// note that W relative and extended indirect must be 16 bits
				if (l -> pb != 0xAF && l -> pb != 0xB0 && l -> pb != 0x9f)
				{
					if ((i >= -128 && i <= 127) || i >= 0xFF80)
					{
						lwasm_register_error(as, l, W_OPERAND_SIZE);

					}
				}
			}
			lwasm_emitexpr(l, e, l -> lint);
		}

		l -> cycle_adj = lwasm_cycle_calc_ind(l);
		return;
	}
	
	if (l -> lint2 == 2)
	{
		lwasm_emitexpr(l, e, 2);

		if (CURPRAGMA(l, PRAGMA_OPERANDSIZE))
		{
			if (instab[l -> insn].ops[2] == 0xbd || instab[l -> insn].ops[2] == 0x7e)
			{
				// check if bsr or bra could be used instead
				lw_expr_t e1, e2;
				int offs;
				e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
				e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e, e2);
				lw_expr_destroy(e2);
				e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e1, l -> addr);
				lw_expr_destroy(e1);
				lwasm_reduce_expr(as, e2);
				if (lw_expr_istype(e2, lw_expr_type_int))
				{
					offs = lw_expr_intval(e2);
					if (offs >= -128 && offs <= 127)
					{
						lwasm_register_error(as, l, W_OPERAND_SIZE);
					}
				}
				lw_expr_destroy(e2);
			}
		}
	}
	else
	{
		lwasm_emitexpr(l, e, 1);
	}
}

// the various insn_gen? functions have an immediate mode of ? bits
PARSEFUNC(insn_parse_gen0)
{
	if (**p == '#')
	{
		lwasm_register_error(as, l, E_IMMEDIATE_INVALID);
		return;
	}
	
	// handle non-immediate
	insn_parse_gen_aux(as, l, p, 0);
}

RESOLVEFUNC(insn_resolve_gen0)
{
	if (l -> len != -1)
		return;

	// handle non-immediate
	insn_resolve_gen_aux(as, l, force, 0);
}

EMITFUNC(insn_emit_gen0)
{
	insn_emit_gen_aux(as, l, -1);
}

PARSEFUNC(insn_parse_gen8)
{
	l -> genmode = 8;
	if (**p == '#')
	{
		lw_expr_t e;
		
		(*p)++;
		as -> exprwidth = 8;
		e = lwasm_parse_expr(as, p);
		as -> exprwidth = 16;
		if (!e)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		l -> len = OPLEN(instab[l -> insn].ops[3]) + 1;
		l -> lint2 = 3;
		lwasm_save_expr(l, 0, e);
		return;
	}
	
	// handle non-immediate
	insn_parse_gen_aux(as, l, p, 0);
	if (l -> lint2 != -1)
	{
		if (l -> lint2 == 0)
		{
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		}
		else if (l -> lint2 == 2)
		{
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 2;
		}
		else if (l -> lint2 == 1 && l -> lint != -1)
		{
			if (l -> lint == 3)
				l -> len = OPLEN(instab[l -> insn].ops[1]) + 1;
			else
				l -> len = OPLEN(instab[l -> insn].ops[1]) + l -> lint + 1;
		}
	}
}

RESOLVEFUNC(insn_resolve_gen8)
{
	if (l -> len != -1)
		return;

	// handle non-immediate
	insn_resolve_gen_aux(as, l, force, 0);
}

EMITFUNC(insn_emit_gen8)
{
	if (l -> lint2 == 3)
	{
		lw_expr_t e;
		e = lwasm_fetch_expr(l, 0);
		if (lw_expr_istype(e, lw_expr_type_int))
		{
			int i;
			i = lw_expr_intval(e);
			if (i < -128 || i > 255)
			{
				lwasm_register_error(as, l, E_BYTE_OVERFLOW);
			}
		}

		lwasm_emitop(l, instab[l -> insn].ops[3]);
		lwasm_emitexpr(l, e, 1);
		return;
	}

	insn_emit_gen_aux(as, l, -1);
}

PARSEFUNC(insn_parse_gen16)
{
	l -> genmode = 16;
	if (**p == '#')
	{
		lw_expr_t e;
		
		(*p)++;
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		l -> len = OPLEN(instab[l -> insn].ops[3]) + 2;
		l -> lint2 = 3;
		lwasm_save_expr(l, 0, e);
		return;
	}
	
	// handle non-immediate
	insn_parse_gen_aux(as, l, p, 0);
	if (l -> lint2 != -1)
	{
		if (l -> lint2 == 0)
		{
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		}
		else if (l -> lint2 == 2)
		{
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 2;
		}
		else if (l -> lint2 == 1 && l -> lint != -1)
		{
			if (l -> lint == 3)
				l -> len = OPLEN(instab[l -> insn].ops[1]) + 1;
			else
				l -> len = OPLEN(instab[l -> insn].ops[1]) + l -> lint + 1;
		}
	}
}

RESOLVEFUNC(insn_resolve_gen16)
{
	if (l -> len != -1)
		return;

	// handle non-immediate
	insn_resolve_gen_aux(as, l, force, 0);
}

EMITFUNC(insn_emit_gen16)
{
	if (l -> lint2 == 3)
	{
		lw_expr_t e;
		e = lwasm_fetch_expr(l, 0);
		lwasm_emitop(l, instab[l -> insn].ops[3]);
		lwasm_emitexpr(l, e, 2);
		return;
	}

	insn_emit_gen_aux(as, l, -1);
}

PARSEFUNC(insn_parse_gen32)
{
	l -> genmode = 32;
	if (**p == '#')
	{
		lw_expr_t e;
		
		(*p)++;
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		l -> len = OPLEN(instab[l -> insn].ops[3]) + 4;
		l -> lint2 = 3;
		lwasm_save_expr(l, 0, e);
		return;
	}
	
	// handle non-immediate
	insn_parse_gen_aux(as, l, p, 0);
	if (l -> lint2 != -1)
	{
		if (l -> lint2 == 0)
		{
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		}
		else if (l -> lint2 == 2)
		{
			l -> len = OPLEN(instab[l -> insn].ops[2]) + 2;
		}
		else if (l -> lint2 == 1 && l -> lint != -1)
		{
			if (l -> lint == 3)
				l -> len = OPLEN(instab[l -> insn].ops[1]) + 1;
			else
				l -> len = OPLEN(instab[l -> insn].ops[1]) + l -> lint + 1;
		}
	}
}

RESOLVEFUNC(insn_resolve_gen32)
{
	if (l -> len != -1)
		return;

	// handle non-immediate
	insn_resolve_gen_aux(as, l, force, 0);
}

EMITFUNC(insn_emit_gen32)
{
	if (l -> lint2 == 3)
	{
		lw_expr_t e;
		e = lwasm_fetch_expr(l, 0);
		lwasm_emitop(l, instab[l -> insn].ops[3]);
		lwasm_emitexpr(l, e, 4);
		return;
	}

	insn_emit_gen_aux(as, l, -1);
}

PARSEFUNC(insn_parse_imm8)
{
	lw_expr_t e;
	
	if (**p == '#')
	{
		(*p)++;

		as -> exprwidth = 8;
		e = lwasm_parse_expr(as, p);
		as -> exprwidth = 16;
		if (!e)
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		lwasm_save_expr(l, 0, e);
	}
	else
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
	}
}

EMITFUNC(insn_emit_imm8)
{
	lw_expr_t e;
	
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	e = lwasm_fetch_expr(l, 0);
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		int i;
		i = lw_expr_intval(e);
		if (i < -128 || i > 255)
		{
			lwasm_register_error(as, l, E_BYTE_OVERFLOW);
		}
	}
	lwasm_emitexpr(l, e, 1);
}
