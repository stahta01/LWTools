/*
lwcc/parse.c

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
#include <string.h>
#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"
#include "tree.h"
#include "parse.h"

#include "parse_c.h"


void *Parse(void *parser, int tokid, struct tokendata *tdata, struct parserinfo *pi);
void *ParseAlloc(void *(*alloc)(size_t size));
void ParseFree(void *parser, void (*free)(void *ptr));

void tokendata_free(struct tokendata *td)
{
	if (td)
	{
		if (td -> strval)
			lw_free(td -> strval);
		lw_free(td);
	}
}

extern char *ptoken_names[];
char *tokendata_name(struct tokendata *td)
{
	if (td -> tokid < 0)
		return "****UNKNOWN****";
	return ptoken_names[td -> tokid];
}

void tokendata_print(FILE *fp, struct tokendata *td)
{
	fprintf(fp, "TOKEN: %s", tokendata_name(td));
	if (td -> strval)
		fprintf(fp, " \"%s\"", td -> strval);
	fprintf(fp, "\n");
}

#define TOK_KW_IF	-1
#define TOK_KW_ELSE	-2
#define TOK_KW_WHILE -3
#define TOK_KW_DO -4
#define TOK_KW_FOR -5
#define TOK_KW_VOID -6
#define TOK_KW_INT -7
#define TOK_KW_CHAR -8
#define TOK_KW_SHORT -9
#define TOK_KW_LONG -10
#define TOK_KW_UNSIGNED -11
#define TOK_KW_SIGNED -12
#define TOK_KW_FLOAT -13
#define TOK_KW_DOUBLE -14
#define TOK_KW_STRUCT -15
#define TOK_KW_UNION -16
#define TOK_KW_TYPEDEF -17
#define TOK_KW_STATIC -18
#define TOK_KW_SWITCH -19
#define TOK_KW_CASE -20
#define TOK_KW_DEFAULT -21
#define TOK_KW_BREAK -22
#define TOK_KW_CONTINUE -23
#define TOK_KW_CONST -24
#define TOK_KW_AUTO -25
#define TOK_KW_ENUM -26
#define TOK_KW_REGISTER -27
#define TOK_KW_SIZEOF -28
#define TOK_KW_VOLATILE -29
#define TOK_KW_RETURN -30
#define TOK_KW_EXTERN -31
#define TOK_KW_GOTO -32
#define TOK_TYPENAME -100

static struct { int tok; char *word; } keyword_list[] = {
	{ TOK_KW_IF, "if" },
	{ TOK_KW_ELSE, "else" },
	{ TOK_KW_WHILE, "while" },
	{ TOK_KW_DO, "do" },
	{ TOK_KW_FOR, "for" },
	{ TOK_KW_VOID, "void" },
	{ TOK_KW_INT, "int" },
	{ TOK_KW_CHAR, "char" },
	{ TOK_KW_SHORT, "short" },
	{ TOK_KW_LONG, "long" },
	{ TOK_KW_UNSIGNED, "unsigned" },
	{ TOK_KW_SIGNED, "signed" },
	{ TOK_KW_FLOAT, "float" },
	{ TOK_KW_DOUBLE, "double" },
	{ TOK_KW_STRUCT, "struct" },
	{ TOK_KW_UNION, "union" },
	{ TOK_KW_TYPEDEF, "typedef" },
	{ TOK_KW_STATIC, "static" },
	{ TOK_KW_SWITCH, "switch" },
	{ TOK_KW_CASE, "case" },
	{ TOK_KW_DEFAULT, "default" },
	{ TOK_KW_BREAK, "break" },
	{ TOK_KW_CONTINUE, "continue" },
	{ TOK_KW_CONST, "const" },
	{ TOK_KW_AUTO, "auto" },
	{ TOK_KW_ENUM, "enum" },
	{ TOK_KW_REGISTER, "register" },
	{ TOK_KW_SIZEOF, "sizeof" },
	{ TOK_KW_VOLATILE, "volatile" },
	{ TOK_KW_RETURN, "return" },
	{ TOK_KW_EXTERN, "extern" },
	{ TOK_KW_GOTO, "goto" },
	{ 0, "" }
}; 

struct token *parse_next(struct preproc_info *pp)
{
	struct token *tok;
	int i;
	
	for (;;)
	{
		tok = preproc_next(pp);
		if (tok -> ttype == TOK_WSPACE)
			continue;
		if (tok -> ttype == TOK_EOL)
			continue;
		if (tok -> ttype == TOK_CHAR)
		{
			// random character
			fprintf(stderr, "Random character %02x\n", tok -> strval[0]);
			if (tok -> strval[0] < 32 || tok -> strval[0] > 126)
				continue;
		}
		break;
	}
	if (tok -> ttype == TOK_IDENT)
	{
		/* convert identifier tokens to their respective meanings */
		for (i = 0; keyword_list[i].tok != TOK_NONE; i++)
		{
			if (strcmp(keyword_list[i].word, tok -> strval) == 0)
			{
				tok -> ttype = keyword_list[i].tok;
				goto out;
			}
		}
		/* check for a registered type here */
	}
