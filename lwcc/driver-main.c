/*
lwcc/driver/main.c

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

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_stringlist.h>

#include <version.h>

#define VERSTRING "lwcc from " PACKAGE_STRING
#define S(x) S2(x)
#define S2(x) #x

#define BASEDIR S(LWCC_LIBDIR)

/* list of compilation phases */
enum phase_t {
	PHASE_DEFAULT = 0,
	PHASE_PREPROCESS,
	PHASE_COMPILE,
	PHASE_ASSEMBLE,
	PHASE_LINK
};

/* these are the names of various programs the compiler calls */
const char *linker_program_name = "lwlink";
const char *compiler_program_name = "lwcc-cc";
const char *assembler_program_name = "lwasm";
const char *preprocessor_program_name = "lwcc-cpp";

/* this will be set to the directory where temporary files get created */
const char *temp_directory = NULL;

/* these are for book keeping if we get interrupted - the volatile and atomic
   types are needed because they are accessed in a signal handler */
static volatile sig_atomic_t sigterm_received = 0;
static volatile sig_atomic_t child_pid = 0;

/* path specified with --sysroot */
const char *sysroot = "";
/* path specified with -isysroot */
const char *isysroot = NULL;

/* record which phase to stop after for -c, -E, and -S */
/* default is to stop after PHASE_LINK */
static int stop_after = PHASE_DEFAULT;

int nostdinc = 0;				// set if -nostdinc is specified
int nostartfiles = 0;			// set if -nostartfiles is specified
int nostdlib = 0;				// set if -nostdlib is specified
int verbose_mode = 0;			// set to number of --verbose arguments
int save_temps = 0;				// set if -save-temps is specified
int debug_mode = 0;				// set if -g specified
int pic_mode = 0;				// set to 1 if -fpic, 2 if -fPIC; last one specified wins
const char *output_file;		// set to the value of the -o option (output file)

/* compiler base directory  - from -B */
const char *basedir = BASEDIR;

/* used to ensure a unique temporary file at every stage */
static int file_counter = 0;

/* these are various string lists used to keep track of things, mostly
   command line arguments. */

lw_stringlist_t input_files;		// input files from command line
lw_stringlist_t runtime_dirs;		// directories to search for runtime files
lw_stringlist_t lib_dirs;			// directories to search for library files
lw_stringlist_t program_dirs;		// directories to search for compiler program components
lw_stringlist_t preproc_args;		// recorded arguments to pass through to the preprocessor
lw_stringlist_t include_dirs;		// include paths specified with -I
lw_stringlist_t includes;			// include paths specified with -include
lw_stringlist_t user_sysincdirs;	// include paths specified with -isystem
lw_stringlist_t asm_args;			// recorded arguments to pass through to the assembler
lw_stringlist_t linker_args;		// recorded arguments to pass through to the linker 
lw_stringlist_t sysincdirs;			// the standard system include directories
lw_stringlist_t tempfiles;			// a list of temporary files created which need to be cleaned up
lw_stringlist_t compiler_args;		// recorded arguments to pass through to the compiler
lw_stringlist_t priv_sysincdirs;	// system include directories for lwcc itself

/* forward delcarations */
static void parse_command_line(int, char **);

/* signal handler for SIGTERM - all it does is record the fact that
   SIGTERM happened and propagate the signal to whatever child process
   might currently be running */
static void exit_on_signal(int sig)
{
	sigterm_received = 1;
	if (child_pid)
		kill(child_pid, SIGTERM);
}

/* utility function to carp about an error condition and bail */
void do_error(const char *f, ...)
{
	va_list arg;
	va_start(arg, f);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, f, arg);
	putc('\n', stderr);
	va_end(arg);
	exit(1);
}

/* utility function to carp about some condition; do not bail */
void do_warning(const char *f, ...)
{
	va_list arg;
	va_start(arg, f);
	fprintf(stderr, "WARNING: ");
	vfprintf(stderr, f, arg);
	putc('\n', stderr);
	va_end(arg);
}

/* utility function to print out an array of strings - stops at the first
   NULL string pointer. */
static void print_array(char **arr)
{
	int c = 0;
	while (*arr)
	{
		if (c)
			printf(" ");
		printf("%s", *arr);
		arr++;
		c = 1;
	}
}

/* expand any search path entries to reflect the sysroot and
   isysroot settings. Note that it does NOT apply to the compiler
   program search path */
