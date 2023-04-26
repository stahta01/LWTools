/*
lwasm.h

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

#ifndef ___lwasm_h_seen___
#define ___lwasm_h_seen___

#ifdef _MSC_VER
#include <lw_win.h>	// windows build
#endif

#include <lw_expr.h>
#include <lw_stringlist.h>
#include <lw_stack.h>

#include <version.h>

// these are allowed chars BELOW 0x80 for symbols
// first is symbol start chars, second is anywhere in symbol
#define SSYMCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_@$"
#define SYMCHARS SSYMCHARS ".?0123456789"

typedef struct asmstate_s asmstate_t;

enum
{
	lwasm_expr_linelen = 1,			// length of ref'd line
	lwasm_expr_lineaddr = 2,		// addr of ref'd line
	lwasm_expr_nextbp = 3,			// next branch point
	lwasm_expr_prevbp = 4,			// previous branch point
	lwasm_expr_syment = 5,			// symbol table entry
	lwasm_expr_import = 6,			// symbol import entry
	lwasm_expr_secbase = 7,			// section base address
	lwasm_expr_linedaddr = 8,		// data address of the line
	lwasm_expr_linedlen = 9			// data length of the line
};

enum lwasm_output_e
{
	OUTPUT_DECB = 0,	// DECB multirecord format
	OUTPUT_BASIC,		// Color BASIC program
	OUTPUT_RAW,			// raw sequence of bytes
	OUTPUT_OBJ,			// proprietary object file format
	OUTPUT_RAWREL,		// raw bytes where ORG causes a SEEK in the file
	OUTPUT_OS9,			// os9 module target
	OUTPUT_SREC,		// motorola SREC format
	OUTPUT_IHEX,		// intel hex format
	OUTPUT_HEX,			// generic hexadecimal format
	OUTPUT_LWMOD        // special module format for LW
};

enum lwasm_flags_e
{
	FLAG_LIST = 0x0001,
	FLAG_DEPEND = 0x0002,
	FLAG_SYMBOLS = 0x004,
	FLAG_DEPENDNOERR = 0x0008,
	FLAG_UNICORNS = 0x0010,
	FLAG_MAP = 0x0020,
	FLAG_SYMBOLS_NOLOCALS = 0x0040,
	FLAG_NOOUT = 0x80,
	FLAG_SYMDUMP = 0x100,
	FLAG_NONE = 0
};

enum lwasm_pragmas_e
{
	PRAGMA_NONE = 0,						// no pragmas in effect
	PRAGMA_DOLLARNOTLOCAL 		= 1 << 0,	// dollar sign does not make a symbol local
	PRAGMA_NOINDEX0TONONE 		= 1 << 1,	// do not change implicit 0,R to ,R
	PRAGMA_UNDEFEXTERN			= 1 << 2,	// undefined symbols are considered to be external
	PRAGMA_CESCAPES 			= 1 << 3,	// allow C style escapes in fcc, fcs, fcn, etc.
	PRAGMA_IMPORTUNDEFEXPORT	= 1 << 4,	// imports symbol if undefined upon export
	PRAGMA_PCASPCR				= 1 << 5,	// treats ,PC as ,PCR instead of constant offset
	PRAGMA_SHADOW				= 1 << 6,	// allow macros to shadow builtin operations
	PRAGMA_NOLIST				= 1 << 7,	// don't show line in listing
	PRAGMA_AUTOBRANCHLENGTH		= 1 << 8,	// automatically select proper length for relative branches
	PRAGMA_EXPORT				= 1 << 9,	// export symbols by default, unless local
	PRAGMA_SYMBOLNOCASE			= 1 << 10,	// symbols defined under this pragma are matched case insensitively
	PRAGMA_CONDUNDEFZERO		= 1 << 11,	// treat undefined symbols as zero in conditionals during pass 1
	PRAGMA_6800COMPAT			= 1 << 12,	// enable 6800 compatibility opcodes
	PRAGMA_FORWARDREFMAX		= 1 << 13,	// force incomplete references on pass 1 to maximum mode
	PRAGMA_6809					= 1 << 14,	// 6809/6309 assembly mode
	PRAGMA_TESTMODE				= 1 << 15,	// enable test mode (for internal unit testing)
	PRAGMA_C					= 1 << 16,	// enable cycle counts
	PRAGMA_CD					= 1 << 17,	// enable detailed cycle count
	PRAGMA_CT					= 1 << 18,	// enable cycle count running total
	PRAGMA_CC					= 1 << 19,	// clear cycle count running total
	PRAGMA_QRTS					= 1 << 20,	// enable BRA ?RTS support
	PRAGMA_M80EXT				= 1 << 21,	// enable Macro-80C assembler extensions
	PRAGMA_6809CONV				= 1 << 22,	// enable 6809 convenience ops
	PRAGMA_6309CONV				= 1 << 23,	// enable 6309 convenience ops
	PRAGMA_NEWSOURCE			= 1 << 24,	// don't use compatibility source format
	PRAGMA_OPERANDSIZE			= 1 << 25,	// warn if operand size is bigger than required
	PRAGMA_EMUEXT				= 1 << 26,  // enable emulator extensions
	PRAGMA_NOOUTPUT             = 1 << 27,  // disable object code output
	PRAGMA_NOEXPANDCOND         = 1 << 28,  // hide conditionals and skipped output in listings
	PRAGMA_CLEARBIT				= 1 << 31	// reserved to indicate negated pragma flag status
};

enum
{
	section_flag_bss = 1,				// BSS section
	section_flag_constant = 2,			// constants - no base offset
	section_flag_none = 0				// no flags
};

typedef struct reloctab_s reloctab_t;
struct reloctab_s
{
	lw_expr_t offset;					// offset of relocation
	int size;							// size of relocation
	lw_expr_t expr;						// relocation expression
	reloctab_t *next;
};

typedef struct sectiontab_s sectiontab_t;
struct sectiontab_s
{
	char *name;							// section name
	int flags;							// section flags;
	lw_expr_t offset;					// offset for next instance
	int oblen;							// size of section output
	int obsize;							// size of output buffer
	int tbase;                          // temporary base value for resolution
	unsigned char *obytes;				// output buffer
	reloctab_t *reloctab;				// table of relocations
	sectiontab_t *next;
};

typedef enum
{
	TF_EMIT = 1,
	TF_ERROR = 2
} lwasm_testflags_t;

typedef enum 
{
	E_6309_INVALID				= 1,
	E_6809_INVALID				= 2,
	E_ALIGNMENT_INVALID			= 3,
	E_BITNUMBER_UNRESOLVED		= 4,
	E_BITNUMBER_INVALID			= 5,
	E_BYTE_OVERFLOW				= 6,
	E_CONDITION_P1				= 7,
	E_DIRECTIVE_OS9_ONLY		= 8,
	E_DIV0						= 9,
	E_EXEC_ADDRESS				= 10,
	E_FILL_INVALID				= 11,
	E_IMMEDIATE_INVALID			= 12,
	E_IMMEDIATE_UNRESOLVED		= 13,
	E_EXPRESSION_BAD			= 14,
	E_EXPRESSION_NOT_CONST		= 15,
	E_EXPRESSION_NOT_RESOLVED	= 16,
	E_FILE_OPEN					= 17,
	E_FILENAME_MISSING			= 18,
	E_INSTRUCTION_FAILED		= 19,
	E_INSTRUCTION_SECTION		= 20,
	E_LINE_ADDRESS				= 21,
	E_LINED_ADDRESS				= 22,
	E_MACRO_DUPE				= 23,
	E_MACRO_ENDM				= 24,
	E_MACRO_NONAME				= 25,
	E_MACRO_RECURSE				= 26,
	E_MODULE_IN					= 27,
	E_MODULE_NOTIN				= 28,
	E_NEGATIVE_BLOCKSIZE		= 29,
	E_NEGATIVE_RESERVATION		= 30,
	E_NW_8						= 31,
	E_OPCODE_BAD				= 32,
	E_OPERAND_BAD				= 33,
	E_OBJTARGET_ONLY			= 34,
	E_PADDING_BAD				= 35,
	E_PRAGMA_UNRECOGNIZED		= 36,
	E_REGISTER_BAD				= 37,
	E_SECTION_END				= 38,
	E_SECTION_EXTDEP			= 39,
	E_SECTION_FLAG				= 40,
	E_SECTION_NAME				= 41,
	E_SECTION_TARGET			= 42,
	E_SETDP_INVALID				= 43,
	E_SETDP_NOT_CONST			= 44,
	E_STRING_BAD				= 45,
	E_STRUCT_DUPE				= 46,
	E_STRUCT_NONAME				= 47,
	E_STRUCT_NOSYMBOL			= 48,
	E_STRUCT_RECURSE			= 49,
	E_SYMBOL_BAD				= 50,
	E_SYMBOL_DUPE				= 51,
	E_SYMBOL_MISSING			= 52,
	E_SYMBOL_UNDEFINED			= 53,
	E_SYMBOL_UNDEFINED_EXPORT	= 54,
	E_UNKNOWN_OPERATION			= 55,
	E_USER_SPECIFIED			= 56,
	E_ORG_NOT_FOUND				= 57,
	E_COMPLEX_INCOMPLETE        = 58,
	E_ILL5						= 59,
	
	/* warnings must be 1000 or greater */

	W_DUPLICATE_SECTION			= 1000,
	W_ENDSTRUCT_WITHOUT			= 1001,
	W_NOT_SUPPORTED				= 1002,
	W_USER_SPECIFIED			= 1003,
	W_OPERAND_SIZE				= 1004
} lwasm_errorcode_t;

