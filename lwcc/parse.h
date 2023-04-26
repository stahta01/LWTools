/*
lwcc/parse.h

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

#ifndef parse_h_seen__
#define parse_h_seen__

#include <stdio.h>
#include "tree.h"

struct tokendata
{
	int tokid;
	unsigned char numval[8];
	char *strval;
};


extern void tokendata_free(struct tokendata *td);

struct parserinfo
{
	node_t *parsetree;
};

extern char *tokendata_name(struct tokendata *td);
extern void tokendata_print(FILE *fp, struct tokendata *td);

#endif // parse_h_seen__
