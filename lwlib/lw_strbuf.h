/*
lwlib/lw_strbuf.h

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

#ifndef ___lw_strbuf_h_seen___
#define ___lw_strbuf_h_seen___

struct lw_strbuf
{
	char *str;
	int bl;
	int bo;
};

extern struct lw_strbuf *lw_strbuf_new(void);
extern void lw_strbuf_add(struct lw_strbuf *, int);
extern char *lw_strbuf_end(struct lw_strbuf  *);

#endif // ___lw_strbuf_h_seen___