typedef struct lwasm_error_s lwasm_error_t;
struct lwasm_error_s
{
	lwasm_errorcode_t code;				// error code
	char *mess;							// actual error message
	int charpos;						// character position on line where parsing stopped
	lwasm_error_t *next;				// ptr to next error
};

struct line_expr_s
{
	lw_expr_t expr;
	int id;
	struct line_expr_s *next;
};

typedef struct line_s line_t;

typedef struct exportlist_s exportlist_t;
struct exportlist_s
{
	char *symbol;						// symbol to export
	struct symtabe *se;					// symbol table entry
	line_t *line;						// line the export is on
	exportlist_t *next;					// next in the export list
};

typedef struct importlist_s importlist_t;
struct importlist_s
{
	char *symbol;						// symbol to import
	importlist_t *next;					// next in the import list
};

typedef enum
{
	CYCLE_ADJ = 1,
	CYCLE_ESTIMATED = 2
} cycle_flags;

struct line_s
{
	lw_expr_t addr;						// assembly address of the line
	lw_expr_t daddr;					// data address of the line (os9 only)
	int len;							// the "size" this line occupies (address space wise) (-1 if unknown)
	int dlen;							// the data "size" this line occupies (-1 if unknown)
	int minlen;							// minimum length
	int maxlen;							// maximum length
	int insn;							// number of insn in insn table
	int symset;							// set if the line symbol was consumed by the instruction
	char *sym;							// symbol, if any, on the line
	unsigned char *output;				// output bytes
	int outputl;						// size of output
	int outputbl;						// size of output buffer
	int dpval;							// direct page value
	int cycle_base;						// base instruction cycle count
	int cycle_adj;						// cycle adjustment
	int	cycle_flags;					// cycle flags
	int genmode;						// generation mode (insn_parse_gen0/8/16)
	int fcc_extras;						// fcc extra bytes
	lwasm_error_t *err;					// list of errors
	lwasm_error_t *warn;				// list of errors
	lwasm_errorcode_t err_testmode;		// error code in testmode
	line_t *prev;						// previous line
	line_t *next;						// next line
	int inmod;							// inside a module?
	sectiontab_t *csect;				// which section are we in?
	struct line_expr_s *exprs;			// expressions used during parsing
	char *lstr;							// string passed forward
	int pb;								// pass forward post byte
	int lint;							// pass forward integer
	int lint2;							// another pass forward integer
	int conditional_return;				// for ?RTS handling (1 if RTS follows)
	asmstate_t *as;						// assembler state data ptr
	int pragmas;						// pragmas in effect for the line
	int context;						// the symbol context number
	char *ltext;						// line number
	char *linespec;						// line spec
	int lineno;							// line number
	int soff;							// struct offset (for listings)
	int dshow;							// data value to show (for listings)
	int dsize;							// set to 1 for 8 bit dshow value
	int isbrpt;							// set to 1 if this line is a branch point
	struct symtabe *dptr;				// symbol value to display

