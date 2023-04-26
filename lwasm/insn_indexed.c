/*
insn_indexed.c
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
for handling indexed mode instructions
*/

#include <ctype.h>
#include <string.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

/*
l -> lint: size of operand (0, 1, 2, -1 if not determined)
l -> pb: actual post byte (from "resolve" stage) or info passed
	forward to the resolve stage (if l -> line is -1); 0x80 is indir
	bits 0-2 are register number
*/
void insn_parse_indexed_aux(asmstate_t *as, line_t *l, char **p)
{
	static const char *regs9 = "X  Y  U  S     PCRPC ";
	static const char *regs  = "X  Y  U  S  W  PCRPC ";
	int i, rn;
	int indir = 0;
	int f0 = 0;
	const char *reglist;
	lw_expr_t e;
	char *tstr;
	

	if (CURPRAGMA(l, PRAGMA_6809))
	{
		reglist = regs9;
	}
	else
	{
		reglist = regs;
	}
	// is it indirect?
	if (**p == '[')
	{
		indir = 1;
		(*p)++;
	}
	lwasm_skip_to_next_token(l, p);
	if (**p == ',')
	{
		int incdec = 0;
		/* we have a pre-dec, post-inc, or no offset mode here */
		(*p)++;
		lwasm_skip_to_next_token(l, p);
		if (**p == '-')
		{
			incdec = -1;
			(*p)++;
			if (**p == '-')
			{
				incdec = -2;
				(*p)++;
			}
			lwasm_skip_to_next_token(l, p);
		}
		/* allowed registers: X, Y, U, S, or W (6309) */
		switch (**p)
		{
		case 'x':
		case 'X':
			rn = 0;
			break;
		
		case 'y':
		case 'Y':
			rn = 1;
			break;
			
		case 'u':
		case 'U':
			rn = 2;
			break;
			
		case 's':
		case 'S':
			rn = 3;
			break;
			
		case 'w':
		case 'W':
			if (CURPRAGMA(l, PRAGMA_6809))
			{
				lwasm_register_error(as, l, E_OPERAND_BAD);
				return;
			}
			rn = 4;
			break;
			
		default:
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		(*p)++;
		lwasm_skip_to_next_token(l, p);
		if (**p == '+')
		{
			if (incdec != 0)
			{
				lwasm_register_error(as, l, E_OPERAND_BAD);
				return;
			}
			incdec = 1;
			(*p)++;
			if (**p == '+')
			{
				incdec = 2;
				(*p)++;
			}
			lwasm_skip_to_next_token(l, p);
		}
		if (indir)
		{
			if (**p != ']')
			{
				lwasm_register_error(as, l, E_OPERAND_BAD);
				return;
			}
			(*p)++;
		}
		if (indir || rn == 4)
		{
			if (incdec == 1 || incdec == -1)
			{
				lwasm_register_error(as, l, E_OPERAND_BAD);
				return;
			}
		}
		if (rn == 4)
		{
			if (indir)
			{
				if (incdec == 0)
					i = 0x90;
				else if (incdec == -2)
					i = 0xF0;
				else
					i = 0xD0;
			}
			else
			{
				if (incdec == 0)
					i = 0x8F;
				else if (incdec == -2)
					i = 0xEF;
				else
					i = 0xCF;
			}
		}
		else
		{
			switch (incdec)
			{
			case 0:
				i = 0x84;
				break;
			case 1:
				i = 0x80;
				break;
			case 2:
				i = 0x81;
				break;
			case -1:
				i = 0x82;
				break;
			case -2:
				i = 0x83;
				break;
			}
			i = (rn << 5) | i | (indir << 4);
		}
		l -> pb = i;
		l -> lint = 0;
		return;
	}
	i = toupper(**p);
	if (
			(i == 'A' || i == 'B' || i == 'D') ||
			(!CURPRAGMA(l, PRAGMA_6809) && (i == 'E' || i == 'F' || i == 'W'))
	   )
	{
		tstr = *p + 1;
		lwasm_skip_to_next_token(l, &tstr);
		if (*tstr == ',')
		{
			*p = tstr + 1;
			lwasm_skip_to_next_token(l, p);
			switch (**p)
			{
			case 'x':
			case 'X':
				rn = 0;
				break;
		
			case 'y':
			case 'Y':
				rn = 1;
				break;
			
			case 'u':
			case 'U':
				rn = 2;
				break;
			
			case 's':
			case 'S':
				rn = 3;
				break;
			
			default:
				lwasm_register_error(as, l, E_OPERAND_BAD);
				return;
			}
			(*p)++;
			lwasm_skip_to_next_token(l, p);
			if (indir)
			{
				if (**p != ']')
				{
					lwasm_register_error(as, l, E_OPERAND_BAD);
					return;
				}
				(*p)++;
			}
			
			switch (i)
			{
			case 'A':
				i = 0x86;
				break;
			
			case 'B':
				i = 0x85;
				break;
			
			case 'D':
				i = 0x8B;
				break;
			
			case 'E':
				i = 0x87;
				break;
			
			case 'F':
				i = 0x8A;
				break;
			
			case 'W':
				i = 0x8E;
				break;
			}
			l -> pb = i | (indir << 4) | (rn << 5);
			l -> lint = 0;
			return;
		}
	}
	
	/* we have the "expression" types now */
	if (**p == '<')
	{
		l -> lint = 1;
		(*p)++;
		if (**p == '<')
		{
			l -> lint = 3;
			(*p)++;
			if (indir)
			{
				lwasm_register_error(as, l, E_ILL5);
				return;
			}
		}
	}
	else if (**p == '>')
	{
		l -> lint = 2;
		(*p)++;
	}
	lwasm_skip_to_next_token(l, p);
	if (**p == '0')
	{
		tstr = *p + 1;
		lwasm_skip_to_next_token(l, &tstr);
		if (*tstr == ',')
		{
			f0 = 1;
		}
	}

	// now we have to evaluate the expression
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, E_OPERAND_BAD);
		return;
	}
	lwasm_save_expr(l, 0, e);
	
	if (**p != ',')
	{
		/* if no comma, we have extended indirect */
		if (l -> lint == 1 || **p != ']')
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		(*p)++;
		l -> lint = 2;
		l -> pb = 0x9F;
		return;
	}
	(*p)++;
	lwasm_skip_to_next_token(l, p);
	// now get the register
	rn = lwasm_lookupreg3(reglist, p);
	if (rn < 0)
	{
		lwasm_register_error(as, l, E_REGISTER_BAD);
		return;
	}
	
	if (indir)
	{
		if (**p != ']')
		{
			lwasm_register_error(as, l, E_OPERAND_BAD);
			return;
		}
		else
			(*p)++;
	}

	if (rn <= 3)
	{
		// X,Y,U,S
		if (l -> lint == 1)
		{
			l -> pb = 0x88 | (rn << 5) | (indir ? 0x10 : 0);
			return;
		}
		else if (l -> lint == 2)
		{
			l -> pb = 0x89 | (rn << 5) | (indir ? 0x10 : 0);
			return;
		}
		else if (l -> lint == 3)
		{
			l -> pb = (rn << 5);
		}
	}

	// nnnn,W is only 16 bit (or 0 bit)
	if (rn == 4)
	{
		if (l -> lint == 1)
		{
			lwasm_register_error(as, l, E_NW_8);
			return;
		}
		else if (l -> lint == 3)
		{
			lwasm_register_error(as, l, E_ILL5);
			return;
		}

		if (l -> lint == 2)
		{
			l -> pb = indir ? 0xb0 : 0xaf;
			l -> lint = 2;
			return;
		}
		
		l -> pb = (0x80 * indir) | rn;

/* [,w] and ,w
			if (indir)
				*b1 = 0x90;
			else
				*b1 = 0x8f;
*/
		return;
	}
	
	// PCR? then we have PC relative addressing (like B??, LB??)
	if (rn == 5 || (rn == 6 && CURPRAGMA(l, PRAGMA_PCASPCR)))
	{
		lw_expr_t e1, e2;
		// external references are handled exactly the same as for
		// relative addressing modes
		// on pass 1, adjust the expression for a subtraction of the
		// current address
		// e - (addr + linelen) => e - addr - linelen
		
		e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
		e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e, e2);
		lw_expr_destroy(e2);
		e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e1, l -> addr);
		lw_expr_destroy(e1);
		lwasm_save_expr(l, 0, e2);
		if (l -> lint == 1)
		{
			l -> pb = indir ? 0x9C : 0x8C;
			return;
		}
		else if (l -> lint == 2)
		{
			l -> pb = indir ? 0x9D : 0x8D;
			return;
		}
		else if (l -> lint == 3)
		{
			lwasm_register_error(as, l, E_ILL5);
			return;
		}
	}
	
	if (rn == 6)
	{
		if (l -> lint == 1)
		{
			l -> pb = indir ? 0x9C : 0x8C;
			return;
		}
		else if (l -> lint == 2)
		{
			l -> pb = indir ? 0x9D : 0x8D;
			return;
		}
		else if (l -> lint == 3)
		{
			lwasm_register_error(as, l, E_ILL5);
			return;
		}
	}

	if (l -> lint != 3)
		l -> pb = (indir * 0x80) | rn | (f0 * 0x40);
}

