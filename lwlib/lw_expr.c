/*
lwexpr.c

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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lw_alloc.h"
#include "lw_expr.h"
#include "lw_error.h"
#include "lw_string.h"

static lw_expr_fn_t *evaluate_special = NULL;
static lw_expr_fn2_t *evaluate_var = NULL;
static lw_expr_fn3_t *parse_term = NULL;

/* Q&D to break out of infinite recursion */
static int level = 0;
static int bailing = 0;
static int parse_compact = 0;

static void (*divzero)(void *priv) = NULL;

static int expr_width = 0;

void lw_expr_setwidth(int w)
{
	expr_width = w;
}

void lw_expr_setdivzero(void (*fn)(void *priv))
{
	divzero = fn;
}

static void lw_expr_divzero(void *priv)
{
	if (divzero)
		(*divzero)(priv);
	else
		fprintf(stderr, "Divide by zero in lw_expr!\n");
}

int lw_expr_istype(lw_expr_t e, int t)
{
	/* NULL expression is never of any type */
	if (!e)
		return 0;
	if (e -> type == t)
		return 1;
	return 0;
}

int lw_expr_intval(lw_expr_t e)
{
	if (e -> type == lw_expr_type_int)
		return e -> value;
	return -1;
}

int lw_expr_whichop(lw_expr_t e)
{
	if (e -> type == lw_expr_type_oper)
	{
		if (e -> value == lw_expr_oper_com8)
			return lw_expr_oper_com;
		return e -> value;
	}
	return -1;
}

int lw_expr_specint(lw_expr_t e)
{
	if (e -> type == lw_expr_type_special)
		return e -> value;
	return -1;
}

void lw_expr_set_term_parser(lw_expr_fn3_t *fn)
{
	parse_term = fn;
}

void lw_expr_set_special_handler(lw_expr_fn_t *fn)
{
	evaluate_special = fn;
}

void lw_expr_set_var_handler(lw_expr_fn2_t *fn)
{
	evaluate_var = fn;
}

lw_expr_t lw_expr_create(void)
{
	lw_expr_t r;
	
	r = lw_alloc(sizeof(struct lw_expr_priv));
	r -> operands = NULL;
	r -> value2 = NULL;
	r -> type = lw_expr_type_int;
	r -> value = 0;
	return r;
}

void lw_expr_destroy(lw_expr_t E)
{
	struct lw_expr_opers *o;
	if (!E)
		return;
	while (E -> operands)
	{
		o = E -> operands;
		E -> operands = o -> next;
		lw_expr_destroy(o -> p);
		lw_free(o);
	}
	if (E -> type == lw_expr_type_var)
		lw_free(E -> value2);
	lw_free(E);
}

/* actually duplicates the entire expression */
void lw_expr_add_operand(lw_expr_t E, lw_expr_t O);
lw_expr_t lw_expr_copy(lw_expr_t E)
{
	lw_expr_t r;
	struct lw_expr_opers *o;
	
	if (!E)
		return NULL;
	r = lw_alloc(sizeof(struct lw_expr_priv));
	*r = *E;
	r -> operands = NULL;
	
	if (E -> type == lw_expr_type_var)
		r -> value2 = lw_strdup(E -> value2);
	for (o = E -> operands; o; o = o -> next)
	{
		lw_expr_add_operand(r, o -> p);
	}
	
	return r;
}

void lw_expr_add_operand(lw_expr_t E, lw_expr_t O)
{
	struct lw_expr_opers *o, *t;
	
	o = lw_alloc(sizeof(struct lw_expr_opers));
	o -> p = lw_expr_copy(O);
	o -> next = NULL;
	for (t = E -> operands; t && t -> next; t = t -> next)
		/* do nothing */ ;
	
	if (t)
		t -> next = o;
	else
		E -> operands = o;
}

lw_expr_t lw_expr_build_aux(int exprtype, va_list args)
{
	lw_expr_t r;
	int t;
	void *p;
	
	lw_expr_t te1, te2;

	r = lw_expr_create();

	switch (exprtype)
	{
	case lw_expr_type_int:
		t = va_arg(args, int);
		r -> type = lw_expr_type_int;
		r -> value = t;
		break;

	case lw_expr_type_var:
		p = va_arg(args, char *);
		r -> type = lw_expr_type_var;
		r -> value2 = lw_strdup(p);
		break;

	case lw_expr_type_special:
		t = va_arg(args, int);
		p = va_arg(args, char *);
		r -> type = lw_expr_type_special;
		r -> value = t;
		r -> value2 = p;
		break;

	case lw_expr_type_oper:
		t = va_arg(args, int);
		te1 = va_arg(args, lw_expr_t);
		if (t != lw_expr_oper_com && t != lw_expr_oper_neg && t != lw_expr_oper_com8)
			te2 = va_arg(args, lw_expr_t);
		else
			te2 = NULL;
		
		r -> type = lw_expr_type_oper;
		r -> value = t;
		lw_expr_add_operand(r, te1);
		if (te2)
			lw_expr_add_operand(r, te2);
		break;
	
	default:
		lw_error("Invalid expression type specified to lw_expr_build");
	}
	
	return r;
}

