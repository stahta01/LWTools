/*
input.c

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

/*
This file is used to handle reading input files. It serves to encapsulate
the entire input system to make porting to different media and systems
less difficult.
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_stringlist.h>
#include <lw_string.h>
#include <lw_error.h>

#include "lwasm.h"
#include "input.h"

/*
Data type for storing input buffers
*/

#define IGNOREERROR (errno == ENOENT && (as -> preprocess || as -> flags & FLAG_DEPENDNOERR))

enum input_types_e
{
	input_type_file,			// regular file, no search path
	input_type_include,			// include path, start from "local"
	input_type_string,			// input from a string

	input_type_error			// invalid input type
};

struct input_stack_node
{
	input_stack_entry *entry;
	struct input_stack_node *next;
};

struct input_stack
{
	struct input_stack *next;
	int type;
	void *data;
	int data2;
	char *filespec;
	struct input_stack_node *stack;
};

static char *make_filename(char *p, char *f)
{
	int l;
	char *r;
	l = strlen(p) + strlen(f) + 1;
	r = lw_alloc(l + 1);
	sprintf(r, "%s/%s", p, f);
	return r;
}

#define IS	((struct input_stack *)(as -> input_data))

struct ifl *ifl_head = NULL;

int input_isinclude(asmstate_t *as)
{
	return IS->type == input_type_include;
}

