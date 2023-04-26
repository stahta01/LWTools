/*
output.c
Copyright © 2009, 2010 William Astle

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


Contains the code for actually outputting the assembled code
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef _MSC_VER
#include <unistd.h>  // for unlink
#endif

#include <lw_alloc.h>
#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

void write_code_raw(asmstate_t *as, FILE *of);
void write_code_decb(asmstate_t *as, FILE *of);
void write_code_BASIC(asmstate_t *as, FILE *of);
void write_code_rawrel(asmstate_t *as, FILE *of);
void write_code_obj(asmstate_t *as, FILE *of);
void write_code_os9(asmstate_t *as, FILE *of);
void write_code_hex(asmstate_t *as, FILE *of);
void write_code_srec(asmstate_t *as, FILE *of);
void write_code_ihex(asmstate_t *as, FILE *of);
void write_code_lwmod(asmstate_t *as, FILE *of);
void write_code_dragon(asmstate_t *as, FILE *of);
void write_code_abs(asmstate_t *as, FILE *of);

// this prevents warnings about not using the return value of fwrite()
// r++ prevents the "set but not used" warnings; should be optimized out
#define writebytes(s, l, c, f)	do { int r; r = fwrite((s), (l), (c), (f)); r++; } while (0)

void do_output(asmstate_t *as)
{
	FILE *of;
	
	if (as -> errorcount > 0)
	{
		fprintf(stderr, "Not doing output due to assembly errors.\n");
		return;
	}
	
	of = fopen(as -> output_file, "wb");
	if (!of)
	{
		fprintf(stderr, "Cannot open '%s' for output", as -> output_file);
		perror("");
		return;
	}

	switch (as -> output_format)
	{
	case OUTPUT_RAW:
		write_code_raw(as, of);
		break;
		
	case OUTPUT_DECB:
		write_code_decb(as, of);
		break;
	
	case OUTPUT_BASIC:
		write_code_BASIC(as, of);
		break;
		
	case OUTPUT_RAWREL:
		write_code_rawrel(as, of);
		break;
	
	case OUTPUT_OBJ:
		write_code_obj(as, of);
		break;

	case OUTPUT_OS9:
		write_code_os9(as, of);
		break;

	case OUTPUT_HEX:
		write_code_hex(as, of);
		break;
		
	case OUTPUT_SREC:
		write_code_srec(as, of);
		break;

	case OUTPUT_IHEX:
		write_code_ihex(as, of);
		break;

	case OUTPUT_LWMOD:
		write_code_lwmod(as, of);
		break;

	case OUTPUT_DRAGON:
		write_code_dragon(as, of);
		break;

	case OUTPUT_ABS:
		write_code_abs(as, of);
		break;

	default:
		fprintf(stderr, "BUG: unrecognized output format when generating output file\n");
		fclose(of);
		unlink(as -> output_file);
		return;
	}

	fclose(of);
}

int write_code_BASIC_fprintf(FILE *of, int linelength, int *linenumber, int value)
{
	// 240 should give enough room for a 5 digit value and a comma with a bit of extra
	// space in case something unusual happens without going over the 249 character
	// limit Color Basic has on input lines.
	if (linelength > 240)
	{
		fprintf(of, "\n");
		linelength = fprintf(of, "%d DATA ", *linenumber);
		*linenumber += 10;
	}
	else
	{
		linelength += fprintf(of, ",");
	}
	linelength += fprintf(of, "%d", value);

	return linelength;
}

void write_code_BASIC(asmstate_t *as, FILE *of)
{
	line_t *cl;
	line_t *startblock = as -> line_head;
	line_t *endblock;
	int linenumber, linelength, startaddress, lastaddress, address;
	int outidx;
	
	fprintf(of, "10 READ A,B\n");
	fprintf(of, "20 IF A=-1 THEN 70\n");
	fprintf(of, "30 FOR C = A TO B\n");
	fprintf(of, "40 READ D:POKE C,D\n");
	fprintf(of, "50 NEXT C\n");
	fprintf(of, "60 GOTO 10\n");
	
	if (as -> execaddr == 0)
	{
		fprintf(of, "70 END");
	}
	else
	{
		fprintf(of, "70 EXEC %d", as -> execaddr);
	}
	
	linenumber = 80;
	linelength = 255;
	
	while(startblock)
	{
		startaddress = -1;
		endblock = NULL;
		
		for (cl = startblock; cl; cl = cl -> next)
		{
			if (cl -> outputl < 0)
				continue;
		
			address = lw_expr_intval(cl -> addr);
		
			if (startaddress == -1)
			{
				startaddress = address;
				lastaddress = address + cl -> outputl - 1;
			}
			else
			{
				if (lastaddress != address - 1)
				{
					endblock = cl;
					break;
				}
				
				lastaddress += cl -> outputl;
			}
		}
	
		if (startaddress != -1)
		{
			linelength = write_code_BASIC_fprintf(of, linelength, &linenumber, startaddress);
			linelength = write_code_BASIC_fprintf(of, linelength, &linenumber, lastaddress);
	
			for (cl = startblock; cl != endblock; cl = cl -> next)
			{
				if (cl -> outputl < 0)
					continue;
		
				for (outidx=0; outidx<cl -> outputl; outidx++)
				{
					linelength = write_code_BASIC_fprintf(of, linelength, &linenumber, cl -> output[outidx]);
				}
			}
		}
	
		startblock = cl;
	}
	
	linelength = write_code_BASIC_fprintf(of, linelength, &linenumber, -1);
	linelength = write_code_BASIC_fprintf(of, linelength, &linenumber, -1);
	
	fprintf(of, "\n");
}


/*
rawrel output treats an ORG directive as an offset from the start of the
file. Undefined results will occur if an ORG directive moves the output
pointer backward. This particular implementation uses "fseek" to handle
ORG requests and to skip over RMBs.

This simple brain damanged method simply does an fseek before outputting
each instruction.
*/
void write_code_rawrel(asmstate_t *as, FILE *of)
{
	line_t *cl;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> outputl <= 0)
			continue;
		
		fseek(of, lw_expr_intval(cl -> addr), SEEK_SET);
		writebytes(cl -> output, cl -> outputl, 1, of);
	}
}