lw_expr_t lw_expr_build(int exprtype, ...)
{
	va_list args;
	lw_expr_t r;
	
	va_start(args, exprtype);
	r = lw_expr_build_aux(exprtype, args);
	va_end(args);
	return r;
}

void lw_expr_print_aux(lw_expr_t E, char **obuf, int *buflen, int *bufloc)
{
	struct lw_expr_opers *o;
	int c = 0;
	char buf[256];

	if (!E)
	{
		strcpy(buf, "(NULL)");
		return;
	}
	for (o = E -> operands; o; o = o -> next)
	{
		c++;
		lw_expr_print_aux(o -> p, obuf, buflen, bufloc);
	}
	
	switch (E -> type)
	{
	case lw_expr_type_int:
		if (E -> value < 0)
			snprintf(buf, 256, "-%#x ", -(E -> value));
		else
			snprintf(buf, 256, "%#x ", E -> value);
		break;
	
	case lw_expr_type_var:
		snprintf(buf, 256, "V(%s) ", (char *)(E -> value2));
		break;
	
	case lw_expr_type_special:
		snprintf(buf, 256, "S(%d,%p) ", E -> value, E -> value2);
		break;

	case lw_expr_type_oper:
		snprintf(buf, 256, "[%d]", c);
		switch (E -> value)
		{
		case lw_expr_oper_plus:
			strcat(buf, "+ ");
			break;
			
		case lw_expr_oper_minus:
			strcat(buf, "- ");
			break;
			
		case lw_expr_oper_times:
			strcat(buf, "* ");
			break;
			
		case lw_expr_oper_divide:
			strcat(buf, "/ ");
			break;
			
		case lw_expr_oper_mod:
			strcat(buf, "% ");
			break;
			
		case lw_expr_oper_intdiv:
			strcat(buf, "\\ ");
			break;
			
		case lw_expr_oper_bwand:
			strcat(buf, "BWAND ");
			break;
			
		case lw_expr_oper_bwor:
			strcat(buf, "BWOR ");
			break;
			
		case lw_expr_oper_bwxor:
			strcat(buf, "BWXOR ");
			break;
			
		case lw_expr_oper_and:
			strcat(buf, "AND ");
			break;
			
		case lw_expr_oper_or:
			strcat(buf, "OR ");
			break;
			
		case lw_expr_oper_neg:
			strcat(buf, "NEG ");
			break;
			
		case lw_expr_oper_com:
			strcat(buf, "COM ");
			break;
		
		case lw_expr_oper_com8:
			strcat(buf, "COM8 ");
			break;
			
		default:
			strcat(buf, "OPER ");
			break;
		}
		break;
	default:
		snprintf(buf, 256, "ERR ");
		break;
	}
	
	c = strlen(buf);
	if (*bufloc + c >= *buflen)
	{
		*buflen += 128;
		*obuf = lw_realloc(*obuf, *buflen);
	}
	strcpy(*obuf + *bufloc, buf);
	*bufloc += c;
}

char *lw_expr_print(lw_expr_t E)
{
	static char *obuf = NULL;
	static int obufsize = 0;

	int obufloc = 0;

	lw_expr_print_aux(E, &obuf, &obufsize, &obufloc);

	return obuf;
}

/*
Return:
nonzero if expressions are the same (identical pointers or matching values)
zero if expressions are not the same

*/
int lw_expr_compare(lw_expr_t E1, lw_expr_t E2)
{
	struct lw_expr_opers *o1, *o2;

	if (E1 == E2)
		return 1;

	if (!E1 || !E2)
		return 0;

	if (!(E1 -> type == E2 -> type && E1 -> value == E2 -> value))
		return 0;

	if (E1 -> type == lw_expr_type_var)
	{
		if (!strcmp(E1 -> value2, E2 -> value2))
			return 1;
		else
			return 0;
	}
	
	if (E1 -> type == lw_expr_type_special)
	{
		if (E1 -> value2 == E2 -> value2)
			return 1;
		else
			return 0;
	}
	
	for (o1 = E1 -> operands, o2 = E2 -> operands; o1 && o2; o1 = o1 -> next, o2 = o2 -> next)
		if (lw_expr_compare(o1 -> p, o2 -> p) == 0)
			return 0;
	if (o1 || o2)
		return 0;	

	return 1;
}

