/*
pass7.c

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

#include <stdio.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

/*
emit pass

Generate object code
*/
void do_pass7(asmstate_t *as)
{
	line_t *cl;

	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		as -> cl = cl;
		if (cl -> insn != -1)
		{
			if (instab[cl -> insn].emit)
			{
				(instab[cl -> insn].emit)(as, cl);
			}

			if (CURPRAGMA(cl, PRAGMA_TESTMODE))
			{
				char* buf;
				int len;
				lwasm_testflags_t flags;
				lwasm_errorcode_t err;

				lwasm_parse_testmode_comment(cl, &flags, &err, &len, &buf);

				if (flags == TF_ERROR && cl -> err_testmode == 0)
				{
					char s[128];
					sprintf(s, "expected %d but assembled OK", err);
					lwasm_error_testmode(cl, s, 0);
				}

				if (flags == TF_EMIT)
				{
					if (cl -> len != len) lwasm_error_testmode(cl, "incorrect assembly (wrong length)", 0);
					if (memcmp(buf, cl -> output, len) != 0) lwasm_error_testmode(cl, "incorrect assembly", 0);
					lw_free(buf);
				}
			}
		}
	}
}
