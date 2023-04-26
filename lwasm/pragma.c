/*
pragma.c

Copyright © 2010 William Astle

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

#include <string.h>
#include <ctype.h>

#include <lw_string.h>
#include <lw_alloc.h>

#include "lwasm.h"
#include "instab.h"
#include "input.h"

struct pragma_list
{
	const char *setstr;
	const char *resetstr;
	int flag;
};

struct pragma_stack_entry
{
	int magic;	// must always be at the start of any input stack entry
	int flag;	// the pragma flag bit
	char str[1];	// magic number - this will be allocated bigger
			// string will be what's needed to re-instate a pragma
};

static const struct pragma_list set_pragmas[] =
{
	{ "dollarnotlocal", "nodollarnotlocal", PRAGMA_DOLLARNOTLOCAL },
	{ "nodollarlocal", "dollarlocal", PRAGMA_DOLLARNOTLOCAL },
	{ "noindex0tonone", "index0tonone", PRAGMA_NOINDEX0TONONE },
	{ "undefextern", "noundefextern", PRAGMA_UNDEFEXTERN },
	{ "cescapes", "nocescapes", PRAGMA_CESCAPES },
	{ "importundefexport", "noimportundefexport", PRAGMA_IMPORTUNDEFEXPORT },
	{ "pcaspcr", "nopcaspcr", PRAGMA_PCASPCR },
	{ "shadow", "noshadow", PRAGMA_SHADOW },
	{ "nolist", "list", PRAGMA_NOLIST },
	{ "autobranchlength", "noautobranchlength", PRAGMA_AUTOBRANCHLENGTH },
	{ "export", "noexport", PRAGMA_EXPORT },
	{ "symbolnocase", "nosymbolnocase", PRAGMA_SYMBOLNOCASE },
	{ "nosymbolcase", "symbolcase", PRAGMA_SYMBOLNOCASE },
	{ "condundefzero", "nocondundefzero", PRAGMA_CONDUNDEFZERO },
	{ "6809", "6309", PRAGMA_6809 },
	{ "6800compat", "no6800compat", PRAGMA_6800COMPAT },
	{ "forwardrefmax", "noforwardrefmax", PRAGMA_FORWARDREFMAX },
	{ "testmode", "notestmode", PRAGMA_TESTMODE },
	{ "c", "noc", PRAGMA_C },
	{ "cc", "nocc", PRAGMA_CC },
	{ "cd", "nocd", PRAGMA_CD },
	{ "ct", "noct", PRAGMA_CT },
	{ "qrts", "noqrts", PRAGMA_QRTS },
	{ "m80ext", "nom80ext", PRAGMA_M80EXT },
	{ "6809conv", "no6809conv", PRAGMA_6809CONV },
	{ "6309conv", "no6309conv", PRAGMA_6309CONV },
	{ "newsource", "nonewsource", PRAGMA_NEWSOURCE },
	{ "nooldsource", "oldsource", PRAGMA_NEWSOURCE },
	{ "operandsizewarning", "nooperandsizewarning", PRAGMA_OPERANDSIZE },
	{ "emuext", "noemuext", PRAGMA_EMUEXT },
	{ "nooutput", "output", PRAGMA_NOOUTPUT },
	{ "noexpandcond", "expandcond", PRAGMA_NOEXPANDCOND },
	{ 0, 0, 0 }
};

int parse_pragma_helper(char *p)
{
	int i;

	for (i = 0; set_pragmas[i].setstr; i++)
	{
		if (!strcasecmp(p, set_pragmas[i].setstr))
		{
			return set_pragmas[i].flag;
			return 1;
		}
		if (!strcasecmp(p, set_pragmas[i].resetstr))
		{
			return set_pragmas[i].flag | PRAGMA_CLEARBIT;
			return 2;
		}
	}

	return 0;
}

int parse_pragma_string(asmstate_t *as, char *str, int ignoreerr)
{
	char *p;
	const char *np = str;
	int pragma;

	while (np)
	{
		p = lw_token(np, ',', &np);
		debug_message(as, 200, "Setting/resetting pragma %s", p);
		pragma = parse_pragma_helper(p);
		debug_message(as, 200, "Got pragma code %08X", pragma);
		lw_free(p);

		if (pragma == 0 && !ignoreerr)
			return 0;

		if (pragma & PRAGMA_CLEARBIT)
			as->pragmas &= ~pragma;
		else
			as->pragmas |= pragma;
		
		debug_message(as, 200, "New pragma state: %08X", as -> pragmas);
	}
	return 1;
}

PARSEFUNC(pseudo_parse_pragma)
{
	char *ps, *t;
	
	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;
	
	l -> len = 0;

	if (parse_pragma_string(as, ps, 0) == 0)
	{
		lwasm_register_error(as, l, E_PRAGMA_UNRECOGNIZED);
	}
	if (as -> pragmas & PRAGMA_NOLIST)
		l -> pragmas |= PRAGMA_NOLIST;
	if (as->pragmas & PRAGMA_CC)
	{
		l->pragmas |= PRAGMA_CC;
		as->pragmas &= ~PRAGMA_CC;
	}
	lw_free(ps);
}

PARSEFUNC(pseudo_parse_starpragma)
{
	char *ps, *t;

	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;

	l -> len = 0;
	
	// *pragma must NEVER throw an error
	parse_pragma_string(as, ps, 1);
	if (as -> pragmas & PRAGMA_NOLIST)
		l -> pragmas |= PRAGMA_NOLIST;
	if (as->pragmas & PRAGMA_CC)
	{
		l->pragmas |= PRAGMA_CC;
		as->pragmas &= ~PRAGMA_CC;
	}
	lw_free(ps);
}

static int pragma_stack_compare(input_stack_entry *e, void *d)
{
	int flag = *((int *)d);
	struct pragma_stack_entry *pse = (struct pragma_stack_entry *)e;

	if (pse -> flag == flag)
		return 1;
	return 0;
}

PARSEFUNC(pseudo_parse_starpragmapop)
{
	char *ps, *t;
	char *pp;
	int i;
	const char *np;
	struct pragma_stack_entry *pse;
	
	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;
	
	l -> len = 0;
	
	// *pragma stuff must never throw an error
	np = ps;

	while (np)
	{
		pp = lw_token(np, ',', &np);
		for (i = 0; set_pragmas[i].setstr; i++)
		{
			if (!strcasecmp(pp, set_pragmas[i].setstr) || !strcasecmp(pp, set_pragmas[i].resetstr))
			{
				pse = (struct pragma_stack_entry *)input_stack_pop(as, 0x42424242, pragma_stack_compare, (void *)&(set_pragmas[i].flag));
				if (pse)
				{
					debug_message(as, 100, "Popped pragma string %s", pse->str);
					parse_pragma_string(as, (char *)&(pse->str), 1);
					lw_free(pse);
				}
				if (set_pragmas[i].flag == PRAGMA_NOLIST)
					l -> pragmas |= PRAGMA_NOLIST;
			}
		}
		lw_free(pp);
	}

	lw_free(ps);
}

PARSEFUNC(pseudo_parse_starpragmapush)
{
	char *ps, *t;
	char *pp;
	int i;
	const char *np;
	struct pragma_stack_entry *pse;
	
	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;
	
	l -> len = 0;
	
	// *pragma stuff must never throw an error
	np = ps;

	while (np)
	{
		pp = lw_token(np, ',', &np);
		for (i = 0; set_pragmas[i].setstr; i++)
		{
			if (!strcasecmp(pp, set_pragmas[i].setstr) || !strcasecmp(pp, set_pragmas[i].resetstr))
			{
				/* found set or reset pragma */
				/* push pragma state */
				if (as -> pragmas & (set_pragmas[i].flag))
				{
					/* use set string */
					t = (char *)set_pragmas[i].setstr;
				}
				else
				{
					/* use reset string */
					t = (char *)set_pragmas[i].resetstr;
				}
				pse = lw_alloc(sizeof(struct pragma_stack_entry) + strlen(t));
				pse -> flag = set_pragmas[i].flag;
				pse -> magic = 0x42424242;
				strcpy((char *)&(pse -> str), t);
				debug_message(as, 100, "Pushed pragma string %s", pse->str);

				input_stack_push(as, (input_stack_entry *)pse);
				
				if (set_pragmas[i].flag == PRAGMA_NOLIST)
					l -> pragmas |= PRAGMA_NOLIST;
			}
		}
		lw_free(pp);
	}

	lw_free(ps);
}