/* return true if E is an operator of type oper */
int lw_expr_isoper(lw_expr_t E, int oper)
{
	if (E -> type == lw_expr_type_oper && E -> value == oper)
		return 1;
	return 0;
}


void lw_expr_simplify_sortconstfirst(lw_expr_t E)
{
	struct lw_expr_opers *o;

	if (E -> type != lw_expr_type_oper)
		return;
	if (E -> value != lw_expr_oper_times && E -> value != lw_expr_oper_plus)
		return;

	for (o = E -> operands; o; o = o -> next)
	{
		if (o -> p -> type == lw_expr_type_oper && (o -> p -> value == lw_expr_oper_times || o -> p -> value == lw_expr_oper_plus))
			lw_expr_simplify_sortconstfirst(o -> p);
	}
	
	for (o = E -> operands; o; o = o -> next)
	{
		if (o -> p -> type == lw_expr_type_int && o != E -> operands)
		{
			struct lw_expr_opers *o2;
			for (o2 = E -> operands; o2 -> next != o; o2 = o2 -> next)
				/* do nothing */ ;
			o2 -> next = o -> next;
			o -> next = E -> operands;
			E -> operands = o;
			o = o2;
		}
	}
}

void lw_expr_sortoperandlist(struct lw_expr_opers **o)
{
//	fprintf(stderr, "lw_expr_sortoperandlist() not yet implemented\n");
}

// return 1 if the operand lists match, 0 if not
// may re-order the argument lists
int lw_expr_simplify_compareoperandlist(struct lw_expr_opers **ol1, struct lw_expr_opers **ol2)
{
	struct lw_expr_opers *o1, *o2;
	
	lw_expr_sortoperandlist(ol1);
	lw_expr_sortoperandlist(ol2);
	
	for (o1 = *ol1, o2 = *ol2; o1 &&  o2; o1 = o1 -> next, o2 = o2 -> next)
	{
		if (!lw_expr_compare(o1 -> p, o2 -> p))
			return 0;
	}
	if (o1 || o2)
		return 0;
	return 1;
}

int lw_expr_simplify_isliketerm(lw_expr_t e1, lw_expr_t e2)
{
	// first term is a "times"
	if (e1 -> type == lw_expr_type_oper && e1 -> value == lw_expr_oper_times)
	{
		// second term is a "times"
		if (e2 -> type == lw_expr_type_oper && e2 -> value == lw_expr_oper_times)
		{
			// both times - easy check
			struct lw_expr_opers *o1, *o2;
			for (o1 = e1 -> operands; o1; o1 = o1 -> next)
				if (o1 -> p -> type != lw_expr_type_int)
					break;
			
			for (o2 = e2 -> operands; o2; o2 = o2 -> next)
				if (o2 -> p -> type != lw_expr_type_int)
					break;
			
			if (lw_expr_simplify_compareoperandlist(&o1, &o2))
				return 1;
			return 0;
		}
		
		// not a times - have to assume it's the operand list
		// with a "1 *" in front if it
		if (!e1 -> operands -> next)
			return 0;
		if (e1 -> operands -> next -> next)
			return 0;
		if (!lw_expr_compare(e1 -> operands -> next -> p, e2))
			return 0;
		return 1;
	}
	
	// e1 is not a times
	if (e2 -> type == lw_expr_type_oper && e2 -> value == lw_expr_oper_times)
	{
		// e2 is a times
		if (e2 -> operands -> next -> next)
			return 0;
		if (!lw_expr_compare(e1, e2 -> operands -> next -> p))
			return 0;
		return 1;
	}
	
	// neither are times
	if (!lw_expr_compare(e1, e2))
		return 0;
	return 1;
}

int lw_expr_contains(lw_expr_t E, lw_expr_t E1)
{
	struct lw_expr_opers *o;
	
	// NULL expr contains nothing :)
	if (!E)
		return 0;
	
	if (E1 -> type != lw_expr_type_var && E1 -> type != lw_expr_type_special)
		return 0;
	
	if (lw_expr_compare(E, E1))
		return 1;
	
	for (o = E -> operands; o; o = o -> next)
	{
		if (lw_expr_contains(o -> p, E1))
			return 1;
	}
	return 0;
}

void lw_expr_simplify_l(lw_expr_t E, void *priv);

