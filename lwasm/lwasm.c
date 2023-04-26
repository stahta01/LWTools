/*
lwasm.c

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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <lw_expr.h>
#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_error.h>

#include "lwasm.h"
#include "instab.h"

void lwasm_skip_to_next_token(line_t *cl, char **p)
{
	if (CURPRAGMA(cl, PRAGMA_NEWSOURCE))
	{
		for (; **p && isspace(**p); (*p)++)
			/* do nothing */ ;
	}
}

int lwasm_expr_exportable(asmstate_t *as, lw_expr_t expr)
{
	return 0;
}

int lwasm_expr_exportval(asmstate_t *as, lw_expr_t expr)
{
	return 0;
}

void lwasm_dividezero(void *priv)
{
	asmstate_t *as = (asmstate_t *)priv;
	lwasm_register_error(as, as -> cl, E_DIV0);
}

lw_expr_t lwasm_evaluate_var(char *var, void *priv)
{
	asmstate_t *as = (asmstate_t *)priv;
	lw_expr_t e;
	importlist_t *im;
	struct symtabe *s;
	
	debug_message(as, 225, "eval var: look up %s", var);
	s = lookup_symbol(as, as -> cl, var);
	if (s)
	{
		debug_message(as, 225, "eval var: symbol found %s = %s (%p)", s -> symbol, lw_expr_print(s -> value), s);
		e = lw_expr_build(lw_expr_type_special, lwasm_expr_syment, s);
		return e;
	}
	
	if (as -> undefzero)
	{
		debug_message(as, 225, "eval var: undefined symbol treated as 0");
		e = lw_expr_build(lw_expr_type_int, 0);
		return e;
	}
	
	// undefined here is undefied unless output is object
	if (as -> output_format != OUTPUT_OBJ)
		goto nomatch;
	
	// check for import
	for (im = as -> importlist; im; im = im -> next)
	{
		if (!strcmp(im -> symbol, var))
			break;
	}
	
	// check for "undefined" to import automatically
	if ((as -> passno != 0) && !im && CURPRAGMA(as -> cl, PRAGMA_UNDEFEXTERN))
	{
		debug_message(as, 225, "eval var: importing undefined symbol");
		im = lw_alloc(sizeof(importlist_t));
		im -> symbol = lw_strdup(var);
		im -> next = as -> importlist;
		as -> importlist = im;
	}
	
	if (!im)
		goto nomatch;

	e = lw_expr_build(lw_expr_type_special, lwasm_expr_import, im);
	return e;

nomatch:
	if (as -> badsymerr)
	{
		lwasm_register_error2(as, as -> cl, E_SYMBOL_UNDEFINED, "%s", var);
	}
	debug_message(as, 225, "eval var: undefined symbol - returning NULL");
	return NULL;
}

lw_expr_t lwasm_evaluate_special(int t, void *ptr, void *priv)
{
	switch (t)
	{
	case lwasm_expr_secbase:
		{
//			sectiontab_t *s = priv;
			asmstate_t *as = priv;
			if (((sectiontab_t *)ptr) -> tbase != -1)
			{
				return lw_expr_build(lw_expr_type_int, ((sectiontab_t *)ptr) -> tbase);
			}
			if (as -> exportcheck && ptr == as -> csect)
				return lw_expr_build(lw_expr_type_int, 0);
			if (((sectiontab_t *)ptr) -> flags & section_flag_constant)
				return lw_expr_build(lw_expr_type_int, 0);
			return NULL;
		}

	case lwasm_expr_linedlen:
		{
			line_t *cl = ptr;
			if (cl -> dlen == -1)
				return NULL;
			return lw_expr_build(lw_expr_type_int, cl -> dlen);
		}
		break;
					
	case lwasm_expr_linelen:
		{
			line_t *cl = ptr;
			if (cl -> len != -1)
				return lw_expr_build(lw_expr_type_int, cl -> len);
				
			if (cl -> as -> pretendmax)
			{
				if (cl -> maxlen != 0)
				{
					//fprintf(stderr, "Pretending max, len = %d\n", cl -> maxlen);
					return lw_expr_build(lw_expr_type_int, cl -> maxlen);
				}
			}
			return NULL;
		}
		break;
		
	case lwasm_expr_linedaddr:
		{
			line_t *cl = ptr;
			return lw_expr_copy(cl -> daddr);
		}
	
	case lwasm_expr_lineaddr:
		{
			line_t *cl = ptr;
			if (cl -> addr)
				return lw_expr_copy(cl -> addr);
			else
				return NULL;
		}
	
	case lwasm_expr_syment:
		{
			struct symtabe *sym = ptr;
			return lw_expr_copy(sym -> value);
		}
	
	case lwasm_expr_import:
		{
			return NULL;
		}
	
	case lwasm_expr_nextbp:
		{
			line_t *cl = ptr;
			for (cl = cl -> next; cl; cl = cl -> next)
			{
				if (cl -> isbrpt)
					break;
			}
			if (cl)
			{
				return lw_expr_copy(cl -> addr);
			}
			return NULL;
		}
	
	case lwasm_expr_prevbp:
		{
			line_t *cl = ptr;
			for (cl = cl -> prev; cl; cl = cl -> prev)
			{
				if (cl -> isbrpt)
					break;
			}
			if (cl)
			{
				return lw_expr_copy(cl -> addr);
			}
			return NULL;
		}
	}
	return NULL;
}

