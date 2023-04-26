/*
lwcc/preproc.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_strbuf.h>
#include <lw_strpool.h>

#include "cpp.h"
#include "symbol.h"
#include "token.h"

static int expand_macro(struct preproc_info *, char *);
static void process_directive(struct preproc_info *);
static long eval_expr(struct preproc_info *);
extern struct token *preproc_lex_next_token(struct preproc_info *);
static long preproc_numval(struct preproc_info *, struct token *);
static int eval_escape(char **);
extern int preproc_lex_fetch_byte(struct preproc_info *);
extern void preproc_lex_unfetch_byte(struct preproc_info *, int);


struct token *preproc_next_processed_token(struct preproc_info *pp)
{
	struct token *ct;

again:
	ct = preproc_next_token(pp);
	if (ct -> ttype == TOK_EOF)
		return ct;
	if (ct -> ttype == TOK_EOL)
	{
		pp -> ppeolseen = 1;
		return ct;
	}

	if (ct -> ttype == TOK_HASH && pp -> ppeolseen == 1)
	{
		// preprocessor directive 
		process_directive(pp);
		goto again;
	}
	// if we're in a false section, don't return the token; keep scanning
	if (pp -> skip_level)
		goto again;

	if (ct -> ttype != TOK_WSPACE)
		pp -> ppeolseen = 0;
	
	if (ct -> ttype == TOK_IDENT)
	{
		// possible macro expansion
		if (expand_macro(pp, ct -> strval))
	 		goto again;
	}
	
	return ct;
}

static struct token *preproc_next_processed_token_nws(struct preproc_info *pp)
{
	struct token *t;
	
	do
	{
		t = preproc_next_processed_token(pp);
	} while (t -> ttype == TOK_WSPACE);
	return t;
}

static struct token *preproc_next_token_nws(struct preproc_info *pp)
{
	struct token *t;
	
	do
	{
		t = preproc_next_token(pp);
	} while (t -> ttype == TOK_WSPACE);
	return t;
}

static void skip_eol(struct preproc_info *pp)
{
	struct token *t;
	
	if (pp -> curtok && pp -> curtok -> ttype == TOK_EOL)
		return;
	do
	{
		t = preproc_next_token(pp);
	} while (t -> ttype != TOK_EOL);
}

static void check_eol(struct preproc_info *pp)
{
	struct token *t;

	t = preproc_next_token_nws(pp);
	if (t -> ttype != TOK_EOL)
		preproc_throw_warning(pp, "Extra text after preprocessor directive");
	skip_eol(pp);
}

static void dir_ifdef(struct preproc_info *pp)
{
	struct token *ct;
	
	if (pp -> skip_level)
	{
		pp -> skip_level++;
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #ifdef");
		skip_eol(pp);
	}
	
	if (symtab_find(pp, ct -> strval) == NULL)
	{
		pp -> skip_level++;
	}
	else
	{
		pp -> found_level++;
	}
	check_eol(pp);
}

static void dir_ifndef(struct preproc_info *pp)
{
	struct token *ct;
	
	if (pp -> skip_level)
	{
		pp -> skip_level++;
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #ifdef");
		skip_eol(pp);
	}
	
	if (symtab_find(pp, ct -> strval) != NULL)
	{
		pp -> skip_level++;
	}
	else
	{
		pp -> found_level++;
	}
	check_eol(pp);
}

static void dir_if(struct preproc_info *pp)
{
	if (pp -> skip_level || !eval_expr(pp))
		pp -> skip_level++;
	else
		pp -> found_level++;
}

static void dir_elif(struct preproc_info *pp)
{
	if (pp -> skip_level == 0)
		pp -> else_skip_level = pp -> found_level;
	if (pp -> skip_level)
	{
		if (pp -> else_skip_level > pp -> found_level)
			;
		else if (--(pp -> skip_level) != 0)
			pp -> skip_level++;
		else if (eval_expr(pp))
			pp -> found_level++;
		else
			pp -> skip_level++;
	}
	else if (pp -> found_level)
	{
		pp -> skip_level++;
		pp -> found_level--;
	}
	else
		preproc_throw_error(pp, "#elif in non-conditional section");
}

static void dir_else(struct preproc_info *pp)
{
	if (pp -> skip_level)
	{
		if (pp -> else_skip_level > pp -> found_level)
			;
		else if (--(pp -> skip_level) != 0)
			pp -> skip_level++;
		else
			pp -> found_level++;
	}
	else if (pp -> found_level)
	{
		pp -> skip_level++;
		pp -> found_level--;
	}
	else
	{
		preproc_throw_error(pp, "#else in non-conditional section");
	}
	if (pp -> else_level == pp -> found_level + pp -> skip_level)
	{
		preproc_throw_error(pp, "Too many #else");
	}
	pp -> else_level = pp -> found_level + pp -> skip_level;
	check_eol(pp);
}

static void dir_endif(struct preproc_info *pp)
{
	if (pp -> skip_level)
		pp -> skip_level--;
	else if (pp -> found_level)
		pp -> found_level--;
	else
		preproc_throw_error(pp, "#endif in non-conditional section");
	if (pp -> skip_level == 0)
		pp -> else_skip_level = 0;
	pp -> else_level = 0;
	check_eol(pp);
}

static void dir_define(struct preproc_info *pp)
{
	struct token_list *tl = NULL;
	struct token *ct;
	int nargs = -1;
	int vargs = 0;
	char *mname = NULL;

	char **arglist = NULL;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}

	ct = preproc_next_token_nws(pp);
	if (ct -> ttype != TOK_IDENT)
		goto baddefine;
	
	mname = lw_strdup(ct -> strval);
	ct = preproc_next_token(pp);
	if (ct -> ttype == TOK_WSPACE)
	{
		/* object like macro */
	}
	else if (ct -> ttype == TOK_EOL)
	{
		/* object like macro - empty value */
		goto out;
	}
	else if (ct -> ttype == TOK_OPAREN)
	{
		/* function like macro - parse args */
		nargs = 0;
		vargs = 0;
		for (;;)
		{
			ct = preproc_next_token_nws(pp);
			if (ct -> ttype == TOK_EOL)
			{
				goto baddefine;
			}
			if (ct -> ttype == TOK_CPAREN)
				break;
			
			if (ct -> ttype == TOK_IDENT)
			{
				/* parameter name */
				nargs++;
				/* record argument name */
				arglist = lw_realloc(arglist, sizeof(char *) * nargs);
				arglist[nargs - 1] = lw_strdup(ct -> strval);
				
				/* check for end of args or comma */
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype == TOK_CPAREN)
					break;
				else if (ct -> ttype == TOK_COMMA)
					continue;
				else
					goto baddefine;
			}
			else if (ct -> ttype == TOK_ELLIPSIS)
			{
				/* variadic macro */
				vargs = 1;
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_CPAREN)
					goto baddefine;
				break;
			}
			else
				goto baddefine;
		}
	}
	else
	{
baddefine:
		preproc_throw_error(pp, "bad #define", ct -> ttype);
baddefine2:
		token_list_destroy(tl);
		skip_eol(pp);
		lw_free(mname);
		while (nargs > 0)
			lw_free(arglist[--nargs]);
		lw_free(arglist);
		return;
	}

	tl = token_list_create();	
	for (;;)
	{
		ct = preproc_next_token(pp);
	
		if (ct -> ttype == TOK_EOL)
			break;
		token_list_append(tl, token_dup(ct));
	}
