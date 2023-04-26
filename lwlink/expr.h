/*
expr.h
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
Definitions for expression evaluator
*/

#ifndef __expr_h_seen__
#define __expr_h_seen__

#ifndef __expr_c_seen__
#define __expr_E__ extern
#else
#define __expr_E__
#endif

// term types
#define LW_TERM_NONE		0
#define LW_TERM_OPER		1	// an operator
#define LW_TERM_INT		2	// 32 bit signed integer
#define LW_TERM_SYM		3	// symbol reference

// operator types
#define LW_OPER_NONE		0
#define LW_OPER_PLUS		1	// +
#define LW_OPER_MINUS	2	// -
#define LW_OPER_TIMES	3	// *
#define LW_OPER_DIVIDE	4	// /
#define LW_OPER_MOD		5	// %
#define LW_OPER_INTDIV	6	// \ (don't end line with \)
#define LW_OPER_BWAND	7	// bitwise AND
#define LW_OPER_BWOR		8	// bitwise OR
#define LW_OPER_BWXOR	9	// bitwise XOR
#define LW_OPER_AND		10	// boolean AND
#define LW_OPER_OR		11	// boolean OR
#define LW_OPER_NEG		12	// - unary negation (2's complement)
#define LW_OPER_COM		13	// ^ unary 1's complement


// term structure
typedef struct lw_expr_term_s
{
	int term_type;		// type of term (see above)
	char *symbol;		// name of a symbol
	int value;			// value of the term (int) or operator number (OPER)
} lw_expr_term_t;

// type for an expression evaluation stack
typedef struct lw_expr_stack_node_s lw_expr_stack_node_t;
struct lw_expr_stack_node_s
{
	lw_expr_term_t		*term;
	lw_expr_stack_node_t	*prev;
	lw_expr_stack_node_t	*next;	
};

typedef struct lw_expr_stack_s
{
	lw_expr_stack_node_t *head;
	lw_expr_stack_node_t *tail;
} lw_expr_stack_t;

__expr_E__ void lw_expr_term_free(lw_expr_term_t *t);
__expr_E__ lw_expr_term_t *lw_expr_term_create_oper(int oper);
__expr_E__ lw_expr_term_t *lw_expr_term_create_sym(char *sym, int symtype);
__expr_E__ lw_expr_term_t *lw_expr_term_create_int(int val);
__expr_E__ lw_expr_term_t *lw_expr_term_dup(lw_expr_term_t *t);

__expr_E__ void lw_expr_stack_free(lw_expr_stack_t *s);
__expr_E__ lw_expr_stack_t *lw_expr_stack_create(void);
__expr_E__ lw_expr_stack_t *lw_expr_stack_dup(lw_expr_stack_t *s);

__expr_E__ void lw_expr_stack_push(lw_expr_stack_t *s, lw_expr_term_t *t);
__expr_E__ lw_expr_term_t *lw_expr_stack_pop(lw_expr_stack_t *s);

// simplify expression
__expr_E__ int lw_expr_reval(lw_expr_stack_t *s, lw_expr_stack_t *(*sfunc)(char *sym, int symtype, void *state), void *state);

// useful macros
// is the expression "simple" (one term)?
#define lw_expr_is_simple(s) ((s) -> head == (s) -> tail)

// is the expression constant?
#define lw_expr_is_constant(s) (lw_expr_is_simple(s) && (!((s) -> head) || (s) -> head -> term -> term_type == LW_TERM_INT))

// get the constant value of an expression or 0 if not constant
#define lw_expr_get_value(s) (lw_expr_is_constant(s) ? ((s) -> head ? (s) -> head -> term -> value : 0) : 0)

#undef __expr_E__

#endif // __expr_h_seen__