const char* lwasm_lookup_error(lwasm_errorcode_t error_code)
{
	switch (error_code)
	{
		case E_6309_INVALID:			return "Illegal use of 6309 instruction in 6809 mode";
		case E_6809_INVALID:			return "Illegal use of 6809 instruction in 6309 mode";
		case E_ALIGNMENT_INVALID:		return "Invalid alignment";
		case E_BITNUMBER_INVALID:		return "Invalid bit number";
		case E_BITNUMBER_UNRESOLVED:	return "Bit number must be fully resolved";
		case E_BYTE_OVERFLOW:			return "Byte overflow";
		case E_CONDITION_P1:			return "Conditions must be constant on pass 1";
		case E_DIRECTIVE_OS9_ONLY:		return "Directive only valid for OS9 target";
		case E_DIV0:					return "Division by zero";
		case E_EXEC_ADDRESS:			return "Exec address not constant!";
		case E_EXPRESSION_BAD:			return "Bad expression";
		case E_EXPRESSION_NOT_CONST:	return "Expression must be constant";
		case E_EXPRESSION_NOT_RESOLVED: return "Expression not fully resolved";
		case E_FILE_OPEN:				return "Cannot open file";
		case E_FILENAME_MISSING:		return "Missing filename";
		case E_FILL_INVALID:			return "Invalid fill length";
		case E_IMMEDIATE_INVALID:		return "Immediate mode not allowed";
		case E_IMMEDIATE_UNRESOLVED:	return "Immediate byte must be fully resolved";
		case E_INSTRUCTION_FAILED:		return "Instruction failed to resolve.";
		case E_INSTRUCTION_SECTION:		return "Instruction generating output outside of a section";
		case E_LINE_ADDRESS:			return "Cannot resolve line address";
		case E_LINED_ADDRESS:			return "Cannot resolve line data address";
		case E_OBJTARGET_ONLY:			return "Only supported for object target";
		case E_OPCODE_BAD:				return "Bad opcode";
		case E_OPERAND_BAD:				return "Bad operand";
		case E_PADDING_BAD:				return "Bad padding";
		case E_PRAGMA_UNRECOGNIZED:		return "Unrecognized pragma string";
		case E_REGISTER_BAD:			return "Bad register";
		case E_SETDP_INVALID:			return "SETDP not permitted for object target";
		case E_SETDP_NOT_CONST:			return "SETDP must be constant on pass 1";
		case E_STRING_BAD:				return "Bad string condition";
		case E_SYMBOL_BAD:				return "Bad symbol";
		case E_SYMBOL_MISSING:			return "Missing symbol";
		case E_SYMBOL_UNDEFINED:		return "Undefined symbol";
		case E_SYMBOL_UNDEFINED_EXPORT: return "Undefined exported symbol";
		case E_MACRO_DUPE:				return "Duplicate macro definition";
		case E_MACRO_ENDM:				return "ENDM without MACRO";
		case E_MACRO_NONAME:			return "Missing macro name";
		case E_MACRO_RECURSE:			return "Attempt to define a macro inside a macro";
		case E_MODULE_IN:				return "Already in a module!";
		case E_MODULE_NOTIN:			return "Not in a module!";
		case E_NEGATIVE_BLOCKSIZE:		return "Negative block sizes make no sense!";
		case E_NEGATIVE_RESERVATION:	return "Negative reservation sizes make no sense!";
		case E_NW_8:					return "n,W cannot be 8 bit";
		case E_SECTION_END:				return "ENDSECTION without SECTION";
		case E_SECTION_EXTDEP:			return "EXTDEP must be within a section";
		case E_SECTION_FLAG:			return "Unrecognized section flag";
		case E_SECTION_NAME:			return "Need section name";
		case E_SECTION_TARGET:			return "Cannot use sections unless using the object target";
		case E_STRUCT_DUPE:				return "Duplicate structure definition";
		case E_STRUCT_NONAME:			return "Cannot declare a structure without a symbol name.";
		case E_STRUCT_NOSYMBOL:			return "Structure definition with no effect - no symbol";
		case E_STRUCT_RECURSE:			return "Attempt to define a structure inside a structure";
		case E_SYMBOL_DUPE:				return "Multiply defined symbol";
		case E_UNKNOWN_OPERATION:		return "Unknown operation";
		case E_ORG_NOT_FOUND:			return "Previous ORG not found";
		case E_COMPLEX_INCOMPLETE:      return "Incomplete expression too complex";
		case E_USER_SPECIFIED:			return "User Specified:";
		case E_ILL5:					return "Illegal 5 bit offset";

		case W_ENDSTRUCT_WITHOUT:		return "ENDSTRUCT without STRUCT";
		case W_DUPLICATE_SECTION:		return "Section flags can only be specified the first time; ignoring duplicate definition";
		case W_NOT_SUPPORTED:			return "Not supported";
		case W_OPERAND_SIZE:			return "Operand size larger than required";
		default:						return "Error";
	}
}