out:
	if (strcmp(mname, "defined") == 0)
	{
		preproc_throw_warning(pp, "attempt to define 'defined' as a macro not allowed");
		goto baddefine2;
	}
	else if (symtab_find(pp, mname) != NULL)
	{
		/* need to do a token compare between the old value and the new value
		   to decide whether to complain */
		preproc_throw_warning(pp, "%s previous defined", mname);
		symtab_undef(pp, mname);
	}
	symtab_define(pp, mname, tl, nargs, arglist, vargs);
	lw_free(mname);
	while (nargs > 0)
		lw_free(arglist[--nargs]);
	lw_free(arglist);
	/* no need to check for EOL here */
}

void preproc_add_macro(struct preproc_info *pp, char *str)
{
	char *s;
	
	pp -> lexstr = lw_strdup(str);
	pp -> lexstrloc = 0;
	s = strchr(pp -> lexstr, '=');
	if (s)
		*s = ' ';
		
	dir_define(pp);
	
	lw_free(pp -> lexstr);
	pp -> lexstr = NULL;
	pp -> lexstrloc = 0;
}

static void dir_undef(struct preproc_info *pp)
{
	struct token *ct;
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #undef");
		skip_eol(pp);
	}
	
	symtab_undef(pp, ct -> strval);
	check_eol(pp);
}

char *streol(struct preproc_info *pp)
{
	struct lw_strbuf *s;
	struct token *ct;
	int i;
		
	s = lw_strbuf_new();
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	while (ct -> ttype != TOK_EOL)
	{
		for (i = 0; ct -> strval[i]; i++)
			lw_strbuf_add(s, ct -> strval[i]);
		ct = preproc_next_token(pp);
	}
	return lw_strbuf_end(s);
}

static void dir_error(struct preproc_info *pp)
{
	char *s;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	s = streol(pp);
	preproc_throw_error(pp, "%s", s);
	lw_free(s);
}

static void dir_warning(struct preproc_info *pp)
{
	char *s;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	s = streol(pp);
	preproc_throw_warning(pp, "%s", s);
	lw_free(s);
}

static char *preproc_file_exists_in_dir(char *dir, char *fn)
{
	int l;
	char *f;
	
	l = snprintf(NULL, 0, "%s/%s", dir, fn);
	f = lw_alloc(l + 1);
	snprintf(f, l + 1, "%s/%s", dir, fn);
	
	if (access(f, R_OK) == 0)
		return f;
	lw_free(f);
	return NULL;
}

