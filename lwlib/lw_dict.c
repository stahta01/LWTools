/*
lw_dict.c

Copyright © 2021 William Astle

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

#include <stdlib.h>
#include <string.h>

#define ___lw_dict_c_seen___
#include "lw_dict.h"
#include "lw_string.h"
#include "lw_alloc.h"

lw_dict_t lw_dict_create(void)
{
	lw_dict_t s;
	
	
	s = lw_alloc(sizeof(struct lw_dict_priv));
	s -> head = NULL;
	return s;
}

void lw_dict_unset(lw_dict_t S, char *key)
{
    struct lw_dict_ent *e1, *e2;
    
    for (e2 = NULL, e1 = S -> head; e1; )
    {
        if (strcmp(key, e1 -> key) == 0)
            break;
        e2 = e1;
        e1 = e1 -> next;
    }
    if (!e1)
        return;
    if (!e2)
        S -> head = e1 -> next;
    else
        e2 -> next = e1 -> next;
    lw_free(e1 -> key);
    lw_free(e1 -> value);
    lw_free(e1);
}

void lw_dict_destroy(lw_dict_t S)
{
	if (S)
	{
	    while (S -> head)
	        lw_dict_unset(S, S -> head -> key);
        lw_free(S);
	}
}

char *lw_dict_get(lw_dict_t S, char *key)
{
    struct lw_dict_ent *e1;
    
    for (e1 = S -> head; e1; e1 = e1 -> next)
        if (strcmp(key, e1 -> key) == 0)
            break;
    if (e1)
        return e1 -> value;
    return "";
}

void lw_dict_set(lw_dict_t S, char *key, char *value)
{
    struct lw_dict_ent *e1;

    for (e1 = S -> head; e1; e1 = e1 -> next)
    {
        if (strcmp(key, e1 -> key) == 0)
        {
            lw_free(e1 -> value);
            e1 -> value = lw_strdup(value);
            return;
        }
    }

    e1 = lw_alloc(sizeof(struct lw_dict_ent));
    e1 -> next = S -> head;
    S -> head = e1;
    e1 -> key = lw_strdup(key);
    e1 -> value = lw_strdup(value);
}

lw_dict_t lw_dict_copy(lw_dict_t S)
{
    lw_dict_t R;
    struct lw_dict_ent *e1;

    R = lw_dict_create();
    for (e1 = S -> head; e1; e1 = e1 -> next)
        lw_dict_set(R, e1 -> key, e1 -> value);
    return R;
}
