/*
lwcc/token.h

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

#ifndef token_h_seen___
#define token_h_seen___

#include <stdio.h>

enum
{
	CPP_NOUNG = -3,
	CPP_EOL = -2,
	CPP_EOF = -1,
};

#define TOK_NONE 0
#define TOK_EOF 1
#define TOK_EOL 2
#define TOK_WSPACE 3
#define TOK_IDENT 4
#define TOK_NUMBER 5
#define TOK_CHAR 6
#define TOK_ADD 8
#define TOK_SUB 9
#define TOK_OPAREN 10
#define TOK_CPAREN 11
#define TOK_NE 12
#define TOK_EQ 13
#define TOK_LE 14
#define TOK_LT 15
#define TOK_GE 16
#define TOK_GT 17
#define TOK_BAND 18
#define TOK_BOR 19
#define TOK_BNOT 20
#define TOK_MOD 21
#define TOK_COMMA 22
#define TOK_ELLIPSIS 23
#define TOK_QMARK 24
#define TOK_COLON 25
#define TOK_OBRACE 26
#define TOK_CBRACE 27
#define TOK_OSQUARE 28
#define TOK_CSQUARE 29
#define TOK_COM 30
#define TOK_EOS 31
#define TOK_HASH 32
#define TOK_DBLHASH 33
#define TOK_XOR 34
#define TOK_XORASS 35
#define TOK_STAR 36
#define TOK_MULASS 37
#define TOK_DIV 38
#define TOK_DIVASS 39
#define TOK_ASS 40
#define TOK_MODASS 41
#define TOK_SUBASS 42
#define TOK_DBLSUB 43
#define TOK_ADDASS 44
#define TOK_DBLADD 45
#define TOK_BWAND 46
#define TOK_BWANDASS 47
#define TOK_BWOR 48
#define TOK_BWORASS 49
#define TOK_LSH 50
#define TOK_LSHASS 51
#define TOK_RSH 52
#define TOK_RSHASS 53
#define TOK_DOT 54
#define TOK_CHR_LIT 55
#define TOK_STR_LIT 56
#define TOK_ARROW 57
#define TOK_ENDEXPAND 58
#define TOK_ERROR 59
#define TOK_MAX 60

struct token
{
	int ttype;				// token type
	char *strval;			// the token value if relevant
	struct token *prev;		// previous token in a list
	struct token *next;		// next token in a list
	struct token_list *list;// pointer to head of list descriptor this token is on
	int lineno;				// line number token came from
	int column;				// character column token came from
	const char *fn;			// file name token came from
};

struct token_list
{
	struct token *head;		// the head of the list
	struct token *tail;		// the tail of the list
};

extern void token_free(struct token *);
extern struct token *token_create(int, char *strval, int, int, const char *);
extern struct token *token_dup(struct token *);
/* add a token to the end of a list */
extern void token_list_append(struct token_list *, struct token *);
/* add a token to the start of a list */
extern void token_list_prepend(struct token_list *, struct token *);
/* remove individual token from whatever list it is on */
extern void token_list_remove(struct token *);
/* replace token with list of tokens specified */
extern void token_list_insert(struct token_list *, struct token *, struct token *);
/* duplicate a list to a new list pointer */
extern struct token_list *token_list_dup(struct token_list *);
/* print a token out */
extern struct token_list *token_list_create(void);
extern void token_list_destroy(struct token_list *);

extern void token_print(struct token *, FILE *);

#endif // token_h_seen___
