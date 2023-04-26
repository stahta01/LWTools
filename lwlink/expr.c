/*
expr.c
Copyright © 2009 William Astle

This file is part of LWLINK.

LWLINK is free software: you can redistribute it and/or modify it under the
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
This file contains the actual expression evaluator
*/

#define __expr_c_seen__

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "expr.h"

lw_expr_stack_t *lw_expr_stack_create(void)
{
	lw_expr_stack_t *s;
	
	s = lw_alloc(sizeof(lw_expr_stack_t));
	s -> head = NULL;
	s -> tail = NULL;
	return s;
}

void lw_expr_stack_free(lw_expr_stack_t *s)
{
	while (s -> head)
	{
		s -> tail = s -> head;
		s -> head = s -> head -> next;
		lw_expr_term_free(s -> tail -> term);
		lw_free(s -> tail);
	}
	lw_free(s);
}

lw_expr_stack_t *lw_expr_stack_dup(lw_expr_stack_t *s)
{
	lw_expr_stack_node_t *t;
	lw_expr_stack_t *s2;
		
	s2 = lw_expr_stack_create();	
	for (t = s -> head; t; t = t -> next)
	{
		lw_expr_stack_push(s2, t -> term);
	}
	return s2;
}

void lw_expr_term_free(lw_expr_term_t *t)
{
	if (t)
	{
		if (t -> term_type == LW_TERM_SYM)
			lw_free(t -> symbol);
		lw_free(t);
	}
}

lw_expr_term_t *lw_expr_term_create_oper(int oper)
{
	lw_expr_term_t *t;

	t = lw_alloc(sizeof(lw_expr_term_t));
	t -> term_type = LW_TERM_OPER;
	t -> value = oper;
	return t;
}

lw_expr_term_t *lw_expr_term_create_int(int val)
{
	lw_expr_term_t *t;
	
	t = lw_alloc(sizeof(lw_expr_term_t));
	t -> term_type = LW_TERM_INT;
	t -> value = val;
	return t;
}

lw_expr_term_t *lw_expr_term_create_sym(char *sym, int symtype)
{
	lw_expr_term_t *t;
	
	t = lw_alloc(sizeof(lw_expr_term_t));
	t -> term_type = LW_TERM_SYM;
	t -> symbol = lw_strdup(sym);
	t -> value = symtype;
	return t;
}

lw_expr_term_t *lw_expr_term_dup(lw_expr_term_t *t)
{
	switch (t -> term_type)
	{
	case LW_TERM_INT:
		return lw_expr_term_create_int(t -> value);
		
	case LW_TERM_OPER:
		return lw_expr_term_create_oper(t -> value);
		
	case LW_TERM_SYM:
		return lw_expr_term_create_sym(t -> symbol, t -> value);
	
	default:
		exit(1);
	}
// can't get here
}

void lw_expr_stack_push(lw_expr_stack_t *s, lw_expr_term_t *t)
{
	lw_expr_stack_node_t *n;

	if (!s)
	{
		exit(1);
	}
	
	n = lw_alloc(sizeof(lw_expr_stack_node_t));
	n -> next = NULL;
	n -> prev = s -> tail;
	n -> term = lw_expr_term_dup(t);
	
	if (s -> head)
	{
		s -> tail -> next = n;
		s -> tail = n;
	}
	else
	{
		s -> head = n;
		s -> tail = n;
	}
}

lw_expr_term_t *lw_expr_stack_pop(lw_expr_stack_t *s)
{
	lw_expr_term_t *t;
	lw_expr_stack_node_t *n;
	
	if (!(s -> tail))
		return NULL;
	
	n = s -> tail;
	s -> tail = n -> prev;
	if (!(n -> prev))
	{
		s -> head = NULL;
	}
	
	t = n -> term;
	n -> term = NULL;
	
	lw_free(n);
	
	return t;
}

