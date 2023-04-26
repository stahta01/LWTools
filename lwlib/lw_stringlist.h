/*
lw_stringlist.h

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

#ifndef ___lw_stringlist_h_seen___
#define ___lw_stringlist_h_seen___


#ifdef ___lw_stringlist_c_seen___

struct lw_stringlist_priv
{
	char **strings;
	int nstrings;
	int cstring;
};
typedef struct lw_stringlist_priv * lw_stringlist_t;

#else /* def ___lw_stringlist_c_seen___ */

typedef void * lw_stringlist_t;
lw_stringlist_t lw_stringlist_create(void);
void lw_stringlist_destroy(lw_stringlist_t S);
void lw_stringlist_addstring(lw_stringlist_t S, char *str);
void lw_stringlist_reset(lw_stringlist_t S);
char *lw_stringlist_current(lw_stringlist_t S);
char *lw_stringlist_next(lw_stringlist_t S);
int lw_stringlist_nstrings(lw_stringlist_t S);
lw_stringlist_t lw_stringlist_copy(lw_stringlist_t S);

#endif /* def ___lw_stringlist_c_seen___ */

#endif /* ___lw_stringlist_h_seen___ */