/*
raw merely writes all the bytes directly to the file as is. ORG is just a
reference for the assembler to handle absolute references. Multiple ORG
statements will produce mostly useless results

However, if a run of RMBs exists at the start that is ended by an ORG
statement, that run of RMBs will not be output as a zero fill.
*/
void write_code_raw(asmstate_t *as, FILE *of)
{
	line_t *cl;
	line_t *sl;
	
	sl = as -> line_head;
	for (cl = sl; cl; cl = cl -> next)
	{
		if (cl -> outputl > 0)
			break;
		if (instab[cl -> insn].flags & lwasm_insn_org)
			sl = cl;
	}
	for (cl = sl; cl; cl = cl -> next)
	{
		if (cl -> len > 0 && cl -> outputl < 0)
		{
			int i;
			for (i = 0; i < cl -> len; i++)
				writebytes("\0", 1, 1, of);
			continue;
		}
		else if (cl -> outputl > 0)
			writebytes(cl -> output, cl -> outputl, 1, of);
	}
}


/*
OS9 target also just writes all the bytes in order. No need for anything
else.
*/
void write_code_os9(asmstate_t *as, FILE *of)
{
	line_t *cl;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
//		if (cl -> inmod == 0)
//			continue;
		if (cl -> len > 0 && cl -> outputl == 0)
		{
			int i;
			for (i = 0; i < cl -> len; i++)
				writebytes("\0", 1, 1, of);
			continue;
		}
		else if (cl -> outputl > 0)
			writebytes(cl -> output, cl -> outputl, 1, of);
	}
}

void write_code_decb(asmstate_t *as, FILE *of)
{
	long preambloc;
	line_t *cl;
	int blocklen = -1;
	int nextcalc = -1;
	unsigned char outbuf[5];
	int caddr;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> outputl < 0)
			continue;
		caddr = lw_expr_intval(cl -> addr);
		if (caddr != nextcalc && cl -> outputl > 0)
		{
			// need preamble here
			if (blocklen > 0)
			{
				// update previous preamble if needed
				fseek(of, preambloc, SEEK_SET);
				outbuf[0] = (blocklen >> 8) & 0xFF;
				outbuf[1] = blocklen & 0xFF;
				writebytes(outbuf, 2, 1, of);
				fseek(of, 0, SEEK_END);
			}
			blocklen = 0;
			nextcalc = caddr;
			outbuf[0] = 0x00;
			outbuf[1] = 0x00;
			outbuf[2] = 0x00;
			outbuf[3] = (nextcalc >> 8) & 0xFF;
			outbuf[4] = nextcalc & 0xFF;
			preambloc = ftell(of) + 1;
			writebytes(outbuf, 5, 1, of);
		}
		nextcalc += cl -> outputl;
		writebytes(cl -> output, cl -> outputl, 1, of);
		blocklen += cl -> outputl;
	}
	if (blocklen > 0)
	{
		fseek(of, preambloc, SEEK_SET);
		outbuf[0] = (blocklen >> 8) & 0xFF;
		outbuf[1] = blocklen & 0xFF;
		writebytes(outbuf, 2, 1, of);
		fseek(of, 0, SEEK_END);
	}
	
	// now write postamble
	outbuf[0] = 0xFF;
	outbuf[1] = 0x00;
	outbuf[2] = 0x00;
	outbuf[3] = (as -> execaddr >> 8) & 0xFF;
	outbuf[4] = (as -> execaddr) & 0xFF;
	writebytes(outbuf, 5, 1, of);
}