/* keeping this as a separate error output for stability in unit test scripts */
void lwasm_error_testmode(line_t *cl, const char* msg, int fatal)
{
	cl -> as -> testmode_errorcount++;
	fprintf(stderr, "line %d: %s : %s\n", cl->lineno, msg, cl->ltext);
	if (fatal == 1) lw_error("aborting\n");
}

/* parse unit test input data from comment field */
void lwasm_parse_testmode_comment(line_t *l, lwasm_testflags_t *flags, lwasm_errorcode_t *err, int *len, char **buf)
{
	*flags = 0;

	if (!l)
		return;

	char* s = strstr(l -> ltext, ";.");
	if (s == NULL) return;

	char* t = strstr(s, ":");
	if (t == NULL)
	{
		/* parse: ;.8E0FCE (emitted code) */

		if (buf == NULL) return;

		int i;
		*flags = TF_EMIT;

		s = s + 2;	/* skip ;. prefix */
		t = s;
		while (*t > 32) t++;

		if ((t - s) & 1)
		{
			lwasm_error_testmode(l, "bad test data (wrong length of hex chars)", 1);
			return;
		}

		*len = (t - s) / 2;

		t = lw_alloc(*len);
		*buf = t;

		for (i = 0; i < *len; i++)
		{
			int val;
			sscanf(s, "%2x", &val);
			*t++ = (char) val;
			s += 2;
		}
	}
	else
	{
		/* parse: ;.E:1000 or ;.E:7 (warnings or errors) */
		*flags = TF_ERROR;

		char ch = toupper(*(t - 1));
		if (ch != 'E') lwasm_error_testmode(l, "bad test data (expected E: flag)", 1);
		sscanf(t + 1, "%d", (int*) err);
	}
}

void lwasm_register_error_real(asmstate_t *as, line_t *l, lwasm_errorcode_t error_code, const char *msg)
{
	lwasm_error_t *e;

	if (!l)
		return;

	if (CURPRAGMA(l, PRAGMA_TESTMODE))
	{
		lwasm_testflags_t flags;
		lwasm_errorcode_t testmode_error_code;
		lwasm_parse_testmode_comment(l, &flags, &testmode_error_code, NULL, NULL);
		if (flags == TF_ERROR)
		{
			l -> len = 0;	/* null out bogus line */
			l -> insn = -1;
			l -> err_testmode = error_code;
			if (testmode_error_code == error_code) return;		/* expected error: ignore and keep assembling */

			char buf[128];
			sprintf(buf, "wrong error code (%d)", error_code);
			lwasm_error_testmode(l, buf, 0);
			return;
		}
	}

	e = lw_alloc(sizeof(lwasm_error_t));

	if (error_code >= 1000)
	{
		e->next = l->warn;
		l->warn = e;
		as->warningcount++;
	}
	else
	{
		e->next = l->err;
		l->err = e;
		as->errorcount++;
	}
	
	e -> code = error_code;
	e -> charpos = -1;

	e -> mess = lw_strdup(msg);
}

void lwasm_register_error(asmstate_t *as, line_t *l, lwasm_errorcode_t err)
{
	lwasm_register_error_real(as, l, err, lwasm_lookup_error(err));
}

void lwasm_register_error2(asmstate_t *as, line_t *l, lwasm_errorcode_t err, const char* fmt, ...)
{
	char errbuff[1024];
	char f[128];

	sprintf(f, "%s %s", lwasm_lookup_error(err), fmt);

	va_list args;

	va_start(args, fmt);

	(void) vsnprintf(errbuff, 1024, f, args);

	lwasm_register_error_real(as, l, err, errbuff);

	va_end(args);
}

int lwasm_next_context(asmstate_t *as)
{
	int r;
	r = as -> nextcontext;
	as -> nextcontext++;
	return r;
}