PARSEFUNC(insn_parse_indexed)
{
	l -> lint = -1;
	insn_parse_indexed_aux(as, l, p);

	if (l -> lint != -1)
	{
		if (l -> lint == 3)
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		else
			l -> len = OPLEN(instab[l -> insn].ops[0]) + l -> lint + 1;
	}
}

void insn_resolve_indexed_aux(asmstate_t *as, line_t *l, int force, int elen)
{
	// here, we have an expression which needs to be
	// resolved; the post byte is determined here as well
	lw_expr_t e, e2;
	int pb = -1;
	int v;
	
	if (l -> len != -1)
		return;

	e = lwasm_fetch_expr(l, 0);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		// temporarily set the instruction length to see if we get a
		// constant for our expression; if so, we can select an instruction
		// size
		e2 = lw_expr_copy(e);
		// magic 2 for 8 bit (post byte + offset)
		l -> len = OPLEN(instab[l -> insn].ops[0]) + elen + 2;
		lwasm_reduce_expr(as, e2);
//		l -> len += 1;
//		e3 = lw_expr_copy(e);
//		lwasm_reduce_expr(as, e3);
		l -> len = -1;
		if (lw_expr_istype(e2, lw_expr_type_int))
		{
			v = lw_expr_intval(e2);
			// we have a reducible expression here which depends on
			// the size of this instruction
			if (v == 0 && !CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) && (l -> pb & 0x07) <= 4)
			{
				if ((l -> pb & 0x07) < 4)
				{
					pb = 0x84 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
				}
				else
				{
					pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
				}
				l -> pb = pb;
				lw_expr_destroy(e2);
				l -> lint = 0;
				return;
			}
			else if (v < -128 || v > 127)
			{
				l -> lint = 2;
				switch (l -> pb & 0x07)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					pb = 0x89 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
					break;
			
				case 4: // W
					pb = (l -> pb & 0x80) ? 0xB0 : 0xAF;
					break;
				
				case 5: // PCR
				case 6: // PC
					pb = (l -> pb & 0x80) ? 0x9D : 0x8D;
					break;
				}
				
				l -> pb = pb;
				lw_expr_destroy(e2);
//				lw_expr_destroy(e3);
				return;
			}
			else if ((l -> pb & 0x80) || ((l -> pb & 0x07) > 3) || v < -16 || v > 15)
			{
				// if not a 5 bit value, is indirect, or is not X,Y,U,S
				l -> lint = 1;
				switch (l -> pb & 0x07)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					pb = 0x88 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
					break;
			
				case 4: // W
					// use 16 bit because W doesn't have 8 bit, unless 0
					if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
					{
						pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
						l -> lint = 0;
					}
					else
					{
						pb = (l -> pb & 0x80) ? 0xB0 : 0xAF;
						l -> lint = 2;
					}
					break;
				
				case 5: // PCR
				case 6: // PC
					pb = (l -> pb & 0x80) ? 0x9C : 0x8C;
					break;
				}
			
				l -> pb = pb;
				lw_expr_destroy(e2);
				return;
			}
			else
			{
				// we have X,Y,U,S and a possible 5 bit here
				l -> lint = 0;
				
				if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
				{
					pb = (l -> pb & 0x03) << 5 | 0x84;
				}	
				else
				{
					pb = ((l -> pb & 0x03) << 5) | (v & 0x1F);
				}
				l -> pb = pb;
				lw_expr_destroy(e2);
				return;
			}
		}
		else
		{
			if ((l -> pb & 0x07) == 5 || (l -> pb & 0x07) == 6)
			{
				// NOTE: this will break in some particularly obscure corner cases
				// which are not likely to show up in normal code. Notably, if, for
				// some reason, the target gets *farther* away if shorter addressing
				// modes are chosen, which should only happen if the symbol is before
				// the instruction in the source file and there is some sort of ORG
				// statement or similar in between which forces the address of this
				// instruction, and the differences happen to cross the 8 bit boundary.
				// For this reason, we use a heuristic and allow a margin on the 8
				// bit boundary conditions.
				v = as -> pretendmax;
				as -> pretendmax = 1;
				lwasm_reduce_expr(as, e2);
				as -> pretendmax = v;
				if (lw_expr_istype(e2, lw_expr_type_int))
				{
					v = lw_expr_intval(e2);
					// Actual range is -128 <= offset <= 127; we're allowing a fudge
					// factor of 25 or so bytes so that we're less likely to accidentally
					// cross into the 16 bit boundary in weird corner cases.
					if (v >= -100 && v <= 100)
					{
						l -> lint = 1;
						l -> pb = (l -> pb & 0x80) ? 0x9C : 0x8C;
						return;
					}
				}
			}
		}
		lw_expr_destroy(e2);
	}
		
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		// we know how big it is
		v = lw_expr_intval(e);
			
		if (v == 0 && !CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) && (l -> pb & 0x07) <= 4 && ((l -> pb & 0x40) == 0))
		{
			if ((l -> pb & 0x07) < 4)
			{
				pb = 0x84 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
			}
			else
			{
				pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
			}
			l -> pb = pb;
			l -> lint = 0;
			return;
		}
		else if (v < -128 || v > 127)
		{
		do16bit:
			l -> lint = 2;
			switch (l -> pb & 0x07)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				pb = 0x89 | (l -> pb & 0x03) << 5 | ((l -> pb & 0x80) ? 0x10 : 0);
				break;
			
			case 4: // W
				pb = (l -> pb & 0x80) ? 0xB0 : 0xAF;
				break;
				
			case 5: // PCR
			case 6: // PC
				pb = (l -> pb & 0x80) ? 0x9D : 0x8D;
				break;
			}
			
			l -> pb = pb;
			return;
		}
		else if ((l -> pb & 0x80) || ((l -> pb & 0x07) > 3) || v < -16 || v > 15)
		{
			// if not a 5 bit value, is indirect, or is not X,Y,U,S
			l -> lint = 1;
			switch (l -> pb & 0x07)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				pb = 0x88 | (l -> pb & 0x03) << 5 | ((l -> pb & 0x80) ? 0x10 : 0);
				break;
			
			case 4: // W
				// use 16 bit because W doesn't have 8 bit, unless 0
				if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
				{
					pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
					l -> lint = 0;
				}
				else
				{
					pb = (l -> pb & 0x80) ? 0xB0 : 0xAF;
					l -> lint = 2;
				}
				break;
				
			case 5: // PCR
			case 6: // PC
				pb = (l -> pb & 0x80) ? 0x9C : 0x8C;
				break;
			}
			
			l -> pb = pb;
			return;
		}
		else
		{
			// we have X,Y,U,S and a possible 5 bit here
			l -> lint = 0;
			
			if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
			{
				pb = (l -> pb & 0x03) << 5 | 0x84;
			}
			else
			{
				pb = ((l -> pb & 0x03) << 5) | (v & 0x1F);
			}
			l -> pb = pb;
			return;
		}
	}
	else
	{
		// we don't know how big it is
		if (!force)
			return;
		// force 16 bit if we don't know
		l -> lint = 2;
		goto do16bit;
	}
}