int fetch_output_byte(line_t *cl, char *value, int *addr)
{
	static int outidx = 0;
	static int lastaddr = -2;
	
	// try to read next byte in current line's output field
	if ((cl -> outputl > 0) && (outidx < cl -> outputl))
	{
		*addr = lw_expr_intval(cl -> addr) + outidx;
		*value = *(cl -> output + outidx++);
		
		// this byte follows the previous byte (contiguous, rc = 1)
		if (*addr == lastaddr + 1)
		{
			lastaddr = *addr;
			return 1;
		}
		
		// this byte does not follow prev byte (disjoint, rc = -1)
		else 
		{
			lastaddr = *addr;
			return -1;
		}
	}

	// no (more) output from this line (rc = 0)
	else
	{
		outidx = 0;
		return 0;
	}
}


/* a simple ASCII hex file format */

void write_code_hex(asmstate_t *as, FILE *of)
{
	const int RECLEN = 16;
	
	line_t *cl;
	char outbyte;
	int outaddr;
	int rc;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
		do
		{
			rc = fetch_output_byte(cl, &outbyte, &outaddr);
			
			// if address jump or xxx0 address, start new line
			if ((rc == -1) || ((rc == 1) && (outaddr % RECLEN == 0)))
			{
				fprintf(of, "\r\n%04X:", (unsigned int)(outaddr & 0xffff));
				fprintf(of, "%02X", (unsigned char)outbyte);
				rc = -1;
			}
			if (rc == 1)
				fprintf(of, ",%02X", (unsigned char)outbyte);
		}
		while (rc);
}


/* Motorola S19 hex file format */

void write_code_srec(asmstate_t *as, FILE *of)
{
	#define SRECLEN 16
	#define HDRLEN 51
	
	line_t *cl;
	char outbyte;
	int outaddr;
	int rc;
	unsigned int i;
	int recaddr = 0;
	unsigned int recdlen = 0;
	unsigned char recdata[SRECLEN];
	int recsum;
	int reccnt = -1;
	char rechdr[HDRLEN];
	
	for (cl = as -> line_head; cl; cl = cl -> next)
		do
		{
			rc = fetch_output_byte(cl, &outbyte, &outaddr);
			
			// if address jump or xxx0 address, start new S1 record
			if ((rc == -1) || ((rc == 1) && (outaddr % SRECLEN == 0)))
			{
				// if not already done so, emit an S0 header record
				if (reccnt < 0)
				{
					// build header from version and filespec
					// e.g. "[lwtools X.Y] filename.asm"
					strcpy(rechdr, "[");
					strcat(rechdr, PACKAGE_STRING);
					strcat(rechdr, "] ");
					i = strlen(rechdr);
					strncat(rechdr, cl -> linespec, HDRLEN - 1 - i);
					recsum = strlen(rechdr) + 3;
					fprintf(of, "S0%02X0000", recsum);
					for (i = 0; i < strlen(rechdr); i++)
					{
						fprintf(of, "%02X", (unsigned char)rechdr[i]);
						recsum += (unsigned char)rechdr[i];
					}
					fprintf(of, "%02X\r\n", (unsigned char)(~recsum));
					reccnt = 0;
				}

				// flush any current S1 record before starting new one
				if (recdlen > 0)
				{
					recsum = recdlen + 3;
					fprintf(of, "S1%02X%04X", recdlen + 3, recaddr & 0xffff);
					for (i = 0; i < recdlen; i++)
					{
						fprintf(of, "%02X", (unsigned char)recdata[i]);
						recsum += (unsigned char)recdata[i];
					}
					recsum += (recaddr >> 8) & 0xFF;
					recsum += recaddr & 0xFF;
					fprintf(of, "%02X\r\n", (unsigned char)(~recsum));
					reccnt += 1;
				}
				
				// now start the new S1 record
				recdlen = 0;
				recaddr = outaddr;
				rc = 1;
			}

			// for each new byte read, add to recdata[]
			if (rc == 1)
				recdata[recdlen++] = outbyte;
		}
		while (rc);
		
	// done with all output lines, flush the final S1 record (if any)
	if (recdlen > 0)
	{
		recsum = recdlen + 3;
		fprintf(of, "S1%02X%04X", recdlen + 3, recaddr & 0xffff);
		for (i = 0; i < recdlen; i++)
		{
			fprintf(of, "%02X", (unsigned char)recdata[i]);
			recsum += (unsigned char)recdata[i];
		}
		recsum += (recaddr >> 8) & 0xFF;
		recsum += recaddr & 0xFF;
		fprintf(of, "%02X\r\n", (unsigned char)(~recsum));
		reccnt += 1;
	}

	// if any S1 records were output, close with S5 and S9 records
	if (reccnt > 0)
	{
		// emit S5 count record
		recsum = 3;
		recsum += (reccnt >> 8) & 0xFF;
		recsum += reccnt & 0xFF;
		fprintf(of, "S503%04X", (unsigned int)reccnt);
		fprintf(of, "%02X\r\n", (unsigned char)(~recsum));
		
		// emit S9 end-of-file record
		recsum = 3;
		recsum += (as -> execaddr >> 8) & 0xFF;
		recsum += (as -> execaddr) & 0xFF;
		fprintf(of, "S903%04X", as -> execaddr & 0xffff);
		fprintf(of, "%02X\r\n", (unsigned char)(~recsum));
	}
}


