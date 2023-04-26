/*
lwcc/tree.h

Copyright © 2013 William Astle

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

#ifndef tree_h_seen___
#define tree_h_seen___

#include <stdio.h>

/* the various node types */
#define NODE_NONE			0	// a node with no type
#define NODE_PROGRAM		1	// the whole program
#define NODE_DECL			2	// a declaration
#define NODE_TYPE_CHAR		3	// a character type
#define NODE_TYPE_SHORT		4	// short int
#define NODE_TYPE_INT		5	// integer
#define NODE_TYPE_LONG		6	// long int
#define NODE_TYPE_LONGLONG	7	// long long
#define NODE_IDENT			8	// an identifier of some kind
#define NODE_TYPE_PTR		9	// a pointer
#define NODE_TYPE_SCHAR		10	// signed char
#define NODE_TYPE_UCHAR		11	// unsigned char
#define NODE_TYPE_USHORT	12	// unsigned short
#define NODE_TYPE_UINT		13	// unsigned int
#define NODE_TYPE_ULONG		14	// unsigned long
#define NODE_TYPE_ULONGLONG	15	// unsigned long long
#define NODE_TYPE_VOID		16	// void
#define NODE_TYPE_FLOAT		17	// float
#define NODE_TYPE_DOUBLE	18	// double
#define NODE_TYPE_LDOUBLE	19	// long double
#define NODE_FUNDEF			20	// function definition
#define NODE_FUNDECL		21	// function declaration
#define NODE_FUNARGS		22	// list of function args
#define NODE_BLOCK			23	// statement block
#define NODE_STMT_RETURN    24  // return statement
#define NODE_CONST_INT      25  // constant integer
#define NODE_OPER_PLUS      26  // addition
#define NODE_OPER_MINUS     27  // subtraction
#define NODE_OPER_TIMES     28  // multiplcation
#define NODE_OPER_DIVIDE    29  // division
#define NODE_OPER_MOD       30  // modulus
#define NODE_OPER_COND      31  // ternary conditional
#define NODE_OPER_FNCALL    32  // function call
#define NODE_OPER_SUBSCRIPT 33  // array subscript
#define NODE_OPER_POSTINC   34  // post increment
#define NODE_OPER_POSTDEC   35  // post decrement
#define NODE_OPER_PTRMEM    36  // member of pointer operator
#define NODE_OPER_OBJMEM    37  // member of object operator
#define NODE_OPER_LSH       38  // left shift
#define NODE_OPER_RSH       39  // right shift
#define NODE_OPER_LT        40  // less than
#define NODE_OPER_LE        41  // less than or equal
#define NODE_OPER_GT        42  // greater than
#define NODE_OPER_GE        43  // greater than or equal
#define NODE_OPER_EQ        44  // equality
#define NODE_OPER_NE        45  // inequality
#define NODE_OPER_BWAND     46  // bitwise and
#define NODE_OPER_BWXOR     47  // bitwise xor
#define NODE_OPER_BWOR      48  // bitwise or
#define NODE_OPER_BAND      49  // boolean and
#define NODE_OPER_BOR       50  // boolean or
#define NODE_OPER_ASS       51  // assignment
#define NODE_OPER_ADDASS    52  // add/assign combo
#define NODE_OPER_SUBASS    53  // subtract/assign combo
#define NODE_OPER_MULASS    54  // multiply/assign combo
#define NODE_OPER_DIVASS    55  // divide/assign combo
#define NODE_OPER_MODASS    56  // modulus/assign combo
#define NODE_OPER_LSHASS    57  // left shift/assign combo
#define NODE_OPER_RSHASS    58  // right shift/assign combo
#define NODE_OPER_BWANDASS  59  // bitwise and/assign combo
#define NODE_OPER_BWXORASS  60  // bitwise xor/assign combo
#define NODE_OPER_BWORASS   61  // bitwise or/assign combo
#define NODE_OPER_COMMA     62  // comma sequential evaluation operator
#define NODE_TYPECAST       63  // type cast to unsigned long long
#define NODE_NUMTYPES		64	// the number of node types

typedef struct node_s node_t;

struct node_s
{
	int type;				// node type
	char *strval;			// any string value associated with the node
	unsigned char ival[8];	// any 64 bit integer value associated with the node, signed or unsigned
	node_t *children;		// pointer to list of child nodes
	node_t *next_child;		// pointer to next child in the list
	node_t *parent;			// pointer to parent node
};

extern node_t *node_create(int, ...);
extern void node_destroy(node_t *);
extern void node_addchild(node_t *, node_t *);
extern void node_removechild(node_t *, node_t *);
extern void node_display(node_t *, FILE *);
extern void node_removechild_destroy(node_t *, node_t *);

#endif // tree_h_seen___
