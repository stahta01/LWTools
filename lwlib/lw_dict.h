/*
lw_dict.h

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

#ifndef ___lw_dict_h_seen___
#define ___lw_dict_h_seen___


#ifdef ___lw_dict_c_seen___

struct lw_dict_ent
{
    char *key;
    char *value;
    struct lw_dict_ent *next;
};

struct lw_dict_priv
{
    struct lw_dict_ent *head;
};
typedef struct lw_dict_priv * lw_dict_t;

#else /* def ___lw_dict_c_seen___ */

typedef void * lw_dict_t;
lw_dict_t lw_dict_create(void);
void lw_dict_destroy(lw_dict_t S);
void lw_dict_set(lw_dict_t S, char *key, char *val);
void lw_dict_unset(lw_dict_t S, char *key);
char *lw_dict_get(lw_dict_t S, char *key);
lw_dict_t lw_dict_copy(lw_dict_t S);

#endif /* def ___lw_dict_c_seen___ */

#endif /* ___lw_dict_h_seen___ */
