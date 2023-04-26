/*
lw_cmdline.c

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lw_alloc.h"
#include "lw_cmdline.h"

#define DOCCOL 30
#define LLEN 78

static struct lw_cmdline_options builtin[3] =
{
	{ "help", '?', 0, 0, "give this help list" },
	{ "usage", 0, 0, 0, "give a short usage message" },
	{ "version", 'V', 0, 0, "print program version" }
};

static int cmpr(const void *e1, const void *e2)
{
	struct lw_cmdline_options **o1, **o2;
	o1 = (struct lw_cmdline_options **)e1;
	o2 = (struct lw_cmdline_options **)e2;
	
	return strcmp((*o1) -> name ? (*o1) -> name : "", (*o2) -> name ? (*o2) -> name : "");
}

static int cmpr2(const void *e1, const void *e2)
{
	struct lw_cmdline_options **o1, **o2;
	o1 = (struct lw_cmdline_options **)e1;
	o2 = (struct lw_cmdline_options **)e2;

	if ((*o1) -> key < ((*o2) -> key))
		return -1;
	if ((*o1) -> key > ((*o2) -> key))
		return 1;
	return 0;
}

static void lw_cmdline_usage(struct lw_cmdline_parser *parser, char *name)
{
	struct lw_cmdline_options **slist, **llist;
	int nopt;
	int i;
	int t;
	int col;
		
	for (nopt = 0; parser -> options[nopt].name || parser -> options[nopt].key || parser -> options[nopt].doc; nopt++)
		/* do nothing */ ;
	
	slist = lw_alloc(sizeof(struct lw_cmdline_options *) * (nopt + 3));
	llist = lw_alloc(sizeof(struct lw_cmdline_options *) * (nopt + 3));	

	for (i = 0; i < nopt; i++)
	{
		slist[i] = &(parser -> options[i]);
		llist[i] = &(parser -> options[i]);
	}
	
	/* now sort the two lists */
	qsort(slist, nopt, sizeof(struct lw_cmdline_options *), cmpr2);
	qsort(llist, nopt, sizeof(struct lw_cmdline_options *), cmpr);
	
	/* now append the automatic options */
	slist[nopt] = &(builtin[0]);
	slist[nopt + 1] = &(builtin[1]);
	slist[nopt + 2] = &(builtin[2]);

	llist[nopt] = &(builtin[0]);
	llist[nopt + 1] = &(builtin[1]);
	llist[nopt + 2] = &(builtin[2]);
	
	/* now show the usage message */
	printf("Usage: %s", name);
	
	col = 7 + strlen(name);
	
	/* print short options that take no args */
	t = 0;
	for (i = 0; i < nopt + 3; i++)
	{
		if (slist[i]->flags & lw_cmdline_opt_hidden)
			continue;
		if (slist[i]->key > 0x20 && slist[i]->key < 0x7f)
		{
			if (slist[i]->arg == NULL)
			{
				if (!t)
				{
					printf(" [-");
					t = 1;
					col += 3;
				}
				printf("%c", slist[i]->key);
				col++;
			}
		}
	}
	if (t)
	{
		col++;
		printf("]");
	}
	
	/* print short options that take args */
	for (i = 0; i < nopt + 3; i++)
	{
		if (slist[i]->flags & lw_cmdline_opt_hidden)
			continue;
		if (slist[i]->key > 0x20 && slist[i]->key < 0x7f && slist[i] -> arg)
		{
			if (slist[i]->flags & lw_cmdline_opt_optional)
			{
				t = 7 + strlen(slist[i] -> arg);
				if (col + t > LLEN)
				{
					printf("\n       ");
					col = 7;
				}
				printf(" [-%c[%s]]", slist[i]->key, slist[i]->arg);
				col += t;
			}
			else
			{
				t = 6 + strlen(slist[i] -> arg);
				if (col + t > LLEN)
				{
					printf("\n       ");
					col = 7;
				}
				printf(" [-%c %s]", slist[i]->key, slist[i]->arg);
				col += t;
			}
		}
	}
	
	/* print long options */
	for (i = 0; i < nopt + 3; i++)
	{
		if (slist[i]->flags & lw_cmdline_opt_hidden)
			continue;
		if (!(llist[i]->name))
			continue;
		if (llist[i]->arg)
		{
			t = strlen(llist[i] -> name) + 6 + strlen(llist[i] -> arg);
			if (llist[i] -> flags & lw_cmdline_opt_optional)
				t += 2;
			if (col + t > LLEN)
			{
				printf("\n       ");
				col = 7;
			}
			if (llist[i] -> flags & lw_cmdline_opt_doc)
			{
				printf(" [%s=%s]", llist[i] -> name, llist[i] -> arg);
				t = strlen(llist[i] -> name) + strlen(llist[i] -> arg) + 3;
			}
			else
			{
				printf(" [--%s%s=%s%s]", 
					llist[i] -> name,
					(llist[i] -> flags & lw_cmdline_opt_optional) ? "[" : "",
					llist[i] -> arg,
					(llist[i] -> flags & lw_cmdline_opt_optional) ? "]" : "");
			}
			col += t;
		}
		else
		{
			t = strlen(llist[i] -> name) + 5;
			if (col + t > LLEN)
			{
				printf("\n       ");
				col = 7;
			}
			if (llist[i] -> flags & lw_cmdline_opt_doc)
			{
				t -= 2;
				printf(" [%s]", llist[i] -> name);
			}
			else
			{
				printf(" [--%s]", llist[i] -> name);
			}
			col += t;
		}
	}
	
	/* print "non option" text */
	if (parser -> args_doc)
	{
		if (col + strlen(parser -> args_doc) + 1 > LLEN)
		{
			printf("\n       ");
		}
		printf(" %s", parser -> args_doc);
	}
	printf("\n");
	
	/* clean up scratch lists */
	lw_free(slist);
	lw_free(llist);
}