void lwasm_emit(line_t *cl, int byte)
{
	if (CURPRAGMA(cl, PRAGMA_NOOUTPUT))
		return;
	if (cl -> as -> output_format == OUTPUT_OBJ && cl -> csect == NULL)
	{
		lwasm_register_error(cl -> as, cl, E_INSTRUCTION_SECTION);
		return;
	}
	if (cl -> outputl < 0)
		cl -> outputl = 0;

	if (cl -> outputl == cl -> outputbl)
	{
		cl -> output = lw_realloc(cl -> output, cl -> outputbl + 8);
		cl -> outputbl += 8;
	}
	cl -> output[cl -> outputl++] = byte & 0xff;
	
	if (cl -> inmod)
	{
		asmstate_t *as = cl -> as;
		// update module CRC
		// this is a direct transliteration from the nitros9 asm source
		// to C; it can, no doubt, be optimized for 32 bit processing  
		byte &= 0xff;

		byte ^= (as -> crc)[0];
		(as -> crc)[0] = (as -> crc)[1];
		(as -> crc)[1] = (as -> crc)[2];
		(as -> crc)[1] ^= (byte >> 7);
		(as -> crc)[2] = (byte << 1); 
		(as -> crc)[1] ^= (byte >> 2);
		(as -> crc)[2] ^= (byte << 6);
		byte ^= (byte << 1);
		byte ^= (byte << 2);
		byte ^= (byte << 4);
		if (byte & 0x80) 
		{
			(as -> crc)[0] ^= 0x80;
		    (as -> crc)[2] ^= 0x21;
		}
	}
}

void lwasm_emitop(line_t *cl, int opc)
{
	if (cl->cycle_base == 0)
		lwasm_cycle_update_count(cl, opc);	/* only call first time, never on postbyte */

	if (opc > 0x100)
		lwasm_emit(cl, opc >> 8);
	lwasm_emit(cl, opc);
}