RESOLVEFUNC(insn_resolve_indexed)
{
	if (l -> lint == -1)
		insn_resolve_indexed_aux(as, l, force, 0);
	
	if (l -> lint != -1 && l -> pb != -1)
	{
		if (l -> lint == 3)
			l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
		else
			l -> len = OPLEN(instab[l -> insn].ops[0]) + l -> lint + 1;
	}
}

void insn_emit_indexed_aux(asmstate_t *as, line_t *l)
{
	lw_expr_t e;
	
	if (l -> lint == 1)
	{
		int i;
		e = lwasm_fetch_expr(l, 0);
		i = lw_expr_intval(e);
		if (i < -128 || i > 127)
		{
			lwasm_register_error(as, l, E_BYTE_OVERFLOW);
		}
	}
	
	// exclude expr,W since that can only be 16 bits
	if (l -> lint == 3)
	{
		int offs;
		e = lwasm_fetch_expr(l, 0);
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
	// note that extended indirect (post byte 0x9f) can only be 16 bits
	else if (l -> lint == 2 && CURPRAGMA(l, PRAGMA_OPERANDSIZE) && (l -> pb != 0xAF && l -> pb != 0xB0 && l -> pb != 0x9f))
	{
		int offs;
		e = lwasm_fetch_expr(l, 0);
		if (lw_expr_istype(e, lw_expr_type_int))
		{
			offs = lw_expr_intval(e);
			if ((offs >= -128 && offs <= 127) || offs >= 0xFF80)
			{
				lwasm_register_error(as, l, W_OPERAND_SIZE);
			}
		}
	}
	
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emitop(l, l -> pb);

	l -> cycle_adj = lwasm_cycle_calc_ind(l);

	if (l -> lint > 0)
	{
		e = lwasm_fetch_expr(l, 0);
		lwasm_emitexpr(l, e, l -> lint);
	}
}

EMITFUNC(insn_emit_indexed)
{
	insn_emit_indexed_aux(as, l);
}
