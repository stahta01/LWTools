/*
debug.c

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

#ifndef LWASM_NODEBUG

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "lwasm.h"
#include "instab.h"

/*

Various debug utilities

*/
void dump_state(asmstate_t *as)
{
	line_t *cl;
//	exportlist_t *ex;
//	struct symtabe *s;
//	importlist_t *im;
	struct line_expr_s *le;
	lwasm_error_t *e;
	
	debug_message(as, 100, "Lines:");
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		debug_message(as, 100, "%p INSN %d (%s) LEN %d DLEN %d PRAGMA %x", cl, cl -> insn, (cl -> insn >= 0) ? instab[cl -> insn].opcode : "<none>", cl -> len, cl -> dlen, cl -> pragmas);
		debug_message(as, 100, "    ADDR: %s", lw_expr_print(cl -> addr));
		debug_message(as, 100, "    DADDR: %s", lw_expr_print(cl -> daddr));
		debug_message(as, 100, "    PB: %02X; LINT: %X; LINT2: %X", cl -> pb, cl -> lint, cl -> lint2);
		for (le = cl -> exprs; le; le = le -> next)
		{
			debug_message(as, 100, "    EXPR %d: %s", le -> id, lw_expr_print(le -> expr));
		}
		if (cl -> outputl > 0)
		{
			int i;
			for (i = 0; i < cl -> outputl; i++)
			{
				debug_message(as, 100, "    OBYTE %02X: %02X", i, cl -> output[i]);
			}
		}
		for (e = cl -> err; e; e = e -> next)
		{
			debug_message(as, 100, "    ERR: %s", e -> mess);
		}
		for (e = cl -> warn; e; e = e -> next)
		{
			debug_message(as, 100, "    WARN: %s", e -> mess);
		}
	}
}

void real_debug_message(asmstate_t *as, int level, const char *fmt, ...)
{
	va_list args;

	if (as -> debug_level < level)
		return;

	if (as -> debug_file == NULL)
		as -> debug_file = stderr;

	va_start(args, fmt);
	
	fprintf(as -> debug_file, "DEBUG %03d (%ld): ", level, (long)time(NULL));
	vfprintf(as -> debug_file, fmt, args);
	fputc('\n', as -> debug_file);

	va_end(args);
}

#endif