static char *preproc_find_file(struct preproc_info *pp, char *fn, int sys)
{
	char *tstr;
	char *pref;
	char *rfn;

	/* pass through absolute paths, dumb as they are */	
	if (fn[0] == '/')
		return lw_strdup(fn);

	if (!sys)
	{
		/* look in the directory with the current file */
		tstr = strchr(pp -> fn, '/');
		if (!tstr)
			pref = lw_strdup(".");
		else
		{
			pref = lw_alloc(tstr - pp -> fn + 1);
			memcpy(pref, pp -> fn, tstr - pp -> fn);
			pref[tstr - pp -> fn] = 0;
		}
		rfn = preproc_file_exists_in_dir(pref, fn);
		lw_free(pref);
		if (rfn)
			return rfn;
		
		/* look in the "quote" dir list */
		lw_stringlist_reset(pp -> quotelist);
		for (pref = lw_stringlist_current(pp -> quotelist); pref; pref = lw_stringlist_next(pp -> quotelist))
		{
			rfn = preproc_file_exists_in_dir(pref, fn);
			if (rfn)
				return rfn;
		}
	}
	
	/* look in the "include" dir list */
	lw_stringlist_reset(pp -> inclist);
	for (pref = lw_stringlist_current(pp -> inclist); pref; pref = lw_stringlist_next(pp -> inclist))
	{
		rfn = preproc_file_exists_in_dir(pref, fn);
		if (rfn)
			return rfn;
	}

	/* the default search list is provided by the driver program */	
	return NULL;
}

static void dir_include(struct preproc_info *pp)
{
	FILE *fp;
	struct token *ct;
	int sys = 0;
	char *fn;
	struct lw_strbuf *strbuf;
	int i;
	struct preproc_info *fs;
	
	ct = preproc_next_token_nws(pp);
	if (ct -> ttype == TOK_STR_LIT)
	{
usrinc:
		sys = strlen(ct -> strval);
		fn = lw_alloc(sys - 1);
		memcpy(fn, ct -> strval + 1, sys - 2);
		fn[sys - 2] = 0;
		sys = 0;
		goto doinc;
	}
	else if (ct -> ttype == TOK_LT)
	{
		strbuf = lw_strbuf_new();
		for (;;)
		{
			int c;
			c = preproc_lex_fetch_byte(pp);
			if (c == CPP_EOL)
			{
				preproc_lex_unfetch_byte(pp, c);
				preproc_throw_error(pp, "Bad #include");
				lw_free(lw_strbuf_end(strbuf));
				break;
			}
			if (c == '>')
				break;
			lw_strbuf_add(strbuf, c);
		}
		ct = preproc_next_token_nws(pp);
		if (ct -> ttype != TOK_EOL)
		{
			preproc_throw_error(pp, "Bad #include");
			skip_eol(pp);
			lw_free(lw_strbuf_end(strbuf));
			return;
		}
		sys = 1;
		fn = lw_strbuf_end(strbuf);
		goto doinc;
	}
	else
	{
		preproc_unget_token(pp, ct);
		// computed include
		ct = preproc_next_processed_token_nws(pp);
		if (ct -> ttype == TOK_STR_LIT)
			goto usrinc;
		else if (ct -> ttype == TOK_LT)
		{
			strbuf = lw_strbuf_new();
			for (;;)
			{
				ct = preproc_next_processed_token(pp);
				if (ct -> ttype == TOK_GT)
					break;
				if (ct -> ttype == TOK_EOL)
				{
					preproc_throw_error(pp, "Bad #include");
					lw_free(lw_strbuf_end(strbuf));
					return;
				}
				for (i = 0; ct -> strval[i]; ct++)
				{
					lw_strbuf_add(strbuf, ct -> strval[i]);
				}
			}
			ct = preproc_next_processed_token_nws(pp);
			if (ct -> ttype != TOK_EOL)
			{
				preproc_throw_error(pp, "Bad #include");
				skip_eol(pp);
				lw_free(lw_strbuf_end(strbuf));
				return;
			}
			sys = 1;
			fn = lw_strbuf_end(strbuf);
			goto doinc;
		}
		else
		{
			skip_eol(pp);
			preproc_throw_error(pp, "Bad #include");
			return;
		}
	}
doinc:
	fn = preproc_find_file(pp, fn, sys);
	if (!fn)
		goto badfile;
	fp = fopen(fn, "rb");
	if (!fp)
	{
		lw_free(fn);
badfile:
		preproc_throw_error(pp, "Cannot open #include file %s - this is fatal", fn);
		exit(1);
	}
	
	/* save the current include file state, etc. */
	fs = lw_alloc(sizeof(struct preproc_info));
	*fs = *pp;
	fs -> n = pp -> filestack;
	pp -> curtok = NULL;
	pp -> filestack = fs;
	pp -> fn = lw_strpool_strdup(pp -> strpool, fn);
	lw_free(fn);
	pp -> fp = fp;
	pp -> ra = CPP_NOUNG;
	pp -> ppeolseen = 1;
	pp -> eolstate = 0;
	pp -> lineno = 1;
	pp -> column = 0;
	pp -> qseen = 0;
	pp -> ungetbufl = 0;
	pp -> ungetbufs = 0;
	pp -> ungetbuf = NULL;
	pp -> unget = 0;
	pp -> eolseen = 0;
	pp -> nlseen = 0;
	pp -> skip_level = 0;
	pp -> found_level = 0;
	pp -> else_level = 0;
	pp -> else_skip_level = 0;
	pp -> tokqueue = NULL;	
	// now get on with processing
}

