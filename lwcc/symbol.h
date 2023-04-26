/*
lwcc/symbol.h

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

#ifndef symbol_h_seen___
#define symbol_h_seen___

#include "cpp.h"
#include "token.h"

struct symtab_e
{
	char *name;				// symbol name
	struct token_list *tl;	// token list the name is defined as, NULL for none
	int nargs;				// number named of arguments - -1 for object like macro
	int vargs;				// set if macro has varargs style
	char **params;			// the names of the parameters
	struct symtab_e *next;	// next entry in list
};

struct symtab_e *symtab_find(struct preproc_info *, char *);
void symtab_undef(struct preproc_info *, char *);
void symtab_define(struct preproc_info *, char *, struct token_list *, int, char **, int);

#endif // symbol_h_seen___