/* Intel hex file format */

void write_code_ihex(asmstate_t *as, FILE *of)
{
	#define IRECLEN 16
	
	line_t *cl;
	char outbyte;
	int outaddr;
	int rc;
	int i;
	int recaddr = 0;
	int recdlen = 0;
	unsigned char recdata[IRECLEN];
	int recsum;
	int reccnt = 0;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
		do
		{
			rc = fetch_output_byte(cl, &outbyte, &outaddr);
			
			// if address jump or xxx0 address, start new ihx record
			if ((rc == -1) || ((rc == 1) && (outaddr % IRECLEN == 0)))
			{
				// flush any current ihex record before starting new one
				if (recdlen > 0)
				{
					recsum = recdlen;
					fprintf(of, ":%02X%04X00", recdlen, recaddr & 0xffff);
					for (i = 0; i < recdlen; i++)
					{
						fprintf(of, "%02X", (unsigned char)recdata[i]);
						recsum += (unsigned char)recdata[i];
					}
					recsum += (recaddr >> 8) & 0xFF;
					recsum += recaddr & 0xFF;
					fprintf(of, "%02X\r\n", (unsigned char)(256 - recsum));
					reccnt += 1;
				}
				
				// now start the new ihex record
				recdlen = 0;
				recaddr = outaddr;
				rc = 1;
			}

			// for each new byte read, add to recdata[]
			if (rc == 1)
				recdata[recdlen++] = outbyte;
		}
		while (rc);
		
	// done with all output lines, flush the final ihex record (if any)
	if (recdlen > 0)
	{
		recsum = recdlen;
		fprintf(of, ":%02X%04X00", recdlen, recaddr & 0xffff);
		for (i = 0; i < recdlen; i++)
		{
			fprintf(of, "%02X", (unsigned char)recdata[i]);
			recsum += (unsigned char)recdata[i];
		}
		recsum += (recaddr >> 8) & 0xFF;
		recsum += recaddr & 0xFF;
		fprintf(of, "%02X\r\n", (unsigned char)(256 - recsum));
		reccnt += 1;
	}

	// if any ihex records were output, close with a "01" record
	if (reccnt > 0)
	{
		fprintf(of, ":00%04X01FF", as -> execaddr & 0xffff);
	}
}
	    
	    
void write_code_obj_sbadd(sectiontab_t *s, unsigned char b)
{
	if (s -> oblen >= s -> obsize)
	{
		s -> obytes = lw_realloc(s -> obytes, s -> obsize + 128);
		s -> obsize += 128;
	}
	s -> obytes[s -> oblen] = b;
	s -> oblen += 1;
}


