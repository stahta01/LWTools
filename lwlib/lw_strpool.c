/*
lwlib/lw_strpool.c

Copyright © 2013 William Astle

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

#include "lw_alloc.h"
#include "lw_string.h"
#include "lw_strpool.h"

struct lw_strpool *lw_strpool_create(void)
{
	struct lw_strpool *sp;
	
	sp = lw_alloc(sizeof(struct lw_strpool));
	sp -> nstrs = 0;
	sp -> strs = NULL;
	return sp;
}

extern void lw_strpool_free(struct lw_strpool *sp)
{
	int i;
	
	for (i = 0; i < sp -> nstrs; i++)
		lw_free(sp -> strs[i]);
	lw_free(sp -> strs);
	lw_free(sp);
}

char *lw_strpool_strdup(struct lw_strpool *sp, const char *s)
{
	int i;
	
	if (!s)
		return NULL;

	/* first do a fast scan for a pointer match */	
	for (i = 0; i < sp -> nstrs; i++)
		if (sp -> strs[i] == s)
			return sp -> strs[i];
	
	/* no match - do a slow scan for a string match */
	for (i = 0; i < sp -> nstrs; i++)
		if (strcmp(sp -> strs[i], s) == 0)
			return sp -> strs[i];
	
	/* no match - create a new string entry */
	sp -> strs = lw_realloc(sp -> strs, sizeof(char *) * (sp -> nstrs + 1));
	sp -> strs[sp -> nstrs] = lw_strdup(s);
	sp -> nstrs++;
	return sp -> strs[sp -> nstrs - 1];
}