static void expand_sysroot(void)
{
	/* list of path lists to process for replacements of = */
	lw_stringlist_t *lists[] = { &sysincdirs, &include_dirs, &user_sysincdirs, &lib_dirs, NULL };
	/* list of replacement strings for = in the same order */
	const char *sysroots[] = { isysroot, isysroot, isysroot, sysroot, NULL };
	size_t i, sysroot_len, value_len;
	char *path;
	lw_stringlist_t newlist;
	lw_stringlist_t working;
	char *s;
	
	/* for each list, run through entry by entry, do any needed replacement
	   and add the entry to a new list. Then replace the old list with the
	   new one. */
	for (i = 0; lists[i] != NULL; i++)
	{
		working = *lists[i];
		newlist = lw_stringlist_create();
		
		lw_stringlist_reset(working);
		for (s = lw_stringlist_current(working); s; s = lw_stringlist_next(working))
		{
			if (s[0] == '=')
			{
				sysroot_len = strlen(sysroots[i]);
				value_len = strlen(s);
				/* note that the skipped = will make up for the trailing NUL */
				path = lw_alloc(sysroot_len + value_len);
				memcpy(path, sysroots[i], sysroot_len);
				/* the +1 here will copy the trailing NUL */
				memcpy(path + sysroot_len, s + 1, value_len);
				lw_stringlist_addstring(newlist, path);
				lw_free(path);
			}
			else
			{
				lw_stringlist_addstring(newlist, s);
			}
		}
		lw_stringlist_destroy(working);
		*lists[i] = newlist;
	}
}

/* look for file fn in path list p which is okay for access mode mode.
   Return a string allocated by lw_alloc. */
static char *find_file(const char *fn, lw_stringlist_t p, int mode)
{
	char *s;
	char *f;
	size_t lf, lp;
	int need_slash;
	
	lf = strlen(fn);
	lw_stringlist_reset(p);
	for (s = lw_stringlist_current(p); s; s = lw_stringlist_next(p))
	{
		lp = strlen(s);
		need_slash = 0;
		if (lp && s[lp - 1] == '/')
			need_slash = 1;
		f = lw_alloc(lp + lf + need_slash + 1);
		memcpy(f, s, lp);
		if (need_slash)
			f[lp] = '/';
		/* +1 gets the NUL */
		memcpy(f + lp + need_slash, fn, lf + 1);
		if (access(f, mode) == 0)
			return f;
		lw_free(f);
	}
	/* if not found anywhere, try the bare filename - it might work */
	return lw_strdup(fn);
}

/* take a string list which contains an argv and execute the specified
   program */
static int execute_program(lw_stringlist_t args)
{
	int argc;
	char **argv;
	int result;
	char *s;
	
	argc = lw_stringlist_nstrings(args);
	argv = lw_alloc(sizeof(char *) * (argc + 1));
	lw_stringlist_reset(args);
	for (result = 0, s = lw_stringlist_current(args); s; s = lw_stringlist_next(args))
	{
		argv[result++] = s;
	}
	argv[result] = NULL;

	if (verbose_mode)
	{
		printf("Executing ");
		print_array(argv);
		printf("\n");
	}
	
	/* bail now if a signal happened */
	if (sigterm_received)
	{
		lw_free(argv);
		return 1;
	}

	/* make sure stdio has flushed everything so that output from the
	   child process doesn't get intermingled */
	fflush(NULL);
	
	/* now make the child process */
	child_pid = fork();
	if (child_pid == 0)
	{
		/* child process */
		/* try executing program */
		execvp(argv[0], argv);
		/* only way to get here is if execvp() failed so carp about it and exit */
		fprintf(stderr, "Exec of %s failed: %s", argv[0], strerror(errno));
		/* exit with failure but don't call any atexit(), etc., functions */
		_exit(127);
	}
	else if (child_pid == -1)
	{
		/* failure to make child process */
		do_error("Failed to execute program %s: %s", argv[0], strerror(errno));
	}
	/* clean up argv */
	lw_free(argv);
	
	/* parent process - wait for child to exit */
	while (waitpid(child_pid, &result, 0) == -1 && errno == EINTR)
		/* do nothing */;
	/* fetch actual return status */
	result = WEXITSTATUS(result);
	if (result)
	{
		/* carp about non-zero return status */
		do_error("%s terminated with status %d", argv[0], result);
	}
	/* return nonzero if signalled to exit */
	return sigterm_received;
}

