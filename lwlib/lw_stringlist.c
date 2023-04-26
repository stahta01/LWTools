/*
lw_stringlist.c

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

#include <stdlib.h>

#define ___lw_stringlist_c_seen___
#include "lw_stringlist.h"
#include "lw_string.h"
#include "lw_alloc.h"

lw_stringlist_t lw_stringlist_create(void)
{
	lw_stringlist_t s;
	
	
	s = lw_alloc(sizeof(struct lw_stringlist_priv));
	s -> strings = NULL;
	s -> nstrings = 0;
	s -> cstring = 0;
	
	return s;
}

void lw_stringlist_destroy(lw_stringlist_t S)
{
	if (S)
	{
		int i;
		for (i = 0; i < S -> nstrings; i++)
		{
			lw_free(S -> strings[i]);
		}
		lw_free(S -> strings);
		lw_free(S);
	}
}

void lw_stringlist_addstring(lw_stringlist_t S, char *str)
{
	S -> strings = lw_realloc(S -> strings, sizeof(char *) * (S -> nstrings + 1));
	S -> strings[S -> nstrings] = lw_strdup(str);
	S -> nstrings++;
}

void lw_stringlist_reset(lw_stringlist_t S)
{
	S -> cstring = 0;
}

char *lw_stringlist_current(lw_stringlist_t S)
{
	if (S -> cstring >= S -> nstrings)
		return NULL;
	return S -> strings[S -> cstring];
}

char *lw_stringlist_next(lw_stringlist_t S)
{
	S -> cstring++;
	return lw_stringlist_current(S);
}

int lw_stringlist_nstrings(lw_stringlist_t S)
{
	return S -> nstrings;
}

lw_stringlist_t lw_stringlist_copy(lw_stringlist_t S)
{
	lw_stringlist_t r;
	
	r = lw_alloc(sizeof(lw_stringlist_t));
	r -> nstrings = S -> nstrings;
	if (S -> nstrings)
	{
		int i;
		
		r -> strings = lw_alloc(sizeof(char *) * S -> nstrings);
		for (i = 0; i < S -> nstrings; i++)
		{
			r -> strings[i] = lw_strdup(S -> strings[i]);
		}
	}
	return r;
}
