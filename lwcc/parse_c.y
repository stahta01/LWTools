%include {
#include <assert.h> // only needed due to a bug in lemon
#include <stdio.h>
#include "parse.h"
#include "tree.h"
}

%token_type { struct tokendata * }
%token_prefix PTOK_
%token_destructor { tokendata_free($$); }
%default_type { node_t * }
%extra_argument { struct parserinfo *pinfo }

program(A) ::= rprogram(B). { A = B; pinfo -> parsetree = A; }

rprogram(A) ::= rprogram(C) globaldecl(B). {
	A = C;
	node_addchild(A, B);
}
rprogram(A) ::= . { A = node_create(NODE_PROGRAM); }

globaldecl(A) ::= vardecl(B). { A = B; }
globaldecl(A) ::= fundecl(B). { A = B; }

vardecl(A) ::= datatype(B) ident(C) ENDS. {
	A = node_create(NODE_DECL, B, C);
}

ident(A) ::= IDENTIFIER(B). { A = node_create(NODE_IDENT, B -> strval); }

datatype(A) ::= typename(B). { A = B; }
datatype(A) ::= datatype(B) STAR. { A = node_create(NODE_TYPE_PTR, B); }

typename(A) ::= KW_VOID. { A = node_create(NODE_TYPE_VOID); }
typename(A) ::= KW_FLOAT. { A = node_create(NODE_TYPE_FLOAT); }
typename(A) ::= KW_DOUBLE. { A = node_create(NODE_TYPE_DOUBLE); }
typename(A) ::= KW_LONG KW_DOUBLE. { A = node_create(NODE_TYPE_LDOUBLE); }
typename(A) ::= baseint(B). { A = B; }
typename(A) ::= KW_UNSIGNED. { A = node_create(NODE_TYPE_UINT); }
typename(A) ::= KW_SIGNED. { A = node_create(NODE_TYPE_INT); }
typename(A) ::= KW_SIGNED baseint(B). { A = B; if (A -> type == NODE_TYPE_CHAR) A -> type = NODE_TYPE_SCHAR; }
typename(A) ::= KW_UNSIGNED baseint(B). {
	A = B;
	switch (A -> type)
	{
	case NODE_TYPE_CHAR:
		A -> type = NODE_TYPE_UCHAR;
		break;
	case NODE_TYPE_SHORT:
		A -> type = NODE_TYPE_USHORT;
		break;
	case NODE_TYPE_INT:
		A -> type = NODE_TYPE_UINT;
		break;
	case NODE_TYPE_LONG:
		A -> type = NODE_TYPE_ULONG;
		break;
	case NODE_TYPE_LONGLONG:
		A -> type = NODE_TYPE_ULONGLONG;
		break;
	}
}

baseint(A) ::= KW_INT. { A = node_create(NODE_TYPE_INT); } 
baseint(A) ::= KW_LONG. { A = node_create(NODE_TYPE_LONG); }
baseint(A) ::= KW_LONG KW_INT. { A = node_create(NODE_TYPE_LONG); }
baseint(A) ::= KW_LONG KW_LONG. { A = node_create(NODE_TYPE_LONGLONG); }
baseint(A) ::= KW_SHORT. { A = node_create(NODE_TYPE_SHORT); }
baseint(A) ::= KW_LONG KW_LONG KW_INT. { A = node_create(NODE_TYPE_LONGLONG); }
baseint(A) ::= KW_SHORT KW_INT. { A = node_create(NODE_TYPE_SHORT); }
baseint(A) ::= KW_CHAR. { A = node_create(NODE_TYPE_CHAR); }


fundecl(A) ::= datatype(B) ident(C) arglist(D) statementblock(E). {
	A = node_create(NODE_FUNDEF, B, C, D, E);
}

fundecl(A) ::= datatype(B) ident(C) arglist(D) ENDS. {
	A = node_create(NODE_FUNDECL, B, C, D);
}

arglist(A) ::= OPAREN CPAREN. { A = node_create(NODE_FUNARGS); }

statementblock(A) ::= OBRACE CBRACE. { A = node_create(NODE_BLOCK); }

%parse_failure {
	fprintf(stderr, "Parse error\n");
}

%stack_overflow {
	fprintf(stderr, "Parser stack overflow\n");
}

%syntax_error {
	fprintf(stderr, "Undexpected token %d: ", TOKEN -> tokid);
	tokendata_print(stderr, TOKEN);
}