/*
construct an output file name as follows:

1. if it is the last phase of compilation and an output file name is
   specified, use that if not specified
2. if it is the last phase or we are saving temporary files, any suffix
   on f is removed and replaced with nsuffix
3. otherwise, a temporary file is created. If necessary, a temporary
   directory is created to hold the temporary file. The name of the temporary
   file is recorded in the tempfiles string list for later cleanup. The name
   of the temporary directory is recorded in temp_directory for later cleanup.
*/
static char *output_name(const char *f, const char *nsuffix, int last)
{
	const char *osuffix;
	char *name;
	size_t lf, ls, len;
	int counter_len;
	
	/* get a new file counter */
	file_counter++;
	
	/* if the output was specified, use it */
	if (last && output_file)
	{
		return lw_strdup(output_file);
	}

	/* find the start of the old suffix */	
	osuffix = strrchr(f, '.');
	if (osuffix != NULL && strchr(osuffix, '/') != NULL)
		osuffix = NULL;
	if (osuffix == NULL)
		osuffix = f + strlen(f);
	
	ls = strlen(nsuffix);
	
	/* if this is the last stage or we're saving temps, use a name derived
	   from the original file name by replacing the suffix with nsuffix */
	if (save_temps || last)
	{
		lf = osuffix - f;
		name = lw_alloc(lf + ls + 1);
		memcpy(name, f, lf);
		/* note that the +1 will copy the trailing NUL */
		memcpy(name + lf, nsuffix, ls + 1);
		return name;
	}

	/* finally, use a temporary file */
	if (temp_directory == NULL)
	{
		/* if we haven't already made a temporary directory, do so */
		const char *dirtempl;
		char *path;
		size_t dirtempl_len;
		int need_slash;
		
		/* look for a TMPFIR environment variable and use that if present
		   but use /tmp as a fallback */
		dirtempl = getenv("TMPDIR");
		if (dirtempl == NULL)
			dirtempl = "/tmp";
		dirtempl_len = strlen(dirtempl);
		/* work out if we need to add a slash on the end of the directory */
		if (dirtempl_len && dirtempl[dirtempl_len - 1] == '/')
			need_slash = 0;
		else
			need_slash = 1;
		/* make a string of the form <tempdir>/lwcc-XXXXXX */
		path = lw_alloc(dirtempl_len + need_slash + 11 + 1);
		memcpy(path, dirtempl, dirtempl_len);
		if (need_slash)
			path[dirtempl_len] = '/';
		memcpy(path + dirtempl_len + need_slash, "lwcc-XXXXXX", 12);
		/* now make a temporary directory */
		if (mkdtemp(path) == NULL)
			do_error("mkdtemp failed: %s", strerror(errno));
		/* record the temporary directory name */
		temp_directory = path;
	}
	/* now create a file name in the temporary directory. The strategy here
	   uses a counter that is passed along and is guaranteed to be unique for
	   every file requested. */
	lf = strlen(temp_directory);
	/* this gets the length of the counter as a string but doesn't actually
	   allocate anything so we can make a string long enough */
	counter_len = snprintf(NULL, 0, "%d", file_counter);
	if (counter_len < 1)
		do_error("snprintf failure: %s", strerror(errno));
	len = lf + 1 + (size_t)counter_len + ls + 1;
	name = lw_alloc(len);
	/* it should be impossible for ths snprintf call to fail */
	snprintf(name, len, "%s/%d%s", temp_directory, file_counter, nsuffix);
	
	/* record the temporary file name for later */
	lw_stringlist_addstring(tempfiles, name);
	return name;
}

/* this calls the actual compiler, passing the contents of compiler_args
   as arguments. It also adds the input file and output file. */
static int compile_file(const char *file, char *input, char **output, const char *suffix)
{
	lw_stringlist_t args;
	char *out;
	int retval;
	char *s;
	
	args = lw_stringlist_create();
	
	/* find the compiler executable and make that argv[0] */
	s = find_file(compiler_program_name, program_dirs, X_OK);
	lw_stringlist_addstring(args, s);
	lw_free(s);
	
	/* add all the saved compiler arguments to argv */
	lw_stringlist_reset(compiler_args);
	for (s = lw_stringlist_current(compiler_args); s; s = lw_stringlist_next(compiler_args))
	{
		lw_stringlist_addstring(args, s);
	}
	/* work out the output file name and add that to argv */
	out = output_name(file, suffix, stop_after == PHASE_COMPILE);
	lw_stringlist_addstring(args, "-o");
	lw_stringlist_addstring(args, out);
	/* add the input file to argv */
	lw_stringlist_addstring(args, input);
	/* if the input file name and the output file name pointers are the same
	   free the input one */
	if (*output == input) 
		lw_free(input);
	/* tell the caller what the output name is */
	*output = out;
	/* actually run the compiler */
	retval = execute_program(args);

	lw_stringlist_destroy(args);
	return retval;
}