out:
	fprintf(stderr, "Lexed: ");
	token_print(tok, stderr);
	fprintf(stderr, " (%d)\n", tok -> ttype);
	return tok;
}

static struct {
	int tokid;
	int ttype;
} toktable[] = {
	{ PTOK_IDENTIFIER,			TOK_IDENT },
	{ PTOK_ENDS,				TOK_EOS },
	{ PTOK_KW_INT,				TOK_KW_INT },
	{ PTOK_KW_LONG,				TOK_KW_LONG },
	{ PTOK_KW_SHORT,			TOK_KW_SHORT },
	{ PTOK_KW_CHAR,				TOK_KW_CHAR },
	{ PTOK_KW_SIGNED,			TOK_KW_SIGNED },
	{ PTOK_KW_UNSIGNED,			TOK_KW_UNSIGNED },
	{ PTOK_STAR,				TOK_STAR },
	{ PTOK_KW_VOID,				TOK_KW_VOID },
	{ PTOK_KW_FLOAT,			TOK_KW_FLOAT },
	{ PTOK_KW_DOUBLE,			TOK_KW_DOUBLE },
	{ PTOK_OBRACE,				TOK_OBRACE },
	{ PTOK_CBRACE,				TOK_CBRACE },
	{ PTOK_OPAREN,				TOK_OPAREN },
	{ PTOK_CPAREN,				TOK_CPAREN },
	{ 0, 0 }
};

static int lookup_ptok(int ttype)
{
	int i;
	for (i = 0; toktable[i].tokid != 0; i++)
		if (toktable[i].ttype == ttype)
			return toktable[i].tokid;
	return -1;
}

node_t *parse_program(struct preproc_info *pp)
{
	struct token *tok;
	struct tokendata *td;
	struct parserinfo pi = { NULL };
	void *parser;
		
	/* the cast below shuts up a warning */
	parser = ParseAlloc((void *)lw_alloc);
	for (;;)
	{
		tok = parse_next(pp);
		if (tok -> ttype == TOK_EOF)
			break;
		
		td = lw_alloc(sizeof(struct tokendata));
		td -> strval = NULL;
		td -> numval[0] = 0;
		td -> numval[1] = 0;
		td -> numval[2] = 0;
		td -> numval[3] = 0;
		td -> numval[4] = 0;
		td -> numval[5] = 0;
		td -> numval[6] = 0;
		td -> numval[7] = 0;
		td -> tokid = lookup_ptok(tok -> ttype);
		if (tok -> strval)
			td -> strval = lw_strdup(tok -> strval);
		
		tokendata_print(stderr, td);
		
		Parse(parser, td -> tokid, td, &pi);
	}
	Parse(parser, 0, NULL, &pi);
	ParseFree(parser, lw_free);
	return pi.parsetree;
}