	int noexpand_start;					// start of a no-expand block
	int noexpand_end;					// end of a no-expand block
	int hideline;						// set if we're going to hide this line on output	
	int hidecond;                       // set if we're going to hide this line due to condition hiding
};

enum
{
	symbol_flag_set = 1,				// symbol was used with "set"
	symbol_flag_nocheck = 2,			// do not check symbol characters
	symbol_flag_nolist = 4,				// no not show symbol in symbol table
	symbol_flag_nocase = 8,				// do not match case of symbol
	symbol_flag_none = 0				// no flags
};

struct symtabe
{
	char *symbol;						// the name of the symbol
	int context;						// symbol context (-1 for global)
	int version;						// version of the symbol (for "set")
	int flags;							// flags for the symbol
	sectiontab_t *section;				// section the symbol is defined in
	lw_expr_t value;					// symbol value
	struct symtabe *left;				// left subtree pointer
	struct symtabe *right;				// right subtree pointer
	struct symtabe *nextver;			// next lower version
};

typedef struct
{
	struct symtabe *head;				// start of symbol table
} symtab_t;

typedef struct macrotab_s macrotab_t;
struct macrotab_s
{
	char *name;							// name of macro
	char **lines;						// macro lines
	int numlines;						// number lines in macro
	int flags;							// flags for the macro
	macrotab_t *next;					// next macro in list
	line_t *definedat;					// the line where the macro definition starts
};