/* this calls the actual assembler, passing the contents of asm_args
   as arguments. It also adds the input file and output file. */
static int assemble_file(const char *file, char *input, char **output, const char *suffix)
{
	lw_stringlist_t args;
	char *out;
	int retval;
	char *s;
	
	args = lw_stringlist_create();
	
	/* find the assembler binary and add that as argv[0] */
	s = find_file(assembler_program_name, program_dirs, X_OK);
	lw_stringlist_addstring(args, s);
	lw_free(s);
	
	/* add some necessary args */
	lw_stringlist_addstring(args, "--format=obj");
	
	/* add asm_args to argv */
	lw_stringlist_reset(asm_args);
	for (s = lw_stringlist_current(asm_args); s; s = lw_stringlist_next(asm_args))
	{
		lw_stringlist_addstring(args, s);
	}
	/* get an output file name and add that to argv */
	out = output_name(file, ".o", stop_after == PHASE_ASSEMBLE);
	lw_stringlist_addstring(args, "-o");
	lw_stringlist_addstring(args, out);
	/* finally, add the input file */
	lw_stringlist_addstring(args, input);
	/* clean up input file name if same as output pointer */
	if (*output == input)
		lw_free(input);
	/* tell caller what file we made */
	*output = out;
	/* actually run the assembler */
	retval = execute_program(args);
	
	lw_stringlist_destroy(args);
	return retval;
}

/* run the preprocessor. Pass along preproc_args and appropriate options
   for all the include directories */
static int preprocess_file(const char *file, char *input, char **output, const char *suffix)
{
	lw_stringlist_t args;
	char *s;
	char *out;
	int retval;
	
	args = lw_stringlist_create();

	/* find the linker binary and make that argv[0] */	
	s = find_file(preprocessor_program_name, program_dirs, X_OK);
	lw_stringlist_addstring(args, s);
	lw_free(s);
	
	/* add preproc_args to argv */
	lw_stringlist_reset(preproc_args);
	for (s = lw_stringlist_current(preproc_args); s; s = lw_stringlist_next(preproc_args))
	{
		lw_stringlist_addstring(args, s);
	}
	
	/* add the include files specified by -i */
	lw_stringlist_reset(includes);
	for (s = lw_stringlist_current(includes); s; s = lw_stringlist_next(includes))
	{
		lw_stringlist_addstring(args, "-i");
		lw_stringlist_addstring(args, s);
	}

	/* add the include directories specified by -I */
	lw_stringlist_reset(include_dirs);
	for (s = lw_stringlist_current(include_dirs); s; s = lw_stringlist_next(include_dirs))
	{
		lw_stringlist_addstring(args, "-I");
		lw_stringlist_addstring(args, s);
	}

	/* add the user specified system include directories (-isystem) */
	lw_stringlist_reset(user_sysincdirs);
	for (s = lw_stringlist_current(user_sysincdirs); s; s = lw_stringlist_next(user_sysincdirs))
	{
		lw_stringlist_addstring(args, "-S");
		lw_stringlist_addstring(args, s);
	}

	/* and, if not -nostdinc, the standard system include directories */
	if (!nostdinc)
	{
		lw_stringlist_reset(priv_sysincdirs);
		for (s = lw_stringlist_current(priv_sysincdirs); s; s = lw_stringlist_next(priv_sysincdirs))
		{	
			lw_stringlist_addstring(args, "-S");
			lw_stringlist_addstring(args, s);
		}
		lw_stringlist_reset(sysincdirs);
		for (s = lw_stringlist_current(sysincdirs); s; s = lw_stringlist_next(sysincdirs))
		{	
			lw_stringlist_addstring(args, "-S");
			lw_stringlist_addstring(args, s);
		}
	}
	
	/* if we stop after preprocessing, output to stdout if no output file */
	if (stop_after == PHASE_PREPROCESS && output_file == NULL)
	{
		out = lw_strdup("-");
	}
	else
	{
		/* otherwise, make an output file */
		out = output_name(file, suffix, stop_after == PHASE_PREPROCESS);
	}
	/* if not stdout, add the output file to argv */
	if (strcmp(out, "-") != 0)
	{
		lw_stringlist_addstring(args, "-o");
		lw_stringlist_addstring(args, out);
	}
	/* add the input file name to argv */
	lw_stringlist_addstring(args, input);

	/* if input and output pointers are same, clean up input */	
	if (*output == input)
		lw_free(input);
	/* tell caller what our output file is */
	*output = out;
	/* finally, actually run the preprocessor */
	retval = execute_program(args);
	
	lw_stringlist_destroy(args);
	return retval;
}

