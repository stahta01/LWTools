/*
strings.c
Copyright © 2021 William Astle

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

Contains stuff associated with generalized string parsing including
interpolation.

A general string is enclosed in double quotes. Within the string, the
following is supported:

%(VAR): a string variable defined with SETSTR
%[SYM]: the value of SYM; must be constant on pass 1 or will resolve to ""
\": a literal "
\%: a literal %
\n: a newline
\r: a carriage return
\t: a tab
\f: a form feed
\e: ESC (0x1b)
\\: a backslash
\xXX: an 8 bit value where XX are hex digits

*/

#include <ctype.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_dict.h>
#include <lw_string.h>
#include <lw_strbuf.h>

#include "lwasm.h"
#include "instab.h"

void lwasm_stringvar_unset(asmstate_t *as, char *strname)
{
    if (!(as -> stringvars))
        return;
    lw_dict_unset(as -> stringvars, strname);
}

void lwasm_stringvar_set(asmstate_t *as, char *strname, char *strval)
{
    if (!(as -> stringvars))
        as -> stringvars = lw_dict_create();
    lw_dict_set(as -> stringvars, strname, strval);
}

char *lwasm_stringvar_get(asmstate_t *as, char *strname)
{
    if (!(as -> stringvars))
        return "";
    return lw_dict_get(as -> stringvars, strname);
}

PARSEFUNC(pseudo_parse_setstr)
{
    char *t1;
    char *strname;
    char *strval;

    l -> len = 0;
    if (!**p)
    {
        lwasm_register_error(as, l, E_OPERAND_BAD);
        return;
    }
    
    for (t1 = *p; *t1 && *t1 != '='; t1++)
        /* do nothing */;
    strname = lw_alloc(t1 - *p + 1);
    strncpy(strname, *p, t1 - *p);
    strname[t1 - *p] = '\0';
    *p = t1;
    if (**p == '\0')
    {
        lwasm_stringvar_unset(l -> as, strname);
        lw_free(strname);
        return;
    }
    (*p)++;
    strval = lwasm_parse_general_string(l, p);
    if (!strval)
    {
        lwasm_stringvar_unset(l -> as, strname);
        lw_free(strname);
        return;
    }
    lwasm_stringvar_set(l -> as, strname, strval);
    lw_free(strval);
}

char *lwasm_parse_general_string(line_t *l, char **p)
{
    struct lw_strbuf *sb;

    if (!**p || isspace(**p))
        return lw_strdup("");
    if (**p != '"')
    {
        lwasm_register_error(l -> as, l, E_OPERAND_BAD);
        return NULL;
    }

    (*p)++;
    sb = lw_strbuf_new();
    while (**p && **p != '"')
    {
        switch (**p)
        {
        case '\\':
            if ((*p)[1])
            {
                (*p)++;
                switch (**p)
                {
                case 'n':
                    lw_strbuf_add(sb, 10);
                    break;
                
                case 'r':
                    lw_strbuf_add(sb, 13);
                    break;
                
                case 't':
                    lw_strbuf_add(sb, 9);
                    break;
                
                case 'f':
                    lw_strbuf_add(sb, 12);
                    break;
                
                case 'e':
                    lw_strbuf_add(sb, 27);
                    break;
                
                case 'x':
                    if ((*p)[1] && (*p)[2])
                    {
                        int d1 = (*p)[1];
                        int d2 = (*p)[2];
                        if (d1 < '0' || (d1 > '9' && d1 < 'A') || (d1 > 'F' && d1 < 'a') || d1 > 'f' ||
                            d2 < '0' || (d2 > '9' && d2 < 'A') || (d2 > 'F' && d2 < 'a') || d2 > 'f')
                        {
                            lw_strbuf_add(sb, 'x');
                        }
                        else
                        {
                            (*p) += 2;
                            d1 -= '0';
                            d2 -= '0';
                            if (d1 > 9)
                                d1 -= 7;
                            if (d1 > 15)
                                d1 -= 32;
                            if (d2 > 9)
                                d2 -= 7;
                            if (d2 > 15)
                                d2 -= 32;
                            lw_strbuf_add(sb, (d1 << 4) | d2);
                        }
                    }
                    else
                    {
                        lw_strbuf_add(sb, 'x');
                    }
                    break;

                default:
                    lw_strbuf_add(sb, **p);
                    break;
                }
            }
            break;

        case '%':
            (*p)++;
            if (**p == '(')
            {
                char *t1;
                // string var
                for (t1 = *p + 1; *t1 && *t1 != ')' && *t1 != '"'; t1++)
                    /* do nothing */ ;
                if (*t1 != ')')
                {
                    lw_strbuf_add(sb, '%');
                    (*p)--;
                }
                else
                {
                    char *strname;
                    strname = lw_alloc(t1 - *p);
                    strncpy(strname, *p + 1, t1 - *p);
                    strname[t1 - *p - 1] = '\0';
                    *p = t1;
                    t1 = lwasm_stringvar_get(l -> as, strname);
                    lw_free(strname);
                    for (strname = t1; *strname; strname++)
                        lw_strbuf_add(sb, *strname);
                }
            }
            else
            {
                // unknown % sequence; back up and output the %
                (*p)--;
                lw_strbuf_add(sb, '%');
            }
            break;
        
        default:
            lw_strbuf_add(sb, **p);
        }
        (*p)++;
    }
    if (**p == '"')
        (*p)++;
    return lw_strbuf_end(sb);
}

