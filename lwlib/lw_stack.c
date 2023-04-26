/*
lw_stack.c

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

#define ___lw_stack_c_seen___
#include "lw_stack.h"
#include "lw_alloc.h"

/* this is technically valid but dubious */
#define NULL 0

lw_stack_t lw_stack_create(void (*freefn)(void *d))
{
	lw_stack_t S;
	
	S = lw_alloc(sizeof(struct lw_stack_priv));
	S -> head = NULL;
	S -> freefn = freefn;
	return S;
}

void *lw_stack_pop(lw_stack_t S)
{
	if (S -> head)
	{
		void *ret, *r2;
		
		ret = S -> head -> data;
		r2 = S -> head;
		S -> head = S -> head -> next;
		lw_free(r2);
		return ret;
	}
	return NULL;
}

void lw_stack_destroy(lw_stack_t S)
{
	void *d;
	
	while ((d = lw_stack_pop(S)))
		(S->freefn)(d);
	lw_free(S);
}

void *lw_stack_top(lw_stack_t S)
{
	if (S -> head)
		return S -> head -> data;
	return NULL;
}

void lw_stack_push(lw_stack_t S, void *item)
{
	struct lw_stack_node_priv *t;
	
	t = lw_alloc(sizeof(struct lw_stack_node_priv));
	t -> next = S -> head;
	S -> head = t;
	t -> data = item;
}