/*
handle an input file through the various stages of compilation. If any
stage decides to handle an input file, that fact is recorded. If control
reaches the end of the function without a file being handled, that
fact is mentioned to the user. Unknown files are passed to the linker
if nothing handles them and linking is to be done. It's possible the linker
will actually know what to do with them.
*/
static int handle_input_file(const char *f)
{
	const char *suffix;
	char *src;
	int handled, retval;
	
	/* note: this needs to handle -x but for now, assume c for stdin */	
	if (strcmp(f, "-") == 0)
	{
		suffix = ".c";
	}
	else
	{
		/* work out the suffix on the file */
		suffix = strrchr(f, '.');
		if (suffix != NULL && strchr(suffix, '/') != NULL)
			suffix = NULL;
		if (suffix == NULL)
			suffix = "";
	}
	
	/* make a copy of the file */
	src = lw_strdup(f);
	
	/* preprocess if appropriate */
	if (strcmp(suffix, ".c") == 0)
	{
		/* preprocessed c input source goes to .i */
		suffix = ".i";
		retval = preprocess_file(f, src, &src, suffix);
		if (retval)
			goto done;
		handled = 1;
	}
	else if (strcmp(suffix, ".S") == 0)
	{
		/* preprocessed asm source goes to .s */
		suffix = ".s";
		retval = preprocess_file(f, src, &src, suffix);
		if (retval)
			goto done;
		handled = 1;
	}
	/* if we're only preprocessing, bail */
	if (stop_after == PHASE_PREPROCESS)
		goto done;
	
	/* now on to compile if appropriate */
	if (strcmp(suffix, ".i") == 0)
	{
		/* preprocessed c source goes to .s after compiling */
		suffix = ".s";
		retval = compile_file(f, src, &src, suffix);
		if (retval)
			goto done;
		handled = 1;
	}
	/* bail if we're only compiling, not assembling */
	if (stop_after == PHASE_COMPILE)
		goto done;
	
	/* assemble if appropriate */
	if (strcmp(suffix, ".s") == 0)
	{
		/* assembler output is an object file */
		suffix = ".o";
		retval = assemble_file(f, src, &src, suffix);
		if (retval)
			goto done;
		handled = 1;
	}
	/* bail if we're not linking */
	if (stop_after == PHASE_ASSEMBLE)
		goto done;

	/* if we get here with a .o unhandled, pretend it is handled */	
	if (strcmp(suffix, ".o") == 0)
		handled = 1;
	
	/* add the final file name to the linker args */
	lw_stringlist_addstring(linker_args, src);
done:
	if (!handled && !retval)
	{
		/* carp about unhandled files if there is no error */
		if (stop_after == PHASE_LINK)
		{
			do_warning("unknown suffix %s; passing file down to linker", suffix);
		}
		else
		{
			do_warning("unknown suffix %s; skipped", suffix);
		}
	}
	/* clean up the file name */
	lw_free(src);
	
	return retval;
}

/*
This actually runs the linker. Along the way, all the files the linker
is supposed to handle will have been added to linker_args.
*/
static int handle_linking(void)
{
	lw_stringlist_t linker_flags;
	char *s;
	int retval;
	
	linker_flags = lw_stringlist_create();
	
	/* find the linker binary and make that argv[0] */
	s = find_file(linker_program_name, program_dirs, X_OK);
	lw_stringlist_addstring(linker_flags, s);
	lw_free(s);
	
	/* tell the linker about the output file name, if specified */
	if (output_file)
	{
		lw_stringlist_addstring(linker_flags, "-o");
		lw_stringlist_addstring(linker_flags, (char *)output_file);
	}
	
	/* add the standard library options if not -nostdlib */
	if (!nostdlib)
	{
	}
	
	/* add the standard startup files if not -nostartfiles */
	if (!nostartfiles)
	{
	}
	
	/* pass along the various input files, etc., to the linker */
	lw_stringlist_reset(linker_args);
	for (s = lw_stringlist_current(linker_args); s; s = lw_stringlist_next(linker_args))
	{
		lw_stringlist_addstring(linker_flags, s);
	}
	
	/* actually run the linker */
	retval = execute_program(linker_flags);
	
	lw_stringlist_destroy(linker_flags);
	return retval;
}

