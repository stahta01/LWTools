With LWTOOLS 4.0, a substantial reorganization of the project has occurred.
This document serves to explain the reasoning behind the various changes.

The most obvious change is that the gnu auto tools have been eliminated.
While they proved useful for initial distribution of the software,
particularly for construction of the win32 binaries, they have since proved
to add an unacceptable level of complexity to every aspect of development
from merely tinkering with source files to doing complete releases. Thus,
the auto tools have been ditched in favour of specific hand tuned help where
required.

The other substantial change is that the source code repository has been
recreated from scratch. The old repository was full of cruft from various
revision control systems that were used over the years (CVS, Subversion, and
Mercurial). It was felt that starting a new Mercurial repository with a
completely clean slate would simplify matters substantially. Thus, the old
repository now serves as an archive.