static void dir_line(struct preproc_info *pp)
{
	struct token *ct;
	long lineno;
	char *estr;
	
	lineno = -1;
	
	ct = preproc_next_processed_token_nws(pp);
	if (ct -> ttype == TOK_NUMBER)
	{
		lineno = strtoul(ct -> strval, &estr, 10);
		if (*estr)
		{
			preproc_throw_error(pp, "Bad #line");
			skip_eol(pp);
			return;
		}
	}
	else
	{
		preproc_throw_error(pp, "Bad #line");
		skip_eol(pp);
		return;
	}
	ct = preproc_next_processed_token_nws(pp);
	if (ct -> ttype == TOK_EOL)
	{
		pp -> lineno = lineno;
		return;
	}
	if (ct -> ttype != TOK_STR_LIT)
	{
		preproc_throw_error(pp, "Bad #line");
		skip_eol(pp);
		return;
	}
	estr = lw_strdup(ct -> strval);
	ct = preproc_next_processed_token_nws(pp);
	if (ct -> ttype != TOK_EOL)
	{
		preproc_throw_error(pp, "Bad #line");
		skip_eol(pp);
		lw_free(estr);
		return;
	}
	pp -> fn = estr;
	pp -> lineno = lineno;
}

static void dir_pragma(struct preproc_info *pp)
{
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	preproc_throw_warning(pp, "Unsupported #pragma");
	skip_eol(pp);
}

struct { char *name; void (*fn)(struct preproc_info *); } dirlist[] =
{
	{ "ifdef", dir_ifdef },
	{ "ifndef", dir_ifndef },
	{ "if", dir_if },
	{ "else", dir_else },
	{ "elif", dir_elif },
	{ "endif", dir_endif },
	{ "define", dir_define },
	{ "undef", dir_undef },
	{ "include", dir_include },
	{ "error", dir_error },
	{ "warning", dir_warning },
	{ "line", dir_line },
	{ "pragma", dir_pragma },
	{ NULL, NULL }
};

static void process_directive(struct preproc_info *pp)
{
	struct token *ct;
	int i;
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	// NULL directive
	if (ct -> ttype == TOK_EOL)
		return;
	
	if (ct -> ttype == TOK_NUMBER)
	{
		// this is probably a file marker from a previous run of the preprocessor
		char *fn;
		struct lw_strbuf *sb;
		
		i = preproc_numval(pp, ct);
		ct  = preproc_next_token_nws(pp);
		if (ct -> ttype != TOK_STR_LIT)
			goto baddir;
		pp -> lineno = i;
		sb = lw_strbuf_new();
		for (fn = ct -> strval; *fn && *fn != '"'; )
		{
			if (*fn == '\\')
			{
				lw_strbuf_add(sb, eval_escape(&fn));
			}
			else
			{
				lw_strbuf_add(sb, *fn++);
			}
		}
		fn = lw_strbuf_end(sb);
		pp -> fn = lw_strpool_strdup(pp -> strpool, fn);
		lw_free(fn);
		skip_eol(pp);
		return;
	}
	
	if (ct -> ttype != TOK_IDENT)
		goto baddir;
	
	for (i = 0; dirlist[i].name; i++)
	{
		if (strcmp(dirlist[i].name, ct -> strval) == 0)
		{
			(*(dirlist[i].fn))(pp);
			return;
		}
	}
baddir:
		preproc_throw_error(pp, "Bad preprocessor directive");
		while (ct -> ttype != TOK_EOL)
			ct = preproc_next_token(pp);
		return;
}

/*
Evaluate a preprocessor expression
*/

/* same as skip_eol() but the EOL token is not consumed */
static void skip_eoe(struct preproc_info *pp)
{
	skip_eol(pp);
	preproc_unget_token(pp, pp -> curtok);
}

static long eval_expr_real(struct preproc_info *, int);

static long eval_term_real(struct preproc_info *pp)
{
	long tval = 0;
	struct token *ct;
	
eval_next:
	ct = preproc_next_processed_token_nws(pp);
	if (ct -> ttype == TOK_EOL)
	{
		preproc_throw_error(pp, "Bad expression");
		return 0;
	}
	
	switch (ct -> ttype)
	{
	case TOK_OPAREN:
		tval = eval_expr_real(pp, 0);
		ct = preproc_next_processed_token_nws(pp);
		if (ct -> ttype != ')')
		{
			preproc_throw_error(pp, "Unbalanced () in expression");
			skip_eoe(pp);
			return 0;
		}
		return tval;

	case TOK_ADD: // unary +
		goto eval_next;

	case TOK_SUB: // unary -	
		tval = eval_expr_real(pp, 200);
		return -tval;

	/* NOTE: we should only get "TOK_IDENT" from an undefined macro */		
	case TOK_IDENT: // some sort of function, symbol, etc.
		if (strcmp(ct -> strval, "defined"))
		{
			/* the defined operator */
			/* any number in the "defined" bit will be
			   treated as a defined symbol, even zero */
			ct = preproc_next_token_nws(pp);
			if (ct -> ttype == TOK_OPAREN)
			{
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_IDENT)
				{
					preproc_throw_error(pp, "Bad expression");
					skip_eoe(pp);
					return 0;
				}
				if (symtab_find(pp, ct -> strval) == NULL)
					tval = 0;
				else
					tval = 1;
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_CPAREN)
				{
					preproc_throw_error(pp, "Bad expression");
					skip_eoe(pp);
					return 0;
				}
				return tval;
			}
			else if (ct -> ttype == TOK_IDENT)
			{
				return (symtab_find(pp, ct -> strval) != NULL) ? 1 : 0;
			}
			preproc_throw_error(pp, "Bad expression");
			skip_eoe(pp);
			return 0;
		}
		/* unknown identifier - it's zero */
		return 0;
	
	/* numbers */
	case TOK_NUMBER:
		return preproc_numval(pp, ct);	
		
	default:
		preproc_throw_error(pp, "Bad expression");
		skip_eoe(pp);
		return 0;
	}
	return 0;
}

