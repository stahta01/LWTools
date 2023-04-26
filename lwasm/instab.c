/*
instab.c
Copyright © 2008 William Astle

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


Contains the instruction table for assembling code
*/
#include <stdlib.h>
#include "instab.h"

// inherent
PARSEFUNC(insn_parse_inh);
#define insn_resolve_inh NULL
EMITFUNC(insn_emit_inh);

// inherent 6800 mode
PARSEFUNC(insn_parse_inh6800);
#define insn_resolve_inh6800 NULL
EMITFUNC(insn_emit_inh6800);

// register to register
PARSEFUNC(insn_parse_rtor);
#define insn_resolve_rtor NULL
EMITFUNC(insn_emit_rtor);

// TFM and variants
PARSEFUNC(insn_parse_tfmrtor);
#define insn_resolve_tfmrtor NULL
EMITFUNC(insn_emit_tfmrtor);
PARSEFUNC(insn_parse_tfm);
#define insn_resolve_tfm NULL
EMITFUNC(insn_emit_tfm);

// register list
PARSEFUNC(insn_parse_rlist);
#define insn_resolve_rlist NULL
EMITFUNC(insn_emit_rlist);

// indexed
PARSEFUNC(insn_parse_indexed);
RESOLVEFUNC(insn_resolve_indexed);
EMITFUNC(insn_emit_indexed);

// generic 32 bit immediate
PARSEFUNC(insn_parse_gen32);
RESOLVEFUNC(insn_resolve_gen32);
EMITFUNC(insn_emit_gen32);

// generic 16 bit immediate
PARSEFUNC(insn_parse_gen16);
RESOLVEFUNC(insn_resolve_gen16);
EMITFUNC(insn_emit_gen16);

// generic 8 bit immediate
PARSEFUNC(insn_parse_gen8);
RESOLVEFUNC(insn_resolve_gen8);
EMITFUNC(insn_emit_gen8);

// generic no immediate
PARSEFUNC(insn_parse_gen0);
RESOLVEFUNC(insn_resolve_gen0);
EMITFUNC(insn_emit_gen0);

// logic memory
PARSEFUNC(insn_parse_logicmem);
RESOLVEFUNC(insn_resolve_logicmem);
EMITFUNC(insn_emit_logicmem);

// 8 bit immediate only
PARSEFUNC(insn_parse_imm8);
#define insn_resolve_imm8 NULL
EMITFUNC(insn_emit_imm8);

// bit to bit ops
PARSEFUNC(insn_parse_bitbit);
#define insn_resolve_bitbit NULL
EMITFUNC(insn_emit_bitbit);

// 8 bit relative
PARSEFUNC(insn_parse_rel8);
#define insn_resolve_rel8 NULL
EMITFUNC(insn_emit_rel8);

// 16 bit relative
PARSEFUNC(insn_parse_rel16);
#define insn_resolve_rel16 NULL
EMITFUNC(insn_emit_rel16);

// generic 8/16 bit relative
PARSEFUNC(insn_parse_relgen);
RESOLVEFUNC(insn_resolve_relgen);
EMITFUNC(insn_emit_relgen);

// MACRO pseudo op
PARSEFUNC(pseudo_parse_macro);
#define pseudo_resolve_macro	NULL
#define pseudo_emit_macro NULL

// ENDM pseudo op
PARSEFUNC(pseudo_parse_endm);
#define pseudo_resolve_endm NULL
#define pseudo_emit_endm NULL

#define pseudo_parse_noop NULL
#define pseudo_resolve_noop NULL
#define pseudo_emit_noop NULL

PARSEFUNC(pseudo_parse_dts);
#define pseudo_resolve_dts NULL
EMITFUNC(pseudo_emit_dts);

PARSEFUNC(pseudo_parse_dtb);
#define pseudo_resolve_dtb NULL
EMITFUNC(pseudo_emit_dtb);

PARSEFUNC(pseudo_parse_end);
#define pseudo_resolve_end NULL
EMITFUNC(pseudo_emit_end);

PARSEFUNC(pseudo_parse_fcb);
#define pseudo_resolve_fcb NULL
EMITFUNC(pseudo_emit_fcb);

PARSEFUNC(pseudo_parse_fdb);
#define pseudo_resolve_fdb NULL
EMITFUNC(pseudo_emit_fdb);

PARSEFUNC(pseudo_parse_fdbs);
#define pseudo_resolve_fdbs NULL
EMITFUNC(pseudo_emit_fdbs);

PARSEFUNC(pseudo_parse_fqb);
#define pseudo_resolve_fqb NULL
EMITFUNC(pseudo_emit_fqb);

PARSEFUNC(pseudo_parse_fcc);
#define pseudo_resolve_fcc NULL
EMITFUNC(pseudo_emit_fcc);

PARSEFUNC(pseudo_parse_fcs);
#define pseudo_resolve_fcs NULL
EMITFUNC(pseudo_emit_fcs);

PARSEFUNC(pseudo_parse_fcn);
#define pseudo_resolve_fcn NULL
EMITFUNC(pseudo_emit_fcn);

PARSEFUNC(pseudo_parse_rmb);
RESOLVEFUNC(pseudo_resolve_rmb);
EMITFUNC(pseudo_emit_rmb);

PARSEFUNC(pseudo_parse_rmd);
RESOLVEFUNC(pseudo_resolve_rmd);
EMITFUNC(pseudo_emit_rmd);

PARSEFUNC(pseudo_parse_rmq);
RESOLVEFUNC(pseudo_resolve_rmq);
EMITFUNC(pseudo_emit_rmq);

PARSEFUNC(pseudo_parse_zmb);
RESOLVEFUNC(pseudo_resolve_zmb);
EMITFUNC(pseudo_emit_zmb);

PARSEFUNC(pseudo_parse_zmd);
RESOLVEFUNC(pseudo_resolve_zmd);
EMITFUNC(pseudo_emit_zmd);

