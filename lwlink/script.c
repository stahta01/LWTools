/*
script.c
Copyright © 2009 William Astle

This file is part of LWLINK.

LWLINK is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Read and parse linker scripts
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwlink.h"

// the built-in OS9 script
// the 000D bit is to handle the module header!
static char *os9_script =
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"section code load 000D\n"
	"section .text\n"
	"section data\n"
	"section .data\n"
	"section bss,bss load 0000\n"
	"section .bss,bss\n"
	"stacksize 32\n"
	"entry __start\n"
	;

// the built-in DECB target linker script
static char *decb_script =
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"section init load 2000\n"
	"section code\n"
	"section *,!bss\n"
	"section *,bss\n"
	"entry 2000\n"
	;

// the built-in SREC target linker script
static char *srec_script =
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"section init load 0400\n"
	"section code\n"
	"section *,!bss\n"
	"section *,bss\n"
	"entry __start\n"
	;

// the built-in RAW target linker script
static char *raw_script = 
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"section init load 0000\n"
	"section code\n"
	"section *,!bss\n"
	"section *,bss\n"
	;

static char *lwex0_script =
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"sectopt .ctors padafter 00,00\n"
	"sectopt .dtors padafter 00,00\n"
	"section init load 0100\n"
	"section .text\n"
	"section .data\n"
	"section .ctors_start\n"
	"section .ctors\n"
	"section .ctors_end\n"
	"section .dtors_start\n"
	"section .dtors\n"
	"section .dtors_end\n"
	"section *,!bss\n"
	"section *,bss\n"
	"entry __start\n"
	"stacksize 0100\n"		// default 256 byte stack
	;

// the "simple" script
static char *simple_script = 
	"define basesympat s_%s\n"
	"define lensympat l_%s\n"
	"section *,!bss\n"
	"section *,bss\n"
	;

linkscript_t linkscript = { 0, NULL, -1 };

void setup_script()
{
	char *script, *oscript;
	long size;

	// read the file if needed
	if (scriptfile)
	{
		FILE *f;
		long bread;
		
		f = fopen(scriptfile, "rb");
		if (!f)
		{
			fprintf(stderr, "Can't open file %s:", scriptfile);
			perror("");
			exit(1);
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		rewind(f);
		
		script = lw_alloc(size + 2);
		
		bread = fread(script, 1, size, f);
		if (bread < size)
		{
			fprintf(stderr, "Short read on file %s (%ld/%ld):", scriptfile, bread, size);
			perror("");
			exit(1);
		}
		fclose(f);
		
		script[size] = '\n';
		script[size + 1] = '\0';
	}
	else
	{
		// fetch defaults based on output mode
		switch (outformat)
		{
		case OUTPUT_RAW:
			script = raw_script;
			break;
		
		case OUTPUT_DECB:
			script = decb_script;
			break;
		
		case OUTPUT_SREC:
			script = srec_script;
			break;
		
		case OUTPUT_LWEX0:
			script = lwex0_script;
			break;
		
		case OUTPUT_OS9:
			script = os9_script;
			break;
			
		default:
			script = simple_script;
			break;
		}
		
		size = strlen(script);
		if (nscriptls)
		{
			char *rscript;
			int i;
			// prepend the "extra" script lines
			for (i = 0; i < nscriptls; i++)
				size += strlen(scriptls[i]) + 1;
			
			rscript = lw_alloc(size + 1);
			oscript = rscript;
			for (i = 0; i < nscriptls; i++)
			{
				oscript += sprintf(oscript, "%s\n", scriptls[i]);
			}
			strcpy(oscript, script);
			script = rscript;
		}
	}

	if (outformat == OUTPUT_LWEX0)
		linkscript.stacksize = 0x100;

	if (outformat == OUTPUT_OS9)
	{
		linkscript.modtype = 0x01;
		linkscript.modlang = 0x01;
		linkscript.modattr = 0x08;
		linkscript.modrev = 0x00;
		linkscript.edition = -1;
		linkscript.name = NULL;
	}

	oscript = script;
	// now parse the script file
	while (*script)
	{
		char *ptr, *ptr2, *line;

		for (ptr = script; *ptr && *ptr != '\n' && *ptr != '\r'; ptr++)
			/* do nothing */ ;
		
		line = lw_alloc(ptr - script + 1);
		memcpy(line, script, ptr - script);
		line[ptr - script] = '\0';

		// skip line terms
		for (script = ptr + 1; *script == '\n' || *script == '\r'; script++)
			/* do nothing */ ;
		
		// ignore leading whitespace
		for (ptr = line; *ptr && isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		// ignore blank lines and comments
		if (!*ptr || *ptr == '#' || *ptr == ';')
			continue;
		
		for (ptr = line; *ptr && !isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		// now ptr points to the char past the first word
		// NUL it out
		if (*ptr)
			*ptr++ = '\0';
		
		// skip spaces after the first word
		for ( ; *ptr && isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		if (!strcmp(line, "sectopt"))
		{
			char *sn;
			char *ptr3;
			sectopt_t *so;
			
			for (ptr2 = ptr; *ptr && !isspace(*ptr2); ptr2++)
				/* do nothing */ ;
			
			if (*ptr2)
				*ptr2++ = '\0';
			
			while (*ptr2 && isspace(*ptr2))
				ptr2++;
			
			// section name is at ptr
			// ptr2 is the option type
			sn = ptr;

			// now ptr2 points to the section option name
			for (ptr3 = ptr2; *ptr3 && !isspace(*ptr3); ptr3++)
				/* do nothing */ ;
			
			if (*ptr3)
				*ptr3++ = 0;
			
			while (*ptr3 && isspace(*ptr3))
				ptr3++;
			
			// now ptr3 points to option value
			for (so = section_opts; so; so = so -> next)
			{
				if (!strcmp(so -> name, sn))
					break;
			}
			
			if (!so)
			{
				so = lw_alloc(sizeof(sectopt_t));
				so -> name = lw_strdup(sn);
				so -> aftersize = 0;
				so -> afterbytes = NULL;
				so -> next = section_opts;
				section_opts = so;
			}
			
			if (!strcmp(ptr2, "padafter"))
			{
				if (so -> afterbytes)
					lw_free(so -> afterbytes);
				so -> aftersize = 0;
				
				for (;;)
				{
					int v;
					char *ptr4;

					while (*ptr3 && isspace(*ptr3))
						ptr3++;
					
					if (!ptr3)
						break;
					
					v = strtoul(ptr3, &ptr4, 16);
					if (ptr3 == ptr4)
						break;
					
					
					so -> afterbytes = lw_realloc(so -> afterbytes, so -> aftersize + 1);
					so -> afterbytes[so -> aftersize] = v;
					so -> aftersize++;
					ptr3 = ptr4;
					while (*ptr3 && isspace(*ptr3))
						ptr3++;

					if (*ptr3 != ',')
						break;

					ptr3++;
				}
			}
			else
			{
				fprintf(stderr, "%s: bad script line: %s %s\n", scriptfile, line, ptr2);
				exit(1);
			}
		}
		else if (!strcmp(line, "define"))
		{
			// parse out the definition type
			for (ptr2 = ptr; *ptr2 && !isspace(*ptr2); ptr2++)
				/* do nothing */ ;
			
			if (*ptr2)
				*ptr2++ = '\0';
			
			while (*ptr2 && isspace(*ptr2))
				ptr2++;
			
			// now ptr points to the define type
			if (!strcmp(ptr, "basesympat"))
			{
				/* section base symbol pattern */
				lw_free(linkscript.basesympat);
				linkscript.basesympat = lw_strdup(ptr2);
			}
			else if (!strcmp(ptr, "lensympat"))
			{
				/* section length symbol pattern */
				lw_free(linkscript.lensympat);
				linkscript.lensympat = lw_strdup(ptr2);
			}
			else
			{
			}
		}
		else if (!strcmp(line, "pad"))
		{
			// padding
			// parse the hex number and stow it
			linkscript.padsize = strtol(ptr, NULL, 16);
			if (linkscript.padsize < 0)
				linkscript.padsize = 0;
		}
		else if (!strcmp(line, "stacksize"))
		{
			// stack size for targets that support it
			// parse the hex number and stow it
			linkscript.padsize = strtol(ptr, NULL, 16);
			if (linkscript.stacksize < 0)
				linkscript.stacksize = 0x100;
		}
		else if (!strcmp(line, "entry"))
		{
			int eaddr;
			
			eaddr = strtol(ptr, &ptr2, 16);
			if (*ptr2)
			{
				linkscript.execaddr = -1;
				linkscript.execsym = lw_strdup(ptr);
			}
			else
			{
				linkscript.execaddr = eaddr;
				linkscript.execsym = NULL;
			}
		}
		else if (!strcmp(line, "section"))
		{
			int growsdown = 0;
			// section
			// parse out the section name and flags
			for (ptr2 = ptr; *ptr2 && !isspace(*ptr2); ptr2++)
				/* do nothing */ ;
			
			if (*ptr2)
				*ptr2++ = '\0';
			
			while (*ptr2 && isspace(*ptr2))
				ptr2++;
				
			// ptr now points to the section name and flags and ptr2
			// to the first non-space character following
			
			// then look for "load <addr>" clause
			if (*ptr2)
			{
				if (!strncmp(ptr2, "load", 4))
				{
					ptr2 += 4;
					while (*ptr2 && isspace(*ptr2))
						ptr2++;
					
				}
				else if (!strncmp(ptr2, "high", 4))
				{
					ptr2 += 4;
					while (*ptr2 && isspace(*ptr2))
						ptr2++;
					growsdown = 1;
				}
				else
				{
					fprintf(stderr, "%s: bad script\n", scriptfile);
					exit(1);
				}
			}
			else
			{
				if (linkscript.nlines > 0)
					growsdown = linkscript.lines[linkscript.nlines - 1].growsdown;
			}
			
			// now ptr2 points to the load address if there is one
			// or NUL if not
			linkscript.lines = lw_realloc(linkscript.lines, sizeof(struct scriptline_s) * (linkscript.nlines + 1));

			linkscript.lines[linkscript.nlines].noflags = 0;
			linkscript.lines[linkscript.nlines].yesflags = 0;
			linkscript.lines[linkscript.nlines].growsdown = growsdown;
			if (*ptr2)
				linkscript.lines[linkscript.nlines].loadat = strtol(ptr2, NULL, 16);
			else
				linkscript.lines[linkscript.nlines].loadat = -1;
			for (ptr2 = ptr; *ptr2 && *ptr2 != ','; ptr2++)
				/* do nothing */ ;
			if (*ptr2)
			{
				*ptr2++ = '\0';
				if (!strcmp(ptr2, "!bss"))
				{
					linkscript.lines[linkscript.nlines].noflags = SECTION_BSS;
				}
				else if (!strcmp(ptr2, "bss"))
				{
					linkscript.lines[linkscript.nlines].yesflags = SECTION_BSS;
				}
				else
				{
					fprintf(stderr, "%s: bad script\n", scriptfile);
					exit(1);
				}
			}
			if (ptr[0] == '*' && ptr[1] == '\0')
				linkscript.lines[linkscript.nlines].sectname = NULL;
			else
				linkscript.lines[linkscript.nlines].sectname = lw_strdup(ptr);
			linkscript.nlines++;
		}
		else
		{
			fprintf(stderr, "%s: bad script line: %s\n", scriptfile, line);
			exit(1);
		}
		lw_free(line);
	}
	
	if (scriptfile || nscriptls)
		lw_free(oscript);
	
	if (entrysym)
	{
			int eaddr;
			char *ptr2;
			
			lw_free(linkscript.execsym);
			
			if (entrysym[0] >= '0' && entrysym[0] <= '9')
			{
				eaddr = strtol(entrysym, &ptr2, 16);
				linkscript.execaddr = eaddr;
				linkscript.execsym = NULL;
			}
			else
			{
				linkscript.execaddr = -1;
				linkscript.execsym = lw_strdup(entrysym);
			}

	}
}