static long eval_expr_real(struct preproc_info *pp, int p)
{
	static const struct operinfo
	{
		int tok;
		int prec;
	} operators[] =
	{
		{ TOK_ADD, 100 },
		{ TOK_SUB, 100 },
		{ TOK_STAR, 150 },
		{ TOK_DIV, 150 },
		{ TOK_MOD, 150 },
		{ TOK_LT, 75 },
		{ TOK_LE, 75 },
		{ TOK_GT, 75 },
		{ TOK_GE, 75 },
		{ TOK_EQ, 70 },
		{ TOK_NE, 70 },
		{ TOK_BAND, 30 },
		{ TOK_BOR, 25 },
		{ TOK_NONE, 0 }
	};
	
	int op;
	long term1, term2, term3;
	struct token *ct;
	
	term1 = eval_term_real(pp);
eval_next:
	ct = preproc_next_processed_token_nws(pp);
	for (op = 0; operators[op].tok != TOK_NONE; op++)
	{
		if (operators[op].tok == ct -> ttype)
			break;
	}
	/* if it isn't a recognized operator, assume end of expression */
	if (operators[op].tok == TOK_NONE)
	{
		preproc_unget_token(pp, ct);
		return term1;
	}

	/* if new operation is not higher than the current precedence, let the previous op finish */
	if (operators[op].prec <= p)
		return term1;

	/* get the second term */
	term2 = eval_expr_real(pp, operators[op].prec);
	
	switch (operators[op].tok)
	{
	case TOK_ADD:
		term3 = term1 + term2;
		break;
	
	case TOK_SUB:
		term3 = term1 - term2;
		break;
	
	case TOK_STAR:
		term3 = term1 * term2;
		break;
	
	case TOK_DIV:
		if (!term2)
		{
			preproc_throw_warning(pp, "Division by zero");
			term3 = 0;
			break;
		}
		term3 = term1 / term2;
		break;
	
	case TOK_MOD:
		if (!term2)
		{
			preproc_throw_warning(pp, "Division by zero");
			term3 = 0;
			break;
		}
		term3 = term1 % term2;
		break;
		
	case TOK_BAND:
		term3 = (term1 && term2);
		break;
	
	case TOK_BOR:
		term3 = (term1 || term2);
		break;
	
	case TOK_EQ:
		term3 = (term1 == term2);
		break;
	
	case TOK_NE:
		term3 = (term1 != term2);
		break;
	
	case TOK_GT:
		term3 = (term1 > term2);
		break;
	
	case TOK_GE:
		term3 = (term1 >= term2);
		break;
	
	case TOK_LT:
		term3 = (term1 < term2);
		break;
	
	case TOK_LE:
		term3 = (term1 <= term2);
		break;

	default:
		term3 = 0;
		break;
	}
	term1 = term3;
	goto eval_next;
}

static long eval_expr(struct preproc_info *pp)
{
	long rv;
	struct token *t;
	
	rv = eval_expr_real(pp, 0);
	t = preproc_next_token_nws(pp);
	if (t -> ttype != TOK_EOL)
	{
		preproc_throw_error(pp, "Bad expression");
		skip_eol(pp);
	}
	return rv;
}

static int eval_escape(char **t)
{
	int c;
	int c2;
	
	if (**t == 0)
		return 0;
	c = *(*t)++;
	int rv = 0;
	
	switch (c)
	{
	case 'n':
		return 10;
	case 'r':
		return 13;
	case 'b':
		return 8;
	case 'e':
		return 27;
	case 'f':
		return 12;
	case 't':
		return 9;
	case 'v':
		return 11;
	case 'a':
		return 7;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7':
		// octal constant
		rv = c - '0';
		c2 = 1;
		for (; c2 < 3; c2++)
		{
			c = *(*t)++;
			if (c < '0' || c > '7')
				break;
			rv = (rv << 3) | (c - '0');
		}
		return rv;
	case 'x':
		// hex constant
		for (;;)
		{
			c = *(*t)++;
			if (c < '0' || (c > '9' && c < 'A') || (c > 'F' && c < 'a') || c > 'f')
				break;
			c = c - '0';
			if (c > 9)
				c -= 7;
			if (c > 15)
				c -= 32;
			rv = (rv << 4) | c;
		}
		return rv & 0xff;
	default:
		return c;
	}
}

