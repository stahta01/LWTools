/*
lw_error.c

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

#include "lw_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void (*lw_error_func)(const char *fmt, ...) = NULL;

void lw_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (lw_error_func)
		(*lw_error_func)(fmt, args);
	else
		vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

void lw_error_setfunc(void (*f)(const char *fmt, ...))
{
	lw_error_func = f;
}
