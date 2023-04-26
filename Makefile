# define anything system specific here
#
# set these variables if needed
# PROGSUFFIX: suffix added to binaries
# BUILDTPREFIX: prefix added to build utilities (cc, etc.) for xcompile
# can also set them when invoking "make"
#PROGSUFFIX := .exe
#BUILDTPREFIX=i586-mingw32msvc-

ifneq ($(DESTDIR),)
INSTALLDIR = $(DESTDIR)/usr/bin
else
INSTALLDIR ?= /usr/local/bin
endif

# this are probably pointless but they will make sure
# the variables are set without overriding the environment
# or automatic values from make itself.
CC ?= cc
AR ?= ar
RANLIB ?= ranlib

# Set variables for cross compiling
ifneq ($(BUILDTPREFIX),)
CC := $(BUILDTPREFIX)$(CC)
AR := $(BUILDTPREFIX)$(AR)
RANLIB := $(BUILDTPREFIX)$(RANLIB)
endif

CPPFLAGS += -I lwlib -Icommon
LDFLAGS += -Llwlib -llw

CFLAGS ?= -O3 -Wall -Wno-char-subscripts

MAIN_TARGETS := lwasm/lwasm$(PROGSUFFIX) \
	lwlink/lwlink$(PROGSUFFIX) \
	lwar/lwar$(PROGSUFFIX) \
	lwlink/lwobjdump$(PROGSUFFIX)

.PHONY: all
all: $(MAIN_TARGETS)

lwar_srcs := add.c extract.c list.c lwar.c main.c remove.c replace.c
lwar_srcs := $(addprefix lwar/,$(lwar_srcs))

lwlib_srcs := lw_alloc.c lw_realloc.c lw_free.c lw_error.c lw_expr.c \
	lw_stack.c lw_string.c lw_stringlist.c lw_cmdline.c
lwlib_srcs := $(addprefix lwlib/,$(lwlib_srcs))

lwlink_srcs := main.c lwlink.c readfiles.c expr.c script.c link.c output.c map.c
lwobjdump_srcs := objdump.c
lwlink_srcs := $(addprefix lwlink/,$(lwlink_srcs))
lwobjdump_srcs := $(addprefix lwlink/,$(lwobjdump_srcs))

lwasm_srcs := cycle.c debug.c input.c insn_bitbit.c insn_gen.c insn_indexed.c \
	insn_inh.c insn_logicmem.c insn_rel.c insn_rlist.c insn_rtor.c insn_tfm.c \
	instab.c list.c lwasm.c macro.c main.c os9.c output.c pass1.c pass2.c \
	pass3.c pass4.c pass5.c pass6.c pass7.c pragma.c pseudo.c section.c \
	struct.c symbol.c symdump.c unicorns.c
lwasm_srcs := $(addprefix lwasm/,$(lwasm_srcs))

lwasm_objs := $(lwasm_srcs:.c=.o)
lwlink_objs := $(lwlink_srcs:.c=.o)
lwar_objs := $(lwar_srcs:.c=.o)
lwlib_objs := $(lwlib_srcs:.c=.o)
lwobjdump_objs := $(lwobjdump_srcs:.c=.o)

lwasm_deps := $(lwasm_srcs:.c=.d)
lwlink_deps := $(lwlink_srcs:.c=.d)
lwar_deps := $(lwar_srcs:.c=.d)
lwlib_deps := $(lwlib_srcs:.c=.d)
lwobjdump_deps := $(lwobjdump_srcs:.c=.d)

.PHONY: lwlink lwasm lwar lwobjdump
lwlink: lwlink/lwlink$(PROGSUFFIX)
lwasm: lwasm/lwasm$(PROGSUFFIX)
lwar: lwar/lwar$(PROGSUFFIX)
lwobjdump: lwlink/lwobjdump$(PROGSUFFIX)

lwasm/lwasm$(PROGSUFFIX): $(lwasm_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwasm_objs) $(LDFLAGS)

lwlink/lwlink$(PROGSUFFIX): $(lwlink_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwlink_objs) $(LDFLAGS)

lwlink/lwobjdump$(PROGSUFFIX): $(lwobjdump_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwobjdump_objs) $(LDFLAGS)

lwar/lwar$(PROGSUFFIX): $(lwar_objs) lwlib
	@echo Linking $@
	@$(CC) -o $@ $(lwar_objs) $(LDFLAGS)

#.PHONY: lwlib
.INTERMEDIATE: lwlib
lwlib: lwlib/liblw.a

lwlib/liblw.a: $(lwlib_objs)
	@echo Linking $@
	@$(AR) rc $@ $(lwlib_objs)
	@$(RANLIB) $@

alldeps := $(lwasm_deps) $(lwlink_deps) $(lwar_deps) $(lwlib_deps) ($lwobjdump_deps)

-include $(alldeps)

extra_clean := $(extra_clean) *~ */*~

%.o: %.c
	@echo "Building dependencies for $@"
	@$(CC) -MM $(CPPFLAGS) -o $*.d $<
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o $*.d:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp
	@echo Building $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
	

.PHONY: clean
clean: $(cleantargs)
	@echo "Cleaning up"
	@rm -f lwlib/liblw.a lwasm/lwasm$(PROGSUFFIX) lwlink/lwlink$(PROGSUFFIX) lwlink/lwobjdump$(PROGSUFFIX) lwar/lwar$(PROGSUFFIX)
	@rm -f $(lwasm_objs) $(lwlink_objs) $(lwar_objs) $(lwlib_objs) $(lwobjdump_objs)
	@rm -f $(extra_clean)
	@rm -f */*.exe

.PHONY: realclean
realclean: clean $(realcleantargs)
	@echo "Cleaning up even more"
	@rm -f $(lwasm_deps) $(lwlink_deps) $(lwar_deps) $(lwlib_deps) $(lwobjdump_deps)

print-%:
	@echo $* = $($*)

.PHONY: install
install: $(MAIN_TARGETS)
	install -d $(INSTALLDIR)
	install $(MAIN_TARGETS) $(INSTALLDIR)

.PHONY: test
test: all test/runtests
	@test/runtests