void lw_expr_simplify_go(lw_expr_t E, void *priv)
{
	struct lw_expr_opers *o;

	// replace subtraction with O1 + -1(O2)...
	// needed for like term collection
	if (E -> type == lw_expr_type_oper && E -> value == lw_expr_oper_minus)
	{
		for (o = E -> operands -> next; o; o = o -> next)
		{
			lw_expr_t e1, e2;
			
			e2 = lw_expr_build(lw_expr_type_int, -1);
			e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_times, e2, o -> p);
			lw_expr_destroy(o -> p);
			lw_expr_destroy(e2);
			o -> p = e1;
		}
		E -> value = lw_expr_oper_plus;
	}

	// turn "NEG" into -1(O) - needed for like term collection
	if (E -> type == lw_expr_type_oper && E -> value == lw_expr_oper_neg)
	{
		lw_expr_t e1;
		
		E -> value = lw_expr_oper_times;
		e1 = lw_expr_build(lw_expr_type_int, -1);
		lw_expr_add_operand(E, e1);
		lw_expr_destroy(e1);
	}
	
again:
	// try to resolve non-constant terms to constants here
	if (E -> type == lw_expr_type_special && evaluate_special)
	{
		lw_expr_t te;
		
		te = evaluate_special(E -> value, E -> value2, priv);
		if (lw_expr_contains(te, E))
			lw_expr_destroy(te);
		else if (te)
		{
			for (o = E -> operands; o; o = o -> next)
				lw_expr_destroy(o -> p);
			if (E -> type == lw_expr_type_var)
				lw_free(E -> value2);
			*E = *te;
			E -> operands = NULL;
	
			if (te -> type == lw_expr_type_var)
				E -> value2 = lw_strdup(te -> value2);
			for (o = te -> operands; o; o = o -> next)
			{
				lw_expr_t xxx;
				xxx = lw_expr_copy(o -> p);
				lw_expr_add_operand(E, xxx);
				lw_expr_destroy(xxx);
			}
			lw_expr_destroy(te);
			goto again;
		}
		return;
	}

	if (E -> type == lw_expr_type_var && evaluate_var)
	{
		lw_expr_t te;
		
		te = evaluate_var(E -> value2, priv);
		if (!te)
			return;
		if (lw_expr_contains(te, E))
			lw_expr_destroy(te);
		else if (te)
		{
			for (o = E -> operands; o; o = o -> next)
				lw_expr_destroy(o -> p);
			if (E -> type == lw_expr_type_var)
				lw_free(E -> value2);
			*E = *te;
			E -> operands = NULL;
	
			if (te -> type == lw_expr_type_var)
				E -> value2 = lw_strdup(te -> value2);
			for (o = te -> operands; o; o = o -> next)
			{
				lw_expr_add_operand(E, lw_expr_copy(o -> p));
			}
			lw_expr_destroy(te);
			goto again;
		}
		return;
	}

	// non-operators have no simplification to do!
	if (E -> type != lw_expr_type_oper)
		return;

	// merge plus operations
	if (E -> value == lw_expr_oper_plus)
	{
	tryagainplus:
		for (o = E -> operands; o; o = o -> next)
		{
			if (o -> p -> type == lw_expr_type_oper && o -> p -> value == lw_expr_oper_plus)
			{
				struct lw_expr_opers *o2;
				// we have a + operation - bring operands up
				
				for (o2 = E -> operands; o2 && o2 -> next != o; o2 = o2 -> next)
					/* do nothing */ ;
				if (o2)
					o2 -> next = o -> p -> operands;
				else
					E -> operands = o -> p -> operands;
				for (o2 = o -> p -> operands; o2 -> next; o2 = o2 -> next)
					/* do nothing */ ;
				o2 -> next = o -> next;
				o -> p -> operands = NULL;
				lw_expr_destroy(o -> p);
				lw_free(o);
				goto tryagainplus;
			}
		}
	}
	
	// merge times operations
	if (E -> value == lw_expr_oper_times)
	{
	tryagaintimes:
		for (o = E -> operands; o; o = o -> next)
		{
			if (o -> p -> type == lw_expr_type_oper && o -> p -> value == lw_expr_oper_times)
			{
				struct lw_expr_opers *o2;
				// we have a + operation - bring operands up
				
				for (o2 = E -> operands; o2 && o2 -> next != o; o2 = o2 -> next)
					/* do nothing */ ;
				if (o2)
					o2 -> next = o -> p -> operands;
				else
					E -> operands = o -> p -> operands;
				for (o2 = o -> p -> operands; o2 -> next; o2 = o2 -> next)
					/* do nothing */ ;
				o2 -> next = o -> next;
				o -> p -> operands = NULL;
				lw_expr_destroy(o -> p);
				lw_free(o);
				goto tryagaintimes;
			}
		}
	}
	
	// simplify operands
	for (o = E -> operands; o; o = o -> next)
		if (o -> p -> type != lw_expr_type_int)
			lw_expr_simplify_l(o -> p, priv);

	for (o = E -> operands; o; o = o -> next)
	{
		if (o -> p -> type != lw_expr_type_int)
			break;
	}

	if (!o)
	{
		// we can do the operation here!
		int tr = -42424242;
		
		switch (E -> value)
		{
		case lw_expr_oper_neg:
			tr = -(E -> operands -> p -> value);
			break;

		case lw_expr_oper_com:
			tr = ~(E -> operands -> p -> value);
			break;
		
		case lw_expr_oper_com8:
			tr = ~(E -> operands -> p -> value) & 0xff;
			break;
		
		case lw_expr_oper_plus:
			tr = E -> operands -> p -> value;
			for (o = E -> operands -> next; o; o = o -> next)
				tr += o -> p -> value;
			break;

		case lw_expr_oper_minus:
			tr = E -> operands -> p -> value;
			for (o = E -> operands -> next; o; o = o -> next)
				tr -= o -> p -> value;
			break;

		case lw_expr_oper_times:
			tr = E -> operands -> p -> value;
			for (o = E -> operands -> next; o; o = o -> next)
				tr *= o -> p -> value;
			break;

		case lw_expr_oper_divide:
			if (E -> operands -> next -> p -> value == 0)
			{
				tr = 0;
				lw_expr_divzero(priv);
				break;
			}
			tr = E -> operands -> p -> value / E -> operands -> next -> p -> value;
			break;
		
		case lw_expr_oper_mod:
			if (E -> operands -> next -> p -> value == 0)
			{
				tr = 0;
				lw_expr_divzero(priv);
				break;
			}
			tr = E -> operands -> p -> value % E -> operands -> next -> p -> value;
			break;
		
		case lw_expr_oper_intdiv:
			if (E -> operands -> next -> p -> value == 0)
			{
				tr = 0;
				lw_expr_divzero(priv);
				break;
			}
			tr = E -> operands -> p -> value / E -> operands -> next -> p -> value;
			break;

		case lw_expr_oper_bwand:
			tr = E -> operands -> p -> value & E -> operands -> next -> p -> value;
			break;

		case lw_expr_oper_bwor:
			tr = E -> operands -> p -> value | E -> operands -> next -> p -> value;
			break;

		case lw_expr_oper_bwxor:
			tr = E -> operands -> p -> value ^ E -> operands -> next -> p -> value;
			break;

		case lw_expr_oper_and:
			tr = E -> operands -> p -> value && E -> operands -> next -> p -> value;
			break;

		case lw_expr_oper_or:
			tr = E -> operands -> p -> value || E -> operands -> next -> p -> value;
			break;
		
		}
		
		while (E -> operands)
		{
			o = E -> operands;
			E -> operands = o -> next;
			lw_expr_destroy(o -> p);
			lw_free(o);
		}
		E -> type = lw_expr_type_int;
		E -> value = tr;
		return;
	}

	if (E -> value == lw_expr_oper_plus)
	{
		lw_expr_t e1;
		int cval = 0;
		
		e1 = lw_expr_create();
		e1 -> operands = E -> operands;
		E -> operands = 0;
		
		for (o = e1 -> operands; o; o = o -> next)
		{
			if (o -> p -> type == lw_expr_type_int)
				cval += o -> p -> value;
			else
				lw_expr_add_operand(E, o -> p);
		}
		lw_expr_destroy(e1);
		if (cval)
		{
			e1 = lw_expr_build(lw_expr_type_int, cval);
			lw_expr_add_operand(E, e1);
			lw_expr_destroy(e1);
		}
	}

	if (E -> value == lw_expr_oper_times)
	{
		lw_expr_t e1;
		int cval = 1;
		
		e1 = lw_expr_create();
		e1 -> operands = E -> operands;
		E -> operands = 0;
		
		for (o = e1 -> operands; o; o = o -> next)
		{
			if (o -> p -> type == lw_expr_type_int)
				cval *= o -> p -> value;
			else
				lw_expr_add_operand(E, o -> p);
		}
		lw_expr_destroy(e1);
		if (cval != 1)
		{
			e1 = lw_expr_build(lw_expr_type_int, cval);
			lw_expr_add_operand(E, e1);
			lw_expr_destroy(e1);
		}
	}

	if (E -> value == lw_expr_oper_times)
	{
		for (o = E -> operands; o; o = o -> next)
		{
			if (o -> p -> type == lw_expr_type_int && o -> p -> value == 0)
			{
				// one operand of times is 0, replace operation with 0
				while (E -> operands)
				{
					o = E -> operands;
					E -> operands = o -> next;
					lw_expr_destroy(o -> p);
					lw_free(o);
				}
				E -> type = lw_expr_type_int;
				E -> value = 0;
				return;
			}
		}
	}
	
	// sort "constants" to the start of each operand list for + and *
	if (E -> value == lw_expr_oper_plus || E -> value == lw_expr_oper_times)
		lw_expr_simplify_sortconstfirst(E);
	
	// look for like terms and collect them together
	if (E -> value == lw_expr_oper_plus)
	{
		struct lw_expr_opers *o2;
		for (o = E -> operands; o; o = o -> next)
		{
			// skip constants
			if (o -> p -> type == lw_expr_type_int)
				continue;
			
			// we have a term to match
			// (o -> p) is first term
			for (o2 = o -> next; o2; o2 = o2 -> next)
			{
				lw_expr_t e1, e2;
				
				if (o2 -> p -> type == lw_expr_type_int)
					continue;

				if (lw_expr_simplify_isliketerm(o -> p, o2 -> p))
				{
					int coef, coef2;
					
					// we have a like term here
					// do something about it
					if (o -> p -> type == lw_expr_type_oper && o -> p -> value == lw_expr_oper_times)
					{
						if (o -> p -> operands -> p -> type == lw_expr_type_int)
							coef = o -> p -> operands -> p -> value;
						else
							coef = 1;
					}
					else
						coef = 1;
					if (o2 -> p -> type == lw_expr_type_oper && o2 -> p -> value == lw_expr_oper_times)
					{
						if (o2 -> p -> operands -> p -> type == lw_expr_type_int)
							coef2 = o2 -> p -> operands -> p -> value;
						else
							coef2 = 1;
					}
					else
						coef2 = 1;
					coef += coef2;
					e1 = lw_expr_create();
					e1 -> type = lw_expr_type_oper;
					e1 -> value = lw_expr_oper_times;
					if (coef != 1)
					{
						e2 = lw_expr_build(lw_expr_type_int, coef);
						lw_expr_add_operand(e1, e2);
						lw_expr_destroy(e2);
					}
					lw_expr_destroy(o -> p);
					o -> p = e1;
					if (o2 -> p -> type == lw_expr_type_oper)
					{
						for (o = o2 -> p -> operands; o; o = o -> next)
						{
							if (o -> p -> type == lw_expr_type_int)
								continue;
							lw_expr_add_operand(e1, o -> p);
						}
					}
					else
					{
						lw_expr_add_operand(e1, o2 -> p);
					}
					lw_expr_destroy(o2 -> p);
					o2 -> p = lw_expr_build(lw_expr_type_int, 0);
					goto again;
				}
			}
		}
	}


	if (E -> value == lw_expr_oper_plus)
	{
		int c = 0, t = 0;
		for (o = E -> operands; o; o = o -> next)
		{
			t++;
			if (!(o -> p -> type == lw_expr_type_int && o -> p -> value == 0))
			{
				c++;
			}
		}
		if (c == 1)
		{
			lw_expr_t r = NULL;
			// find the value and "move it up"
			while (E -> operands)
			{
				o = E -> operands;
				if (o -> p -> type != lw_expr_type_int || o -> p -> value != 0)
				{
					r = lw_expr_copy(o -> p);
				}
				E -> operands = o -> next;
				lw_expr_destroy(o -> p);
				lw_free(o);
			}
			*E = *r;
			lw_free(r);
			return;
		}
		else if (c == 0)
		{
			// replace with 0
			while (E -> operands)
			{
				o = E -> operands;
				E -> operands = o -> next;
				lw_expr_destroy(o -> p);
				lw_free(o);
			}
			E -> type = lw_expr_type_int;
			E -> value = 0;
			return;
		}
		else if (c != t)
		{
			// collapse out zero terms
			struct lw_expr_opers *o2;
			
			for (o = E -> operands; o; o = o -> next)
			{
				if (o -> p -> type == lw_expr_type_int && o -> p -> value == 0)
				{
					if (o == E -> operands)
					{
						E -> operands = o -> next;
						lw_expr_destroy(o -> p);
						lw_free(o);
						o = E -> operands;
					}
					else
					{
						for (o2 = E -> operands; o2 -> next == o; o2 = o2 -> next)
							/* do nothing */ ;
						o2 -> next = o -> next;
						lw_expr_destroy(o -> p);
						lw_free(o);
						o = o2;
					}
				}
			}
		}
		return;
	}
	
	/* handle <int> times <plus> - expand the terms - only with exactly two operands */
	if (E -> value == lw_expr_oper_times)
	{
		lw_expr_t t1;
		lw_expr_t E2;
		lw_expr_t E3;
		if (E -> operands && E -> operands -> next && !(E -> operands -> next -> next))
		{
			if (E -> operands -> p -> type  == lw_expr_type_int)
			{
				/* <int> TIMES <other> */
				E2 = E -> operands -> next -> p;
				E3 = E -> operands -> p;
				if (E2 -> type == lw_expr_type_oper && E2 -> value == lw_expr_oper_plus)
				{
					lw_free(E -> operands -> next);
					lw_free(E -> operands);
					E -> operands = NULL;
					E -> value = lw_expr_oper_plus;
					
					for (o = E2 -> operands; o; o = o -> next)
					{
						t1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_times, E3, o -> p);
						lw_expr_add_operand(E, t1);
						lw_expr_destroy(t1);
					}
					
					lw_expr_destroy(E2);
					lw_expr_destroy(E3);
				}
			}
			else if (E -> operands -> next -> p -> type == lw_expr_type_int)
			{
				/* <other> TIMES <int> */
				E2 = E -> operands -> p;
				E3 = E -> operands -> next -> p;
				if (E2 -> type == lw_expr_type_oper && E2 -> value == lw_expr_oper_plus)
				{
					lw_free(E -> operands -> next);
					lw_free(E -> operands);
					E -> operands = NULL;
					E -> value = lw_expr_oper_plus;
					
					for (o = E2 -> operands; o; o = o -> next)
					{
						t1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_times, E3, o -> p);
						lw_expr_add_operand(E, t1);
					}
					
					lw_expr_destroy(E2);
					lw_expr_destroy(E3);
				}
			}
		}
	}
}