int write_code_obj_expraux(lw_expr_t e, void *of)
{
	int tt;
	int v;
	int count = 1;
	unsigned char buf[16];
	
	tt = lw_expr_type(e);

	switch (tt)
	{
	case lw_expr_type_oper:
		buf[0] =  0x04;
		
		switch (lw_expr_whichop(e))
		{
		case lw_expr_oper_plus:
			buf[1] = 0x01;
			count = lw_expr_operandcount(e) - 1;
			break;
		
		case lw_expr_oper_minus:
			buf[1] = 0x02;
			break;
		
		case lw_expr_oper_times:
			buf[1] = 0x03;
			count = lw_expr_operandcount(e) - 1;
			break;
		
		case lw_expr_oper_divide:
			buf[1] = 0x04;
			break;
		
		case lw_expr_oper_mod:
			buf[1] = 0x05;
			break;
		
		case lw_expr_oper_intdiv:
			buf[1] = 0x06;
			break;
		
		case lw_expr_oper_bwand:
			buf[1] = 0x07;
			break;
		
		case lw_expr_oper_bwor:
			buf[1] = 0x08;
			break;
		
		case lw_expr_oper_bwxor:
			buf[1] = 0x09;
			break;
		
		case lw_expr_oper_and:
			buf[1] = 0x0A;
			break;
		
		case lw_expr_oper_or:
			buf[1] = 0x0B;
			break;
		
		case lw_expr_oper_neg:
			buf[1] = 0x0C;
			break;
		
		case lw_expr_oper_com:
			buf[1] = 0x0D;
			break;

		default:
			buf[1] = 0xff;
		}
		while (count--)
			writebytes(buf, 2, 1, of);
		return 0;

	case lw_expr_type_int:
		v = lw_expr_intval(e);
		buf[0] = 0x01;
		buf[1] = (v >> 8) & 0xff;
		buf[2] = v & 0xff;
		writebytes(buf, 3, 1, of);
		return 0;
		
	case lw_expr_type_special:
		v = lw_expr_specint(e);
		switch (v)
		{
		case lwasm_expr_secbase:
			{
				// replaced with a synthetic symbol
				sectiontab_t *se;
				se = lw_expr_specptr(e);
				
				writebytes("\x03\x02", 2, 1, of);
				writebytes(se -> name, strlen(se -> name) + 1, 1, of);
				return 0;
			}	
		case lwasm_expr_import:
			{
				importlist_t *ie;
				ie = lw_expr_specptr(e);
				buf[0] = 0x02;
				writebytes(buf, 1, 1, of);
				writebytes(ie -> symbol, strlen(ie -> symbol) + 1, 1, of);
				return 0;
			}
		case lwasm_expr_syment:
			{
				struct symtabe *se;
				se = lw_expr_specptr(e);
				buf[0] = 0x03;
				writebytes(buf, 1, 1, of);
				writebytes(se -> symbol, strlen(se -> symbol), 1, of);
				if (se -> context != -1)
				{
					sprintf((char *)buf, "\x01%d", se -> context);
					writebytes(buf, strlen((char *)buf), 1, of);
				}
				writebytes("", 1, 1, of);
				return 0;
			}
		}
			
	default:
		// unrecognized term type - replace with integer 0
//		fprintf(stderr, "Unrecognized term type: %s\n", lw_expr_print(e));
		buf[0] = 0x01;
		buf[1] = 0x00;
		buf[2] = 0x00;
		writebytes(buf, 3, 1, of);
		break;
	}
	return 0;
}

void write_code_obj_auxsym(asmstate_t *as, FILE *of, sectiontab_t *s, struct symtabe *se2)
{
	struct symtabe *se;
	unsigned char buf[16];
		
	if (!se2)
		return;
	write_code_obj_auxsym(as, of, s, se2 -> left);
	
	for (se = se2; se; se = se -> nextver)
	{
		lw_expr_t te;
		
		debug_message(as, 200, "Consider symbol %s (%p) for export in section %p", se -> symbol, se -> section, s);
		
		// ignore symbols not in this section
		if (se -> section != s)
			continue;
		
		debug_message(as, 200, "  In section");
			
		if (se -> flags & symbol_flag_set)
			continue;
			
		debug_message(as, 200, "  Not symbol_flag_set");
			
		te = lw_expr_copy(se -> value);
		debug_message(as, 200, "  Value=%s", lw_expr_print(te));
		as -> exportcheck = 1;
		as -> csect = s;
		lwasm_reduce_expr(as, te);
		as -> exportcheck = 0;

		debug_message(as, 200, "  Value2=%s", lw_expr_print(te));
			
		// don't output non-constant symbols
		if (!lw_expr_istype(te, lw_expr_type_int))
		{
			lw_expr_destroy(te);
			continue;
		}

		writebytes(se -> symbol, strlen(se -> symbol), 1, of);
		if (se -> context >= 0)
		{
			writebytes("\x01", 1, 1, of);
			sprintf((char *)buf, "%d", se -> context);
			writebytes(buf, strlen((char *)buf), 1, of);
		}
		// the "" is NOT an error
		writebytes("", 1, 1, of);
			
		// write the address
		buf[0] = (lw_expr_intval(te) >> 8) & 0xff;
		buf[1] = lw_expr_intval(te) & 0xff;
		writebytes(buf, 2, 1, of);
		lw_expr_destroy(te);
	}
	write_code_obj_auxsym(as, of, s, se2 -> right);
}