/*
Do various setup tasks, process the command line, handle the input files,
and clean up.
*/
int main(int argc, char **argv)
{
	char *ap;
	int retval;
	
	input_files = lw_stringlist_create();
	runtime_dirs = lw_stringlist_create();
	lib_dirs = lw_stringlist_create();
	program_dirs = lw_stringlist_create();
	preproc_args = lw_stringlist_create();
	include_dirs = lw_stringlist_create();
	user_sysincdirs = lw_stringlist_create();
	asm_args = lw_stringlist_create();
	linker_args = lw_stringlist_create();
	sysincdirs = lw_stringlist_create();
	includes = lw_stringlist_create();
	tempfiles = lw_stringlist_create();
	compiler_args = lw_stringlist_create();
	priv_sysincdirs = lw_stringlist_create();
		
	parse_command_line(argc, argv);
	if (stop_after == PHASE_DEFAULT)
		stop_after = PHASE_LINK;

	if (verbose_mode)
		printf("%s\n", VERSTRING);

	if (isysroot == NULL)
		isysroot = sysroot;
	expand_sysroot();
	
	if (stop_after != PHASE_LINK && output_file && lw_stringlist_nstrings(input_files) > 1)
	{
		do_error("-o cannot be specified with multiple inputs unless linking");
	}
	
	// default to stdout for preprocessing
	if (stop_after == PHASE_PREPROCESS && output_file == NULL)
		output_file = "-";
	
	if (lw_stringlist_nstrings(input_files) == 0)
		do_error("No input files specified");

	/* handle -B here */
	ap = lw_alloc(strlen(basedir) + 10);
	strcpy(ap, basedir);
	strcat(ap, "/bin");
	lw_stringlist_addstring(program_dirs, ap);
	strcpy(ap, basedir);
	strcat(ap, "/lib");
	lw_stringlist_addstring(runtime_dirs, ap);
	strcpy(ap, basedir);
	strcat(ap, "/include");
	lw_stringlist_addstring(priv_sysincdirs, ap);
	lw_free(ap);
	
	retval = 0;
	/* make sure we exit if interrupted */
	signal(SIGTERM, exit_on_signal);
	
	/* handle input files */
	lw_stringlist_reset(input_files);
	for (ap = lw_stringlist_current(input_files); ap; ap = lw_stringlist_next(input_files))
	{
		if (handle_input_file(ap))
			retval = 1;
	}

	if (!retval && stop_after >= PHASE_LINK)
	{
		retval = handle_linking();
	}

	/* if a signal nixed us, mention the fact */
	if (sigterm_received)
		do_warning("Terminating on signal");

	/* clean up temporary files */
	if (!save_temps)
	{
		lw_stringlist_reset(tempfiles);
		for (ap = lw_stringlist_current(tempfiles); ap; ap = lw_stringlist_next(tempfiles))
		{
			if (unlink(ap) == -1)
			{
				do_warning("Removal of %s failed: %s", ap, strerror(errno));
			}
		}
		if (temp_directory)
		{
			if (rmdir(temp_directory) == -1)
			{
				do_warning("Removal of temporary directory %s failed: %s", temp_directory, strerror(errno));
			}
		}
	}

	/* be polite and clean up all the string lists */
	lw_stringlist_destroy(input_files);
	lw_stringlist_destroy(runtime_dirs);
	lw_stringlist_destroy(lib_dirs);
	lw_stringlist_destroy(program_dirs);
	lw_stringlist_destroy(preproc_args);
	lw_stringlist_destroy(include_dirs);
	lw_stringlist_destroy(user_sysincdirs);
	lw_stringlist_destroy(asm_args);
	lw_stringlist_destroy(linker_args);
	lw_stringlist_destroy(sysincdirs);
	lw_stringlist_destroy(includes);
	lw_stringlist_destroy(tempfiles);
	lw_stringlist_destroy(compiler_args);
	lw_stringlist_destroy(priv_sysincdirs);
	return retval;	
}

