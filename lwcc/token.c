/*
lwcc/token.c

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

#include <lw_alloc.h>
#include <lw_string.h>

#include "token.h"

struct token *token_create(int ttype, char *strval, int row, int col, const char *fn)
{
	struct token *t;
	
	t = lw_alloc(sizeof(struct token));
	t -> ttype = ttype;
	if (strval)
		t -> strval = lw_strdup(strval);
	else
		t -> strval = NULL;
	t -> lineno = row;
	t -> column = col;
	t -> fn = fn;
	t -> next = NULL;
	t -> prev = NULL;
	t -> list = NULL;
	return t;
}

void token_free(struct token *t)
{
	lw_free(t -> strval);
	lw_free(t);
}

struct token *token_dup(struct token *t)
{
	struct token *t2;
	
	t2 = lw_alloc(sizeof(struct token));
	t2 -> ttype = t -> ttype;
	t2 -> lineno = t -> lineno;
	t2 -> column = t -> column;
	t2 -> list = NULL;
	t2 -> next = NULL;
	t2 -> prev = NULL;
	if (t -> strval)
		t2 -> strval = lw_strdup(t -> strval);
	else
		t2 -> strval = NULL;
	return t2;
}

static struct { int ttype; char *tstr; } tok_strs[] =
{
	{ TOK_WSPACE, " " },
	{ TOK_EOL, "\n" },
	{ TOK_DIV, "/" },
	{ TOK_ADD, "+" },
	{ TOK_SUB, "-" },
	{ TOK_OPAREN, "(" },
	{ TOK_CPAREN, ")" },
	{ TOK_NE, "!=" },
	{ TOK_EQ, "==" },
	{ TOK_LE, "<=" },
	{ TOK_LT, "<" },
	{ TOK_GE, ">=" },
	{ TOK_GT, ">" },
	{ TOK_BAND, "&&" },
	{ TOK_BOR, "||" },
	{ TOK_BNOT, "!" },
	{ TOK_MOD, "%"},
	{ TOK_COMMA, "," },
	{ TOK_ELLIPSIS, "..." },
	{ TOK_QMARK, "?" },
	{ TOK_COLON, ":" },
	{ TOK_OBRACE, "{" },
	{ TOK_CBRACE, "}" },
	{ TOK_OSQUARE, "[" },
	{ TOK_CSQUARE, "]" },
	{ TOK_COM, "~" },
	{ TOK_EOS, ";" },
	{ TOK_HASH, "#" },
	{ TOK_DBLHASH, "##" },
	{ TOK_XOR, "^" },
	{ TOK_XORASS, "^=" },
	{ TOK_STAR, "*" },
	{ TOK_MULASS, "*=" },
	{ TOK_DIVASS, "/=" },
	{ TOK_ASS, "=" },
	{ TOK_MODASS, "%=" },
	{ TOK_SUBASS, "-=" },
	{ TOK_DBLSUB, "--" },
	{ TOK_ADDASS, "+=" },
	{ TOK_DBLADD, "++" },
	{ TOK_BWAND, "&" },
	{ TOK_BWANDASS, "&=" },
	{ TOK_BWOR, "|" },
	{ TOK_BWORASS, "|=" },
	{ TOK_LSH, "<<" },
	{ TOK_LSHASS, "<<=" },
	{ TOK_RSH, ">>" },
	{ TOK_RSHASS, ">>=" },
	{ TOK_DOT, "." },
	{ TOK_ARROW, "->" },
	{ TOK_NONE, "" }
};

void token_print(struct token *t, FILE *f)
{
	int i;
	for (i = 0; tok_strs[i].ttype != TOK_NONE; i++)
	{
		if (tok_strs[i].ttype == t -> ttype)
		{
			fprintf(f, "%s", tok_strs[i].tstr);
			break;
		}
	}
	if (t -> strval)
		fprintf(f, "%s", t -> strval);
}

/* token list management */
struct token_list *token_list_create(void)
{
	struct token_list *tl;
	tl = lw_alloc(sizeof(struct token_list));
	tl -> head = NULL;
	tl -> tail = NULL;
	return tl;
}

void token_list_destroy(struct token_list *tl)
{
	if (tl == NULL)
		return;
	while (tl -> head)
	{
		tl -> tail = tl -> head;
		tl -> head = tl -> head -> next;
		token_free(tl -> tail);
	}
	lw_free(tl);
}

void token_list_append(struct token_list *tl, struct token *tok)
{
	tok -> list = tl;
	if (tl -> head == NULL)
	{
		tl -> head = tl -> tail = tok;
		tok -> next = tok -> prev = NULL;
		return;
	}
	tl -> tail -> next = tok;
	tok -> prev = tl -> tail;
	tl -> tail = tok;
	tok -> next = NULL;
	return;
}

void token_list_remove(struct token *tok)
{
	if (tok -> list == NULL)
		return;

	if (tok -> prev)
		tok -> prev -> next = tok -> next;
	if (tok -> next)
		tok -> next -> prev = tok -> prev;
	if (tok == tok -> list -> head)
		tok -> list -> head = tok -> next;
	if (tok == tok -> list -> tail)
		tok -> list -> tail = tok -> prev;
	tok -> list = NULL;
}

void token_list_prepend(struct token_list *tl, struct token *tok)
{
	tok -> list = tl;
	if (tl -> head == NULL)
	{
		tl -> head = tl -> tail = tok;
		tok -> next = tok -> prev = NULL;
	}
	tl -> head -> prev = tok;
	tok -> next = tl -> head;
	tl -> head = tok;
	tok -> prev = NULL;
}

void token_list_insert(struct token_list *tl, struct token *after, struct token *newt)
{
	struct token *t;
	
	if (after == NULL || tl -> head == NULL)
	{
		token_list_prepend(tl, newt);
		return;
	}
	
	for (t = tl -> head; t && t != after; t = t -> next)
		/* do nothing */ ;
	if (!t)
	{
		token_list_append(tl, newt);
		return;
	}
	newt -> prev = t;
	newt -> next = t -> next;
	if (t -> next)
		t -> next -> prev = newt;
	else
		tl -> tail = newt;
	t -> next = newt;
}

struct token_list *token_list_dup(struct token_list *tl)
{
	struct token_list *nl;
	struct token *t;
	
	nl = token_list_create();
	for (t = tl -> head; t; t = t -> next)
	{
		token_list_append(nl, token_dup(t));
	}
	return nl;
}