void write_code_obj(asmstate_t *as, FILE *of)
{
	line_t *l;
	sectiontab_t *s;
	reloctab_t *re;
	exportlist_t *ex;

	int i;
	unsigned char buf[16];

	// output the magic number and file header
	// the 8 is NOT an error
	writebytes("LWOBJ16", 8, 1, of);
	
	// run through the entire system and build the byte streams for each
	// section; at the same time, generate a list of "local" symbols to
	// output for each section
	// NOTE: for "local" symbols, we will append \x01 and the ascii string
	// of the context identifier (so sym in context 1 would be "sym\x011"
	// we can do this because the linker can handle symbols with any
	// character other than NUL.
	// also we will generate a list of incomplete references for each
	// section along with the actual definition that will be output
	
	// once all this information is generated, we will output each section
	// to the file
	
	// NOTE: we build everything in memory then output it because the
	// assembler accepts multiple instances of the same section but the
	// linker expects only one instance of each section in the object file
	// so we need to collect all the various pieces of a section together
	// (also, the assembler treated multiple instances of the same section
	// as continuations of previous sections so we would need to collect
	// them together anyway.
	
	for (l = as -> line_head; l; l = l -> next)
	{
		if (l -> csect)
		{
			// we're in a section - need to output some bytes
			if (l -> outputl > 0)
				for (i = 0; i < l -> outputl; i++)
					write_code_obj_sbadd(l -> csect, l -> output[i]);
			else if (l -> outputl == 0 || l -> outputl == -1)
				for (i = 0; i < l -> len; i++)
					write_code_obj_sbadd(l -> csect, 0);
		}
	}
	
	// run through the sections
	for (s = as -> sections; s; s = s -> next)
	{
		// write the name
		writebytes(s -> name, strlen(s -> name) + 1, 1, of);
		
		// write the flags
		if (s -> flags & section_flag_bss)
			writebytes("\x01", 1, 1, of);
		if (s -> flags & section_flag_constant)
			writebytes("\x02", 1, 1, of);
			
		// indicate end of flags - the "" is NOT an error
		writebytes("", 1, 1, of);
		
		// now the local symbols
		
		// a symbol for section base address
		if ((s -> flags & section_flag_constant) == 0)
		{
			writebytes("\x02", 1, 1, of);
			writebytes(s -> name, strlen(s -> name) + 1, 1, of);
			// address 0; "\0" is not an error
			writebytes("\0", 2, 1, of);
		}
		
		write_code_obj_auxsym(as, of, s, as -> symtab.head);
		// flag end of local symbol table - "" is NOT an error
		writebytes("", 1, 1, of);
		
		// now the exports -- FIXME
		for (ex = as -> exportlist; ex; ex = ex -> next)
		{
			int eval;
			lw_expr_t te;
			line_t tl = { 0 };
			
			if (ex -> se == NULL)
				continue;
			if (ex -> se -> section != s)
				continue;
			te = lw_expr_copy(ex -> se -> value);
			as -> csect = ex -> se -> section;
			as -> exportcheck = 1;
			tl.as = as;
			as -> cl = &tl;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
			as -> cl = NULL;
			if (!lw_expr_istype(te, lw_expr_type_int))
			{
				lw_expr_destroy(te);
				continue;
			}
			eval = lw_expr_intval(te);
			lw_expr_destroy(te);
			writebytes(ex -> symbol, strlen(ex -> symbol) + 1, 1, of);
			buf[0] = (eval >> 8) & 0xff;
			buf[1] = eval & 0xff;
			writebytes(buf, 2, 1, of);
		}
	
		// flag end of exported symbols - "" is NOT an error
		writebytes("", 1, 1, of);
		
		// FIXME - relocation table
		for (re = s -> reloctab; re; re = re -> next)
		{
			int offset;
			lw_expr_t te;
			line_t tl = { 0 };
			
			tl.as = as;
			as -> cl = &tl;
			as -> csect = s;
			as -> exportcheck = 1;
			
			if (re -> expr == NULL)
			{
				// this is an error but we'll simply ignore it
				// and not output this expression
				continue;
			}
			
			// work through each term in the expression and output
			// the proper equivalent to the object file
			if (re -> size == 1)
			{
				// flag an 8 bit relocation (low 8 bits will be used)
				buf[0] = 0xFF;
				buf[1] = 0x01;
				writebytes(buf, 2, 1, of);
			}
			
			te = lw_expr_copy(re -> offset);
			lwasm_reduce_expr(as, te);
			if (!lw_expr_istype(te, lw_expr_type_int))
			{
				lw_expr_destroy(te);
				continue;
			}
			offset = lw_expr_intval(te);
			lw_expr_destroy(te);
			
			// output expression
			lw_expr_testterms(re -> expr, write_code_obj_expraux, of);
			
			// flag end of expressions
			writebytes("", 1, 1, of);
			
			// write the offset
			buf[0] = (offset >> 8) & 0xff;
			buf[1] = offset & 0xff;
			writebytes(buf, 2, 1, of);
		}

		// flag end of incomplete references list
		writebytes("", 1, 1, of);
		
		// now blast out the code
		
		// length
		if (s -> flags & section_flag_constant)
		{
			buf[0] = 0;
			buf[1] = 0;
		}
		else
		{
			buf[0] = s -> oblen >> 8 & 0xff;
			buf[1] = s -> oblen & 0xff;
		}
		writebytes(buf, 2, 1, of);
		
		
		if (!(s -> flags & section_flag_bss) && !(s -> flags & section_flag_constant))
		{
			writebytes(s -> obytes, s -> oblen, 1, of);
		}
	}
	
	// flag no more sections
	// the "" is NOT an error
	writebytes("", 1, 1, of);
}


