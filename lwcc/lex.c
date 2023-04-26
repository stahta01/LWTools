/*
lwcc/lex.c

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

#include <ctype.h>
#include <stdio.h>

#include <lw_alloc.h>
#include <lw_strbuf.h>

#include "cpp.h"
#include "token.h"

/* fetch a raw input byte from the current file. Will return CPP_EOF if
   EOF is encountered and CPP_EOL if an end of line sequence is encountered.
   End of line is defined as either CR, CRLF, LF, or LFCR. CPP_EOL is
   returned on the first CR or LF encountered. The complementary CR or LF
   is munched, if present, when the *next* character is read. This always
   operates on file_stack.

   This function also accounts for line numbers in input files and also
   character columns.
*/
static int fetch_byte_ll(struct preproc_info *pp)
{
	int c;

	if (pp -> eolstate != 0)	
	{
		pp -> lineno++;
		pp -> column = 0;
	}
	c = getc(pp -> fp);
	pp -> column++;
	if (pp -> eolstate == 1)
	{
		// just saw CR, munch LF
		if (c == 10)
			c = getc(pp -> fp);
		pp -> eolstate = 0;
	}
	else if (pp -> eolstate == 2)
	{
		// just saw LF, much CR
		if (c == 13)
			c = getc(pp -> fp);
		pp -> eolstate = 0;
	}
	
	if (c == 10)
	{
		// we have LF - end of line, flag to munch CR
		pp -> eolstate = 2;
		c = CPP_EOL;
	}
	else if (c == 13)
	{
		// we have CR - end of line, flag to munch LF
		pp -> eolstate = 1;
		c = CPP_EOL;
	}
	else if (c == EOF)
	{
		c = CPP_EOF;
	}
	return c;
}

/* This function takes a sequence of bytes from the _ll function above
   and does trigraph interpretation on it, but only if the global
   trigraphs is nonzero. */
static int fetch_byte_tg(struct preproc_info *pp)
{
	int c;
	
	if (!pp -> trigraphs)
	{
		c = fetch_byte_ll(pp);
	}
	else
	{
		/* we have to do the trigraph shit here */
		if (pp -> ra != CPP_NOUNG)
		{
			if (pp -> qseen > 0)
			{
				c = '?';
				pp -> qseen -= 1;
				return c;
			}
			else
			{
				c = pp -> ra;
				pp -> ra = CPP_NOUNG;
				return c;
			}
		}
	
		c = fetch_byte_ll(pp);
		while (c == '?')
		{
			pp -> qseen++;
			c = fetch_byte_ll(pp);
		}
	
		if (pp -> qseen >= 2)
		{
			// we have a trigraph
			switch (c)
			{
			case '=':
				c = '#';
				pp -> qseen -= 2;
				break;
			
			case '/':
				c = '\\';
				pp -> qseen -= 2;
				break;
		
			case '\'':
				c = '^';
				pp -> qseen -= 2;
				break;
		
			case '(':
				c = '[';
				pp -> qseen -= 2;
				break;
		
			case ')':
				c = ']';
				pp -> qseen -= 2;
				break;
		
			case '!':
				c = '|';
				pp -> qseen -= 2;
				break;
		
			case '<':
				c = '{';
				pp -> qseen -= 2;
				break;
		
			case '>':
				c = '}';
				pp -> qseen -= 2;
				break;
		
			case '-':
				c = '~';
				pp -> qseen -= 2;
				break;
			}
			if (pp -> qseen > 0)
			{
				pp -> ra = c;
				c = '?';
				pp -> qseen--;
			}
		}
		else if (pp -> qseen > 0)
		{
			pp -> ra = c;
			c = '?';
			pp -> qseen--;
		}
	}
	return c;
}

/* This function puts a byte back onto the front of the input stream used
   by fetch_byte(). Theoretically, an unlimited number of characters can
   be unfetched. Line and column counting may be incorrect if unfetched
   characters cross a token boundary. */
void preproc_lex_unfetch_byte(struct preproc_info *pp, int c)
{
	if (pp -> lexstr)
	{
		if (c == CPP_EOL)
			return;
		if (pp -> lexstrloc > 0)
		{
			pp -> lexstrloc--;
			return;
		}
	}

	if (pp -> ungetbufl >= pp -> ungetbufs)
	{
		pp -> ungetbufs += 100;
		pp -> ungetbuf = lw_realloc(pp -> ungetbuf, pp -> ungetbufs);
	}
	pp -> ungetbuf[pp -> ungetbufl++] = c;
}

