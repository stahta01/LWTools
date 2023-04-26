/*
insn_inh.c
Copyright © 2010 William Astle

This file is part of LWASM.

LWASM is free software: you can redistribute it and/or modify it under the
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

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(insn_parse_inh)
{
	l -> len = OPLEN(instab[l -> insn].ops[0]);
	skip_operand(p);
}

EMITFUNC(insn_emit_inh)
{
	lwasm_emitop(l, instab[l -> insn].ops[0]);
}

PARSEFUNC(insn_parse_inh6800)
{
	// there may be two operations here so check for both
	l -> len = OPLEN(instab[l -> insn].ops[0]);
	if (instab[l -> insn].ops[1] >= 0)
		l -> len += OPLEN(instab[l -> insn].ops[1]);
	skip_operand(p);
}

EMITFUNC(insn_emit_inh6800)
{
	// there may be two operations here so check for both
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	if (instab[l -> insn].ops[1] >= 0)
		lwasm_emitop(l, instab[l -> insn].ops[1]);
	l -> cycle_base = instab[l -> insn].ops[3];
}

int negq_ops[] = { 0x10, 0x43, 0x10, 0x53, 0x10, 0x31, 0xc6, 0x10, 0x31, 0xc0 };
#define negq_size (sizeof(negq_ops)/sizeof(int))

PARSEFUNC(insn_parse_conv)
{
	int i;
	l -> len = 0;
	for (i = 0; i <= 2; i++)
	{
		if (instab[l -> insn].ops[i] >= 0)
			l -> len += OPLEN(instab[l -> insn].ops[i]);
	}

	/* negq */
	if (instab[l -> insn].ops[0] == -1)
		l -> len = negq_size;
	skip_operand(p);
}

EMITFUNC(insn_emit_conv)
{
	int i;
	for (i = 0; i <= 2; i++)
	{
		if (instab[l -> insn].ops[i] >= 0)
			lwasm_emitop(l, instab[l -> insn].ops[i]);
	}

	/* special case for negq */
	if (instab[l -> insn].ops[0] == -1)
	{
		// 1043   (2)  comd
		// 1053   (2)  comw
		// 1031C6 (4)  adcr 0,w
		// 1031C0 (4)  adcr 0,d

		for (i = 0; i < negq_size; i++)
			lwasm_emitop(l, negq_ops[i]);
	}

	l->cycle_base = instab[l -> insn].ops[3];
}