lw_expr_t lwasm_parse_term(char **p, void *priv)
{
	asmstate_t *as = priv;
	int neg = 1;
	int val;
	
	if (!**p)
		return NULL;
	
	if (**p == '.'
			&& !((*p)[1] >= 'A' && (*p)[1] <= 'Z')
			&& !((*p)[1] >= 'a' && (*p)[1] <= 'z')
			&& !((*p)[1] >= '0' && (*p)[1] <= '9')
		)
	{
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_linedaddr, as -> cl);
	}
	
	if (**p == '*')
	{
		// special "symbol" for current line addr (*)
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_lineaddr, as -> cl);
	}
	
	// branch points
	if (**p == '<')
	{
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_prevbp, as -> cl);
	}
	if (**p == '>')
	{
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_nextbp, as -> cl);
	}
	
	// double ascii constant
	if (**p == '"')
	{
		int v;
		(*p)++;
		if (!**p)
			return NULL;
		if (!*((*p)+1))
			return NULL;
		v = (unsigned char)**p << 8 | (unsigned char)*((*p)+1);
		(*p) += 2;
		
		if (**p == '"')
			(*p)++;
		
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	/* double ASCII constant, like LDD #'MG */
	if (CURPRAGMA(as->cl, PRAGMA_M80EXT))
	{
		if (((**p == '"') || (**p == '\'')) && (as->cl->genmode == 16))
		{
			int v;
			(*p)++;
			if (!**p)
				return NULL;
			if (!*((*p) + 1))
				return NULL;
			v = (unsigned char) **p << 8 | (unsigned char) *((*p) + 1);
			(*p) += 2;

			if ((**p == '"') || (**p == '\''))
				(*p)++;

			return lw_expr_build(lw_expr_type_int, v);
		}
	}

	/* single ASCII constant, like LDA #'E */
	if (**p == '\'')
	{
		int v;
		
		(*p)++;
		if (!**p)
			return NULL;
		
		v = (unsigned char)**p;
		(*p)++;
		
		if (**p == '\'')
			(*p)++;
		
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	if (**p == '&')
	{
		val = 0;
		// decimal constant
		(*p)++;
		
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (!**p || !strchr("0123456789", **p))
		{
			(*p)--;
			if (neg < 0)
				(*p)--;
			return NULL;
		}

		while (**p && strchr("0123456789", **p))
		{
			val = val * 10 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, val * neg);
	}

	if (**p == '%')
	{
		val = 0;
		// binary constant
		(*p)++;

		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (**p != '0' && **p != '1')
		{
			(*p)--;
			if (neg < 0)
				(*p)--;
			return NULL;
		}

		while (**p && (**p == '0' || **p == '1'))
		{
			val = val * 2 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, val * neg);
	}
	
	if (**p == '$')
	{
		// hexadecimal constant
		int v = 0, v2;
		(*p)++;
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (!**p || !strchr("0123456789abcdefABCDEF", **p))
		{
			(*p)--;
			if (neg < 0)
				(*p)--;
			return NULL;
		}
		while (**p && strchr("0123456789abcdefABCDEF", **p))
		{
			v2 = toupper(**p) - '0';
			if (v2 > 9)
				v2 -= 7;
			v = v * 16 + v2;
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v * neg);
	}
	
	if (**p == '0' && (*((*p)+1) == 'x' || *((*p)+1) == 'X'))
	{
		// hexadecimal constant, C style
		int v = 0, v2;
		(*p)+=2;

		if (!**p || !strchr("0123456789abcdefABCDEF", **p))
		{
			(*p) -= 2;
			return NULL;
		}
		while (**p && strchr("0123456789abcdefABCDEF", **p))
		{
			v2 = toupper(**p) - '0';
			if (v2 > 9)
				v2 -= 7;
			v = v * 16 + v2;
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	if (**p == '@' && (*((*p)+1) >= '0' && *((*p)+1) <= '7'))
	{
		// octal constant
		int v = 0;
		(*p)++;
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}


		if (!**p || !strchr("01234567", **p))
		{
			(*p)--;
			if (neg < 0)
				(*p)--;
			return NULL;
		}
		
		while (**p && strchr("01234567", **p))
		{
			v = v * 8 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v * neg);
	}
	

	// symbol or bare decimal or suffix constant here
	do
	{
		int havedol = 0;
		int l = 0;
		
		while ((*p)[l] && strchr(SYMCHARS, (*p)[l]))
		{
			if ((*p)[l] == '$')
				havedol = 1;
			l++;
		}
		if (l == 0)
			return NULL;

		if ((*p)[l] == '{')
		{
			while ((*p)[l] && (*p)[l] != '}')
				l++;
			l++;
		}
		
		if (havedol || **p < '0' || **p > '9')
		{
			// have a symbol here
			char *sym;
			lw_expr_t term;
			
			sym = lw_strndup(*p, l);
			(*p) += l;
			term = lw_expr_build(lw_expr_type_var, sym);
			lw_free(sym);
			return term;
		}
	} while (0);
	
	if (!**p)
		return NULL;
	
	// we have a numeric constant here, either decimal or postfix base notation
	{
		int decval = 0, binval = 0, hexval = 0, octval = 0;
		int valtype = 15; // 1 = bin, 2 = oct, 4 = dec, 8 = hex
		int bindone = 0;
		int val;
		int dval;
		
		while (1)
		{
			if (!**p || !strchr("0123456789ABCDEFabcdefqhoQHO", **p))
			{
				// we can legally be bin or decimal here
				if (bindone)
				{
					// just finished a binary value
					val = binval;
					break;
				}
				else if (valtype & 4)
				{
					val = decval;
					break;
				}
				else
				{
					// bad value
					return NULL;
				}
			}
			
			dval = toupper(**p);
			(*p)++;
			
			if (bindone)
			{
				// any characters past "B" means it is not binary
				bindone = 0;
				valtype &= 14;
			}
			
			switch (dval)
			{
			case 'Q':
			case 'O':
				if (valtype & 2)
				{
					val = octval;
					valtype = -1;
					break;
				}
				else
				{
					return NULL;
				}
				/* can't get here */
			
			case 'H':
				if (valtype & 8)
				{
					val = hexval;
					valtype = -1;
					break;
				}
				else
				{
					return NULL;
				}
				/* can't get here */
			
			case 'B':
				// this is a bit of a sticky one since B may be a
				// hex number instead of the end of a binary number
				// so it falls through to the digit case
				if (valtype & 1)
				{
					// could still be binary of hex
					bindone = 1;
					valtype = 9;
				}
				/* fall through intented */
			
			default:
				// digit
				dval -= '0';
				if (dval > 9)
					dval -= 7;
				if (valtype & 8)
					hexval = hexval * 16 + dval;
				if (valtype & 4)
				{
					if (dval > 9)
						valtype &= 11;
					else
						decval = decval * 10 + dval;
				}
				if (valtype & 2)
				{
					if (dval > 7)
						valtype &= 13;
					else
						octval = octval * 8 + dval;
				}
				if (valtype & 1)
				{
					if (dval > 1)
						valtype &= 14;
					else
						binval = binval * 2 + dval;
				}
			}
			if (valtype == -1)
				break;
			
			// return if no more valid types
			if (valtype == 0)
				return NULL;
			
			val = decval; // in case we fall through	
		} 
		
		// get here if we have a value
		return lw_expr_build(lw_expr_type_int, val);
	}
	// can't get here
}

lw_expr_t lwasm_parse_expr(asmstate_t *as, char **p)
{
	lw_expr_t e;

	if (as->exprwidth != 16)	
	{
		lw_expr_setwidth(as->exprwidth);
		if (CURPRAGMA(as -> cl, PRAGMA_NEWSOURCE))
			e = lw_expr_parse(p, as);
		else
			e = lw_expr_parse_compact(p, as);
		lw_expr_setwidth(0);
	}
	else
	{
		if (CURPRAGMA(as -> cl, PRAGMA_NEWSOURCE))
			e = lw_expr_parse(p, as);
		else
			e = lw_expr_parse_compact(p, as);
	}
	lwasm_skip_to_next_token(as -> cl, p);
	return e;
}

int lwasm_reduce_expr(asmstate_t *as, lw_expr_t expr)
{
	if (expr)
		lw_expr_simplify(expr, as);
	return 0;
}

void lwasm_save_expr(line_t *cl, int id, lw_expr_t expr)
{
	struct line_expr_s *e;
	
	for (e = cl -> exprs; e; e = e -> next)
	{
		if (e -> id == id)
		{
			lw_expr_destroy(e -> expr);
			e -> expr = expr;
			return;
		}
	}
	
	e = lw_alloc(sizeof(struct line_expr_s));
	e -> expr = expr;
	e -> id = id;
	e -> next = cl -> exprs;
	cl -> exprs = e;
}

lw_expr_t lwasm_fetch_expr(line_t *cl, int id)
{
	struct line_expr_s *e;
	
	for (e = cl -> exprs; e; e = e -> next)
	{
		if (e -> id == id)
		{
			return e -> expr;
		}
	}
	return NULL;
}

void skip_operand_real(line_t *cl, char **p)
{
	if (CURPRAGMA(cl, PRAGMA_NEWSOURCE))
		return;
	for (; **p && !isspace(**p); (*p)++)
		/* do nothing */ ;
}

struct auxdata {
	int v;
	int oc;
	int ms;
};

int lwasm_emitexpr_auxlwmod(lw_expr_t expr, void *arg)
{
	struct auxdata *ad = arg;
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		ad -> v = lw_expr_intval(expr);
		return 0;
	}
	if (lw_expr_istype(expr, lw_expr_type_special))
	{
		if (lw_expr_specint(expr) == lwasm_expr_secbase)
		{
			sectiontab_t *s;
			s = lw_expr_specptr(expr);
			if (strcmp(s -> name, "main") == 0)
			{
				ad -> ms = 1;
				return 0;
			}
			if (strcmp(s -> name, "bss"))
				return -1;
			return 0;
		}
		return -1;
	}
	if (lw_expr_whichop(expr) == lw_expr_oper_plus)
	{
		if (ad -> oc)
			return -1;
		ad -> oc = 1;
		return 0;
	}
	return -1;
}

int lwasm_emitexpr(line_t *l, lw_expr_t expr, int size)
{
	int v = 0;
	int ol;
	
	ol = l -> outputl;
	if (ol == -1)
		ol = 0;
		
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		v = lw_expr_intval(expr);
	}
	// handle external/cross-section/incomplete references here
	else
	{
		if (l -> as -> output_format == OUTPUT_LWMOD)
		{
			reloctab_t *re;
			lw_expr_t te;
			struct auxdata ad;
			ad.v = 0;
			ad.oc = 0;
			ad.ms = 0;
			
			if (l -> csect == NULL)
			{
				lwasm_register_error(l -> as, l, E_INSTRUCTION_SECTION);
				return -1;
			}
			if (size != 2)
			{
				lwasm_register_error(l -> as, l, E_OPERAND_BAD);
				return -1;
			}
			// we have a 16 bit reference here - we need to check to make sure
			// it's at most a + or - with the BSS section base
			v = lw_expr_whichop(expr);
			if (v == -1)
			{
				v = 0;
				if (lw_expr_testterms(expr, lwasm_emitexpr_auxlwmod, &ad) != 0)
				{
					lwasm_register_error(l -> as, l, E_COMPLEX_INCOMPLETE);
					return -1;
				}
				v = ad.v;
			}
			else if (v == lw_expr_oper_plus)
			{
				v = 0;
				if (lw_expr_operandcount(expr) > 2)
				{
					lwasm_register_error(l -> as, l, E_COMPLEX_INCOMPLETE);
					return -1;
				}
				if (lw_expr_testterms(expr, lwasm_emitexpr_auxlwmod, &ad) != 0)
				{
					lwasm_register_error(l -> as, l, E_COMPLEX_INCOMPLETE);
					return -1;
				}
				v = ad.v;
			}
			else
			{
				lwasm_register_error(l -> as, l, E_COMPLEX_INCOMPLETE);
				return -1;
			}

			// add "expression" record to section table
			re = lw_alloc(sizeof(reloctab_t));
			re -> next = l -> csect -> reloctab;
			l -> csect -> reloctab = re;
			te = lw_expr_build(lw_expr_type_int, ol);
			re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
			lw_expr_destroy(te);
			lwasm_reduce_expr(l -> as, re -> offset);
			re -> size = size;
			if (ad.ms == 1)
				re -> expr = lw_expr_copy(expr);
			else
				re -> expr = NULL;

			lwasm_emit(l, v >> 8);
			lwasm_emit(l, v & 0xff);
			return 0;
		}
		else if (l -> as -> output_format == OUTPUT_OBJ)
		{
			reloctab_t *re;
			lw_expr_t te;

			if (l -> csect == NULL)
			{
				lwasm_register_error(l -> as, l, E_INSTRUCTION_SECTION);
				return -1;
			}
			
			if (size == 4)
			{
				// create a two part reference because lwlink doesn't
				// support 32 bit references
				lw_expr_t te2;
				te = lw_expr_build(lw_expr_type_int, 0x10000);
				te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_divide, expr, te);
				lw_expr_destroy(te);
				
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> expr = te2;
				re -> size = 2;

				te = lw_expr_build(lw_expr_type_int, 0xFFFF);
				te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_bwand, expr, te);
				lw_expr_destroy(te);
				
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol + 2);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> expr = te2;
				re -> size = 2;
			}
			else
			{
				// add "expression" record to section table
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> size = size;
				re -> expr = lw_expr_copy(expr);
			}
			for (v = 0; v < size; v++)
				lwasm_emit(l, 0);
			return 0;
		}
		lwasm_register_error(l->as, l, E_EXPRESSION_NOT_RESOLVED);
		return -1;
	}
	
	switch (size)
	{
	case 4:
		lwasm_emit(l, v >> 24);
		lwasm_emit(l, v >> 16);
		/* fallthrough intended */
			
	case 2:
		lwasm_emit(l, v >> 8);
		/* fallthrough intended */
		
	case 1:
		lwasm_emit(l, v);
	}
	
	return 0;
}