/* This function retrieves a byte from the input stream. It performs
   backslash-newline splicing on the returned bytes. Any character
   retrieved from the unfetch buffer is presumed to have already passed
   the backslash-newline filter. */
static int fetch_byte(struct preproc_info *pp)
{
	int c;

	if (pp -> lexstr)
	{
		if (pp -> lexstr[pp -> lexstrloc])
			return pp -> lexstr[pp -> lexstrloc++];
		else
			return CPP_EOL;
	}

	if (pp -> ungetbufl > 0)
	{
		pp -> ungetbufl--;
		c = pp -> ungetbuf[pp -> ungetbufl];
		if (pp -> ungetbufl == 0)
		{
			lw_free(pp -> ungetbuf);
			pp -> ungetbuf = NULL;
			pp -> ungetbufs = 0;
		}
		return c;
	}
	
again:
	if (pp -> unget != CPP_NOUNG)
	{
		c = pp -> unget;
		pp -> unget = CPP_NOUNG;
	}
	else
	{
		c = fetch_byte_tg(pp);
	}
	if (c == '\\')
	{
		int c2;
		c2 = fetch_byte_tg(pp);
		if (c2 == CPP_EOL)
			goto again;
		else
			pp -> unget = c2;
	}
	return c;
}



/*
Lex a token off the current input file.

Returned tokens are as follows:

* all words starting with [a-zA-Z_] are returned as TOK_IDENT
* numbers are returned as their appropriate type
* all whitespace in a sequence, including comments, is returned as
  a single instance of TOK_WSPACE
* TOK_EOL is returned in the case of the end of a line
* TOK_EOF is returned when the end of the file is reached
* If no TOK_EOL appears before TOK_EOF, a TOK_EOL will be synthesised
* Any symbolic operator, etc., recognized by C will be returned as such
  a token
* TOK_HASH will be returned for a #
* trigraphs will be interpreted
* backslash-newline will be interpreted
* any instance of CR, LF, CRLF, or LFCR will be interpreted as TOK_EOL
*/


int preproc_lex_fetch_byte(struct preproc_info *pp)
{
	int c;
	c = fetch_byte(pp);
	if (c == CPP_EOF && pp -> eolseen == 0)
	{
		preproc_throw_warning(pp, "No newline at end of file");
		pp -> eolseen = 1;
		return CPP_EOL;
	}
	
	if (c == CPP_EOL)
	{
		pp -> eolseen = 1;
		return c;
	}

	pp -> eolseen = 0;
	
	/* convert comments to a single space here */
	if (c == '/')
	{
		int c2;
		c2 = fetch_byte(pp);
		if (c2 == '/')
		{
			/* single line comment */
			c = ' ';
			for (;;)
			{
				c2 = fetch_byte(pp);
				if (c2 == CPP_EOF || c2 == CPP_EOL)
					break;
			}
			preproc_lex_unfetch_byte(pp, c2);
		}
		else if (c2 == '*')
		{
			/* block comment */
			c = ' ';
			for (;;)
			{
				c2 = fetch_byte(pp);
				if (c2 == CPP_EOF)
				{
					preproc_lex_unfetch_byte(pp, c);
					break;
				}
				if (c2 == '*')
				{
					/* maybe end of comment */
					c2 = preproc_lex_fetch_byte(pp);
					if (c2 == '/')
						break;
				}
			}
		}
		else
		{
			/* not a comment - restore lookahead character */
			preproc_lex_unfetch_byte(pp, c2);
		}
	}
	return c;
}

struct token *preproc_lex_next_token(struct preproc_info *pp)
{
	int sline = pp -> lineno;
	int scol = pp -> column;
	char *strval = NULL;
	int ttype = TOK_NONE;
	int c, c2;
	int cl;
	struct lw_strbuf *strbuf;
	struct token *t = NULL;
	struct preproc_info *fs;
					
fileagain:
	c = preproc_lex_fetch_byte(pp);
	if (c == CPP_EOF)
	{
		if (pp -> nlseen == 0)
		{
			c = CPP_EOL;
		}
	}

	if (pp -> lineno != sline)
	{
		sline = pp -> lineno;
		scol = pp -> column;
	}
	
