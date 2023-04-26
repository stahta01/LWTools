/*
cycle.c

Copyright (c) 2015 Erik Gavriluk
Dual-licensed BSD 3-CLAUSE and GPL as per below.
Thanks to John Kowalksi for cycle count verification.

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

#include "lwasm.h"

typedef struct
{
	int opc;
	int cycles_6809;
	int cycles_6309;
	cycle_flags flags;
} cycletable_t;

static cycletable_t cycletable[] =
{
	{ 0x3a, 3, 1, 0 },					// ABX

	{ 0x89, 2, 2, 0 },					// ADCA
	{ 0x99, 4, 3, 0 },
	{ 0xa9, 4, 4, CYCLE_ADJ },
	{ 0xb9, 5, 4, 0 },

	{ 0xc9, 2, 2, 0 },					// ADCB
	{ 0xd9, 4, 3, 0 },
	{ 0xe9, 4, 4, CYCLE_ADJ },
	{ 0xf9, 5, 4, 0 },

	{ 0x1089, 5, 4, 0 },				// ADCD
	{ 0x1099, 7, 5, 0 },
	{ 0x10a9, 7, 6, CYCLE_ADJ },
	{ 0x10b9, 8, 6, 0 },

	{ 0x8b, 2, 2, 0 },					// ADDA
	{ 0x9b, 4, 3, 0 },
	{ 0xab, 4, 4, CYCLE_ADJ },
	{ 0xbb, 5, 4, 0 },

	{ 0xcb, 2, 2, 0 },					// ADDB
	{ 0xdb, 4, 3, 0 },
	{ 0xeb, 4, 4, CYCLE_ADJ },
	{ 0xfb, 5, 4, 0 },

	{ 0xc3, 4, 3, 0 },					// ADDD
	{ 0xd3, 6, 4, 0 },
	{ 0xe3, 6, 5, CYCLE_ADJ },
	{ 0xf3, 7, 5, 0 },

	{ 0x118b, 3, 3, 0 },				// ADDE
	{ 0x119b, 5, 4, 0 },
	{ 0x11ab, 5, 5, CYCLE_ADJ },
	{ 0x11bb, 6, 5, 0 },

	{ 0x11cb, 3, 3, 0 },				// ADDF
	{ 0x11db, 5, 4, 0 },
	{ 0x11eb, 5, 5, CYCLE_ADJ },
	{ 0x11fb, 6, 5, 0 },

	{ 0x1031, 4, 4, 0 },				// ADCR
	{ 0x1030, 4, 4, 0 },				// ADDR
	{ 0x1034, 4, 4, 0 },				// ANDR
	{ 0x1037, 4, 4, 0 },				// CMPR
	{ 0x1036, 4, 4, 0 },				// EORR
	{ 0x1035, 4, 4, 0 },				// ORR
	{ 0x1033, 4, 4, 0 },				// SBCR
	{ 0x1032, 4, 4, 0 },				// SUBR

	{ 0x108b, 5, 4, 0 },				// ADDW
	{ 0x109b, 7, 5, 0 },
	{ 0x10ab, 7, 6, CYCLE_ADJ },
	{ 0x10bb, 8, 6, 0 },

	{ 0x02, 6, 6, 0 },					// AIM
	{ 0x62, 7, 7, CYCLE_ADJ },
	{ 0x72, 7, 7, 0 },

	{ 0x84, 2, 2, 0 },					// ANDA
	{ 0x94, 4, 3, 0 },
	{ 0xa4, 4, 4, CYCLE_ADJ },
	{ 0xb4, 5, 4, 0 },

	{ 0xc4, 2, 2, 0 },					// ANDB
	{ 0xd4, 4, 3, 0 },
	{ 0xe4, 4, 4, CYCLE_ADJ },
	{ 0xf4, 5, 4, 0 },

	{ 0x1c, 3, 2, 0 },					// ANDCC

	{ 0x1084, 5, 4, 0 },				// ANDD
	{ 0x1094, 7, 5, 0 },
	{ 0x10a4, 7, 6, CYCLE_ADJ },
	{ 0x10b4, 8, 6, 0 },

	{ 0x48, 2, 1, 0 },					// ASL
	{ 0x58, 2, 1, 0 },
	{ 0x1048, 3, 2, 0 },
	{ 0x08, 6, 5, 0 },
	{ 0x68, 6, 6, CYCLE_ADJ },
	{ 0x78, 7, 6, 0 },

	{ 0x47, 2, 1, 0 },					// ASR
	{ 0x57, 2, 1, 0 },
	{ 0x1047, 3, 2, 0 },
	{ 0x07, 6, 5, 0 },
	{ 0x67, 6, 6, CYCLE_ADJ },
	{ 0x77, 7, 6, 0 },

	{ 0x1130, 7, 6, 0 },				// BAND
	{ 0x1131, 7, 6, 0 },				// BIAND
	{ 0x1132, 7, 6, 0 },				// BOR
	{ 0x1133, 7, 6, 0 },				// BIOR
	{ 0x1134, 7, 6, 0 },				// BEOR
	{ 0x1135, 7, 6, 0 },				// BIEOR

	{ 0x85, 2, 2, 0 },					// BITA
	{ 0x95, 4, 3, 0 },
	{ 0xa5, 4, 4, CYCLE_ADJ },
	{ 0xb5, 5, 4, 0 },

	{ 0xc5, 2, 2, 0 },					// BITB
	{ 0xd5, 4, 3, 0 },
	{ 0xe5, 4, 4, CYCLE_ADJ },
	{ 0xf5, 5, 4, 0 },

	{ 0x1085, 5, 4, 0 },				// BITD
	{ 0x1095, 7, 5, 0 },
	{ 0x10a5, 7, 6, CYCLE_ADJ },
	{ 0x10b5, 8, 6, 0 },

	{ 0x113c, 4, 4, 0 },				// BITMD

	{ 0x20, 3, 3, 0 },					// BRA
	{ 0x21, 3, 3, 0 },
	{ 0x22, 3, 3, 0 },
	{ 0x23, 3, 3, 0 },
	{ 0x24, 3, 3, 0 },
	{ 0x25, 3, 3, 0 },
	{ 0x26, 3, 3, 0 },
	{ 0x27, 3, 3, 0 },
	{ 0x28, 3, 3, 0 },
	{ 0x29, 3, 3, 0 },
	{ 0x2a, 3, 3, 0 },
	{ 0x2b, 3, 3, 0 },
	{ 0x2c, 3, 3, 0 },
	{ 0x2d, 3, 3, 0 },
	{ 0x2e, 3, 3, 0 },
	{ 0x2f, 3, 3, 0 },

	{ 0x8d, 7, 6, 0 },					// BSR

	{ 0x4f, 2, 1, 0 },					// CLR
	{ 0x5f, 2, 1, 0 },
	{ 0x104f, 3, 2, 0 },
	{ 0x114f, 3, 2, 0 },
	{ 0x115f, 3, 2, 0 },
	{ 0x105f, 3, 2, 0 },
	{ 0x0f, 6, 5, 0 },
	{ 0x6f, 6, 6, CYCLE_ADJ },
	{ 0x7f, 7, 6, 0 },

	{ 0x81, 2, 2, 0 },					// CMPA
	{ 0x91, 4, 3, 0 },
	{ 0xa1, 4, 4, CYCLE_ADJ },
	{ 0xb1, 5, 4, 0 },

	{ 0xc1, 2, 2, 0 },					// CMPB
	{ 0xd1, 4, 3, 0 },
	{ 0xe1, 4, 4, CYCLE_ADJ },
	{ 0xf1, 5, 4, 0 },

	{ 0x1083, 5, 4, 0 },				// CMPD
	{ 0x1093, 7, 5, 0 },
	{ 0x10a3, 7, 6, CYCLE_ADJ },
	{ 0x10b3, 8, 6, 0 },

	{ 0x1181, 3, 3, 0 },				// CMPE
	{ 0x1191, 5, 4, 0 },
	{ 0x11a1, 5, 5, CYCLE_ADJ },
	{ 0x11b1, 6, 5, 0 },

	{ 0x11c1, 3, 3, 0 },				// CMPF
	{ 0x11d1, 5, 4, 0 },
	{ 0x11e1, 5, 5, CYCLE_ADJ },
	{ 0x11f1, 6, 5, 0 },

	{ 0x118c, 5, 4, 0 },				// CMPS
	{ 0x119c, 7, 5, 0 },
	{ 0x11ac, 7, 6, CYCLE_ADJ },
	{ 0x11bc, 8, 6, 0 },

	{ 0x1183, 5, 4, 0 },				// CMPU
	{ 0x1193, 7, 5, 0 },
	{ 0x11a3, 7, 6, CYCLE_ADJ },
	{ 0x11b3, 8, 6, 0 },

	{ 0x1081, 5, 4, 0 },				// CMPW
	{ 0x1091, 7, 5, 0 },
	{ 0x10a1, 7, 6, CYCLE_ADJ },
	{ 0x10b1, 8, 6, 0 },

	{ 0x8c, 4, 3, 0 },					// CMPX
	{ 0x9c, 6, 4, 0 },
	{ 0xac, 6, 5, CYCLE_ADJ },
	{ 0xbc, 7, 5, 0 },

	{ 0x108c, 5, 4, 0 },				// CMPY
	{ 0x109c, 7, 5, 0 },
	{ 0x10ac, 7, 6, CYCLE_ADJ },
	{ 0x10bc, 8, 6, 0 },

	{ 0x43, 2, 1, 0 },					// COMA
	{ 0x53, 2, 1, 0 },					// COMB
	{ 0x1043, 3, 2, 0 },				// COMD
	{ 0x1143, 3, 2, 0 },				// COME
	{ 0x1153, 3, 2, 0 },				// COMF
	{ 0x1053, 3, 2, 0 },				// COMW

	{ 0x03, 6, 5, 0 },					// COM
	{ 0x63, 6, 6, CYCLE_ADJ },
	{ 0x73, 7, 6, 0 },

	{ 0x3c, 22, 20, CYCLE_ESTIMATED },	// CWAI

	{ 0x19, 2, 1, 0 },					// DAA

	{ 0x4a, 2, 1, 0 },					// DECA
	{ 0x5a, 2, 1, 0 },					// DECB
	{ 0x104a, 3, 2, 0 },				// DECD
	{ 0x114a, 3, 2, 0 },				// DECE
	{ 0x115a, 3, 2, 0 },				// DECF
	{ 0x105a, 3, 2, 0 },				// DECW

	{ 0x0a, 6, 5, 0 },					// DEC
	{ 0x6a, 6, 6, CYCLE_ADJ },
	{ 0x7a, 7, 6, 0 },

	{ 0x118d, 25, 25, 0 },				// DIVD
	{ 0x119d, 27, 26, 0 },
	{ 0x11ad, 27, 27, CYCLE_ADJ },
	{ 0x11bd, 28, 27, 0 },

	{ 0x118e, 34, 34, 0 },				// DIVQ
	{ 0x119e, 36, 35, 0 },
	{ 0x11ae, 36, 36, 0 },
	{ 0x11be, 37, 36, 0 },

	{ 0x05, 6, 6, 0 },					// EIM
	{ 0x65, 7, 7, CYCLE_ADJ },
	{ 0x75, 7, 7, 0 },

	{ 0x88, 2, 2, 0 },					// EORA
	{ 0x98, 4, 3, 0 },
	{ 0xa8, 4, 4, CYCLE_ADJ },
	{ 0xb8, 5, 4, 0 },

	{ 0xc8, 2, 2, 0 },					// EORB
	{ 0xd8, 4, 3, 0 },
	{ 0xe8, 4, 4, CYCLE_ADJ },
	{ 0xf8, 5, 4, 0 },

	{ 0x1088, 5, 4, 0 },				// EORD
	{ 0x1098, 7, 5, 0 },
	{ 0x10a8, 7, 6, CYCLE_ADJ },
	{ 0x10b8, 8, 6, 0 },

	{ 0x1e, 8, 5, 0 },					// EXG

	{ 0x4c, 2, 1, 0 },					// INCA
	{ 0x5c, 2, 1, 0 },					// INCB
	{ 0x104c, 3, 2, 0 },				// INCD
	{ 0x114c, 3, 2, 0 },				// INCE
	{ 0x115c, 3, 2, 0 },				// INCF
	{ 0x105c, 3, 2, 0 },				// INCW

	{ 0x0c, 6, 5, 0 },					// INC
	{ 0x6c, 6, 6, CYCLE_ADJ },
	{ 0x7c, 7, 6, 0 },

	{ 0x0e, 3, 2, 0 },					// JMP
	{ 0x6e, 3, 3, CYCLE_ADJ },
	{ 0x7e, 4, 3, 0 },

	{ 0x9d, 7, 6, 0 },					// JSR
	{ 0xad, 7, 6, CYCLE_ADJ },
	{ 0xbd, 8, 7, 0 },

	{ 0x16, 5, 4, 0 },					// LBRA
	{ 0x17, 9, 7, 0 },					// LBSR
	{ 0x1021, 5, 5, 0 },				// LBRN
	{ 0x1022, 5, 5, 0 },				// remaining long branches are estimated on 6809 only:
	{ 0x1023, 5, 5, 0 },				//
	{ 0x1024, 5, 5, 0 },				// 6809: 5 cycles, +1 cycle if the branch is taken
	{ 0x1025, 5, 5, 0 },				// 6309 native: always 5 cycles
	{ 0x1026, 5, 5, 0 },				//
	{ 0x1027, 5, 5, 0 },				// this is handled as a special case elsewhere
	{ 0x1028, 5, 5, 0 },		
	{ 0x1029, 5, 5, 0 },		
	{ 0x102a, 5, 5, 0 },
	{ 0x102b, 5, 5, 0 },
	{ 0x102c, 5, 5, 0 },
	{ 0x102d, 5, 5, 0 },
	{ 0x102e, 5, 5, 0 },
	{ 0x102f, 5, 5, 0 },

	{ 0x86, 2, 2, 0 },					// LDA
	{ 0x96, 4, 3, 0 },
	{ 0xa6, 4, 4, CYCLE_ADJ },
	{ 0xb6, 5, 4, 0 },

	{ 0xc6, 2, 2, 0 },					// LDB
	{ 0xd6, 4, 3 },
	{ 0xe6, 4, 4, CYCLE_ADJ },
	{ 0xf6, 5, 4, 0 },

	{ 0x1136, 7, 6, 0 },				// LDBT

	{ 0xcc, 3, 3, 0 },					// LDD
	{ 0xdc, 5, 4, 0 },
	{ 0xec, 5, 5, CYCLE_ADJ },
	{ 0xfc, 6, 5, 0 },

	{ 0x1186, 3, 3, 0 },				// LDE
	{ 0x1196, 5, 4, 0 },
	{ 0x11a6, 5, 5, CYCLE_ADJ },
	{ 0x11b6, 6, 5, 0 },

	{ 0x11c6, 3, 3, 0 },				// LDF
	{ 0x11d6, 5, 4, 0 },
	{ 0x11e6, 5, 5, CYCLE_ADJ },
	{ 0x11f6, 6, 5, 0 },

	{ 0xcd, 5, 5, 0 },					// LDQ
	{ 0x10dc, 8, 7, 0 },
	{ 0x10ec, 8, 8, CYCLE_ADJ },
	{ 0x10fc, 9, 8, 0 },

	{ 0x10ce, 4, 4, 0 },				// LDS
	{ 0x10de, 6, 5, 0 },
	{ 0x10ee, 6, 6, CYCLE_ADJ },
	{ 0x10fe, 7, 6, 0 },

	{ 0xce, 3, 3, 0 },					// LDU
	{ 0xde, 5, 4, 0 },
	{ 0xee, 5, 5, CYCLE_ADJ },
	{ 0xfe, 6, 5, 0 },

	{ 0x1086, 4, 4, 0 },				// LDW
	{ 0x1096, 6, 5, 0 },
	{ 0x10a6, 6, 6, CYCLE_ADJ },
	{ 0x10b6, 7, 6, 0 },

	{ 0x8e, 3, 3, 0 },					// LDX
	{ 0x9e, 5, 4, 0 },
	{ 0xae, 5, 5, CYCLE_ADJ },
	{ 0xbe, 6, 5, 0 },

	{ 0x108e, 4, 4, 0 },				// LDY
	{ 0x109e, 6, 5, 0 },
	{ 0x10ae, 6, 6, CYCLE_ADJ },
	{ 0x10be, 7, 6, 0 },

	{ 0x113d, 5, 5, 0 },				// LDMD

	{ 0x30, 4, 4, CYCLE_ADJ },			// LEA
	{ 0x31, 4, 4, CYCLE_ADJ },
	{ 0x32, 4, 4, CYCLE_ADJ },
	{ 0x33, 4, 4, CYCLE_ADJ },

	{ 0x44, 2, 1, 0 },					// LSRA
	{ 0x54, 2, 1, 0 },					// LSRB
	{ 0x1044, 3, 2, 0 },				// LSRD
	{ 0x1054, 3, 2, 0 },				// LSRW

	{ 0x04, 6, 5, 0 },					// LSR
	{ 0x64, 6, 6, CYCLE_ADJ },
	{ 0x74, 7, 6, 0 },

	{ 0x3d, 11, 10, 0 },				// MUL

	{ 0x118f, 28, 28, 0 },				// MULD
	{ 0x119f, 30, 29, 0 },	
	{ 0x11af, 30, 30, CYCLE_ADJ },
	{ 0x11bf, 31, 30, 0 },

	{ 0x40, 2, 1, 0 },					// NEGA
	{ 0x50, 2, 1, 0 },					// NEGB
	{ 0x1040, 3, 2, 0 },				// NEGD

	{ 0x00, 6, 5, 0 },					// NEG
	{ 0x60, 6, 6, CYCLE_ADJ },
	{ 0x70, 7, 6, 0 },

	{ 0x12, 2, 1, 0 },					// NOP

	{ 0x01, 6, 6, 0 },					// OIM
	{ 0x61, 7, 7, CYCLE_ADJ },
	{ 0x71, 7, 7, 0 },

	{ 0x8a, 2, 2, 0 },					// ORA
	{ 0x9a, 4, 3, 0 },
	{ 0xaa, 4, 4, CYCLE_ADJ },
	{ 0xba, 5, 4, 0 },

	{ 0xca, 2, 2, 0 },					// ORB
	{ 0xda, 4, 3, 0 },
	{ 0xea, 4, 4, CYCLE_ADJ },
	{ 0xfa, 5, 4, 0 },

	{ 0x1a, 3, 2, 0 },					// ORCC

	{ 0x108a, 5, 4, 0 },				// ORD
	{ 0x109a, 7, 5, 0 },
	{ 0x10aa, 7, 6, CYCLE_ADJ },
	{ 0x10ba, 8, 6, 0 },

	{ 0x34, 5, 4, CYCLE_ADJ },			// PSHS
	{ 0x36, 5, 4, CYCLE_ADJ },			// PSHU
	{ 0x35, 5, 4, CYCLE_ADJ },			// PULS
	{ 0x37, 5, 4, CYCLE_ADJ },			// PULU
	{ 0x1038, 6, 6, 0 },				// PSHSW
	{ 0x103a, 6, 6, 0 },				// PSHUW
	{ 0x1039, 6, 6, 0 },				// PULSW
	{ 0x103b, 6, 6, 0 },				// PULUW

	{ 0x49, 2, 1, 0 },					// ROLA
	{ 0x59, 2, 1, 0 },					// ROLB
	{ 0x1049, 3, 2, 0 },				// ROLD
	{ 0x1059, 3, 2, 0 },				// ROLW

	{ 0x09, 6, 5, 0 },					// ROL
	{ 0x69, 6, 6, CYCLE_ADJ },
	{ 0x79, 7, 6, 0 },

	{ 0x46, 2, 1, 0 },					// RORA
	{ 0x56, 2, 1, 0 },					// RORB
	{ 0x1046, 3, 2, 0 },				// RORD
	{ 0x1056, 3, 2, 0 },				// RORW

	{ 0x06, 6, 5, 0 },					// ROR
	{ 0x66, 6, 6, CYCLE_ADJ },
	{ 0x76, 7, 6, 0 },

	{ 0x3b, 6, 17, CYCLE_ESTIMATED },	// RTI

	{ 0x39, 5, 4, 0 },					// RTS

	{ 0x82, 2, 2, 0 },					// SBCA
	{ 0x92, 4, 3, 0 },
	{ 0xa2, 4, 4, CYCLE_ADJ },
	{ 0xb2, 5, 4, 0 },

	{ 0xc2, 2, 2, 0 },					// SBCB
	{ 0xd2, 4, 3, 0 },
	{ 0xe2, 4, 4, CYCLE_ADJ },
	{ 0xf2, 5, 4, 0 },

	{ 0x1082, 5, 4, 0 },				// SBCD
	{ 0x1092, 7, 5, 0 },
	{ 0x10a2, 7, 6, CYCLE_ADJ },
	{ 0x10b2, 8, 6, 0 },

	{ 0x1d, 2, 1, 0 },					// SEX
	{ 0x14, 4, 4, 0 },					// SEXW

	{ 0x97, 4, 3, 0 },					// STA
	{ 0xa7, 4, 4, CYCLE_ADJ },
	{ 0xb7, 5, 4, 0 },

	{ 0xd7, 4, 3, 0 },					// STB
	{ 0xe7, 4, 4, CYCLE_ADJ },
	{ 0xf7, 5, 4, 0 },

	{ 0x1137, 8, 7, 0 },				// STBT

	{ 0xdd, 5, 4, 0 },					// STD
	{ 0xed, 5, 5, CYCLE_ADJ },
	{ 0xfd, 6, 5, 0 },

	{ 0x1197, 5, 4, 0 },				// STE
	{ 0x11a7, 5, 5, CYCLE_ADJ },
	{ 0x11b7, 6, 5, 0 },

	{ 0x11d7, 5, 4, 0 },				// STF
	{ 0x11e7, 5, 5, CYCLE_ADJ },
	{ 0x11f7, 6, 5, 0 },

	{ 0x10dd, 8, 7, 0 },				// STQ
	{ 0x10ed, 8, 8, CYCLE_ADJ },
	{ 0x10fd, 9, 8, 0 },

	{ 0x10df, 6, 5, 0 },				// STS
	{ 0x10ef, 6, 6, CYCLE_ADJ },
	{ 0x10ff, 7, 6, 0 },

	{ 0xdf, 5, 4, 0 },					// STU
	{ 0xef, 5, 5, CYCLE_ADJ },
	{ 0xff, 6, 5, 0 },

	{ 0x1097, 6, 5, 0 },				// STW
	{ 0x10a7, 6, 6, CYCLE_ADJ },
	{ 0x10b7, 7, 6, 0 },

	{ 0x9f, 5, 4, 0 },					// STX
	{ 0xaf, 5, 5, CYCLE_ADJ },
	{ 0xbf, 6, 5, 0 },

	{ 0x109f, 6, 5, 0 },				// STY
	{ 0x10af, 6, 6, CYCLE_ADJ },
	{ 0x10bf, 7, 6, 0 },

	{ 0x80, 2, 2, 0 },					// SUBA
	{ 0x90, 4, 3, 0 },
	{ 0xa0, 4, 4, CYCLE_ADJ },
	{ 0xb0, 5, 4, 0 },

	{ 0xc0, 2, 2, 0 },					// SUBB
	{ 0xd0, 4, 3, 0 },
	{ 0xe0, 4, 4, CYCLE_ADJ },
	{ 0xf0, 5, 4, 0 },

	{ 0x83, 4, 3, 0 },					// SUBD
	{ 0x93, 6, 4, 0 },
	{ 0xa3, 6, 5, CYCLE_ADJ },
	{ 0xb3, 7, 5, 0 },

	{ 0x1180, 3, 3, 0 },				// SUBE
	{ 0x1190, 5, 4, 0 },
	{ 0x11a0, 5, 5, CYCLE_ADJ },
	{ 0x11b0, 6, 5, 0 },

	{ 0x11c0, 3, 3, 0 },				// SUBF
	{ 0x11d0, 5, 4, 0 },
	{ 0x11e0, 5, 5, CYCLE_ADJ },
	{ 0x11f0, 6, 5, 0 },

	{ 0x1080, 5, 4, 0 },				// SUBW
	{ 0x1090, 7, 5, 0 },
	{ 0x10a0, 7, 6, CYCLE_ADJ },
	{ 0x10b0, 8, 6, 0 },

	{ 0x3f, 19, 21, 0 },				// SWI
	{ 0x103f, 20, 22, 0 },				// SWI2
	{ 0x113f, 20, 22, 0 },				// SWI3

	{ 0x13, 2, 1, CYCLE_ESTIMATED },	// SYNC

	{ 0x1f, 6, 4, 0 },					// TFR

	{ 0x0b, 6, 6, 0 },				// TIM
	{ 0x6b, 7, 7, CYCLE_ADJ },
	{ 0x7b, 7, 7, 0 },

	{ 0x1138, 6, 6, CYCLE_ESTIMATED },	// TFM
	{ 0x1139, 6, 6, CYCLE_ESTIMATED },	
	{ 0x113a, 6, 6, CYCLE_ESTIMATED },	
	{ 0x113b, 6, 6, CYCLE_ESTIMATED },	

	{ 0x4d, 2, 1, 0 },					// TSTA
	{ 0x5d, 2, 1, 0 },					// TSTB
	{ 0x104d, 3, 2, 0 },				// TSTD
	{ 0x114d, 3, 2, 0 },				// TSTE
	{ 0x115d, 3, 2, 0 },				// TSTF
	{ 0x105d, 3, 2, 0 },				// TSTW

	{ 0x0d, 6, 4, 0 },					// TST
	{ 0x6d, 6, 5, CYCLE_ADJ },
	{ 0x7d, 7, 5, 0 },

	{ -1, -1 }
};

typedef struct
{
	int cycles_6809_indexed;
	int cycles_6309_indexed;
	int cycles_6809_indirect;
	int cycles_6309_indirect;
} indtab_t;

static indtab_t indtab[] =
{
	{ 2, 1, -1, -1, },	// 0000		,R+   
	{ 3, 2, 6, 6, },	// 0001		,R++	[,R++]
	{ 2, 1, -1, -1, },	// 0010		,-R   
	{ 3, 2, 6, 6, },	// 0011		,--R	[,--R]
	{ 0, 0, 3, 3, },	// 0100		,R		[,R]
	{ 1, 1, 4, 4, },	// 0101		B,R		[B,R]
	{ 1, 1, 4, 4, },	// 0110		A,R		[A,R]
	{ 1, 1, 1, 1, },	// 0111		E,R		[E,R]
	{ 1, 1, 4, 4, },	// 1000		n,R		[n,R]
	{ 4, 3, 7, 7, },	// 1001		n,R		[n,R]  (16 bit)
	{ 1, 1, 1, 1, },	// 1010		F,R		[F,R] 
	{ 4, 2, 4, 4, },	// 1011		D,R		[D,R] 
	{ 1, 1, 4, 4, },	// 1100		n,PC	[n,PC]
	{ 5, 3, 8, 8, },	// 1101		n,PC	[n,PC] (16 bit)
	{ 4, 1, 4, 4, },	// 1110		W,R		[W,R]
	{ -1, -1, 5, 5, },	// 1111				[n]	   (16 bit)
};

/* calculates additional ticks from post byte in both indexed and indirect modes */
int lwasm_cycle_calc_ind(line_t *cl)
{
	int pb = cl->pb;

	if ((pb & 0x80) == 0) /* 5 bit offset */
		return 1;

	// These need special handling because the *register* bits determine the specific operation (and, thus, cycle counts)
	if (!CURPRAGMA(cl, PRAGMA_6809))
	{
		switch (pb)
		{
		case 0x8f:  // ,W
			return 0;
		case 0xaf:  // n,W (16 bit)
			return 2;
		case 0xcf:  // ,W++
		case 0xef:  // ,--W
			return 1;
		case 0x90:  // [,W]
			return 3;
		case 0xb0: // [n,W] (16 bit)
			return 5;
		case 0xd0: // [,W++]
		case 0xf0: // [,--W]
			return 4;
		}
	}

	if (pb & 0x10) /* indirect */
		return CURPRAGMA(cl, PRAGMA_6809) ? indtab[pb & 0xf].cycles_6809_indirect : indtab[pb & 0xf].cycles_6309_indirect;
	else
		return CURPRAGMA(cl, PRAGMA_6809) ? indtab[pb & 0xf].cycles_6809_indexed : indtab[pb & 0xf].cycles_6309_indexed;
}

/* calculate additional ticks from post byte in rlist (PSHS A,B,X...) */
int lwasm_cycle_calc_rlist(line_t *cl)
{
	int i, cycles = 0;

	for (i = 0; i < 8; i++)
	{
		// 1 cycle for 8 bit regs CC-DP (bits 0-3)
		// otherwise 2
		if (cl->pb & 1 << i)
			cycles += (i <= 3) ? 1 : 2;
	}

	return cycles;
}

void lwasm_cycle_update_count(line_t *cl, int opc)
{
	int i;
	for (i = 0; 1; i++)
	{
		if (cycletable[i].opc == -1) return;

		if (cycletable[i].opc == opc)
		{
			cl->cycle_base = CURPRAGMA(cl, PRAGMA_6809) ? cycletable[i].cycles_6809 : cycletable[i].cycles_6309;
			cl->cycle_flags = cycletable[i].flags;
			cl->cycle_adj = 0;

			// long branches are estimated on 6809
			if (CURPRAGMA(cl, PRAGMA_6809) && (opc >= 0x1022 && opc <= 0x102f))
				cl->cycle_flags |= CYCLE_ESTIMATED;

			return;
		}
	}
}
