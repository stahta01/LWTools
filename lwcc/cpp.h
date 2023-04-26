/*
lwcc/cpp.h

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

#ifndef cpp_h_seen___
#define cpp_h_seen___

#include <stdio.h>

#include <lw_stringlist.h>

//#include "symbol.h"
#include "token.h"

#define TOKBUFSIZE 32

struct expand_e
{
	struct expand_e *next;
	struct symtab_e *s;		// symbol table entry of the expanding symbol
};

struct preproc_info
{
	const char *fn;
	FILE *fp;
	struct token *tokqueue;
	struct token *curtok;
	void (*errorcb)(const char *);
	void (*warningcb)(const char *);
	int eolstate;			// internal for use in handling newlines
	int lineno;				// the current input line number
	int column;				// the current input column
	int trigraphs;			// nonzero if we're going to handle trigraphs
	int ra;
	int qseen;
	int ungetbufl;
	int ungetbufs;
	int *ungetbuf;
	int unget;
	int eolseen;
	int nlseen;
	int ppeolseen;			// nonzero if we've seen only whitespace (or nothing) since a newline
	int skip_level;			// nonzero if we're in a false conditional
	int found_level;		// nonzero if we're in a true conditional
	int else_level;			// for counting #else directives
	int else_skip_level;	// ditto
	struct symtab_e *sh;	// the preprocessor's symbol table
	struct token *sourcelist;	// for expanding a list of tokens
	struct expand_e *expand_list;	// record of which macros are currently being expanded
	char *lexstr;			// for lexing a string (token pasting)
	int lexstrloc;			// ditto
	struct preproc_info *n;	// next in file stack
	struct preproc_info *filestack;	// stack of saved files during include
	struct lw_strpool *strpool;
	lw_stringlist_t quotelist;
	lw_stringlist_t inclist;
};

extern struct preproc_info *preproc_init(const char *);
extern struct token *preproc_next_token(struct preproc_info *);
extern struct token *preproc_next_processed_token(struct preproc_info *);
extern void preproc_finish(struct preproc_info *);
extern void preproc_register_error_callback(struct preproc_info *, void (*)(const char *));
extern void preproc_register_warning_callback(struct preproc_info *, void (*)(const char *));
extern void preproc_throw_error(struct preproc_info *, const char *, ...);
extern void preproc_throw_warning(struct preproc_info *, const char *, ...);
extern void preproc_unget_token(struct preproc_info *, struct token *);
extern void preproc_add_include(struct preproc_info *, char *, int);
extern void preproc_add_macro(struct preproc_info *, char *);
extern struct token *preproc_next(struct preproc_info *);

#endif // cpp_h_seen___