void lw_expr_simplify_l(lw_expr_t E, void *priv)
{
	lw_expr_t te;
	int c;
	
	(level)++;
	// bail out if the level gets too deep
	if (level >= 500 || bailing)
	{
		bailing = 1;
		level--;
		if (level == 0)
			bailing = 0;
		return;
	}
	do
	{
		te = lw_expr_copy(E);
		lw_expr_simplify_go(E, priv);
		c = 0;
		if (lw_expr_compare(te, E) == 0)
			c = 1;
		lw_expr_destroy(te);
	}
	while (c);
	(level)--;
}

void lw_expr_simplify(lw_expr_t E, void *priv)
{
	if (E -> type == lw_expr_type_int)
		return;
	lw_expr_simplify_l(E, priv);
}

/*

The following two functions are co-routines which evaluate an infix
expression.  lw_expr_parse_term checks for unary prefix operators then, if
none found, passes the string off the the defined helper function to
determine what the term really is. It also handles parentheses.

lw_expr_parse_expr evaluates actual expressions with infix operators. It
respects the order of operations.

The end of an expression is determined by the presence of any of the
following conditions:

1. a NUL character
2. a whitespace character (if parse mode is "COMPACT")
3. a )
4. a ,
5. any character that is not recognized as a term

lw_expr_parse_term returns NULL if there is no term (end of expr, etc.)

lw_expr_parse_expr returns NULL if there is no expression or on a syntax
error.

*/