	if (c == CPP_EOF)
	{
		/* check if we fell off the end of an include file */
		if (pp -> filestack)
		{
			if (pp -> skip_level || pp -> found_level)
			{
				preproc_throw_error(pp, "Unbalanced conditionals in include file");
			}
			fclose(pp -> fp);
			fs = pp -> filestack;
			*pp = *fs;
			pp -> filestack = fs -> n;
			goto fileagain;
		}
		else
		{
			ttype = TOK_EOF;
			goto out;
		}
	}
	if (c == CPP_EOL)
	{
		pp -> nlseen = 1;
		ttype = TOK_EOL;
		goto out;
	}

	pp -> nlseen = 0;
	if (isspace(c))
	{
		while (isspace(c))
			c = preproc_lex_fetch_byte(pp);
		preproc_lex_unfetch_byte(pp, c);
		ttype = TOK_WSPACE;
		goto out;
	}
	
	switch (c)
	{
	case '?':
		ttype = TOK_QMARK;
		goto out;
		
	case ':':
		ttype = TOK_COLON;
		goto out;
		
	case ',':
		ttype = TOK_COMMA;
		goto out;
		
	case '(':
		ttype = TOK_OPAREN;
		goto out;
		
	case ')':
		ttype = TOK_CPAREN;
		goto out;
		
	case '{':
		ttype = TOK_OBRACE;
		goto out;
		
	case '}':
		ttype = TOK_CBRACE;
		goto out;
		
	case '[':
		ttype = TOK_OSQUARE;
		goto out;
		
	case ']':
		ttype = TOK_CSQUARE;
		goto out;
		
	case '~':
		ttype = TOK_COM;
		goto out;
		
	case ';':
		ttype = TOK_EOS;
		goto out;
	
	/* and now for the possible multi character tokens */
	case '#':
		ttype = TOK_HASH;
		c = preproc_lex_fetch_byte(pp);
		if (c == '#')
			ttype = TOK_DBLHASH;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '^':
		ttype = TOK_XOR;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_XORASS;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '!':
		ttype = TOK_BNOT;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_NE;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '*':
		ttype = TOK_STAR;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_MULASS;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '/':
		ttype = TOK_DIV;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_DIVASS;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '=':
		ttype = TOK_ASS;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_EQ;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '%':
		ttype = TOK_MOD;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_MODASS;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '-':
		ttype = TOK_SUB;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_SUBASS;
		else if (c == '-')
			ttype = TOK_DBLSUB;
		else if (c == '>')
			ttype = TOK_ARROW;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '+':
		ttype = TOK_ADD;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_ADDASS;
		else if (c == '+')
			ttype = TOK_DBLADD;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	

	case '&':
		ttype = TOK_BWAND;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_BWANDASS;
		else if (c == '&')
			ttype = TOK_BAND;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;

	case '|':
		ttype = TOK_BWOR;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_BWORASS;
		else if (c == '|')
			ttype = TOK_BOR;
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;

	case '<':
		ttype = TOK_LT;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_LE;
		else if (c == '<')
		{
			ttype = TOK_LSH;
			c = preproc_lex_fetch_byte(pp);
			if (c == '=')
				ttype = TOK_LSHASS;
			else
				preproc_lex_unfetch_byte(pp, c);
		}
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
		
	case '>':
		ttype = TOK_GT;
		c = preproc_lex_fetch_byte(pp);
		if (c == '=')
			ttype = TOK_GE;
		else if (c == '>')
		{
			ttype = TOK_RSH;
			c = preproc_lex_fetch_byte(pp);
			if (c == '=')
				ttype = TOK_RSHASS;
			else
				preproc_lex_unfetch_byte(pp, c);
		}
		else
			preproc_lex_unfetch_byte(pp, c);
		goto out;
	
	case '\'':
		/* character constant - turns into a  uint */
chrlit:
		cl = 0;
		strbuf = lw_strbuf_new();
		for (;;)
		{
			c = preproc_lex_fetch_byte(pp);
			if (c == CPP_EOF || c == CPP_EOL || c == '\'')
				break;
			cl++;
			if (c == '\\')
			{
				lw_strbuf_add(strbuf, '\\');
				c = preproc_lex_fetch_byte(pp);
				if (c == CPP_EOF || c == CPP_EOL)
				{
					if (!pp -> lexstr)
						preproc_throw_error(pp, "Invalid character constant");
					ttype = TOK_ERROR;
					strval = lw_strbuf_end(strbuf);
					goto out;
				}
				cl++;
				lw_strbuf_add(strbuf, c);
				continue;
			}
			lw_strbuf_add(strbuf, c);
		}
		strval = lw_strbuf_end(strbuf);
		if (cl == 0)
		{
			ttype = TOK_ERROR;
			if (!pp -> lexstr)
				preproc_throw_error(pp, "Invalid character constant");
		}
		else
			ttype = TOK_CHR_LIT;
		goto out;

	case '"':
strlit:
		/* string literal */
		strbuf = lw_strbuf_new();
		lw_strbuf_add(strbuf, '"');
		for (;;)
		{
			c = preproc_lex_fetch_byte(pp);
			if (c == CPP_EOF || c == CPP_EOL)
			{
				ttype = TOK_ERROR;
				strval = lw_strbuf_end(strbuf);
				if (!pp -> lexstr)
					preproc_throw_error(pp, "Invalid string constant");
				goto out;
			}
			if (c == '"')
				break;
			if (c == '\\')
			{
				lw_strbuf_add(strbuf, '\\');
				c = preproc_lex_fetch_byte(pp);
				if (c == CPP_EOF || c == CPP_EOL)
				{
					ttype = TOK_ERROR;
					if (!pp -> lexstr)
						preproc_throw_error(pp, "Invalid string constant");
					strval = lw_strbuf_end(strbuf);
					goto out;
				}
				lw_strbuf_add(strbuf, c);
				continue;
			}
			lw_strbuf_add(strbuf, c);
		}
		lw_strbuf_add(strbuf, '"');
		strval = lw_strbuf_end(strbuf);
		ttype = TOK_STR_LIT;
		goto out;

	case 'L':
		/* check for wide string or wide char const */
		c2 = preproc_lex_fetch_byte(pp);
		if (c2 == '\'')
		{
			goto chrlit;
		}
		else if (c2 == '"')
		{
			goto strlit;
		}
		preproc_lex_unfetch_byte(pp, c2);
		/* fall through for identifier */
	case '_':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
	case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
	case 's': case 't': case 'u': case 'v': case 'w': case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z':
		/* we have an identifier here */
		strbuf = lw_strbuf_new();
		lw_strbuf_add(strbuf, c);
		for (;;)
		{
			c = preproc_lex_fetch_byte(pp);
			if ((c == '_') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
			{
				lw_strbuf_add(strbuf, c);
				continue;
			}
			else
			{
				lw_strbuf_add(strbuf, 0);
				strval = lw_strbuf_end(strbuf);
				break;
			}
		}
		preproc_lex_unfetch_byte(pp, c);
		ttype = TOK_IDENT;
		goto out;

	case '.':
		c = preproc_lex_fetch_byte(pp);
		if (c >= '0' && c <= '9')
		{
			strbuf = lw_strbuf_new();
			lw_strbuf_add(strbuf, '.');
			goto numlit;
		}
		else if (c == '.')
		{
			c = preproc_lex_fetch_byte(pp);
			if (c == '.')
			{
				ttype = TOK_ELLIPSIS;
				goto out;
			}
			preproc_lex_unfetch_byte(pp, c);
		}
		preproc_lex_unfetch_byte(pp, c);
		ttype = TOK_DOT;
		goto out;

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		strbuf = lw_strbuf_new();
numlit:
		ttype = TOK_NUMBER;
		lw_strbuf_add(strbuf, c);
		for (;;)
		{
			c = preproc_lex_fetch_byte(pp);
			if (!((c == '_') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
				break;
			lw_strbuf_add(strbuf, c);
			if (c == 'e' || c == 'E' || c == 'p' || c == 'P')
			{
				c = preproc_lex_fetch_byte(pp);
				if (c == '+' || c == '-')
				{
					lw_strbuf_add(strbuf, c);
					continue;
				}
				preproc_lex_unfetch_byte(pp, c);
			}
		}
		strval = lw_strbuf_end(strbuf);
		preproc_lex_unfetch_byte(pp, c);
		goto out;
		
	default:
		ttype = TOK_CHAR;
		strval = lw_alloc(2);
		strval[0] = c;
		strval[1] = 0;
		break;
	}
out:	
	t = token_create(ttype, strval, sline, scol, pp -> fn);
	lw_free(strval);
	return t;
}