static void lw_cmdline_help(struct lw_cmdline_parser *parser, char *name)
{
	struct lw_cmdline_options **llist;
	int nopt;
	int i;
	char *tstr;
	int col = 0;
	int noequ;
	
	tstr = parser -> doc;
	for (nopt = 0; parser -> options[nopt].name || parser -> options[nopt].key || parser -> options[nopt].doc; nopt++)
		/* do nothing */ ;
	
	llist = lw_alloc(sizeof(struct lw_cmdline_options *) * (nopt + 3));	

	for (i = 0; i < nopt; i++)
	{
		llist[i] = &(parser -> options[i]);
	}
	
	/* now sort the list */
	qsort(llist, nopt, sizeof(struct lw_cmdline_options *), cmpr);
	
	/* now append the automatic options */
	llist[nopt] = &(builtin[0]);
	llist[nopt + 1] = &(builtin[1]);
	llist[nopt + 2] = &(builtin[2]);
	
	/* print brief usage */
	printf("Usage: %s [OPTION...] %s\n", name, parser -> args_doc ? parser -> args_doc : "");
	if (tstr)
	{
		while (*tstr && *tstr != '\v')
			fputc(*tstr++, stdout);
		if (*tstr)
			tstr++;
	}
	fputc('\n', stdout);
	fputc('\n', stdout);

	/* display options - do it the naïve way for now */
	for (i = 0; i < (nopt + 3); i++)
	{
		if (llist[i]->flags & lw_cmdline_opt_hidden)
			continue;
		noequ = 0;
		if (llist[i] -> flags & lw_cmdline_opt_doc)
		{
			col = strlen(llist[i] -> name) + 2;
			printf("  %s", llist[i] -> name);
			noequ = 1;
		}
		else if (llist[i] -> key > 0x20 && llist[i] -> key < 0x7F)
		{
			printf("  -%c", llist[i] -> key);
			col = 5;
			if (llist[i] -> name)
			{
				col++;
				fputc(',', stdout);
			}
			fputc(' ', stdout);
		}
		else
		{
			printf("      ");
			col = 6;
		}
		if (llist[i] -> name && !(llist[i] -> flags & lw_cmdline_opt_doc))
		{
			col += 2 + strlen(llist[i] -> name);
			printf("--%s", llist[i] -> name);
		}
		if (llist[i] -> arg)
		{
			if (llist[i] -> flags & lw_cmdline_opt_optional)
			{
				col++;
				fputc('[', stdout);
			}
			if (!noequ)
			{
				fputc('=', stdout);
				col++;
			}
			printf("%s", llist[i] -> arg);
			col += strlen(llist[i] -> arg);
			if (llist[i] -> flags & lw_cmdline_opt_optional)
			{
				col++;
				fputc(']', stdout);
			}
		}
		if (llist[i] -> doc)
		{
			char *s = llist[i] -> doc;
			char *s2;
			
			while (*s && isspace(*s))
				s++;

			if (col > DOCCOL)
			{
				fputc('\n', stdout);
				col = 0;
			}
			while (*s)
			{
				while (col < (DOCCOL - 1))
				{
					fputc(' ', stdout);
					col++;
				}
				for (s2 = s; *s2 && !isspace(*s2); s2++)
					/* do nothing */ ;
				if ((col + (s2 - s) + 1) > LLEN && col >= DOCCOL)
				{
					/* next line */
					fputc('\n', stdout);
					col = 0;
					continue;
				}
				col++;
				fputc(' ', stdout);
				while (s != s2)
				{
					fputc(*s, stdout);
					col++;
					s++;
				}
				while (*s && isspace(*s))
					s++;
			}
		}
		fputc('\n', stdout);
	}

	printf("\nMandatory or optional arguments to long options are also mandatory or optional\nfor any corresponding short options.\n");

	if (*tstr)
	{
		printf("\n%s\n", tstr);
	}

	/* clean up scratch lists */
	lw_free(llist);
}