/* convert a numeric string to a number */
long preproc_numval(struct preproc_info *pp, struct token *t)
{
	unsigned long long rv = 0;
	unsigned long long rv2 = 0;
	char *tstr = t -> strval;
	int radix = 10;
	int c;
	int ovf = 0;
	union { long sv; unsigned long uv; } tv;
		
	if (t -> ttype == TOK_CHR_LIT)
	{
		tstr++;
		while (*tstr && *tstr != '\'')
		{
			if (*tstr == '\\')
			{
				tstr++;
				c = eval_escape(&tstr);
			}
			else
				c = *tstr++;
			rv = (rv << 8) | c;
			if (rv / radix < rv2)
				ovf = 1;
			rv2 = rv;
			
		}
		goto done;
	}
	
	
	if (*tstr == '0')
	{
		radix = 8;
		tstr++;
		if (*tstr == 'x')
		{
			radix = 16;
			tstr++;
		}
	}
	while (*tstr)
	{
		c = *tstr++;
		if (c < '0' || (c > '9' && c < 'A') || (c > 'F' && c < 'a') || c > 'f')
			break;
		c -= '0';
		if (c > 9)
			c -= 7;
		if (c > 15)
			c -= 32;
		if (c >= radix)
			break;
		rv = rv * radix + c;
		if (rv / radix < rv2)
			ovf = 1;
		rv2 = rv;
	}
	tstr--;
	while (*tstr == 'l' || *tstr == 'L')
		tstr++;
	tv.uv = rv;
	if (tv.sv < 0 && radix == 10)
		ovf = 1;
done:
	if (ovf)
		preproc_throw_error(pp, "Constant out of range: %s", t -> strval);
	return rv;
}

/*
Below here is the logic for expanding a macro
*/
static char *stringify(struct token_list *tli)
{
	struct lw_strbuf *s;
	int ws = 0;
	struct token *tl = tli -> head;
	
	s = lw_strbuf_new();
	lw_strbuf_add(s, '"');

	while (tl && tl -> ttype == TOK_WSPACE)
		tl = tl -> next;
	
	for (; tl; tl = tl -> next)
	{
		if (tl -> ttype == TOK_WSPACE)
		{
			ws = 1;
			continue;
		}
		if (ws)
		{
			lw_strbuf_add(s, ' ');
		}
		for (ws = 0; tl -> strval[ws]; ws++)
		{
			if (tl -> ttype == TOK_STR_LIT || tl -> ttype == TOK_CHR_LIT)
			{
				if (tl -> strval[ws] == '"' || tl -> strval[ws] == '\\')
					lw_strbuf_add(s, '\\');
			}
		}
		ws = 0;
	}
	
	lw_strbuf_add(s, '"');
	return lw_strbuf_end(s);
}

static int macro_arg(struct symtab_e *s, char *str)
{
	int i;
	if (strcmp(str, "__VA_ARGS__") == 0)
		i = s -> nargs;
	else
		for (i = 0; i < s -> nargs; i++)
			if (strcmp(s -> params[i], str) == 0)
				break;
	if (i == s -> nargs)
		if (s -> vargs == 0)
			return -1;
	return i;
}

/* return list to tokens as a result of ## expansion */
static struct token_list *paste_tokens(struct preproc_info *pp, struct symtab_e *s, struct token_list **arglist, struct token *t1, struct token *t2)
{
	struct token_list *left;
	struct token_list *right;
	char *tstr;
	struct token *ttok;
	int i;
	
	if (t1 -> ttype == TOK_IDENT)
	{
		i = macro_arg(s, t1 -> strval);
		if (i == -1)
		{
			left = token_list_create();
			token_list_append(left, token_dup(t1));
		}
		else
		{
			left = token_list_dup(arglist[i]);
		}
	}
	else
	{
		left = token_list_create();
		token_list_append(left, token_dup(t1));
	}
	// munch trailing white space
	while (left -> tail && left -> tail -> ttype == TOK_WSPACE)
	{
		token_list_remove(left -> tail);
	}

	if (t2 -> ttype == TOK_IDENT)
	{
		i = macro_arg(s, t2 -> strval);
		if (i == -1)
		{
			right = token_list_create();
			token_list_append(right, token_dup(t2));
		}
		else
		{
			right = token_list_dup(arglist[i]);
		}
	}
	else
	{
		right = token_list_create();
		token_list_append(right, token_dup(t2));
	}
	// munch leading white space
	while (right -> head && right -> head -> ttype == TOK_WSPACE)
	{
		token_list_remove(right -> head);
	}

	// nothing to append at all
	if (left -> head != NULL && right -> head == NULL)
	{
		// right arg is empty - use left
		token_list_destroy(right);
		return left;
	}
	if (left -> head == NULL && right -> head != NULL)
	{
		// left arg is empty, use right
		token_list_destroy(left);
		return right;
	}
	if (left -> head == NULL && right -> head == NULL)
	{
		// both empty, use left
		token_list_destroy(right);
		return left;
	}
	
	// both non-empty - past left tail with right head
	// then past the right list onto the left
	tstr = lw_alloc(strlen(left -> tail -> strval) + strlen(right -> head -> strval) + 1);
	strcpy(tstr, left -> tail -> strval);
	strcat(tstr, right -> head -> strval);
	