lw_expr_t lw_expr_parse_expr(char **p, void *priv, int prec);

static void lw_expr_parse_next_tok(char **p)
{
	if (parse_compact)
		return;
	for (; **p && isspace(**p); (*p)++)
		/* do nothing */ ;
}

lw_expr_t lw_expr_parse_term(char **p, void *priv)
{
	lw_expr_t term, term2;
	
eval_next:
	lw_expr_parse_next_tok(p);

	if (!**p || isspace(**p) || **p == ')' || **p == ']')
		return NULL;
	// parentheses
	if (**p == '(')
	{
		(*p)++;
		term = lw_expr_parse_expr(p, priv, 0);
		lw_expr_parse_next_tok(p);
		if (**p != ')')
		{
			lw_expr_destroy(term);
			return NULL;
		}
		(*p)++;
		return term;
	}
	
	// unary +
	if (**p == '+')
	{
		(*p)++;
		goto eval_next;
	}
	
	// unary - (prec 200)
	if (**p == '-')
	{
		(*p)++;
		term = lw_expr_parse_expr(p, priv, 200);
		if (!term)
			return NULL;
		
		term2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_neg, term);
		lw_expr_destroy(term);
		return term2;
	}
	
	// unary ^ or ~ (complement, prec 200)
	if (**p == '^' || **p == '~')
	{
		(*p)++;
		term = lw_expr_parse_expr(p, priv, 200);
		if (!term)
			return NULL;
		
		if (expr_width == 8)
			term2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_com8, term);
		else
			term2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_com, term);
		lw_expr_destroy(term);
		return term2;
	}
	
	// non-operator - pass to caller
	return parse_term(p, priv);
}

