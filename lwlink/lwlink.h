/*
lwlink.h
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

Contains the main defs used by the linker
*/


#ifndef __lwlink_h_seen__
#define __lwlink_h_seen__

#include "expr.h"

#define OUTPUT_DECB		0	// DECB multirecord format
#define OUTPUT_RAW		1	// raw sequence of bytes; BSS is just not included
#define OUTPUT_LWEX0	2	// LWOS LWEX binary version 0
#define OUTPUT_OS9		3	// OS9 object code module
#define OUTPUT_SREC		4	// motorola SREC format
#define OUTPUT_RAW2     5   // raw sequence of bytes, BSS converted to NULs
#define OUTPUT_IHEX     6   // IHEX output format

typedef struct symtab_s symtab_t;
struct symtab_s
{
	unsigned char *sym;		// symbol name
	int offset;				// local offset
//	int realval;			// resolved value
	symtab_t *next;			// next symbol
};

#define RELOC_NORM	0		// all default (16 bit)
#define RELOC_8BIT	1		// only use the low 8 bits for the reloc
typedef struct reloc_s reloc_t;
struct reloc_s
{
	int offset;				// where in the section
	int flags;				// flags for the relocation
	lw_expr_stack_t *expr;	// the expression to calculate it
	reloc_t *next;			// ptr to next relocation
};

typedef struct fileinfo_s fileinfo_t;

#define SECTION_BSS		1
#define SECTION_CONST	2
typedef struct
{
	unsigned char *name;	// name of the section
	int flags;				// section flags
	int codesize;			// size of the code
	unsigned char *code;	// pointer to the code
	int loadaddress;		// the actual load address of the section
	int processed;			// was the section processed yet?
		
	symtab_t *localsyms;	// local symbol table
	symtab_t *exportedsyms;	// exported symbols table
	
	reloc_t *incompletes;	// table of incomplete references
	
	fileinfo_t *file;		// the file we are in
	
	int aftersize;			// add this many bytes after section on output
	unsigned char *afterbytes;	// add these bytes after section on output
} section_t;

struct fileinfo_s
{
	char *filename;
	unsigned char *filedata;
	int filesize;
	section_t *sections;
	int nsections;
	int islib;				// set to true if the file is a "-l" option

	int forced;				// set to true if the file is a "forced" include

	// "sub" files (like in archives or libraries)
	int nsubs;
	fileinfo_t **subs;
	fileinfo_t *parent;
};

struct section_list
{
	section_t *ptr;		// ptr to section structure
	int forceaddr;		// was this force to an address by the link script?
};


typedef struct sectopt_s sectopt_t;
struct sectopt_s
{
	char *name;					// section name
	int aftersize;				// number of bytes to append to section
	unsigned char *afterbytes;	// the bytes to store after the section
	sectopt_t *next;			// next section option
};

#ifndef __link_c_seen__
extern struct section_list *sectlist;
extern int nsects;
extern sectopt_t *section_opts;
#endif


#ifndef __lwlink_c_seen__

extern int debug_level;
extern int outformat;
extern char *outfile;
extern int ninputfiles;
extern fileinfo_t **inputfiles;
extern char *scriptfile;
extern char *entrysym;

extern int nlibdirs;
extern char **libdirs;

extern int nscriptls;
extern char **scriptls;

extern int symerr;

extern char *map_file;

extern char *sysroot;

#define __lwlink_E__ extern
#else
#define __lwlink_E__
#endif // __lwlink_c_seen__

__lwlink_E__ void add_input_file(char *fn);
__lwlink_E__ void add_input_library(char *fn);
__lwlink_E__ void add_library_search(char *fn);
__lwlink_E__ void add_section_base(char *fn);
__lwlink_E__ char *sanitize_symbol(char *sym);

#undef __lwlink_E__

struct scriptline_s
{
	char *sectname;				// name of section, NULL for wildcard
	int loadat;					// address to load at (or -1)
	int noflags;				// flags to NOT have
	int yesflags;				// flags to HAVE
	int growsdown;				// sections are placed descending in memory
};

typedef struct
{
	int nlines;					// number of lines in the script
	struct scriptline_s *lines;	// the parsed script lines (section)
	int padsize;				// the size to pad to, -1 for none
	char *execsym;				// entry point symbol
	int execaddr;				// execution address (entry point)
	int stacksize;				// stack size
	int modtype;				// module type
	int modlang;				// module language
	int modattr;				// module attributes
	int modrev;					// module revision
	int edition;				// module edition
	char *name;					// module name
	char *basesympat;			// pattern for section base symbol (%s for section name)
	char *lensympat;			// pattern for section length symbol (%s for section name)
} linkscript_t;

#ifndef __script_c_seen__
extern linkscript_t linkscript;
#endif

typedef struct symlist_s symlist_t;

struct symlist_s
{
	char *sym;
	int val;
	symlist_t *next;
	
};

extern symlist_t *symlist;

#endif //__lwlink_h_seen__