	pp -> lexstr = tstr;
	pp -> lexstrloc = 0;
	
	ttok = preproc_lex_next_token(pp);
	if (ttok -> ttype != TOK_ERROR && pp -> lexstr[pp -> lexstrloc] == 0)
	{
		// we have a new token here
		token_list_remove(left -> tail);
		token_list_remove(right -> head);
		token_list_append(left, token_dup(ttok));
	}
	lw_free(tstr);
	pp -> lexstr = NULL;
	pp -> lexstrloc = 0;
	for (ttok = right -> head; ttok; ttok = ttok -> next)
	{
		token_list_append(left, token_dup(ttok));
	}
	token_list_destroy(right);
	return left;
}

static int expand_macro(struct preproc_info *pp, char *mname)
{
	struct symtab_e *s;
	struct token *t, *t2, *t3;
	int nargs = 0;
	struct expand_e *e;
	struct token_list **exparglist = NULL;
	struct token_list **arglist = NULL;
	int i;
	int pcount;
	char *tstr;
	struct token_list *expand_list;
	int repl;
	struct token_list *rtl;
	
	// check for built in macros
	if (strcmp(mname, "__FILE__") == 0)
	{
		struct lw_strbuf *sb;
		
		sb = lw_strbuf_new();
		lw_strbuf_add(sb, '"');
		for (tstr = (char *)(pp -> fn); *tstr; tstr++)
		{
			if (*tstr == 32 || (*tstr > 34 && *tstr < 127))
			{
				lw_strbuf_add(sb, *tstr);
			}
			else
			{
				lw_strbuf_add(sb, '\\');
				lw_strbuf_add(sb, (*tstr >> 6) + '0');
				lw_strbuf_add(sb, ((*tstr >> 3) & 7) + '0');
				lw_strbuf_add(sb, (*tstr & 7) + '0');
			}
		}
		lw_strbuf_add(sb, '"');
		tstr = lw_strbuf_end(sb);
		preproc_unget_token(pp, token_create(TOK_STR_LIT, tstr, pp -> lineno, pp -> column, pp -> fn));
		lw_free(tstr);
		return 1;
	}
	else if (strcmp(mname, "__LINE__") == 0)
	{
		char nbuf[25];
		snprintf(nbuf, 25, "%d", pp -> lineno);
		preproc_unget_token(pp, token_create(TOK_NUMBER, nbuf, pp -> lineno, pp -> column, pp -> fn));
		return 1;
	}
	else if (strcmp(mname, "__DATE__") == 0)
	{
		char dbuf[14];
		struct tm *tv;
		time_t tm;
		static char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		
		tm = time(NULL);
		tv = localtime(&tm);
		snprintf(dbuf, 14, "\"%s %2d %04d\"", months[tv -> tm_mon], tv -> tm_mday, tv -> tm_year + 1900);
		preproc_unget_token(pp, token_create(TOK_STR_LIT, dbuf, pp -> lineno, pp -> column, pp -> fn));
		return 1;
	}
	else if (strcmp(mname, "__TIME__") == 0)
	{
		char tbuf[11];
		struct tm *tv;
		time_t tm;
		
		tm = time(NULL);
		tv = localtime(&tm);
		snprintf(tbuf, 11, "\"%02d:%02d:%02d\"", tv -> tm_hour, tv -> tm_min, tv -> tm_sec);
		preproc_unget_token(pp, token_create(TOK_STR_LIT, tbuf, pp -> lineno, pp -> column, pp -> fn));
		return 1;
	}
	
	s = symtab_find(pp, mname);
	if (!s)
		return 0;
	
	for (e = pp -> expand_list; e; e = e -> next)
	{
		/* don't expand if we're already expanding the same macro */
		if (e -> s == s)
			return 0;
	}

	if (s -> nargs == -1)
	{
		/* short circuit NULL expansion */
		if (s -> tl == NULL)
			return 1;

		goto expandmacro;
	}
	
	// look for opening paren after optional whitespace
	t2 = NULL;
	t = NULL;
	for (;;)
	{
		t = preproc_next_token(pp);
		if (t -> ttype != TOK_WSPACE && t -> ttype != TOK_EOL)
			break;
		t -> next = t2;
		t2 = t2;
	}
	if (t -> ttype != TOK_OPAREN)
	{
		// not a function-like invocation
		while (t2)
		{
			t = t2 -> next;
			preproc_unget_token(pp, t2);
			t2 = t;
		}
		return 0;
	}
	
	// parse parameters here
	t = preproc_next_token_nws(pp);
	nargs = 1;
	arglist = lw_alloc(sizeof(struct token_list *));
	arglist[0] = token_list_create();
	t2 = NULL;
	
	while (t -> ttype != TOK_CPAREN)
	{
		pcount = 0;
		if (t -> ttype == TOK_EOF)
		{
			preproc_throw_error(pp, "Unexpected EOF in macro call");
			break;
		}
		if (t -> ttype == TOK_EOL)
			continue;
		if (t -> ttype == TOK_OPAREN)
			pcount++;
		else if (t -> ttype == TOK_CPAREN && pcount)
			pcount--;
		if (t -> ttype == TOK_COMMA && pcount == 0)
		{
			if (!(s -> vargs) || (nargs > s -> nargs))
			{
				nargs++;
				arglist = lw_realloc(arglist, sizeof(struct token_list *) * nargs);
				arglist[nargs - 1] = token_list_create();
				t2 = NULL;
				continue;
			}
		}
		token_list_append(arglist[nargs - 1], token_dup(t));
	}

	if (s -> vargs)
	{
		if (nargs <= s -> nargs)
		{
			preproc_throw_error(pp, "Wrong number of arguments (%d) for variadic macro %s which takes %d arguments", nargs, mname, s -> nargs);
		}
	}
	else
	{
		if (s -> nargs != nargs && !(s -> nargs == 0 && nargs == 1 && arglist[nargs - 1]))
		{
			preproc_throw_error(pp, "Wrong number of arguments (%d) for macro %s which takes %d arguments", nargs, mname, s -> nargs);
		}
	}

	/* now calculate the pre-expansions of the arguments */
	exparglist = lw_alloc(nargs * sizeof(struct token_list *));
	for (i = 0; i < nargs; i++)
	{
		exparglist[i] = token_list_create();
		// NOTE: do nothing if empty argument
		if (arglist[i] == NULL || arglist[i] -> head == NULL)
			continue;
		pp -> sourcelist = arglist[i]->head;
		for (;;)
		{
			t = preproc_next_processed_token(pp);
			if (t -> ttype == TOK_EOF)
				break;
			token_list_append(exparglist[i], token_dup(t));
		}
	}

expandmacro:
	expand_list = token_list_dup(s -> tl);

	// scan for stringification and handle it
	repl = 0;
	while (repl == 0)
	{
		for (t = expand_list -> head; t; t = t -> next)
		{
			if (t -> ttype == TOK_HASH && t -> next && t -> next -> ttype == TOK_IDENT)
			{
				i = macro_arg(s, t -> next -> strval);
				if (i != -1)
				{
					repl = 1;
					tstr = stringify(arglist[i]);
					token_list_remove(t -> next);
					token_list_insert(expand_list, t, token_create(TOK_STR_LIT, tstr, t -> lineno, t -> column, t -> fn));
					token_list_remove(t);
					lw_free(tstr);
					break;
				}
			}
		}
		repl = 1;
	}


	// scan for concatenation and handle it	
	
	for (t = expand_list -> head; t; t = t -> next)
	{
		if (t -> ttype == TOK_DBLHASH)
		{
			// have a concatenation operator here
			for (t2 = t -> prev; t2; t2 = t2 -> prev)
			{
				if (t2 -> ttype != TOK_WSPACE)
					break;
			}
			for (t3 = t -> next; t3; t3 = t3 -> next)
			{
				if (t3 -> ttype != TOK_WSPACE)
					break;
			}
			// if no non-whitespace before or after, ignore it
			if (!t2 || !t3)
				continue;
			// eat the whitespace before and after
			while (t -> prev != t2)
				token_list_remove(t -> prev);
			while (t -> next != t3)
				token_list_remove(t -> next);
			// now paste t -> prev with t -> next and replace t with the result
			// continue scanning for ## at t -> next -> next
			t3 = t -> next -> next;
			
			rtl = paste_tokens(pp, s, arglist, t -> prev, t -> next);
			token_list_remove(t -> next);
			token_list_remove(t -> prev);
			t2 = t -> prev;
			token_list_remove(t);
			for (t = rtl -> head; t; t = t -> next)
			{
				token_list_insert(expand_list, t2, token_dup(t));
			}
			t = t3 -> prev;
			token_list_destroy(rtl);
		}
	}
	
	// now scan for arguments and expand them
	for (t = expand_list -> head; t; t = t -> next)
	{
	again:
		if (t -> ttype == TOK_IDENT)
		{
			/* identifiers might need expansion to arguments */
			i = macro_arg(s, t -> strval);
			if (i != -1)
			{
				t3 = t -> next;
				for (t2 = exparglist[i] -> tail; t2; t2 = t2 -> prev)
					token_list_insert(expand_list, t, token_dup(t2));
				token_list_remove(t);
				t = t3;
				goto again;
			}
		}
	}

	/* put the new expansion in front of the input, if relevant; if we
	   expanded to nothing, no need to create an expansion record or
	   put anything into the input queue */
	if (expand_list -> head)
	{
		token_list_append(expand_list, token_create(TOK_ENDEXPAND, "", -1, -1, ""));
		
		// move the expanded list into the token queue
		for (t = expand_list -> tail; t; t = t -> prev)
			preproc_unget_token(pp, token_dup(t));
		
		/* set up expansion record */
		e = lw_alloc(sizeof(struct expand_e));
		e -> next = pp -> expand_list;
		pp -> expand_list = e;
		e -> s = s;
	}
	
	/* now clean up */
	token_list_destroy(expand_list);
	for (i = 0; i < nargs; i++)
	{
		token_list_destroy(arglist[i]);
		token_list_destroy(exparglist[i]);
	}
	lw_free(arglist);
	lw_free(exparglist);
	
	return 1;
}

struct token *preproc_next(struct preproc_info *pp)
{
	struct token *t;
	
	t = preproc_next_processed_token(pp);
	pp -> curtok = NULL;
	return t;
}
