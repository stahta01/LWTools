/*
lw_win.h

Copyright © 2015 William Astle

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

#ifndef ___lw_win_h_seen___
#define ___lw_win_h_seen___

#ifdef _MSC_VER

#include "lw_string.h"
#include <string.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define unlink _unlink


#if _MSC_VER < 1900
// For older Microsoft stuff without snprintf
int c99_snprintf(char* str, size_t size, const char* format, ...);

#define snprintf c99_snprintf
#endif

#endif // _MSC_VER defined

#endif /* ___lw_win_h_seen___ */
