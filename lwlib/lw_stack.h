/*
lw_stack.h

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

#ifndef ___lw_stack_h_seen___
#define ___lw_stack_h_seen___


#ifdef ___lw_stack_c_seen___
struct lw_stack_node_priv
{
	void *data;
	struct lw_stack_node_priv *next;
};

struct lw_stack_priv
{
	struct lw_stack_node_priv *head;
	void (*freefn)(void *d);
};

typedef struct lw_stack_priv * lw_stack_t;


#else /* def ___lw_stack_c_seen___ */

typedef void * lw_stack_t;
lw_stack_t lw_stack_create(void (*freefn)(void *d));
void lw_stack_destroy(lw_stack_t S);
void *lw_stack_top(lw_stack_t S);
void *lw_stack_pop(lw_stack_t S);
void lw_stack_push(lw_stack_t S, void *item);

#endif /* def ___lw_stack_c_seen___ */

#endif /* ___lw_stack_h_seen___ */