PARSEFUNC(pseudo_parse_zmq);
RESOLVEFUNC(pseudo_resolve_zmq);
EMITFUNC(pseudo_emit_zmq);

PARSEFUNC(pseudo_parse_org);
#define pseudo_resolve_org NULL
#define pseudo_emit_org NULL

PARSEFUNC(pseudo_parse_reorg);
#define pseudo_resolve_reorg NULL
#define pseudo_emit_reorg NULL

PARSEFUNC(pseudo_parse_equ);
#define pseudo_resolve_equ NULL
#define pseudo_emit_equ NULL

PARSEFUNC(pseudo_parse_set);
#define pseudo_resolve_set NULL
#define pseudo_emit_set NULL

PARSEFUNC(pseudo_parse_setdp);
#define pseudo_resolve_setdp NULL
#define pseudo_emit_setdp NULL

PARSEFUNC(pseudo_parse_ifp1);
#define pseudo_resolve_ifp1 NULL
#define pseudo_emit_ifp1 NULL

PARSEFUNC(pseudo_parse_ifp2);
#define pseudo_resolve_ifp2 NULL
#define pseudo_emit_ifp2 NULL

PARSEFUNC(pseudo_parse_ifne);
#define pseudo_resolve_ifne NULL
#define pseudo_emit_ifne NULL

PARSEFUNC(pseudo_parse_ifeq);
#define pseudo_resolve_ifeq NULL
#define pseudo_emit_ifeq NULL

PARSEFUNC(pseudo_parse_iflt);
#define pseudo_resolve_iflt NULL
#define pseudo_emit_iflt NULL

PARSEFUNC(pseudo_parse_ifle);
#define pseudo_resolve_ifle NULL
#define pseudo_emit_ifle NULL

PARSEFUNC(pseudo_parse_ifgt);
#define pseudo_resolve_ifgt NULL
#define pseudo_emit_ifgt NULL

PARSEFUNC(pseudo_parse_ifge);
#define pseudo_resolve_ifge NULL
#define pseudo_emit_ifge NULL

PARSEFUNC(pseudo_parse_ifdef);
#define pseudo_resolve_ifdef NULL
#define pseudo_emit_ifdef NULL

PARSEFUNC(pseudo_parse_ifndef);
#define pseudo_resolve_ifndef NULL
#define pseudo_emit_ifndef NULL

PARSEFUNC(pseudo_parse_ifpragma);
#define pseudo_resolve_ifpragma NULL
#define pseudo_emit_ifpragma NULL

PARSEFUNC(pseudo_parse_ifstr);
#define pseudo_resolve_ifstr NULL
#define pseudo_emit_ifstr NULL

PARSEFUNC(pseudo_parse_endc);
#define pseudo_resolve_endc NULL
#define pseudo_emit_endc NULL

PARSEFUNC(pseudo_parse_else);
#define pseudo_resolve_else NULL
#define pseudo_emit_else NULL

PARSEFUNC(pseudo_parse_pragma);
#define pseudo_resolve_pragma NULL
#define pseudo_emit_pragma NULL

PARSEFUNC(pseudo_parse_starpragma);
#define pseudo_resolve_starpragma NULL
#define pseudo_emit_starpragma NULL

PARSEFUNC(pseudo_parse_starpragmapush);
#define pseudo_resolve_starpragmapush NULL
#define pseudo_emit_starpragmapush NULL

PARSEFUNC(pseudo_parse_starpragmapop);
#define pseudo_resolve_starpragmapop NULL
#define pseudo_emit_starpragmapop NULL

PARSEFUNC(pseudo_parse_section);
#define pseudo_resolve_section NULL
#define pseudo_emit_section NULL

PARSEFUNC(pseudo_parse_endsection);
#define pseudo_resolve_endsection NULL
#define pseudo_emit_endsection NULL

PARSEFUNC(pseudo_parse_error);
#define pseudo_resolve_error NULL
#define pseudo_emit_error NULL

PARSEFUNC(pseudo_parse_warning);
#define pseudo_resolve_warning NULL
#define pseudo_emit_warning NULL

PARSEFUNC(pseudo_parse_os9);
#define pseudo_resolve_os9 NULL
EMITFUNC(pseudo_emit_os9);

PARSEFUNC(pseudo_parse_mod);
#define pseudo_resolve_mod NULL
EMITFUNC(pseudo_emit_mod);

PARSEFUNC(pseudo_parse_emod);
#define pseudo_resolve_emod NULL
EMITFUNC(pseudo_emit_emod);

PARSEFUNC(pseudo_parse_extdep);
#define pseudo_resolve_extdep NULL
#define pseudo_emit_extdep NULL

PARSEFUNC(pseudo_parse_extern);
#define pseudo_resolve_extern NULL
#define pseudo_emit_extern NULL

PARSEFUNC(pseudo_parse_export);
#define pseudo_resolve_export NULL
#define pseudo_emit_export NULL

PARSEFUNC(pseudo_parse_includebin);
#define pseudo_resolve_includebin NULL
EMITFUNC(pseudo_emit_includebin);

PARSEFUNC(pseudo_parse_include);
#define pseudo_resolve_include NULL
#define pseudo_emit_include NULL

PARSEFUNC(pseudo_parse_align);
RESOLVEFUNC(pseudo_resolve_align);
EMITFUNC(pseudo_emit_align);

PARSEFUNC(pseudo_parse_fill);
RESOLVEFUNC(pseudo_resolve_fill);
EMITFUNC(pseudo_emit_fill);

PARSEFUNC(pseudo_parse_struct);
#define pseudo_resolve_struct NULL
#define pseudo_emit_struct NULL

PARSEFUNC(pseudo_parse_endstruct);
#define pseudo_resolve_endstruct NULL
#define pseudo_emit_endstruct NULL

