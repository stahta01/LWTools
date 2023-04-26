/*
lwlib/lw_strpool.h

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

#ifndef ___lw_strpool_h_seen___
#define ___lw_strpool_h_seen___

struct lw_strpool
{
	int nstrs;
	char **strs;
};

extern struct lw_strpool *lw_strpool_create(void);
extern void lw_strpool_free(struct lw_strpool *);
extern char *lw_strpool_strdup(struct lw_strpool *, const char *);

#endif // ___lw_strpool_h_seen____
