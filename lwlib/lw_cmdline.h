/*
lw_cmdline.h

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

/*

This argument parser is patterned after argp from the gnu C library but
has much of the functionality removed. It is provided here because not
every system has glibc.

Most notably, it does not support option groups or i18n.

*/

#ifndef ___lw_cmdline_h_seen___
#define ___lw_cmdline_h_seen___

struct lw_cmdline_options
{
	char *name;
	int key;
	char *arg;
	int flags;
	char *doc;
};

enum
{
	lw_cmdline_opt_optional = 1,
	lw_cmdline_opt_hidden = 2,
/*	lw_cmdline_opt_alias = 4,*/
/*	lw_cmdline_opt_nousage = 0x10,*/
	lw_cmdline_opt_doc = 0x80
};

enum
{
	lw_cmdline_err_unknown = -1
};

enum
{
	lw_cmdline_key_arg = -1,
	lw_cmdline_key_end = 0
};

struct lw_cmdline_parser
{
	struct lw_cmdline_options *options;
	int (*parser)(int key, char *arg, void *input);
	char *args_doc;
	char *doc;
	char *program_version;
};

int lw_cmdline_parse(struct lw_cmdline_parser *parser, int argc, char **argv, unsigned flags, int *arg_index, void *input);

#endif /* ___lw_cmdline_h_seen___ */
