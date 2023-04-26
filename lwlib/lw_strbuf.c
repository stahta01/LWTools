/*
lwlib/lw_strbuf.c

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

#include "lw_alloc.h"
#include "lw_strbuf.h"

struct lw_strbuf *lw_strbuf_new(void)
{
	struct lw_strbuf *lw_strbuf;
	
	lw_strbuf = lw_alloc(sizeof(struct lw_strbuf));
	lw_strbuf -> str = NULL;
	lw_strbuf -> bo = 0;
	lw_strbuf -> bl = 0;
	return lw_strbuf;
}

void lw_strbuf_add(struct lw_strbuf *lw_strbuf, int c)
{
	if (lw_strbuf -> bo >= lw_strbuf -> bl)
	{
		lw_strbuf -> bl += 100;
		lw_strbuf -> str = lw_realloc(lw_strbuf -> str, lw_strbuf -> bl);
	}
	lw_strbuf -> str[lw_strbuf -> bo++] = c;
}

char *lw_strbuf_end(struct lw_strbuf *lw_strbuf)
{
	char *rv;

	lw_strbuf_add(lw_strbuf, 0);
	rv = lw_strbuf -> str;
	lw_free(lw_strbuf);
	return rv;
}