lw_expr_t lw_expr_parse_expr(char **p, void *priv, int prec)
{
	static const struct operinfo
	{
		int opernum;
		char *operstr;
		int operprec;
	} operators[] =
	{
		{ lw_expr_oper_plus, "+", 100 },
		{ lw_expr_oper_minus, "-", 100 },
		{ lw_expr_oper_times, "*", 150 },
		{ lw_expr_oper_divide, "/", 150 },
		{ lw_expr_oper_mod, "%", 150 },
		{ lw_expr_oper_intdiv, "\\", 150 },
		
		{ lw_expr_oper_and, "&&", 25 },
		{ lw_expr_oper_or, "||", 25 },
		
		{ lw_expr_oper_bwand, "&", 50 },
		{ lw_expr_oper_bwor, "|", 50 },
		{ lw_expr_oper_bwor, "!", 50 },
		{ lw_expr_oper_bwxor, "^", 50 },
		
		{ lw_expr_oper_none, "", 0 }
	};
	
	int opern, i;
	lw_expr_t term1, term2, term3;
	
	lw_expr_parse_next_tok(p);
	if (!**p || isspace(**p) || **p == ')' || **p == ',' || **p == ']' || **p == ';')
		return NULL;

	term1 = lw_expr_parse_term(p, priv);
	if (!term1)
		return NULL;

eval_next:
	lw_expr_parse_next_tok(p);
	if (!**p || isspace(**p) || **p == ')' || **p == ',' || **p == ']' || **p == ';')
		return term1;
	
	// expecting an operator here
	for (opern = 0; operators[opern].opernum != lw_expr_oper_none; opern++)
	{
		for (i = 0; (*p)[i] && operators[opern].operstr[i] && ((*p)[i] == operators[opern].operstr[i]); i++)
			/* do nothing */;
		if (operators[opern].operstr[i] == '\0')
			break;
	}
	
	if (operators[opern].opernum == lw_expr_oper_none)
	{
		// unrecognized operator
		lw_expr_destroy(term1);
		return NULL;
	}

	// operator number is in opern, length of oper in i
	
	// logic:
	// if the precedence of this operation is <= to the "prec" flag,
	// we simply return without advancing the input pointer; the operator
	// will be evaluated again in the enclosing function call
	if (operators[opern].operprec <= prec)
		return term1;
	
	// logic:
	// we have a higher precedence operator here so we will advance the
	// input pointer to the next term and let the expression evaluator
	// loose on it after which time we will push our operator onto the
	// stack and then go on with the expression evaluation
	(*p) += i;
	
	// evaluate next expression(s) of higher precedence
	term2 = lw_expr_parse_expr(p, priv, operators[opern].operprec);
	if (!term2)
	{
		lw_expr_destroy(term1);
		return NULL;
	}
	
	// now create operator
	term3 = lw_expr_build(lw_expr_type_oper, operators[opern].opernum, term1, term2);
	lw_expr_destroy(term1);
	lw_expr_destroy(term2);
	
	// the new "expression" is the next "left operand"
	term1 = term3;
	
	// continue evaluating
	goto eval_next;
}

lw_expr_t lw_expr_parse(char **p, void *priv)
{
	parse_compact = 0;
	return lw_expr_parse_expr(p, priv, 0);
}

lw_expr_t lw_expr_parse_compact(char **p, void *priv)
{
	parse_compact = 1;
	return lw_expr_parse_expr(p, priv, 0);
}
	

int lw_expr_testterms(lw_expr_t e, lw_expr_testfn_t *fn, void *priv)
{
	struct lw_expr_opers *o;
	int r;
	
	for (o = e -> operands; o; o = o -> next)
	{
		r = lw_expr_testterms(o -> p, fn, priv);
		if (r)
			return r;
	}
	return (fn)(e, priv);
}

int lw_expr_type(lw_expr_t e)
{
	return e -> type;
}

void *lw_expr_specptr(lw_expr_t e)
{
	return e -> value2;
}

int lw_expr_operandcount(lw_expr_t e)
{
	int count = 0;
	struct lw_expr_opers *o;
	
	if (e -> type != lw_expr_type_oper)
		return 0;
	
	for (o = e -> operands; o; o = o -> next)
		count++;
	
	return count;
}