/*
take an expression stack s and scan for operations that can be completed

return -1 on error, 0 on no error

possible errors are: division by zero or unknown operator

theory of operation:

scan the stack for an operator which has two constants preceding it (binary)
or 1 constant preceding it (unary) and if found, perform the calculation
and replace the operator and its operands with the result

repeat the scan until no futher simplications are found or if there are no
further operators or only a single term remains

*/
int lw_expr_reval(lw_expr_stack_t *s, lw_expr_stack_t *(*sfunc)(char *sym, int stype, void *state), void *state)
{
	lw_expr_stack_node_t *n;
	lw_expr_stack_t *ss;
	int c;
	
next_iter_sym:
	// resolve symbols
	// symbols that do not resolve to an expression are left alone
	for (c = 0, n = s -> head; n; n = n -> next)
	{
		if (n -> term -> term_type == LW_TERM_SYM)
		{
			ss = sfunc(n -> term -> symbol, n -> term -> value, state);
			if (ss)
			{
				c++;
				// splice in the result stack
				if (n -> prev)
				{
					n -> prev -> next = ss -> head;
				}
				else
				{
					s -> head = ss -> head;
				}
				ss -> head -> prev = n -> prev;
				ss -> tail -> next = n -> next;
				if (n -> next)
				{
					n -> next -> prev = ss -> tail;
				}
				else
				{
					s -> tail = ss -> tail;
				}
				lw_expr_term_free(n -> term);
				lw_free(n);
				n = ss -> tail;
				
				ss -> head = NULL;
				ss -> tail = NULL;
				lw_expr_stack_free(ss);
			}
		}
	}
	if (c)
		goto next_iter_sym;

next_iter:	
	// a single term
	if (s -> head == s -> tail)
		return 0;
	
	// search for an operator
	for (n = s -> head; n; n = n -> next)
	{
		if (n -> term -> term_type == LW_TERM_OPER)
		{
			if (n -> term -> value == LW_OPER_NEG
				|| n -> term -> value == LW_OPER_COM
				)
			{
				// unary operator
				if (n -> prev && n -> prev -> term -> term_type == LW_TERM_INT)
				{
					// a unary operator we can resolve
					// we do the op then remove the term "n" is pointing at
					if (n -> term -> value == LW_OPER_NEG)
					{
						n -> prev -> term -> value = -(n -> prev -> term -> value);
					}
					else if (n -> term -> value == LW_OPER_COM)
					{
						n -> prev -> term -> value = ~(n -> prev -> term -> value);
					}
					n -> prev -> next = n -> next;
					if (n -> next)
						n -> next -> prev = n -> prev;
					else
						s -> tail = n -> prev;	
					
					lw_expr_term_free(n -> term);
					lw_free(n);
					break;
				}
			}
			else
			{
				// binary operator
				if (n -> prev && n -> prev -> prev && n -> prev -> term -> term_type == LW_TERM_INT && n -> prev -> prev -> term -> term_type == LW_TERM_INT)
				{
					// a binary operator we can resolve
					switch (n -> term -> value)
					{
					case LW_OPER_PLUS:
						n -> prev -> prev -> term -> value += n -> prev -> term -> value;
						break;

					case LW_OPER_MINUS:
						n -> prev -> prev -> term -> value -= n -> prev -> term -> value;
						break;

					case LW_OPER_TIMES:
						n -> prev -> prev -> term -> value *= n -> prev -> term -> value;
						break;

					case LW_OPER_DIVIDE:
						if (n -> prev -> term -> value == 0)
							return -1;
						n -> prev -> prev -> term -> value /= n -> prev -> term -> value;
						break;

					case LW_OPER_MOD:
						if (n -> prev -> term -> value == 0)
							return -1;
						n -> prev -> prev -> term -> value %= n -> prev -> term -> value;
						break;

					case LW_OPER_INTDIV:
						if (n -> prev -> term -> value == 0)
							return -1;
						n -> prev -> prev -> term -> value /= n -> prev -> term -> value;
						break;

					case LW_OPER_BWAND:
						n -> prev -> prev -> term -> value &= n -> prev -> term -> value;
						break;

					case LW_OPER_BWOR:
						n -> prev -> prev -> term -> value |= n -> prev -> term -> value;
						break;

					case LW_OPER_BWXOR:
						n -> prev -> prev -> term -> value ^= n -> prev -> term -> value;
						break;

					case LW_OPER_AND:
						n -> prev -> prev -> term -> value = (n -> prev -> term -> value && n -> prev -> prev -> term -> value) ? 1 : 0;
						break;

					case LW_OPER_OR:
						n -> prev -> prev -> term -> value = (n -> prev -> term -> value || n -> prev -> prev -> term -> value) ? 1 : 0;
						break;

					default:
						// return error if unknown operator!
						return -1;
					}

					// now remove the two unneeded entries from the stack
					n -> prev -> prev -> next = n -> next;
					if (n -> next)
						n -> next -> prev = n -> prev -> prev;
					else
						s -> tail = n -> prev -> prev;	
					
					lw_expr_term_free(n -> term);
					lw_expr_term_free(n -> prev -> term);
					lw_free(n -> prev);
					lw_free(n);
					break;
				}
			}
		}
	}
	// note for the terminally confused about dynamic memory and pointers:
	// n will not be NULL even after the lw_free calls above so
	// this test will still work (n will be a dangling pointer)
	// (n will only be NULL if we didn't find any operators to simplify)
	if (n)
		goto next_iter;
	
	return 0;
}