void write_code_lwmod(asmstate_t *as, FILE *of)
{
	line_t *l;
	sectiontab_t *s;
	reloctab_t *re;
	int initsize, bsssize, mainsize, callsnum, namesize;
	unsigned char *initcode, *maincode, *callscode, *namecode;
	int relocsize;
	unsigned char *reloccode;
	int tsize, bssoff;
	int initaddr = -1;

	int i;
	unsigned char buf[16];

	// the magic number
	buf[0] = 0x8f;
	buf[1] = 0xcf;
	
	// run through the entire system and build the byte streams for each
	// section; we will make sure we only have simple references for
	// any undefined references. That means at most an ADD (or SUB) operation
	// with a single BSS symbol reference and a single constant value.
	// We will use the constant value in the code stream and record the
	// offset in a separate code stream for the BSS relocation table.
	
	// We build everything in memory here because we need to calculate the
	// sizes of everything before we can output the complete header.
	
	for (l = as -> line_head; l; l = l -> next)
	{
		if (l -> csect)
		{
			// we're in a section - need to output some bytes
			if (l -> outputl > 0)
				for (i = 0; i < l -> outputl; i++)
					write_code_obj_sbadd(l -> csect, l -> output[i]);
			else if (l -> outputl == 0 || l -> outputl == -1)
				for (i = 0; i < l -> len; i++)
					write_code_obj_sbadd(l -> csect, 0);
		}
	}
	
	// now run through sections and set various parameters
	initsize = 0;
	bsssize = 0;
	mainsize = 0;
	callsnum = 0;
	callscode = NULL;
	maincode = NULL;
	initcode = NULL;
	namecode = NULL;
	namesize = 0;
	relocsize = 0;
	for (s = as -> sections; s; s = s -> next)
	{
		if (!strcmp(s -> name, "bss"))
		{
			bsssize = s -> oblen;
		}
		else if (!strcmp(s -> name, "main"))
		{
			maincode = s -> obytes;
			mainsize = s -> oblen;
		}
		else if (!strcmp(s -> name, "init"))
		{
			initcode = s -> obytes;
			initsize = s -> oblen;
		}
		else if (!strcmp(s -> name, "calls"))
		{
			callscode = s -> obytes;
			callsnum = s -> oblen / 2;
		}
		else if (!strcmp(s -> name, "modname"))
		{
			namecode = s -> obytes;
			namesize = 0;
		}
		for (re = s -> reloctab; re; re = re -> next)
		{
			if (re -> expr == NULL)
				relocsize += 2;
		}
	}
	if (namesize == 0)
	{
		namecode = (unsigned char *)(as -> output_file);
	}
	else
	{
		if (namecode[namesize - 1] != '\0')
		{
			namecode[namesize - 1] = '\0';
		}
		if (!*namecode)
			namecode = (unsigned char *)(as -> output_file);
	}
	namesize = strlen((char *)namecode);

	tsize = namesize + 1 + initsize + mainsize + callsnum * 2 + relocsize + 11;
	bssoff = namesize + 1 + mainsize + callsnum * 2 + 11;
	// set up section base addresses
	for (s = as -> sections; s; s = s -> next)
	{
		if (!strcmp(s -> name, "main"))
		{
			s -> tbase = 11 + namesize + 1 + callsnum * 2;
		}
		else if (!strcmp(s -> name, "init"))
		{
			s -> tbase = bssoff + relocsize;
		}
		else if (!strcmp(s -> name, "calls"))
		{
			s -> tbase = 11;
		}
		else if (!strcmp(s -> name, "modname"))
		{
			s -> tbase = 11 + callsnum * 2;
		}
	}

	// resolve the "init" address
	if (as -> execaddr_expr)
	{
		// need to resolve address with proper section bases
		lwasm_reduce_expr(as, as -> execaddr_expr);
		initaddr = lw_expr_intval(as -> execaddr_expr);
	}
	else
	{
		initaddr = as -> execaddr;
	}
	
	// build relocation data
	reloccode = NULL;
	if (relocsize)
	{
		unsigned char *tptr;
		reloccode = lw_alloc(relocsize);
		tptr = reloccode;
		
		for (s = as -> sections; s; s = s -> next)
		{
			for (re = s -> reloctab; re; re = re -> next)
			{
				lw_expr_t te;
				line_t tl;
				int offset;
			
				tl.as = as;
				as -> cl = &tl;
				as -> csect = s;
//				as -> exportcheck = 1;

				if (re -> expr)
				{
					int val;
					int x;
					
					te = lw_expr_copy(re -> expr);
					lwasm_reduce_expr(as, te);
					if (!lw_expr_istype(te, lw_expr_type_int))
					{
						val = 0;
					}
					else
					{
						val = lw_expr_intval(te);
					}
					lw_expr_destroy(te);
					x = s -> tbase;
					s -> tbase = 0;
					te = lw_expr_copy(re -> offset);
					lwasm_reduce_expr(as, te);
					offset = lw_expr_intval(te);
					lw_expr_destroy(te);
					s -> tbase = x;
					// offset *should* be the offset in the section
					s -> obytes[offset] = val >> 8;
					s -> obytes[offset + 1] = val & 0xff;
					continue;
				}
				
				offset = 0;
				te = lw_expr_copy(re -> offset);
				lwasm_reduce_expr(as, te);
				if (!lw_expr_istype(te, lw_expr_type_int))
				{
					lw_expr_destroy(te);
					offset = 0;
					continue;
				}
				offset = lw_expr_intval(te);
				lw_expr_destroy(te);
				//offset += sbase;
				
				*tptr++ = offset >> 8;
				*tptr++ = offset & 0xff;
			}
		}
	}

	// total size
	buf[2] = tsize >> 8;
	buf[3] = tsize & 0xff;
	// offset to BSS relocs
	buf[4] = bssoff >> 8;
	buf[5] = bssoff & 0xff;
	// BSS size
	buf[6] = bsssize >> 8;
	buf[7] = bsssize & 0xff;
	// init routine offset
	buf[8] = initaddr >> 8;
	buf[9] = initaddr & 0xff;
	// number of call entries
	buf[10] = callsnum;
	// write the header
	writebytes(buf, 11, 1, of);
	// call data
	if (callsnum)
		writebytes(callscode, callsnum * 2, 1, of);
	// module name
	writebytes(namecode, namesize + 1, 1, of);
	// main code
	if (mainsize)
		writebytes(maincode, mainsize, 1, of);
	// bss relocs
	if (relocsize)
		writebytes(reloccode, relocsize, 1, of);
	// init stuff
	if (initsize)
		writebytes(initcode, initsize, 1, of);
}