struct option_e
{
	char *optbase;				// base name of option, with -
	int needarg;				// nonzero if option needs argument
	int noextra;				// nonzero if there must not be anything after optbase
	int optcode;				// option code (passed to fn)
	void *optptr;				// pointer for opt (passed to fn)
	int (*fn)(char *, char *, int, void *); // function to handle argument, NULL to ignore it
};

enum CMD_MISC {
	CMD_MISC_VERSION,
	CMD_MISC_OPTIMIZE,
};

enum OPT_ARG {
	OPT_ARG_OPT = 0,			// argument is optional
	OPT_ARG_SEP = 1,			// argument may be separate
	OPT_ARG_INC = 2,			// argument must not be separate
};

/* set an integer at *optptr to optcode */
static int cmdline_set_int(char *opt, char *optarg, int optcode, void *optptr)
{
	*((int *)optptr) = optcode;
	return 0;
}

/* set a string at *optptr to optarg */
static int cmdline_set_string(char *opt, char *optarg, int optcode, void *optptr)
{
	char **s = (char **)optptr;
	*s = optarg;
	
	return 0;
}

/* set a string at *optptr to optarg */
static int cmdline_set_stringifnull(char *opt, char *optarg, int optcode, void *optptr)
{
	char **s = (char **)optptr;
	
	if (*s)
		do_error("Multiple %.*s options specified", optcode ? optcode : strlen(opt), opt);
	*s = optarg;
	
	return 0;
}

/* split arg on commas and add the results to string list *optptr */
static int cmdline_argsplit(char *opt, char *arg, int optcode, void *optptr)
{
	lw_stringlist_t l = *(lw_stringlist_t *)optptr;
	char *next;
	
	for (; arg != NULL; arg = next)
	{
		next = strchr(arg, ',');
		if (next != NULL)
			*next++ = '\0';
		lw_stringlist_addstring(l, arg);
	}
	return 0;
}

/* add opt to string list *optptr */
static int cmdline_arglist(char *opt, char *arg, int optcode, void *optptr)
{
	lw_stringlist_t l = *(lw_stringlist_t *)optptr;

	lw_stringlist_addstring(l, opt);
	return 0;
}

/* add optarg to string list *optptr */
static int cmdline_optarglist(char *opt, char *optarg, int optcode, void *optptr)
{
	lw_stringlist_t l = *(lw_stringlist_t *)optptr;

	lw_stringlist_addstring(l, optarg);
	return 0;
}

static int cmdline_misc(char *opt, char *optarg, int optcode, void *optptr)
{
	switch (optcode)
	{
	case CMD_MISC_VERSION:
		printf("%s\n", VERSTRING);
		exit(0);

	case CMD_MISC_OPTIMIZE:
		if (!optarg)
			return 0;
		switch (*optarg)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case 's':
			return 0;
		}
		return -1;

	default:
		return -1;
	}
	return 0;
}

static int cmdline_set_intifzero(char *opt, char *optarg, int optcode, void *optptr)
{
	int *iv = (int *)optptr;
	
	if (*iv && *iv != optcode)
	{
		do_error("conflicting compiler option specified: %s", opt);
	}
	*iv = optcode;
	return 0;
}

