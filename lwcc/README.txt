This is the lwcc C compiler for lwtools. It was written using various other
C compilers as guides. Special thanks to the developers of the PCC compiler.
While none of the actual code from PCC was actually used, much of compiler
itself served as a template for creating lwcc.


LIMITATIONS AND DESIGN CHOICES
==============================

The direct interface to both the compiler proper and the preprocessor is
specifically undefined. Indeed, the preprocessor may, in fact, not be a
separate program at all. Relying on the specific format of the output of the
preprocessor is specifically forbidden even though it is possible to obtain
preprocessed output from the compiler driver. This is provided for debugging
purposes only.

The preprocessor supports variadic macros. It also supports stringification,
and token concatenation but only within a macro expansion. There are
examples online that use the construct "#__LINE__" to get a string version
of the current line number.

The preprocessor defaults to ignoring trigraphs because they are basically a
stupid idea on any current system. They have their place for systems where
creating the nine characters specified by the trigraphs is very difficult or
impossible. It is possible, however, to instruct the preprocessor to decode
trigraph sequences.

The nonstandard "#pragma once" feature is not supported at all. The effect
is easily accomplished using standard macros and conditionals. It is,
therefore, unneeded complexity.

The nonstandard idea of preprocessor assertions is also completely
unsupported. It is just as easy to test predefined macros and such tests are
much more portable.

The preprocessor supports __LINE__, __FILE__, __DATE__, and __TIME__. The
compiler itself supports __func__ as a predefined string constant if
encountered because there is no way for the preprocessor to determine what
function it occurs within. The preprocessor does not define __STDC__,
__STDC_VERSION__, or __STDC_HOSTED__. I have seen no truly useful purpose
for these and since lwcc does not, at this time, conform to any known C
standard, it would be incorrect to define the first two.

The compiler driver may define additional macros depending on its idea of
the context.


RUNTIME INFORMATION
===================

The compiler driver has a built in base directory where it searches for its
various components as needed. In the discussion below, BASEDIR stands for
that directory.

BASEDIR may be specified by the -B option to the driver. Care must be taken
when doing so, however, because specifying an invalid -B will cause the
compiler to fail completely. It will completely override the built in search
paths for the compiler provided files and programs.

Because BASEDIR is part of the actual compiler, it is not affected by
--sysroot or -isysroot options.

If BASEDIR does not exist, compiler component programs will be searched for
in the standard execution paths. This may lead to incorrect results so it is
important to make certain that the specified BASEDIR exists.

If -B is not specified, the default BASEDIR is
$(PREFIX)/lib/lwcc/$(VERSION)/ where PREFIX is the build prefix from the
Makefile and VERSION is the lwtools version.

The contents of BASEDIR are as follows:

BASEDIR/bin

Various binaries for the parts of the compiler system. Notably, this
includes the preprocessor and compiler proper. The specific names and
contents of this directory cannot be relied upon and these programs should
not be called directly. Ever. Don't do it.


BASEDIR/lib

This directory contains various libraries that provide support for any
portion of the compiler's output. The driver will arrange to pass the
appropriate arguments to the linker to include these as required.

The most notable file in this directory is liblwcc.a wich contains the
support routines for the compiler's code generation. Depending on ABI and
code generation options supported, there may be multiple versions of
liblwcc.a. The driver will arrange for the correct one to be referenced.


BASEDIR/include

This directory contains any C header files that the compiler provides.
Notably, this includes stdint.h, stdarg.h, and setjmp.h as these are
specific to the compiler. The driver will arrange for this directory to be
searched prior to the standard system directories so that these files will
override any present in those directories.