int lwasm_lookupreg2(const char *regs, char **p)
{
	int rval = 0;
	
	while (*regs)
	{
		if (toupper(**p) == *regs)
		{
			if (regs[1] == ' ' && !isalpha(*(*p + 1)))
				break;
			if (toupper(*(*p + 1)) == regs[1])
				break;
		}
		regs += 2;
		rval++;
	}
	if (!*regs)
		return -1;
	if (regs[1] == ' ')
		(*p)++;
	else
		(*p) += 2;
	return rval;
}

int lwasm_lookupreg3(const char *regs, char **p)
{
	int rval = 0;
	
	while (*regs)
	{
		if (toupper(**p) == *regs)
		{
			if (regs[1] == ' ' && !isalpha(*(*p + 1)))
				break;
			if (toupper(*(*p + 1)) == regs[1])
			{
				if (regs[2] == ' ' && !isalpha(*(*p + 2)))
					break;
				if (toupper(*(*p + 2)) == regs[2])
					break;
			}
		}
		regs += 3;
		rval++;
	}
	if (!*regs)
		return -1;
	if (regs[1] == ' ')
		(*p)++;
	else if (regs[2] == ' ')
		(*p) += 2;
	else
		(*p) += 3;
	return rval;
}

void lwasm_show_errors(asmstate_t *as)
{
	line_t *cl;
	lwasm_error_t *e;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (!(cl -> err) && !(cl -> warn))
			continue;

		// trim "include:" if it appears
		char* s = cl->linespec;
		if ((strlen(s) > 8) && (s[7] == ':')) s += 8;
		while (*s == ' ') s++;

		for (e = cl -> err; e; e = e -> next)
		{
			fprintf(stderr, "%s(%d) : ERROR : %s\n", s, cl->lineno, e->mess);
		}
		for (e = cl -> warn; e; e = e -> next)
		{
			fprintf(stderr, "%s(%d) : WARNING : %s\n", s, cl->lineno, e->mess);
		}
		fprintf(stderr, "%s:%05d %s\n\n", cl -> linespec, cl -> lineno, cl -> ltext);
	}
}