struct option_e optionlist[] =
{
	{ "--version",			OPT_ARG_OPT, 	1,	CMD_MISC_VERSION, 	NULL,			cmdline_misc },
	{ "--sysroot=",			OPT_ARG_INC,	0,	0,					&sysroot,		cmdline_set_string },
	{ "-B",					OPT_ARG_INC,	0,	0,					&basedir,		cmdline_set_string },
	{ "-C",					OPT_ARG_OPT,	1,	0,					&preproc_args,	cmdline_arglist },
	{ "-c",					OPT_ARG_OPT,	1,	PHASE_ASSEMBLE,		&stop_after,	cmdline_set_intifzero },
	{ "-D",					OPT_ARG_INC,	0,	0,					&preproc_args,	cmdline_arglist },
	{ "-E",					OPT_ARG_OPT,	1,	PHASE_PREPROCESS,	&stop_after,	cmdline_set_intifzero },
	{ "-fPIC",				OPT_ARG_OPT,	1,	2,					&pic_mode,		cmdline_set_int },
	{ "-fpic",				OPT_ARG_OPT,	1,	1,					&pic_mode,		cmdline_set_int },
	{ "-g",					OPT_ARG_OPT,	1,	1,					&debug_mode,	cmdline_set_int },
	{ "-I",					OPT_ARG_SEP,	0,	0,					&include_dirs,	cmdline_optarglist },
	{ "-include",			OPT_ARG_SEP,	1,	0,					&includes,		cmdline_optarglist },
	{ "-isysroot",			OPT_ARG_SEP,	1,	0,					&isysroot,		cmdline_set_string },
	{ "-isystem",			OPT_ARG_SEP,	1,	0,					&user_sysincdirs, cmdline_optarglist },
	{ "-M",					OPT_ARG_OPT,	1,	0,					&preproc_args,	cmdline_arglist },
	{ "-nostartfiles",		OPT_ARG_OPT,	1,	1,					&nostartfiles,	cmdline_set_int },
	{ "-nostdinc",			OPT_ARG_OPT,	1,	1,					&nostdinc,		cmdline_set_int },
	{ "-nostdlib",			OPT_ARG_OPT,	1,	1,					&nostdlib,		cmdline_set_int },
	{ "-O",					OPT_ARG_OPT,	0,	CMD_MISC_OPTIMIZE,	NULL,			cmdline_misc },
	{ "-o",					OPT_ARG_SEP,	0,	2,					&output_file,	cmdline_set_stringifnull },
	{ "-S",					OPT_ARG_OPT,	1,	PHASE_COMPILE,		&stop_after,	cmdline_set_intifzero },
	{ "-save-temps",		OPT_ARG_OPT,	1,	1,					&save_temps,	cmdline_set_int },
	{ "-trigraphs",			OPT_ARG_OPT,	1,	0,					&preproc_args,	cmdline_arglist },
	{ "-U",					OPT_ARG_INC,	0,	0,					&preproc_args,	cmdline_arglist },
	{ "-v",					OPT_ARG_OPT,	1,	1,					&verbose_mode,	cmdline_set_int },
	{ "-Wp,",				OPT_ARG_INC,	0,	0,					&preproc_args,	cmdline_argsplit },
	{ "-Wa,",				OPT_ARG_INC,	0,	0,					&asm_args,		cmdline_argsplit },
	{ "-Wl,",				OPT_ARG_INC,	0,	0,					&linker_args,	cmdline_argsplit },
	{ "-W",					OPT_ARG_INC,	0,	0,					NULL,			NULL }, /* warning options */
	{ "-x",					OPT_ARG_SEP,	1,	0,					NULL,			NULL }, /* language options */
	{ NULL, 0, 0 }
};

static void parse_command_line(int argc, char **argv)
{
	int i, j, olen, ilen;
	char *optarg;
	
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-' || argv[i][1] == '\0')
		{
			/* we have a non-option argument */
			lw_stringlist_addstring(input_files, argv[i]);
			continue;
		}
		olen = strlen(argv[i]);
		for (j = 0; optionlist[j].optbase; j++)
		{
			ilen = strlen(optionlist[j].optbase);
			/* if length of optbase is longer than argv[i], it can't match */
			if (ilen > olen)
				continue;
			/* does the base match? */
			if (strncmp(optionlist[j].optbase, argv[i], ilen) == 0)
				break;
		}
		if (optionlist[j].optbase == NULL)
		{
			do_error("Unsupported option %s", argv[i]);
		}
		/* is the option supposed to be exact? */
		if (optionlist[j].noextra && argv[i][ilen] != '\0')
		{
			do_error("Unsupported option %s", argv[i]);
		}
		/* is there an argument? */
		optarg = NULL;
		if (argv[i][ilen])
			optarg = argv[i] + ilen;
		if (!optarg && optionlist[j].needarg == 1)
		{
			if (i == argc)
			{
				do_error("Option %s requires an argument", argv[i]);
			}
			optarg = argv[++i];
		}
		if (!optarg && optionlist[j].needarg == 2)
		{
			do_error("Option %s requires an argument", argv[i]);
		}
		/* handle the option */
		if (optionlist[j].fn)
		{
			if ((*(optionlist[j].fn))(argv[i], optarg, optionlist[j].optcode, optionlist[j].optptr) != 0)
				do_error("Unsupported option %s %s", argv[i], optarg ? optarg : "");
		}
	}
}