int lw_cmdline_parse(struct lw_cmdline_parser *parser, int argc, char **argv, unsigned flags, int *arg_index, void *input)
{
	int i, j, r;
	int firstarg;
	int nextarg;
	char *tstr;
	int cch;
		
	/* first, permute the argv array so that all option arguments are at the start */
	for (i = 1, firstarg = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && argv[i][1])
		{
			/* have an option arg */
			if (firstarg == i)
			{
				firstarg++;
				continue;
			}
			tstr = argv[i];
			for (j = i; j > firstarg; j--)
			{
				argv[j] = argv[j - 1];
			}
			argv[firstarg] = tstr;
			firstarg++;
			if (argv[firstarg - 1][1] == '-' && argv[firstarg - 1][2] == 0)
				break;
		}
	}

	/* now start parsing options */
	nextarg = firstarg;
	i = 1;
	cch = 0;
	while (i < firstarg)
	{
		if (cch > 0 && argv[i][cch] == 0)
		{
			i++;
			cch = 0;
			continue;
		}

		if (cch > 0)
			goto shortopt;
		
		/* skip the "--" option */
		if (argv[i][1] == '-' && argv[i][2] == 0)
			break;
		
		if (argv[i][1] == '-')
		{
			goto longopt;
		}
		
		cch = 1;
	shortopt:
		/* handle a short option here */
		
		/* automatic options */
		if (argv[i][cch] == '?')
			goto do_help;
		if (argv[i][cch] == 'V')
			goto do_version;
		/* look up key */
		for (j = 0; parser -> options[j].name || parser -> options[j].key || parser -> options[j].doc; j++)
			if (parser -> options[j].key == argv[i][cch])
				break;
		cch++;
		tstr = argv[i] + cch;
		if (*tstr == 0)
			tstr = NULL;
		if (!tstr && (parser -> options[j].flags & lw_cmdline_opt_optional) == 0)
		{
			/* only consume the next arg if the argument is optional */
			if (nextarg < argc)
				tstr = argv[nextarg];
			else
				tstr = NULL;
		}
		goto common;

	longopt:
		if (strcmp(argv[i], "--help") == 0)
			goto do_help;
		if (strcmp(argv[i], "--usage") == 0)
			goto do_usage;
		if (strcmp(argv[i], "--version") == 0)
			goto do_version;
		/* look up name */
		
		for (j = 2; argv[i][j] && argv[i][j] != '='; j++)
			/* do nothing */ ;
		tstr = lw_alloc(j - 1);
		strncpy(tstr, argv[i] + 2, j - 2);
		tstr[j - 2] = 0;
		if (argv[i][j] == '=')
			j++;
		cch = j;
		for (j = 0; parser -> options[j].name || parser -> options[j].key || parser -> options[j].doc; j++)
		{
			if (parser -> options[j].name && strcmp(parser -> options[j].name, tstr) == 0)
				break;
		}
		lw_free(tstr);
		tstr = argv[i] + cch;
		if (*tstr == 0)
			tstr = NULL;
		cch = 0;
		i++;
		
	common:
		/* j will be the offset into the option table when we get here */
		/* cch will be zero and tstr will point to the arg if it's a long option */
		/* cch will be > 0 and tstr points to the theoretical option, either within */
		/* this string or "nextarg" */
		if (parser -> options[j].name == NULL)
		{
			if (cch)
				fprintf(stderr, "Unknown option '%c'. See %s --usage.\n", argv[i][cch - 1], argv[0]);
			else
				fprintf(stderr, "Unknown option '%s'. See %s --usage.\n", argv[i - 1], argv[0]);
			return EINVAL;
		}
		if (parser -> options[j].arg)
		{
			if (tstr && cch && argv[i][cch] == 0)
				nextarg++;
			
			//if (!*tstr)
			//	tstr = NULL;
			
			/* move on to next argument if we have an arg specified */
			if (tstr && cch && argv[i][cch] != 0)
			{
				i++;
				cch = 0;
			}
			
			if (!tstr && (parser -> options[j].flags & lw_cmdline_opt_optional) == 0)
			{
				fprintf(stderr, "Option %s requires argument.\n", parser -> options[j].name);
				return EINVAL;
			}
		}
		r = (*(parser -> parser))(parser -> options[j].key, tstr, input);
		if (r != 0)
			return r;
	}
	/* handle non-option args */
	if (arg_index)
		*arg_index = nextarg;
	for (i = nextarg; i < argc; i++)
	{
		r = (*(parser -> parser))(lw_cmdline_key_arg, argv[i], input);
		if (r != 0)
			return r;
	}
	r = (*(parser -> parser))(lw_cmdline_key_end, NULL, input);
	return r;

do_help:
	lw_cmdline_help(parser, argv[0]);
	exit(0);

do_version:
	printf("%s\n", parser -> program_version);
	exit(0);

do_usage:
	lw_cmdline_usage(parser, argv[0]);
	exit(0);
}