// convenience ops
PARSEFUNC(insn_parse_conv);
#define insn_resolve_conv NULL
EMITFUNC(insn_emit_conv);

instab_t instab[] =
{
	// 6809 convenience instructions
	{ "asrd",		{	0x4756,	-1,		-1,		 4 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "clrd",		{	0x4f5f,	-1,		-1,		 4 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "comd",		{	0x4353,	-1,		-1,		 4 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "lsld",		{	0x5849,	-1,		-1,		 4 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "lsrd",		{	0x4456,	-1,		-1,		 4 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "negd",		{	0x4353,	0x83ff, 0xff,	 8 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },
	{ "tstd",		{	0xed7e,	-1,		-1,		 6 },	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6809conv },

	// 6309 convenience instructions
	{ "asrq",		{	0x1047,	0x1056,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "clrq",		{	0x104f,	0x105f,	-1,		 4 	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "comq",		{	0x1043,	0x1053,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "lsle",		{	0x1030,	0xee,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "lslf",		{	0x1030,	0xff,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "lslq",		{	0x1030, 0x6610,	0x49,	 6	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "lsrq",		{	0x1044,	0x1056,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "nege",		{	0x1032,	0xce,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "negf",		{	0x1032,	0xcf,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "negw",		{	0x1032,	0xc6,	-1,		 4	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "negq",		{	-1,		-1,		-1,		12	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },
	{ "tstq",		{	0x10ed,	0x7c,	-1,		 9	},	insn_parse_conv,		insn_resolve_conv,				insn_emit_conv,				lwasm_insn_is6309conv },

	// emulator extensions
	{ "break",		{ 0x113e,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_isemuext },
	{ "log",		{ 0x103e,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_isemuext },

	{ "abx",		{	0x3a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "adca",		{	0x99,	0xa9,	0xb9,	0x89},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "adcb",		{	0xd9,	0xe9,	0xf9,	0xc9},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "adcd",		{	0x1099,	0x10a9,	0x10b9,	0x1089},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "adcr",		{	0x1031,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "adda",		{	0x9b,	0xab,	0xbb,	0x8b},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "addb",		{	0xdb,	0xeb,	0xfb,	0xcb},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "addd",		{	0xd3,	0xe3,	0xf3,	0xc3},	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "adde",		{	0x119b,	0x11ab,	0x11bb,	0x118b},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "addf",		{	0x11db,	0x11eb,	0x11fb,	0x11cb},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "addr",		{	0x1030,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "addw",		{	0x109b,	0x10ab,	0x10bb,	0x108b},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "aim",		{	0x02,	0x62,	0x72,	-1	},	insn_parse_logicmem,	insn_resolve_logicmem,			insn_emit_logicmem,			lwasm_insn_is6309},
	{ "anda",		{	0x94,	0xa4,	0xb4,	0x84},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "andb",		{	0xd4,	0xe4,	0xf4,	0xc4},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "andcc",		{	0x1c,	-1,		-1,		0x1c},	insn_parse_imm8,		insn_resolve_imm8,				insn_emit_imm8,				lwasm_insn_normal},
	{ "andd",		{	0x1094,	0x10a4,	0x10b4,	0x1084},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "andr",		{	0x1034,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "asl",		{	0x08,	0x68,	0x78,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "asla",		{	0x48,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "aslb",		{	0x58,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "asld",		{	0x1048,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "asr",		{	0x07,	0x67,	0x77,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "asra",		{	0x47,	-1,		-1,		-1	}, 	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "asrb",		{	0x57,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "asrd",		{	0x1047,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},

	{ "band",		{	0x1130,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "bcc",		{	0x24,	8,		0x24,	0x1024},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bcs",		{	0x25,	8,		0x25,	0x1025},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "beor",		{	0x1134,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "beq",		{	0x27,	8,		0x27,	0x1027},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bge",		{	0x2c,	8,		0x2c,	0x102c},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bgt",		{	0x2e,	8,		0x2e,	0x102e},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bhi",		{	0x22,	8,		0x22,	0x1022},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bhs",		{	0x24,	8,		0x24,	0x1024},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "biand",		{	0x1131,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "bieor",		{	0x1135,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "bior",		{	0x1133, -1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "bita",		{	0x95,	0xa5,	0xb5,	0x85},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "bitb",		{	0xd5,	0xe5,	0xf5,	0xc5},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "bitd",		{	0x1095,	0x10a5,	0x10b5,	0x1085},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "bitmd",		{	0x113c, -1,		-1,		0x113c},insn_parse_imm8,		insn_resolve_imm8,				insn_emit_imm8,				lwasm_insn_is6309},
	{ "ble",		{	0x2f,	8,		0x2f,	0x102f},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "blo",		{	0x25,	8,		0x25,	0x1025},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bls",		{	0x23,	8,		0x23,	0x1023},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "blt",		{	0x2d,	8,		0x2d,	0x102d},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bmi",		{	0x2b,	8,		0x2b,	0x102b},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bne",		{	0x26, 	8,		0x26,	0x1026},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bor",		{	0x1132,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "bpl",		{	0x2a,	8,		0x2a,	0x102a},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bra",		{	0x20,	8,		0x20,	0x16},	insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "brn",		{	0x21,	8,		0x21,	0x1021},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bsr",		{	0x8d,	8,		0x8d,	0x17},	insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bvc",		{	0x28,	8,		0x28,	0x1028},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},
	{ "bvs",		{	0x29,	8,		0x29,	0x1029},	insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,				lwasm_insn_normal},

	{ "clr",		{	0x0f,	0x6f,	0x7f,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "clra",		{	0x4f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "clrb",		{	0x5f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "clrd",		{	0x104f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "clre",		{	0x114f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "clrf",		{	0x115f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "clrw",		{	0x105f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "cmpa",		{	0x91,	0xa1,	0xb1,	0x81},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "cmpb",		{	0xd1,	0xe1,	0xf1,	0xc1},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "cmpd",		{	0x1093,	0x10a3,	0x10b3,	0x1083},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "cmpe",		{	0x1191,	0x11a1,	0x11b1,	0x1181},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "cmpf",		{	0x11d1,	0x11e1,	0x11f1,	0x11c1},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "cmpr",		{	0x1037,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "cmps",		{	0x119c,	0x11ac,	0x11bc,	0x118c},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "cmpu",		{	0x1193,	0x11a3,	0x11b3,	0x1183},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "cmpw",		{	0x1091,	0x10a1,	0x10b1,	0x1081},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "cmpx",		{	0x9c,	0xac,	0xbc,	0x8c}, 	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "cmpy",		{	0x109c,	0x10ac,	0x10bc,	0x108c},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "com",		{	0x03,	0x63,	0x73,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "coma",		{	0x43,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "comb",		{	0x53,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "comd",		{	0x1043,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "come",		{	0x1143,	-1, 	-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "comf", 		{	0x1153, -1,		-1,		-1	}, 	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "comw",		{	0x1053,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "cwai",		{	0x3c, 	-1,		-1,		-1	},	insn_parse_imm8,		insn_resolve_imm8,				insn_emit_imm8,				lwasm_insn_normal},
	
	{ "daa",		{	0x19,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "dec",		{	0x0a,	0x6a,	0x7a,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "deca",		{	0x4a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "decb",		{	0x5a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "decd",		{	0x104a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "dece",		{	0x114a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "decf",		{	0x115a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "decw",		{	0x105a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "divd",		{	0x119d,	0x11ad,	0x11bd,	0x118d},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "divq",		{	0x119e,	0x11ae,	0x11be,	0x118e},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},

	{ "eim",		{	0x05,	0x65,	0x75,	-1	},	insn_parse_logicmem,	insn_resolve_logicmem,			insn_emit_logicmem,			lwasm_insn_is6309},
	{ "eora",		{	0x98,	0xa8,	0xb8,	0x88},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "eorb",		{	0xd8,	0xe8,	0xf8,	0xc8},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "eord",		{	0x1098,	0x10a8,	0x10b8,	0x1088},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "eorr",		{	0x1036,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "exg",		{	0x1e,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_normal},

	{ "hcf",		{	0x14,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6809},

	{ "inc",		{	0x0c,	0x6c,	0x7c,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "inca",		{	0x4c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "incb",		{	0x5c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "incd",		{	0x104c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "ince",		{	0x114c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "incf",		{	0x115c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "incw",		{	0x105c,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	
	{ "jmp",		{	0x0e,	0x6e,	0x7e,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "jsr",		{	0x9d,	0xad,	0xbd,	-1	}, 	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	
	{ "lbcc",		{	0x1024,	16,		0x24,	0x1024},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbcs",		{	0x1025,	16,		0x25,	0x1025},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbeq",		{	0x1027,	16,		0x27,	0x1027},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbge",		{	0x102c,	16,		0x2c,	0x102c},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbgt",		{	0x102e,	16,		0x2e,	0x102e},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbhi",		{	0x1022,	16,		0x22,	0x1022},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbhs",		{	0x1024,	16,		0x24,	0x1024},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lble",		{	0x102f,	16,		0x2f,	0x102f},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lblo",		{	0x1025,	16,		0x25,	0x1025},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbls",		{	0x1023,	16,		0x23,	0x1023},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lblt",		{	0x102d, 16,		0x2d,	0x102d},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbmi",		{	0x102b,	16,		0x2b,	0x102b},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbne",		{	0x1026,	16,		0x26,	0x1026},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbpl",		{	0x102a,	16,		0x2a,	0x102a},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbra",		{	0x16,	16,		0x20,	0x16},	insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbrn",		{	0x1021,	16,		0x21,	0x1021},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbsr",		{	0x17,	16,		0x8d,	0x17},	insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbvc",		{	0x1028,	16,		0x28,	0x1028},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lbvs",		{	0x1029,	16,		0x29,	0x1029},insn_parse_relgen,		insn_resolve_relgen,				insn_emit_relgen,			lwasm_insn_normal},
	{ "lda",		{	0x96,	0xa6,	0xb6,	0x86},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "ldb",		{	0xd6,	0xe6,	0xf6,	0xc6},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "ldbt",		{	0x1136,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "ldd",		{	0xdc,	0xec,	0xfc,	0xcc},	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "lde",		{	0x1196,	0x11a6,	0x11b6,	0x1186},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "ldf",		{	0x11d6,	0x11e6,	0x11f6,	0x11c6},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "ldq",		{	0x10dc,	0x10ec,	0x10fc,	0xcd},	insn_parse_gen32,		insn_resolve_gen32,				insn_emit_gen32,			lwasm_insn_is6309},
	{ "lds",		{	0x10de,	0x10ee,	0x10fe,	0x10ce},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "ldu",		{ 	0xde,	0xee,	0xfe,	0xce},	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "ldw",		{	0x1096,	0x10a6,	0x10b6,	0x1086},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "ldx",		{	0x9e,	0xae,	0xbe,	0x8e},	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "ldy",		{	0x109e,	0x10ae,	0x10be,	0x108e},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "ldmd",		{	0x113d, -1,		-1,		0x113d},insn_parse_imm8,		insn_resolve_imm8,				insn_emit_imm8,				lwasm_insn_is6309},
	{ "leas",		{	0x32,	-1,		-1,		-1	},	insn_parse_indexed,		insn_resolve_indexed,			insn_emit_indexed,			lwasm_insn_normal},
	{ "leau",		{	0x33,	-1,		-1,		-1	},	insn_parse_indexed,		insn_resolve_indexed,			insn_emit_indexed,			lwasm_insn_normal},
	{ "leax",		{	0x30,	-1,		-1,		-1	},	insn_parse_indexed,		insn_resolve_indexed,			insn_emit_indexed,			lwasm_insn_normal},
	{ "leay",		{	0x31,	-1,		-1,		-1	},	insn_parse_indexed,		insn_resolve_indexed,			insn_emit_indexed,			lwasm_insn_normal},
	{ "lsl",		{	0x08,	0x68,	0x78,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "lsla",		{	0x48,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "lslb",		{	0x58,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "lsld",		{	0x1048,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "lsr",		{	0x04,	0x64,	0x74,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "lsra",		{	0x44,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "lsrb",		{	0x54,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "lsrd",		{	0x1044,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "lsrw",		{	0x1054,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},

	{ "mul",		{	0x3d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "muld",		{	0x119f,	0x11af,	0x11bf,	0x118f},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	
	{ "neg",		{	0x00,	0x60,	0x70,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "nega",		{	0x40,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "negb",		{	0x50,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "negd",		{	0x1040,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},

	{ "nop",		{	0x12,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	
	{ "oim",		{	0x01,	0x61,	0x71,	-1	},	insn_parse_logicmem,	insn_resolve_logicmem,			insn_emit_logicmem,			lwasm_insn_is6309},
	{ "ora",		{	0x9a,	0xaa,	0xba,	0x8a},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "orb",		{	0xda,	0xea,	0xfa,	0xca},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "orcc",		{	0x1a,	-1,		-1,	0x1a	},	insn_parse_imm8,		insn_resolve_imm8,				insn_emit_imm8,				lwasm_insn_normal},
	{ "ord",		{	0x109a,	0x10aa,	0x10ba,	0x108a},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "orr",		{	0x1035,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	
	{ "pshs",		{	0x34,	-1,		-1,		-1	},	insn_parse_rlist,		insn_resolve_rlist,				insn_emit_rlist,				lwasm_insn_normal},
	{ "pshsw",		{	0x1038,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "pshu",		{	0x36,	-1,		-1,		-1	},	insn_parse_rlist,		insn_resolve_rlist,				insn_emit_rlist,				lwasm_insn_normal},
	{ "pshuw",		{	0x103a,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "puls",		{	0x35,	-1,		-1,		-1	},	insn_parse_rlist,		insn_resolve_rlist,				insn_emit_rlist,				lwasm_insn_normal},
	{ "pulsw",		{	0x1039,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "pulu",		{	0x37,	-1,		-1,		-1	},	insn_parse_rlist,		insn_resolve_rlist,				insn_emit_rlist,				lwasm_insn_normal},
	{ "puluw",		{	0x103b,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	
	{ "reset",		{	0x3e,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6809},
	{ "rhf",		{	0x14,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6809},
	{ "rol",		{	0x09,	0x69,	0x79,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "rola",		{	0x49,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "rolb",		{	0x59,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "rold",		{	0x1049,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "rolw",		{	0x1059,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "ror",		{	0x06,	0x66,	0x76,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "rora",		{	0x46,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "rorb",		{	0x56,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "rord",		{	0x1046,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "rorw",		{	0x1056,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "rti",		{	0x3b,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "rts",		{	0x39,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	
	{ "sbca",		{	0x92,	0xa2,	0xb2,	0x82},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "sbcb",		{	0xd2,	0xe2,	0xf2,	0xc2},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "sbcd",		{	0x1092,	0x10a2,	0x10b2,	0x1082},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "sbcr",		{	0x1033,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "sex",		{	0x1d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "sexw",		{	0x14,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "sta",		{	0x97,	0xa7,	0xb7,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "stb",		{	0xd7,	0xe7,	0xf7,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "stbt",		{	0x1137,	-1,		-1,		-1	},	insn_parse_bitbit,		insn_resolve_bitbit,			insn_emit_bitbit,			lwasm_insn_is6309},
	{ "std",		{	0xdd,	0xed,	0xfd,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "ste",		{	0x1197,	0x11a7,	0x11b7,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_is6309},
	{ "stf",		{	0x11d7,	0x11e7,	0x11f7,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_is6309},
	{ "stq",		{	0x10dd,	0x10ed,	0x10fd,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_is6309},
	{ "sts",		{	0x10df,	0x10ef,	0x10ff,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "stu",		{	0xdf,	0xef,	0xff,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "stw",		{	0x1097,	0x10a7,	0x10b7,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_is6309},
	{ "stx",		{	0x9f,	0xaf,	0xbf,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "sty",		{	0x109f,	0x10af,	0x10bf,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "suba",		{	0x90,	0xa0,	0xb0,	0x80},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "subb",		{	0xd0,	0xe0,	0xf0,	0xc0},	insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_normal},
	{ "subd",		{	0x93,	0xa3,	0xb3,	0x83},	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_normal},
	{ "sube",		{	0x1190,	0x11a0,	0x11b0,	0x1180},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "subf",		{	0x11d0,	0x11e0,	0x11f0,	0x11c0},insn_parse_gen8,		insn_resolve_gen8,				insn_emit_gen8,				lwasm_insn_is6309},
	{ "subr",		{	0x1032,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_is6309},
	{ "subw",		{	0x1090,	0x10a0,	0x10b0,	0x1080},insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6309},
	{ "swi",		{	0x3f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "swi2",		{	0x103f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "swi3",		{	0x113f,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "sync",		{	0x13,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	
	// note: 			r+,r+	r-,r-	r+,r	r,r+
	{ "tfm",		{	0x1138,	0x1139,	0x113a,	0x113b },insn_parse_tfm,		insn_resolve_tfm,				insn_emit_tfm,				lwasm_insn_is6309},

	// compatibility opcodes for tfm in other assemblers
	{ "copy",		{	0x1138, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "copy+",		{	0x1138, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "tfrp",		{	0x1138, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	
	{ "copy-",		{	0x1139, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "tfrm",		{	0x1139, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	
	{ "imp",		{	0x113a, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "implode",	{	0x113a, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "tfrs",		{	0x113a, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	
	{ "exp",		{	0x113b, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "expand",		{	0x113b, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},
	{ "tfrr",		{	0x113b, -1, 	-1, 	-1},	insn_parse_tfmrtor,		insn_resolve_tfmrtor,			insn_emit_tfmrtor,			lwasm_insn_is6309},

	{ "tfr",		{	0x1f,	-1,		-1,		-1	},	insn_parse_rtor,		insn_resolve_rtor,				insn_emit_rtor,				lwasm_insn_normal},
	{ "tim",		{	0x0b,	0x6b,	0x7b,	-1	},	insn_parse_logicmem,	insn_resolve_logicmem,			insn_emit_logicmem,			lwasm_insn_is6309},
	{ "tst",		{	0x0d,	0x6d,	0x7d,	-1	},	insn_parse_gen0,		insn_resolve_gen0,				insn_emit_gen0,				lwasm_insn_normal},
	{ "tsta",		{	0x4d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "tstb",		{	0x5d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_normal},
	{ "tstd",		{	0x104d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "tste",		{	0x114d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "tstf",		{	0x115d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},
	{ "tstw",		{	0x105d,	-1,		-1,		-1	},	insn_parse_inh,			insn_resolve_inh,				insn_emit_inh,				lwasm_insn_is6309},

	{ "org",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_org,		pseudo_resolve_org,				pseudo_emit_org,			lwasm_insn_normal},
	{ "reorg",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_reorg,		pseudo_resolve_reorg,			pseudo_emit_reorg,			lwasm_insn_normal},
	{ "equ",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_equ,		pseudo_resolve_equ,				pseudo_emit_equ,			lwasm_insn_setsym},
	{ "=",			{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_equ,		pseudo_resolve_equ,				pseudo_emit_equ,			lwasm_insn_setsym},

	{ "extern",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_extern,	pseudo_resolve_extern,			pseudo_emit_extern,			lwasm_insn_setsym},
	{ "external",	{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_extern,	pseudo_resolve_extern,			pseudo_emit_extern,			lwasm_insn_setsym},
	{ "import",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_extern,	pseudo_resolve_extern,			pseudo_emit_extern,			lwasm_insn_setsym},
	{ "export",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_export,	pseudo_resolve_export,			pseudo_emit_export,			lwasm_insn_setsym},
	{ "extdep",		{	-1,		-1,		-1,		-1 },	pseudo_parse_extdep,	pseudo_resolve_extdep,			pseudo_emit_extdep,			lwasm_insn_setsym},

	{ "rmb", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_rmb,		pseudo_resolve_rmb,				pseudo_emit_rmb,			lwasm_insn_struct | lwasm_insn_setdata},
	{ "rmd", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_rmd,		pseudo_resolve_rmd,				pseudo_emit_rmd,			lwasm_insn_struct | lwasm_insn_setdata},
	{ "rmw", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_rmd,		pseudo_resolve_rmd,				pseudo_emit_rmd,			lwasm_insn_struct | lwasm_insn_setdata},
	{ "rmq", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_rmq,		pseudo_resolve_rmq,				pseudo_emit_rmq,			lwasm_insn_struct | lwasm_insn_setdata},

	{ "zmb", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_zmb,		pseudo_resolve_zmb,				pseudo_emit_zmb,			lwasm_insn_normal},
	{ "bsz", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_zmb,		pseudo_resolve_zmb,				pseudo_emit_zmb,			lwasm_insn_normal},
	{ "fzb", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_zmb,		pseudo_resolve_zmb,				pseudo_emit_zmb,			lwasm_insn_normal},
	{ "zmd", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_zmd,		pseudo_resolve_zmd,				pseudo_emit_zmd,			lwasm_insn_normal},
	{ "zmq", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_zmq,		pseudo_resolve_zmq,				pseudo_emit_zmq,			lwasm_insn_normal},

	{ "fcc",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fcc,		pseudo_resolve_fcc,				pseudo_emit_fcc,			lwasm_insn_normal},
	{ "fcn",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fcn,		pseudo_resolve_fcn,				pseudo_emit_fcn,			lwasm_insn_normal},
	{ "fcs",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fcs,		pseudo_resolve_fcs,				pseudo_emit_fcs,			lwasm_insn_normal},

	{ "fcb",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fcb,		pseudo_resolve_fcb,				pseudo_emit_fcb,			lwasm_insn_normal},
	{ "fdb",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fdb,		pseudo_resolve_fdb,				pseudo_emit_fdb,			lwasm_insn_normal},
	{ "fdbs",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fdbs,		pseudo_resolve_fdbs,			pseudo_emit_fdbs,			lwasm_insn_normal},
	{ "fqb",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_fqb,		pseudo_resolve_fqb,				pseudo_emit_fqb,			lwasm_insn_normal},
	{ "end", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_end,		pseudo_resolve_end,				pseudo_emit_end,			lwasm_insn_normal},

	{ "includebin", {	-1, 	-1, 	-1, 	-1},	pseudo_parse_includebin,pseudo_resolve_includebin,		pseudo_emit_includebin,		lwasm_insn_normal},
	{ "include",	{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_include,	pseudo_resolve_include,			pseudo_emit_include,		lwasm_insn_normal},
	{ "incl",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_include,	pseudo_resolve_include,			pseudo_emit_include,		lwasm_insn_normal},
	{ "use",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_include,	pseudo_resolve_include,			pseudo_emit_include,		lwasm_insn_normal},
	
	{ "align", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_align,		pseudo_resolve_align,			pseudo_emit_align,			lwasm_insn_normal},
	{ "fill",		{	-1,		-1,		-1,		-1 },	pseudo_parse_fill,		pseudo_resolve_fill,			pseudo_emit_fill,			lwasm_insn_normal},

	{ "error",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_error,		pseudo_resolve_error,			pseudo_emit_error,			lwasm_insn_normal},
	{ "warning",	{	-1, 	-1, 	-1, 	-1},	pseudo_parse_warning,	pseudo_resolve_warning,			pseudo_emit_warning,		lwasm_insn_normal},
	{ "msg",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_warning,	pseudo_resolve_warning,			pseudo_emit_warning,		lwasm_insn_normal},

	// these are *dangerous*
	{ "ifp1",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_ifp1,		pseudo_resolve_ifp1,			pseudo_emit_ifp1,			lwasm_insn_cond},
	{ "ifp2",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_ifp2,		pseudo_resolve_ifp2,			pseudo_emit_ifp2,			lwasm_insn_cond},

	{ "ifeq",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifeq,		pseudo_resolve_ifeq,			pseudo_emit_ifeq,			lwasm_insn_cond},
	{ "ifne",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifne,		pseudo_resolve_ifne,			pseudo_emit_ifne,			lwasm_insn_cond},
	{ "if",			{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifne,		pseudo_resolve_ifne,			pseudo_emit_ifne,			lwasm_insn_cond},
	{ "ifgt",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifgt,		pseudo_resolve_ifgt,			pseudo_emit_ifgt,			lwasm_insn_cond},
	{ "ifge",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifge,		pseudo_resolve_ifge,			pseudo_emit_ifge,			lwasm_insn_cond},
	{ "iflt",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_iflt,		pseudo_resolve_iflt,			pseudo_emit_iflt,			lwasm_insn_cond},
	{ "ifle",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifle,		pseudo_resolve_ifle,			pseudo_emit_ifle,			lwasm_insn_cond},
	{ "endc",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_endc,		pseudo_resolve_endc,			pseudo_emit_endc,			lwasm_insn_cond},
	{ "endif",      {	-1,		-1,		-1,		-1},	pseudo_parse_endc,		pseudo_resolve_endc,			pseudo_emit_endc,			lwasm_insn_cond},
	{ "else",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_else,		pseudo_resolve_else,			pseudo_emit_else,			lwasm_insn_cond},
	{ "ifdef",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_ifdef,		pseudo_resolve_ifdef,			pseudo_emit_ifdef,			lwasm_insn_cond},
	{ "ifndef",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_ifndef,	pseudo_resolve_ifndef,			pseudo_emit_ifndef,			lwasm_insn_cond},
	{ "ifpragma",	{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifpragma,	pseudo_resolve_ifpragma,		pseudo_emit_ifpragma,		lwasm_insn_cond},
	{ "ifopt",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_ifpragma,	pseudo_resolve_ifpragma,		pseudo_emit_ifpragma,		lwasm_insn_cond},

	// string operations, mostly useful in macros
	{ "ifstr",		{	-1,		-1,		-1,		-1},	pseudo_parse_ifstr,		pseudo_resolve_ifstr,			pseudo_emit_ifstr,			lwasm_insn_cond},

	{ "macro",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_macro,		pseudo_resolve_macro,			pseudo_emit_macro,			lwasm_insn_cond | lwasm_insn_setsym},
	{ "macr",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_macro,		pseudo_resolve_macro,			pseudo_emit_macro,			lwasm_insn_cond | lwasm_insn_setsym},
	{ "endm",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_endm,		pseudo_resolve_endm,			pseudo_emit_endm,			lwasm_insn_cond | lwasm_insn_setsym | lwasm_insn_endm},

	{ "setdp", 		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_setdp,		pseudo_resolve_setdp,			pseudo_emit_setdp,			lwasm_insn_normal},
	{ "set",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_set,		pseudo_resolve_set,				pseudo_emit_set,			lwasm_insn_setsym},


	{ "section",	{	-1, 	-1, 	-1, 	-1},	pseudo_parse_section,	pseudo_resolve_section,			pseudo_emit_section,		lwasm_insn_normal},
	{ "sect",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_section,	pseudo_resolve_section,			pseudo_emit_section,		lwasm_insn_normal},
	{ "endsect",	{	-1, 	-1, 	-1, 	-1},	pseudo_parse_endsection,pseudo_resolve_endsection,		pseudo_emit_endsection,		lwasm_insn_normal},
	{ "endsection",	{	-1,		-1, 	-1, 	-1},	pseudo_parse_endsection,pseudo_resolve_endsection,		pseudo_emit_endsection,		lwasm_insn_normal},

	{ "struct",		{	-1,		-1,		-1,		-1},	pseudo_parse_struct,	pseudo_resolve_struct,			pseudo_emit_struct,			lwasm_insn_normal},
	{ "ends",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_endstruct,	pseudo_resolve_endstruct,		pseudo_emit_endstruct,		lwasm_insn_struct},
	{ "endstruct",	{	-1,		-1,		-1,		-1},	pseudo_parse_endstruct,	pseudo_resolve_endstruct,		pseudo_emit_endstruct,		lwasm_insn_struct},
	

	{ "pragma",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_pragma,	pseudo_resolve_pragma,			pseudo_emit_pragma,			lwasm_insn_normal},
	{ "*pragma",	{	-1, 	-1, 	-1, 	-1},	pseudo_parse_starpragma,pseudo_resolve_starpragma,		pseudo_emit_starpragma,		lwasm_insn_normal},
	{ "opt",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_starpragma,pseudo_resolve_starpragma,		pseudo_emit_starpragma,		lwasm_insn_normal},
	{ "*pragmapush",	{	-1,	-1, 	-1,	-1},	pseudo_parse_starpragmapush, pseudo_resolve_starpragmapush, pseudo_emit_starpragmapush,	lwasm_insn_normal},
	{ "*pragmapop",	{	-1,	-1, 	-1,	-1},	pseudo_parse_starpragmapop, pseudo_resolve_starpragmapop, pseudo_emit_starpragmapop,	lwasm_insn_normal},
	
	
	// for os9 target
	{ "os9",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_os9,		pseudo_resolve_os9,				pseudo_emit_os9,			lwasm_insn_normal},
	{ "mod",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_mod,		pseudo_resolve_mod,				pseudo_emit_mod,			lwasm_insn_normal},
	{ "emod",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_emod,		pseudo_resolve_emod,			pseudo_emit_emod,			lwasm_insn_normal},

	// for compatibility with gcc6809 output...

	{ ".area",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_section,	pseudo_resolve_section,			pseudo_emit_section,		lwasm_insn_normal},
	{ ".globl",		{	-1, 	-1, 	-1, 	-1}, 	pseudo_parse_export,	pseudo_resolve_export,			pseudo_emit_export,			lwasm_insn_normal},

	{ ".module",	{	-1, 	-1, 	-1, 	-1},	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	
	{ ".4byte",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fqb,		pseudo_resolve_fqb,				pseudo_emit_fqb,			lwasm_insn_normal},
	{ ".quad",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fqb,		pseudo_resolve_fqb,				pseudo_emit_fqb,			lwasm_insn_normal},
	
	{ ".word",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fdb,		pseudo_resolve_fdb,				pseudo_emit_fdb,			lwasm_insn_normal},
	{ ".dw",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fdb,		pseudo_resolve_fdb,				pseudo_emit_fdb,			lwasm_insn_normal},

	{ ".byte",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcb,		pseudo_resolve_fcb,				pseudo_emit_fcb,			lwasm_insn_normal},
	{ ".db",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcb,		pseudo_resolve_fcb,				pseudo_emit_fcb,			lwasm_insn_normal},

	{ ".ascii",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcc,		pseudo_resolve_fcc,				pseudo_emit_fcc,			lwasm_insn_normal},
	{ ".str",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcc,		pseudo_resolve_fcc,				pseudo_emit_fcc,			lwasm_insn_normal},
	
	{ ".ascis",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcs,		pseudo_resolve_fcs,				pseudo_emit_fcs,			lwasm_insn_normal},
	{ ".strs",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcs,		pseudo_resolve_fcs,				pseudo_emit_fcs,			lwasm_insn_normal},
	
	{ ".asciz",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcn,		pseudo_resolve_fcn,				pseudo_emit_fcn,			lwasm_insn_normal},
	{ ".strz",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_fcn,		pseudo_resolve_fcn,				pseudo_emit_fcn,			lwasm_insn_normal},

	{ ".blkb",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_rmb,		pseudo_resolve_rmb,				pseudo_emit_rmb,			lwasm_insn_struct | lwasm_insn_setdata},
	{ ".ds",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_rmb,		pseudo_resolve_rmb,				pseudo_emit_rmb,			lwasm_insn_struct | lwasm_insn_setdata},
	{ ".rs",		{	-1, 	-1, 	-1, 	-1},	pseudo_parse_rmb,		pseudo_resolve_rmb,				pseudo_emit_rmb,			lwasm_insn_struct | lwasm_insn_setdata},

	// for compatibility
	{ ".end", 		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_end,		pseudo_resolve_end,				pseudo_emit_end,			lwasm_insn_normal},

	// date and time stamps
	{ "dts",		{	-1,		-1,		-1,		-1 },	pseudo_parse_dts,		pseudo_resolve_dts,				pseudo_emit_dts,			lwasm_insn_normal},
	{ "dtb",		{	-1,		-1,		-1,		-1 },	pseudo_parse_dtb,		pseudo_resolve_dtb,				pseudo_emit_dtb,			lwasm_insn_normal},

	// extra ops that are ignored because they are generally only for
	// pretty printing the listing
	{ "nam",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	{ "pag",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	{ "page",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	{ "spc",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	{ "ttl",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},
	{ ".bank",		{	-1, 	-1, 	-1, 	-1 },	pseudo_parse_noop,		pseudo_resolve_noop,			pseudo_emit_noop,			lwasm_insn_normal},

	// for 6800 compatibility
	{ "aba",		{	0x3404,	0xabe0,	-1,		12 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "cba",		{	0x3404,	0xa1e0,	-1,		12 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "clc",		{	0x1cfe,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "clf",		{	0x1cbf,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "cli",		{	0x1cef,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "clif",		{	0x1caf,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "clv",		{	0x1cfd,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "cpx",		{	0x9c,	0xac,	0xbc,	0x8c}, 	insn_parse_gen16,		insn_resolve_gen16,				insn_emit_gen16,			lwasm_insn_is6800},
	{ "des",		{	0x327f,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "dex",		{	0x301f,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "dey",		{	0x313f,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "ins",		{	0x3261,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "inx",		{	0x3001,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "iny",		{	0x3121,	-1,		-1,		 5 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "sba",		{	0x3404,	0xa0e0,	-1,		12 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "sec",		{	0x1a01,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "sef",		{	0x1a40,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "sei",		{	0x1a10,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "seif",		{	0x1a50,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "sev",		{	0x1a02,	-1,		-1,		 3 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "tab",		{	0x1f89,	0x4d,	-1,		 8 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "tap",		{	0x1f8a,	-1,		-1,		 6 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "tba",		{	0x1f98,	0x4d,	-1,		 8 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "tpa",		{	0x1fa8,	-1,		-1,		 6 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "tsx",		{	0x1f41,	-1,		-1,		 6 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "txs",		{	0x1f14,	-1,		-1,		 6 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },
	{ "wai",		{	0x3cff,	-1,		-1,		22 },	insn_parse_inh6800,		insn_resolve_inh6800,			insn_emit_inh6800,			lwasm_insn_is6800 },

	// flag end of table
	{ NULL,			{	-1, 	-1, 	-1, 	-1 },	NULL,					NULL,							NULL,						lwasm_insn_normal}
};