enum
{
	macro_noexpand = 1					// set to not expland the macro by default in listing
};

typedef struct structtab_s structtab_t;
typedef struct structtab_field_s structtab_field_t;

struct structtab_field_s
{
	char *name;							// structure field name - NULL for anonymous
	int size;							// structure field size
	structtab_t *substruct;				// sub structure if there is one
	structtab_field_t *next;			// next field entry
};

struct structtab_s
{
	char *name;							// name of structure
	int size;							// number of bytes taken by struct
	structtab_field_t *fields;			// fields in the structure
	structtab_t *next;					// next structure
	line_t *definedat;					// line where structure is defined
};

struct asmstate_s
{
	int output_format;					// output format
	int debug_level;					// level of debugging requested
	FILE *debug_file;					// FILE * to output debug messages to
	int flags;							// assembly flags
	int pragmas;						// pragmas currently in effect
	int errorcount;						// number of errors encountered
	int warningcount;					// number of warnings issued
	int testmode_errorcount;			// number of errors in testmode
	int inmacro;						// are we in a macro?
	int instruct;						// are w in a structure?
	int skipcond;						// skipping a condition?
	int skipcount;						// depth of "skipping"
	int skipmacro;						// are we skipping in a macro?	
	int endseen;						// have we seen an "end" pseudo?
	int execaddr;						// address from "end"
	lw_expr_t execaddr_expr;            // address from "end" but as an expression
	int inmod;							// inside an os9 module?
	int undefzero;						// used for handling "condundefzero"
	int pretendmax;						// set if we need to pretend the instruction is max length
	unsigned char crc[3];				// crc accumulator
	int cycle_total;					// cycle count accumulator
	int badsymerr;						// throw error on undef sym if set

	line_t *line_head;					// start of lines list
	line_t *line_tail;					// tail of lines list

	line_t *cl;							// current line pointer
	
	sectiontab_t *csect;				// current section
	
	int context;						// the current "context"
	int nextcontext;					// the next available context
	