static int input_isabsolute(const char *s)
{
#if defined(WIN32) || defined(WIN64)
	// aiming for the root of the current drive - treat as absolute
	if (s[0] == '/')
		return 1;
	// check for drive letter stuff
	if (!s[0] || !s[1])
		return 0;
	if (s[1] != ':')
		return 0;
	if ((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z'))
		return 1;
	return 0;
#else
	/* this is suitable for unix-like systems */
	if (s[0] == '/')
		return 1;
	return 0;
#endif
}

/* this adds real filenames that were opened to a list */
void input_add_to_resource_list(asmstate_t *as, const char *s)
{
	struct ifl *ifl;
	
	/* first see if the file is already referenced */
	for (ifl = ifl_head; ifl; ifl = ifl -> next)
	{
		if (strcmp(s, ifl -> fn) == 0)
			break;
	}
	if (!ifl)
	{
		ifl = lw_alloc(sizeof(struct ifl));
		ifl -> next = ifl_head;
		ifl_head = ifl;
		ifl -> fn = lw_strdup(s);
	}
}

void input_init(asmstate_t *as)
{
	struct input_stack *t;

	if (as -> file_dir)
		lw_stack_destroy(as -> file_dir);
	as -> file_dir = lw_stack_create(lw_free);
	as -> includelist = lw_stack_create(lw_free);
	lw_stringlist_reset(as -> input_files);
	while (IS)
	{
		t = IS;
		as -> input_data = IS -> next;
		lw_free(t);
	}
}

void input_pushpath(asmstate_t *as, char *fn)
{
	/* take apart fn into path and filename then push the path */
	/* onto the current file path stack */
	
	/* also add it to the list of files included */
	char *dn, *dp;
//	int o;

	dn = lw_strdup(fn);
	lw_stack_push(as -> includelist, dn);

	dn = lw_strdup(fn);
	dp = dn + strlen(dn);
	
	while (--dp != dn)
	{
		if (*dp == '/')
			break;
	}
	if (*dp == '/')
		*dp = '\0';
	
	if (dp == dn)
	{
		lw_free(dn);
		dn = lw_strdup(".");
		lw_stack_push(as -> file_dir, dn);
		return;
	}
	dp = lw_strdup(dn);
	lw_free(dn);
	lw_stack_push(as -> file_dir, dp);
}

void input_openstring(asmstate_t *as, char *s, char *str)
{
	struct input_stack *t;
	
	t = lw_alloc(sizeof(struct input_stack));
	t -> filespec = lw_strdup(s);

	t -> type = input_type_string;
	t -> data = lw_strdup(str);
	t -> data2 = 0;
	t -> next = IS;
	t -> stack = NULL;
	as -> input_data = t;
//	t -> filespec = lw_strdup(s);
}

void input_open(asmstate_t *as, char *s)
{
	struct input_stack *t;
	char *s2;
	char *p, *p2;

	t = lw_alloc(sizeof(struct input_stack));
	t -> filespec = lw_strdup(s);

	for (s2 = s; *s2 && (*s2 != ':'); s2++)
		/* do nothing */ ;
	if (input_isabsolute(s) || !*s2)
	{
		t -> type = input_type_file;
	}
	else
	{
		char *ts;
		
		ts = lw_strndup(s, s2 - s);
		s = s2 + 1;
		if (!strcmp(ts, "include"))
			t -> type = input_type_include;
		else if (!strcmp(ts, "file"))
			t -> type = input_type_file;
		else
			t -> type = input_type_error;
		
		lw_free(ts);
	}
	t -> next = as -> input_data;
	t -> stack = NULL;
	as -> input_data = t;
	
	switch (IS -> type)
	{
	case input_type_include:
		/* first check for absolute path and if so, skip path */
		if (input_isabsolute(s))
		{
			/* absolute path */
			IS -> data = fopen(s, "rb");
			debug_message(as, 1, "Opening (abs) %s", s);
			if (!IS -> data && !IGNOREERROR)
			{
				lw_error("Cannot open file '%s': %s\n", s, strerror(errno));
			}
			else
			{
				as -> fileerr = 1;
			}
			input_pushpath(as, s);
			input_add_to_resource_list(as, s);
			return;
		}
		
		/* relative path, check relative to "current file" directory */
		p = lw_stack_top(as -> file_dir);
		p2 = make_filename(p, s);
		debug_message(as, 1, "Open: (cd) %s\n", p2);
		IS -> data = fopen(p2, "rb");
		if (IS -> data)
		{
			input_pushpath(as, p2);
			input_add_to_resource_list(as, p2);
			lw_free(p2);
			return;
		}
		debug_message(as, 2, "Failed to open: (cd) %s (%s)\n", p2, strerror(errno));
		lw_free(p2);

		/* now check relative to entries in the search path */
		lw_stringlist_reset(as -> include_list);
		while ((p = lw_stringlist_current(as -> include_list)))
		{
			p2 = make_filename(p, s);
		debug_message(as, 1, "Open (sp): %s\n", p2);
			IS -> data = fopen(p2, "rb");
			if (IS -> data)
			{
				input_pushpath(as, p2);
				input_add_to_resource_list(as, p2);
				lw_free(p2);
				return;
			}
		debug_message(as, 2, "Failed to open: (sp) %s (%s)\n", p2, strerror(errno));
			lw_free(p2);
			lw_stringlist_next(as -> include_list);
		}
		if (IGNOREERROR)
		{
			input_pushpath(as, s);
			as -> fileerr = 1;
			return;
		}
		lw_error("Cannot open include file '%s': %s\n", s, strerror(errno));
		break;
		
	case input_type_file:
		debug_message(as, 1, "Opening (reg): %s\n", s);
		IS -> data = fopen(s, "rb");

		if (!IS -> data)
		{
			lw_error("Cannot open file '%s': %s\n", s, strerror(errno));
		}
		input_pushpath(as, s);
		input_add_to_resource_list(as, s);
		return;
	}

	lw_error("Cannot figure out how to open '%s'.\n", t -> filespec);
}

FILE *input_open_standalone(asmstate_t *as, char *s, char **rfn)
{
//	char *s2;
	FILE *fp;
	char *p, *p2;

	debug_message(as, 2, "Open file (st) %s", s);
	/* first check for absolute path and if so, skip path */
	if (input_isabsolute(s))
	{
		/* absolute path */
		debug_message(as, 2, "Open file (st abs) %s", s);
		if (as -> flags & FLAG_DEPEND)
			printf("%s\n", s);
		fp = fopen(s, "rb");
		if (!fp)
		{
			return NULL;
		}
		if (rfn)
			*rfn = lw_strdup(s);
		input_add_to_resource_list(as, s);
		return fp;
	}

	/* relative path, check relative to "current file" directory */
	p = lw_stack_top(as -> file_dir);
	p2 = make_filename(p ? p : "", s);
	debug_message(as, 2, "Open file (st cd) %s", p2);
	fp = fopen(p2, "rb");
	if (fp)
	{
		if (as -> flags & FLAG_DEPEND)
			printf("%s\n", p2);
		input_add_to_resource_list(as, p2);
		if (rfn)
			*rfn = lw_strdup(p2);
		lw_free(p2);
		return fp;
	}
	lw_free(p2);

	/* now check relative to entries in the search path */
	lw_stringlist_reset(as -> include_list);
	while ((p = lw_stringlist_current(as -> include_list)))
	{
		p2 = make_filename(p, s);
		debug_message(as, 2, "Open file (st ip) %s", p2);
		fp = fopen(p2, "rb");
		if (fp)
		{
			if (as -> flags & FLAG_DEPEND)
				printf("%s\n", p2);
			input_add_to_resource_list(as, p2);
			if (rfn)
				*rfn = lw_strdup(p2);
			lw_free(p2);
			return fp;
		}
		lw_free(p2);
		lw_stringlist_next(as -> include_list);
	}

	// last ditch output for dependencies
	if (as -> flags & FLAG_DEPEND)
	{
		p = lw_stack_top(as -> file_dir);
		p2 = make_filename(p ? p : "", s);
		printf("%s\n", p2);
		lw_free(p2);
	}
	
	return NULL;
}

char *input_readline(asmstate_t *as)
{
	char *s;
	char linebuff[2049];
	int lbloc;
	int eol = 0;
	
	/* if no file is open, open one */
nextfile:
	if (!IS) {
		s = lw_stringlist_current(as -> input_files);
		if (!s)
			return NULL;
		lw_stringlist_next(as -> input_files);
		input_open(as, s);
	}
	
	switch (IS -> type)
	{
	case input_type_file:
	case input_type_include:
		/* read from a file */
		lbloc = 0;
		for (;;)
		{
			int c, c2;
			c = EOF;
			if (IS -> data)
				c = fgetc(IS -> data);
			if (c == EOF)
			{
				if (lbloc == 0)
				{
					struct input_stack *t;
					struct input_stack_node *n;
					if (IS -> data)
						fclose(IS -> data);
					lw_free(lw_stack_pop(as -> file_dir));
					lw_free(IS -> filespec);
					t = IS -> next;
					while (IS -> stack)
					{
						n = IS -> stack;
						IS -> stack = n -> next;
						lw_free(n -> entry);
						lw_free(n);
					}
					lw_free(IS);
					as -> input_data = t;
					goto nextfile;
				}
				linebuff[lbloc] = '\0';
				eol = 1;
			}
			else if (c == '\r')
			{
				linebuff[lbloc] = '\0';
				eol = 1;
				c2 = fgetc(IS -> data);
				if (c2 == EOF)
					c = EOF;  
				else if (c2 != '\n')
					ungetc(c2, IS -> data);  
			}
			else if (c == '\n')
			{
				linebuff[lbloc] = '\0';
				eol = 1;
				c2 = fgetc(IS -> data);
				if (c2 == EOF)
					c = EOF;  
				else if (c2 != '\r')
					ungetc(c2, IS -> data);  
			}
			else
			{
				if (lbloc < 2048)
					linebuff[lbloc++] = c;
			}
			if (eol)
			{
				s = lw_strdup(linebuff);
				return s;
			}
		}

	case input_type_string:
		/* read from a string */
		if (((char *)(IS -> data))[IS -> data2] == '\0')
		{
			struct input_stack *t;
			struct input_stack_node *n;
			lw_free(IS -> data);
			lw_free(IS -> filespec);
			t = IS -> next;
			while (IS -> stack)
			{
				n = IS -> stack;
				IS -> stack = n -> next;
				lw_free(n -> entry);
				lw_free(n);
			}
			lw_free(IS);
			as -> input_data = t;
			goto nextfile;
		}
		s = (char *)(IS -> data);
		lbloc = 0;
		for (;;)
		{
			int c;
			c = s[IS -> data2];
			if (c)
				IS -> data2++;
			if (c == '\0')
			{
				linebuff[lbloc] = '\0';
				eol = 1;
			}
			else if (c == '\r')
			{
				linebuff[lbloc] = '\0';
				eol = 1;
				if (s[IS -> data2] == '\n')
					IS -> data2++;
			}
			else if (c == '\n')
			{
				linebuff[lbloc] = '\0';
				eol = 1;
				if (s[IS -> data2] == '\r')
					IS -> data2++;
			}
			else
			{
				if (lbloc < 2048)
					linebuff[lbloc++] = c;
			}
			if (eol)
			{
				s = lw_strdup(linebuff);
				return s;
			}
		}
	
	default:
		lw_error("Problem reading from unknown input type\n");
		return NULL;
	}
}

char *input_curspec(asmstate_t *as)
{
	if (IS)
		return IS -> filespec;
	return NULL;
}

void input_stack_push(asmstate_t *as, input_stack_entry *e)
{
	struct input_stack_node *n;

	n = lw_alloc(sizeof(struct input_stack_node));	
	n -> next = IS -> stack;
	n -> entry = e;
	IS -> stack = n;
}

input_stack_entry *input_stack_pop(asmstate_t *as, int magic, int (*fn)(input_stack_entry *e, void *data), void *data)
{
	struct input_stack_node *n, *n2;
	input_stack_entry *e2;
	
	n2 = NULL;
	for (n = IS -> stack; n; n = n -> next)
	{
		if (n -> entry -> magic == magic)
		{
			if ((*fn)(n -> entry, data))
			{
				/* we have a match */
				e2 = n -> entry;
				if (n2)
					n2 -> next = n -> next;
				else
					IS -> stack = n -> next;
				lw_free(n);
				return e2;
			}
		}
		n2 = n;
	}
	return NULL;
}
