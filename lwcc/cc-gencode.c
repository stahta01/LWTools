/*
lwcc/cc-gencode.c

Copyright © 2019 William Astle

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
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "tree.h"

char *generate_nextlabel(void)
{
    static int labelnum = 0;
    char buf[16];
    
    sprintf(buf, "L%d", labelnum++);
    return lw_strdup(buf);
}

void generate_code(node_t *n, FILE *output);

void generate_code_shift(node_t *n, FILE *output, int dir)
{
    generate_code(n -> children, output);
    if (n -> children -> next_child -> type == NODE_CONST_INT)
    {
        long ival;
        int i;
        ival = strtol(n -> children -> next_child -> strval, NULL, 0);
        if (ival <= 0)
            return;
        if (ival >= 16)
        {
            fprintf(output, "\tldd #%d\n", dir == 1 ? 0 : -1);
            return;
        }
        if (ival >= 8)
        {
            if (dir == 1)
            {
                fprintf(output, "\ttfr b,a\n\tclrb\n");
                for (i = ival - 8; i > 0; i--)
                {
                    fprintf(output, "\tlsla\n");
                }
            }
            else
            {
                fprintf(output, "\ttfr a,b\n\tsex\n");
                for (i = ival - 8; i > 0; i--)
                {
                    fprintf(output, "\tasrb\n");
                }
            }
            return;
        }
        for (i = ival; i > 0; i--)
        {
            if (dir == 1)
            {
                fprintf(output, "\taslb\n\trola\n");
            }
            else
            {
                fprintf(output, "\tasra\n\trorb\n");
            }
        }
        return;
    }
    else
    {
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tjsr ___%ssh16\n\tpuls d\n", dir == 1 ? "l" : "r");
    }
}


void generate_code(node_t *n, FILE *output)
{
    node_t *nn;
    char *label1, *label2;

    switch (n -> type)
    {
    // function definition - output prologue, then statements, then epilogue
    case NODE_FUNDEF:
        fprintf(output, "\tsection .text\n\texport _%s\n_%s\n", n->children->next_child->strval, n->children->next_child->strval);
        generate_code(n->children->next_child->next_child->next_child, output);
        fprintf(output, "\trts\n");
        break;
    
    case NODE_CONST_INT:
        fprintf(output, "\tldd #%s\n", n->strval);
        break;

    case NODE_OPER_PLUS:
        generate_code(n->children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\taddd ,s++\n");
        break;
    
    case NODE_OPER_MINUS:
        generate_code(n->children, output);
        fprintf(output, "\tpshs d,x\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tstd 2,s\n\tpuls d\n\tsubd ,s++\n");
        break;

    case NODE_OPER_TIMES:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tjsr ___mul16i\n\tpuls d\n");
        break;

    case NODE_OPER_DIVIDE:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tjsr ___div16i\n\tpuls d\n");
        break;
    
    case NODE_OPER_MOD:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tjsr ___mod16i\n\tpuls d\n");
        break;

    case NODE_OPER_LSH:
        generate_code_shift(n, output, 1);
        break;

    case NODE_OPER_RSH:
        generate_code_shift(n, output, 0);
        break;

    case NODE_OPER_COND:
        label1 = generate_nextlabel();
        label2 = generate_nextlabel();
        generate_code(n -> children, output);
        fprintf(output, "\tsubd #0\n\tbeq %s\n", label1);
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tbra %s\n%s\n", label2, label1);
        generate_code(n -> children -> next_child -> next_child, output);
        fprintf(output, "%s\n", label2);
        lw_free(label1);
        lw_free(label2);
        break;
    
    case NODE_OPER_COMMA:
        generate_code(n -> children, output);
        generate_code(n -> children -> next_child, output);
        break;
    
    case NODE_OPER_BWAND:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tandb 1,s\n\tanda ,s++\n");
        break;

    case NODE_OPER_BWOR:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\torb 1,s\n\tora ,s++\n");
        break;

    case NODE_OPER_BWXOR:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\teorb 1,s\n\teora ,s++\n");
        break;

    case NODE_OPER_BAND:
        label1 = generate_nextlabel();
        generate_code(n -> children, output);
        fprintf(output, "\tsubd #0\n\tbeq %s\n", label1);
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tsubd #0\n\tbeq %s\n\tldd #1\n%s\n", label1, label1);
        lw_free(label1);
        break;

    case NODE_OPER_BOR:
        label1 = generate_nextlabel();
        label2 = generate_nextlabel();
        generate_code(n -> children, output);
        fprintf(output, "\tsubd #0\n\tbne %s\n", label1);
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tsubd #0\n\tbeq %s\n%s\tldd #1\n%s\n", label2, label1, label2);
        lw_free(label1);
        lw_free(label2);
        break; 

    case NODE_OPER_NE:
    case NODE_OPER_EQ:
    case NODE_OPER_LT:
    case NODE_OPER_GT:
    case NODE_OPER_LE:
    case NODE_OPER_GE:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n -> children -> next_child, output);
        fprintf(output, "\tsubd ,s++\n");
        label1 = generate_nextlabel();
        label2 = generate_nextlabel();
        fprintf(output, "\t%s %s\n", (
            (n -> type == NODE_OPER_NE ? "bne" :
            (n -> type == NODE_OPER_EQ ? "beq" :
            (n -> type == NODE_OPER_LT ? "bge" :
            (n -> type == NODE_OPER_GT ? "ble" :
            (n -> type == NODE_OPER_LE ? "bgt" :
            (n -> type == NODE_OPER_GE ? "blt" :
            "foobar"))))))
        ), label1);
        fprintf(output, "\tldd #0\n\tbra %s\n%s\tldd #1\n%s\n", label2, label1, label2);
        lw_free(label1);
        lw_free(label2);
        break;
    
    default:
        for (nn = n -> children; nn; nn = nn -> next_child)
            generate_code(nn, output);
        break;
    }
}