/*
this does any passes and other gymnastics that might be useful
to see if an expression reduces early
*/
void do_pass3(asmstate_t *as);
void do_pass4_aux(asmstate_t *as, int force);

void lwasm_interim_reduce(asmstate_t *as)
{
	do_pass3(as);
//	do_pass4_aux(as, 0);
}

lw_expr_t lwasm_parse_cond(asmstate_t *as, char **p)
{
	lw_expr_t e;

	debug_message(as, 250, "Parsing condition");
	e = lwasm_parse_expr(as, p);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	
	if (!e)
	{
		lwasm_register_error(as, as -> cl, E_EXPRESSION_BAD);
		return NULL;
	}

	/* handle condundefzero */
	if (CURPRAGMA(as -> cl, PRAGMA_CONDUNDEFZERO))
	{
		as -> undefzero = 1;
		lwasm_reduce_expr(as, e);
		as -> undefzero = 0;
	}

	/* we need to simplify the expression here */
	debug_message(as, 250, "Doing interim reductions");
	lwasm_interim_reduce(as);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	debug_message(as, 250, "Reducing expression");
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
/*	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
*/

	lwasm_save_expr(as -> cl, 4242, e);

	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		debug_message(as, 250, "Non-constant expression");
		lwasm_register_error(as, as -> cl, E_CONDITION_P1);
		return NULL;
	}
	debug_message(as, 250, "Returning expression");
	return e;
}

