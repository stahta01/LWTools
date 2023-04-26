/*
lw_string.c

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

#include <string.h>
#include <stdlib.h>

#include "lw_alloc.h"
#include "lw_string.h"

char *lw_strdup(const char *s)
{
	char *r;
	
	if (!s)
		s = "(null)";

	r = lw_alloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

char *lw_strndup(const char *s, int len)
{
	char *r;
	int sl;
	
	sl = strlen(s);
	if (sl > len)
		sl = len;
	
	r = lw_alloc(sl + 1);
	memmove(r, s, sl);
	r[sl] = '\0';
	return r;
}

char *lw_token(const char *s, int sep, const char **ap)
{
	const char *p;
	char *r;

	if (!s)
		return NULL;
	
	p = strchr(s, sep);
	if (!p)
	{
		if (ap)
			*ap = NULL;
		return lw_strdup(s);
	}
	
	r = lw_alloc(p - s + 1);
	strncpy(r, (char *)s, p - s);
	r[p - s] = '\0';
	
	if (ap)
	{
		while (*p && *p == sep)
			p++;
		*ap = p;
	}
	return r;
}