void write_code_abs_calc(asmstate_t *as, unsigned int *start, unsigned int *length)
{
	line_t *cl;
	unsigned int lowaddr = 65535;
	unsigned int highaddr = 0;
	char outbyte;
	int outaddr, rc;

	// if not specified, calculate
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		do
		{
			rc = fetch_output_byte(cl, &outbyte, &outaddr);
			if (rc)
			{
				if (outaddr < lowaddr)
					lowaddr = outaddr;
				if (outaddr > highaddr)
					highaddr = outaddr;
			}
		}
		while (rc);
	
		*length = (lowaddr > highaddr) ? 0 : 1 + highaddr - lowaddr;
		*start = (lowaddr > highaddr ) ? 0 : lowaddr;
	}
}

void write_code_abs_aux(asmstate_t *as, FILE *of, unsigned int start, unsigned int header_size)
{
	line_t *cl;

	char outbyte;
	int outaddr, rc;

	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		do
		{
			rc = fetch_output_byte(cl, &outbyte, &outaddr);
			
			// if first byte to write or output stream jumps address, seek
			if (rc == -1)
			{
				fseek(of,(long int) header_size + outaddr - start, SEEK_SET);
			}
			if (rc) fputc(outbyte,of);
		}
		while (rc);
	}

}

/* Write a DragonDOS binary file */

void write_code_dragon(asmstate_t *as, FILE *of)
{
	unsigned char headerbuf[9];
	unsigned int start, length;

	write_code_abs_calc(as, &start, &length);

	headerbuf[0] = 0x55; // magic $55
	headerbuf[1] = 0x02; // binary file
	headerbuf[2] = (start >> 8) & 0xFF;
	headerbuf[3] = (start) & 0xFF;
	headerbuf[4] = (length >> 8) & 0xFF;
	headerbuf[5] = (length) & 0xFF;
	headerbuf[6] = (as -> execaddr >> 8) & 0xFF;
	headerbuf[7] = (as -> execaddr) & 0xFF;
	headerbuf[8] = 0xAA; // magic $AA

	writebytes(headerbuf, 9, 1, of);

	write_code_abs_aux(as, of, start, 9);

}

/* Write a monolithic binary block, respecting absolute address segments from ORG directives */
/* Uses fseek, requires lowest code address and header offset size */
/* Out of order ORG addresses are handled */

void write_code_abs(asmstate_t *as, FILE *of)
{
	unsigned int start, length;
	
	write_code_abs_calc(as, &start, &length);
	write_code_abs_aux(as, of, start, 0);
}
