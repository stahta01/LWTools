/*
lwcc/cpp.c

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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_stringlist.h>
#include <lw_strpool.h>
#include "cpp.h"


struct token *preproc_lex_next_token(struct preproc_info *);

struct preproc_info *preproc_init(const char *fn)
{
	FILE *fp;
	struct preproc_info *pp;
	
	if (!fn || (fn[0] == '-' && fn[1] == '0'))
	{
		fp = stdin;
	}
	else
	{
		fp = fopen(fn, "rb");
	}
	if (!fp)
		return NULL;
	
	pp = lw_alloc(sizeof(struct preproc_info));
	memset(pp, 0, sizeof(struct preproc_info));
	pp -> strpool = lw_strpool_create();
	pp -> fn = lw_strpool_strdup(pp -> strpool, fn);
	pp -> fp = fp;
	pp -> ra = CPP_NOUNG;
	pp -> unget = CPP_NOUNG;
	pp -> ppeolseen = 1;
	pp -> lineno = 1;
	pp -> n = NULL;
	pp -> quotelist = lw_stringlist_create();
	pp -> inclist = lw_stringlist_create();
	return pp;
}

void preproc_add_include(struct preproc_info *pp, char *dir, int sys)
{
	if (sys)
		lw_stringlist_addstring(pp -> inclist, dir);
	else
		lw_stringlist_addstring(pp -> quotelist, dir);
}

struct token *preproc_next_token(struct preproc_info *pp)
{
	struct token *t;
	
	if (pp -> curtok)
		token_free(pp -> curtok);

	/*
	If there is a list of tokens to process, move it to the "unget" queue
	with an EOF marker at the end of it.
	*/	
	if (pp -> sourcelist)
	{
		for (t = pp -> sourcelist; t -> next; t = t -> next)
			/* do nothing */ ;
		t -> next = token_create(TOK_EOF, NULL, -1, -1, "");
		t -> next -> next = pp -> tokqueue;
		pp -> tokqueue = pp -> sourcelist;
		pp -> sourcelist = NULL;
	}
again:
	if (pp -> tokqueue)
	{
		t = pp -> tokqueue;
		pp -> tokqueue = t -> next;
		if (pp -> tokqueue)
			pp -> tokqueue -> prev = NULL;
		t -> next = NULL;
		t -> prev = NULL;
		pp -> curtok = t;
		goto ret;
	}
	pp -> curtok = preproc_lex_next_token(pp);
	t = pp -> curtok;
ret:
	if (t -> ttype == TOK_ENDEXPAND)
	{
		struct expand_e *e;
		e = pp -> expand_list;
		pp -> expand_list = e -> next;
		lw_free(e);
		goto again;
	}
	return t;
}

void preproc_unget_token(struct preproc_info *pp, struct token *t)
{
	t -> next = pp -> tokqueue;
	pp -> tokqueue = t;
	if (pp -> curtok == t)
		pp -> curtok = NULL;
}

void preproc_finish(struct preproc_info *pp)
{
	fclose(pp -> fp);
	lw_stringlist_destroy(pp -> inclist);
	lw_stringlist_destroy(pp -> quotelist);
	if (pp -> curtok)
		token_free(pp -> curtok);
	while (pp -> tokqueue)
	{
		preproc_next_token(pp);
		token_free(pp -> curtok);
	}
	lw_strpool_free(pp -> strpool);
	lw_free(pp);
}

void preproc_register_error_callback(struct preproc_info *pp, void (*cb)(const char *))
{
	pp -> errorcb = cb;
}

void preproc_register_warning_callback(struct preproc_info *pp, void (*cb)(const char *))
{
	pp -> warningcb = cb;
}

static void preproc_throw_error_default(const char *m)
{
	fprintf(stderr, "ERROR: %s\n", m);
}

static void preproc_throw_warning_default(const char *m)
{
	fprintf(stderr, "WARNING: %s\n", m);
}

static void preproc_throw_message(struct preproc_info *pp, void (*cb)(const char *), const char *m, va_list args)
{
	int s, s2;
	char *b;
	
	s2 = snprintf(NULL, 0, "(%s:%d:%d) ", pp -> fn, pp -> lineno, pp -> column);
	s = vsnprintf(NULL, 0, m, args);
	b = lw_alloc(s + s2 + 1);
	snprintf(b, s2 + 1, "(%s:%d:%d) ", pp -> fn, pp -> lineno, pp -> column);
	vsnprintf(b + s2, s + 1, m, args);
	(*cb)(b);
	lw_free(b);
}

void preproc_throw_error(struct preproc_info *pp, const char *m, ...)
{
	va_list args;
	va_start(args, m);
	preproc_throw_message(pp, pp -> errorcb ? pp -> errorcb : preproc_throw_error_default, m, args);
	va_end(args);
	exit(1);
}

void preproc_throw_warning(struct preproc_info *pp, const char *m, ...)
{
	va_list args;
	va_start(args, m);
	preproc_throw_message(pp, pp -> warningcb ? pp -> warningcb : preproc_throw_warning_default, m, args);
	va_end(args);
}