struct range_data
{
	int min;
	int max;
	asmstate_t *as;
};
int lwasm_calculate_range(asmstate_t *as, lw_expr_t expr, int *min, int *max);
int lwasm_calculate_range_tf(lw_expr_t e, void *info)
{
	struct range_data *rd = info;
	int i;
	
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		i = lw_expr_intval(e);
		rd -> min += i;
		rd -> max += i;
		return 0;
	}
	
	if (lw_expr_istype(e, lw_expr_type_special))
	{
		line_t *l;
		if (lw_expr_specint(e) != lwasm_expr_linelen)
		{
			rd -> min = -1;
			return -1;
		}
		l = (line_t *)lw_expr_specptr(e);
		if (l -> len == -1)
		{
			rd -> min += l -> minlen;
			rd -> max += l -> maxlen;
		}
		else
		{
			rd -> min += l -> len;
		}
		return 0;
	}
	
	if (lw_expr_istype(e, lw_expr_type_var))
	{
		lw_expr_t te;
		te = lw_expr_copy(e);
		lwasm_reduce_expr(rd -> as, te);
		if (lw_expr_istype(te, lw_expr_type_int))
		{
			i = lw_expr_intval(te);
			rd -> min += i;
			rd -> max += i;
		}
		else
		{
			rd -> min = -1;
		}
		lw_expr_destroy(te);
		if (rd -> min == -1)
			return -1;
		return 0;
	}
	
	if (lw_expr_istype(e, lw_expr_type_oper))
	{
		if (lw_expr_whichop(e) == lw_expr_oper_plus)
			return 0;
		rd -> min = -1;
		return -1;
	}
	
	rd -> min = -1;
	return -1;
}

int lwasm_calculate_range(asmstate_t *as, lw_expr_t expr, int *min, int *max)
{
	struct range_data rd;
	
	rd.min = 0;
	rd.max = 0;
	rd.as = as;
	
	if (!expr)
		return -1;
	
	lw_expr_testterms(expr, lwasm_calculate_range_tf, (void *)&rd);
	*min = rd.min;
	*max = rd.max;
	if (rd.min == -1)
		return -1;
	return 0;
}

void lwasm_reduce_line_exprs(line_t *cl)
{
	asmstate_t *as;
	struct line_expr_s *le;
	int i;
			
	as = cl -> as;
	as -> cl = cl;
			
	// simplify address
	lwasm_reduce_expr(as, cl -> addr);
		
	// simplify data address
	lwasm_reduce_expr(as, cl -> daddr);

	// simplify each expression
	for (i = 0, le = cl -> exprs; le; le = le -> next, i++)
	{
		debug_message(as, 101, "Reduce expressions: (pre) exp[%d] = %s", i, lw_expr_print(le -> expr));
		lwasm_reduce_expr(as, le -> expr);
		debug_message(as, 100, "Reduce expressions: exp[%d] = %s", i, lw_expr_print(le -> expr));
	}
			
	if (cl -> len == -1 || cl -> dlen == -1)
	{
		// try resolving the instruction length
		// but don't force resolution
		if (cl -> insn >= 0 && instab[cl -> insn].resolve)
		{
			(instab[cl -> insn].resolve)(as, cl, 0);
			if ((cl -> inmod == 0) && cl -> len >= 0 && cl -> dlen >= 0)
			{
				if (cl -> len == 0)
					cl -> len = cl -> dlen;
				else
					cl -> dlen = cl -> len;
			}
		}
	}
	debug_message(as, 100, "Reduce expressions: len = %d", cl -> len);
	debug_message(as, 100, "Reduce expressions: dlen = %d", cl -> dlen);
	debug_message(as, 100, "Reduce expressions: addr = %s", lw_expr_print(cl -> addr));
	debug_message(as, 100, "Reduce expressions: daddr = %s", lw_expr_print(cl -> daddr));
}
