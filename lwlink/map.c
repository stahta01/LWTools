/*
map.c
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


Output information about the linking process
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>

#include "lwlink.h"

struct symliste
{
	char *name;
	char *fn;
	int addr;
	int ext;
	struct symliste *next;
};

void display_map(void)
{
	FILE *of;
	int sn;
	int std = 0;
	struct symliste *slist = NULL;
	struct symliste *ce, *pe, *ne;
	symtab_t *sym;
	int i;
	symlist_t *se;
	
	if (!strcmp(map_file, "-"))
	{
		std = 1;
		of = stdout;
	}
	else
	{
		of = fopen(map_file, "w");
		if (!of)
		{
			fprintf(stderr, "Cannot open map file - using stdout\n");
			std = 1;
			of = stdout;
		}
	}

	// display section list
	for (sn = 0; sn < nsects; sn++)
	{
		fprintf(of, "Section: %s (%s) load at %04X, length %04X\n",
				sanitize_symbol((char*)(sectlist[sn].ptr -> name)),
				sectlist[sn].ptr -> file -> filename,
				sectlist[sn].ptr -> loadaddress,
				sectlist[sn].ptr -> codesize
			);
	}

	// generate a sorted list of symbols and display it
	
	for (se = symlist; se; se = se -> next)
	{
		for (pe = NULL, ce = slist; ce; ce = ce -> next)
		{
			i = strcmp(ce -> name, se -> sym);
			if (i > 0)
			{
				break;
			}
			pe = ce;
		}
		
		ne = lw_alloc(sizeof(struct symliste));
		ne -> ext = 1;
		ne -> addr = se -> val;
		ne -> next = ce;
		ne -> name = se -> sym;
		ne -> fn = "<synthetic>";
		if (pe)
			pe -> next = ne;
		else
			slist = ne;
	}
	
	for (sn = 0; sn < nsects; sn++)
	{
		for (sym = sectlist[sn].ptr -> localsyms; sym; sym = sym -> next)
		{
			for (pe = NULL, ce = slist; ce; ce = ce -> next)
			{
				i = strcmp(ce -> name, (char *)(sym -> sym));
				if (i == 0)
				{
					i = strcmp(ce -> fn, sectlist[sn].ptr -> file -> filename);
				}
				if (i > 0)
					break;
				pe = ce;
			}
			ne = lw_alloc(sizeof(struct symliste));
			ne -> ext = 0;
			if (sectlist[sn].ptr -> flags & SECTION_CONST)
				ne -> addr = sym -> offset;
			else
				ne -> addr = sym -> offset + sectlist[sn].ptr -> loadaddress;
			ne -> next = ce;
			ne -> name = (char *)(sym -> sym);
			ne -> fn = sectlist[sn].ptr -> file -> filename;
			if (pe)
				pe -> next = ne;
			else
				slist = ne;
		}
	}
	
	for (ce = slist; ce; ce = ce -> next)
	{
		fprintf(of, "Symbol: %s (%s) = %04X\n", sanitize_symbol(ce -> name), ce -> fn, ce -> addr);
	}

	if (!std)
		fclose(of);
}