	symtab_t symtab;					// meta data for the symbol table
	macrotab_t *macros;					// macro table
	sectiontab_t *sections;				// section table
	exportlist_t *exportlist;			// list of exported symbols
	importlist_t *importlist;			// list of imported symbols
	char *list_file;					// name of file to list to
	char *symbol_dump_file;				// name of file to dump symbol table to
	int tabwidth;						// tab width in list file
	char *map_file;						// name of map file
	char *output_file;					// output file name	
	lw_stringlist_t input_files;		// files to assemble
	void *input_data;					// opaque data used by the input system
	lw_stringlist_t include_list;		// include paths
	lw_stack_t file_dir;				// stack of the "current file" dir
	lw_stack_t includelist;

	structtab_t *structs;				// defined structures
	structtab_t *cstruct;				// current structure
	lw_expr_t savedaddr;				// old address counter before struct started	
	int exportcheck;					// set if we need to collapse out the section base to 0
	int passno;							// set to the current pass number
	int preprocess;						// set if we are prepocessing
	int fileerr;						// flags error opening file
	int exprwidth;						// the bit width of the expression being evaluated
	int listnofile;						// nonzero to suppress printing file name in listings
};

struct symtabe *register_symbol(asmstate_t *as, line_t *cl, char *sym, lw_expr_t value, int flags);
struct symtabe *lookup_symbol(asmstate_t *as, line_t *cl, char *sym);

int parse_pragma_helper(char *p);

int lwasm_cycle_calc_ind(line_t *cl);
int lwasm_cycle_calc_rlist(line_t *cl);
void lwasm_cycle_update_count(line_t *cl, int opc);

void lwasm_parse_testmode_comment(line_t *cl, lwasm_testflags_t *flags, lwasm_errorcode_t *err, int *len, char **buf);
void lwasm_error_testmode(line_t *cl, const char* msg, int fatal);

void lwasm_register_error(asmstate_t *as, line_t *cl, lwasm_errorcode_t err);
void lwasm_register_error2(asmstate_t *as, line_t *cl, lwasm_errorcode_t err, const char* fmt, ...);

int lwasm_next_context(asmstate_t *as);
void lwasm_emit(line_t *cl, int byte);
void lwasm_emitop(line_t *cl, int opc);

void lwasm_save_expr(line_t *cl, int id, lw_expr_t expr);
lw_expr_t lwasm_fetch_expr(line_t *cl, int id);
lw_expr_t lwasm_parse_expr(asmstate_t *as, char **p);
int lwasm_emitexpr(line_t *cl, lw_expr_t expr, int s);

void skip_operand_real(line_t *l, char **p);
/* this macro can only be used where "l" is the current line pointer */
#define skip_operand(p) skip_operand_real(l, p)

int lwasm_lookupreg2(const char *rlist, char **p);
int lwasm_lookupreg3(const char *rlist, char **p);

void lwasm_show_errors(asmstate_t *as);

int lwasm_reduce_expr(asmstate_t *as, lw_expr_t expr);

lw_expr_t lwasm_parse_cond(asmstate_t *as, char **p);

int lwasm_calculate_range(asmstate_t *as, lw_expr_t expr, int *min, int *max);

void lwasm_reduce_line_exprs(line_t *cl);

#ifdef LWASM_NODEBUG
#define debug_message(...)
#define dump_state(...)
#else
void real_debug_message(asmstate_t *as, int level, const char *fmt, ...);
void dump_state(asmstate_t *as);

#define debug_message(as,level,...) do { asmstate_t *ras = (as); int rlevel = (level); if (ras->debug_level >= rlevel) { real_debug_message(ras, rlevel, __VA_ARGS__); } } while (0)
#endif

#define OPLEN(op) (((op)>0xFF)?2:1)
#define CURPRAGMA(l,p)	(((l) && ((l)->pragmas & (p))) ? 1 : 0)

/* some functions for parsing */
/* skip to the start of the next token if the current parsing mode allows it */
void lwasm_skip_to_next_token(line_t *cl, char **p);

#endif /* ___lwasm_h_seen___ */
