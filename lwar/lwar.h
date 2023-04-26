/*
lwar.h
Copyright © 2009 William Astle

This file is part of LWAR.

LWAR is free software: you can redistribute it and/or modify it under the
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


#define LWAR_OP_LIST	1
#define LWAR_OP_ADD		2
#define LWAR_OP_REMOVE	3
#define LWAR_OP_CREATE	4
#define LWAR_OP_EXTRACT	5
#define LWAR_OP_REPLACE	6

#ifndef __lwar_h_seen__
#define __lwar_h_seen__

#ifndef __lwar_c_seen__

extern char *archive_file;
extern int debug_level;
extern int operation;
extern int nfiles;
extern char **files;
extern int mergeflag;
extern int filename_flag;

//typedef void * ARHANDLE;

#define AR_MODE_RD		1
#define AR_MODE_WR		2
#define AR_MODE_RW		3
#define AR_MODE_CREATE	4


#define __lwar_E__ extern
#else
#define __lwar_E__
#endif // __lwar_c_seen__

__lwar_E__ void add_file_name(char *fn);

__lwar_E__ char *get_file_name(char *fn);

//__lwar_E__ ARHANDLE open_archive(char *fn, int mode);

#undef __lwar_E__

#endif //__lwar_h_seen__
