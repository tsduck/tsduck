#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Common makefile definitions for the TSDuck project.
#  To be included in all makefiles in the project.
#
#-----------------------------------------------------------------------------

# If no target precedes the inclusion of this file, use "default" as target.

.PHONY: first default
first: default
	@true

# Representation of characters which cannot be literally used in makefile expressions but
# can be substituted from a variable. $(NL) contains a new-line and shall be defined like
# this, with two lines between "define" and "endef", to get one new-line.

EMPTY =
SPACE = $(EMPTY) $(EMPTY)
COMMA = ,
define NL


endef

# Check if "make -k" is specified (ie. continue on error).

CONTINUE := $(if $(findstring k,$(filter-out --%,$(MAKEFLAGS))),true,)

# The function RECURSE recurses the current make targets in the specified subdirectories.
# Example: $(call RECURSE,src test1 test2)

RECURSE = for dir in $(1); do if [[ -d $$dir ]]; then $(MAKE) -C $$dir $@ $(if $(CONTINUE),,|| exit $$?); fi; done

# Some nerds define exotic shells as default. Stay to a known shell.
# Skip initialization files to speed up and reproduceability.
# Print all commands if SHELL_VERBOSE is defined.

SHELL = /usr/bin/env bash --noprofile --norc $(if $(SHELL_VERBOSE),-x)

# A function which preserves line separators in the output of a shell command.
# Warning: this is GNU Make Black Magic, think twice before modifying...

_SHELL_LINES = $(subst -=EOL=-,$(NL),$(shell ($(1)) | sed 's/$$/-=EOL=-/'))

# Build most make variables from a script. This used to be done here in the makefile but
# it became too complicated and was moved to a script. All input variables are passed on
# the command line. In the generated output, all variables are exported and transmitted
# to sub-makes. To avoid "argument too long" on the input command line, we remove known
# long variables which are either useless or rebuilt from scratch in the script.
# Note about _ROOTDIR: The directory which contains the currently included Makefile is
# the project root.

_ROOTDIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST)))))
_EXCLUDES = ^% @% *% \%% <% >% +% -% _% ?% .% PS1 PS2 PROMPT_% DBUS_% HIST% ALL_% ANDROID_% PREPROCESS.% COMPILE.% LINK.% LINT.% CPPFLAGS CXXFLAGS %_INCLUDES GITHUB_%
_VARIABLES = $(filter-out $(_EXCLUDES),$(.VARIABLES)) ALL_INCLUDES
$(eval $(call _SHELL_LINES,$(_ROOTDIR)/scripts/make-config.sh $(foreach v,$(_VARIABLES),$(v)='$($(v))')))

# Compilation rules

$(OBJDIR)/%.o: %.cpp
	$(call LOG,[CXX] $<) mkdir -p $(OBJDIR); $(CXX) $(CXXFLAGS) -c -o $@ $<
$(BINDIR)/%: $(OBJDIR)/%.o
	$(call LOG,[LD] $@) $(CXX) $(LDFLAGS) $^ $(LDLIBS_EXTRA) $(LDLIBS) -o $@
$(BINDIR)/%$(SO_SUFFIX): $(OBJDIR)/%.o
	$(call LOG,[LD] $@) $(CXX) $(SOFLAGS) $(LDFLAGS) $^ $(LDLIBS_EXTRA) $(LDLIBS) -shared -o $@

# Static linking.

.PHONY: static
static:
	+@$(MAKE) STATIC=true

#-----------------------------------------------------------------------------
# Dependency (.dep) files
#-----------------------------------------------------------------------------

# For each xxx.cpp source file, there is one xxx.dep file in the same
# directory as xxx.o. The file xxx.dep lists all .h dependencies for xxx.cpp.
#
# When make is invoked, the .dep files are automatically updated. This is
# required for most targets, those which build files. Some targets are
# informational or do not need to build object files. In this case,
# rebuilding the .dep files is useless but harmless. However, in some
# cases such as the standard "clean" targets, the .dep files should
# not be rebuilt. In fact, they are deleted by the target. In these
# cases, the "clean" targets must not implicitly rebuild the .dep
# files. The standard "clean" targets are automatically added in NODEPS,
# there is no need to set them. However, if a makefile defines some
# non-standard "clean" targets, it should set them in NODEPS before
# inclusion of this make file.
#
# If $(DONT_BUILD_DEPS) is defined, do not attempt to build the header
# dependency files. Useful when the source file depend on dynamically
# generated header files (initially non-existent).
#
# $(NODEPS) lists the make targets in the parent makefile which shall
# not enforce the generation of header dependency files. This is a bit
# complicated and, most of the time, there is no need to set this variable.
#
# $(NODEPS_SRC) lists local source files for which we should not generate
# a .dep, probably because they are not used on that platform and trying
# to parse them for .dep would return an error.

$(OBJDIR)/%.dep: %.cpp
	$(call LOG,[DEP] $<) mkdir -p $(OBJDIR); \
	$(CXX) -MM $(CPPFLAGS) -MT $(OBJDIR)/$*.o -MT $@ $< >$@ || rm -f $@

NODEPS += clean cmacros coverity cppcheck cppcheck-xml cxxmacros distclean doxygen \
          flawfinder cloc listvars scan-build unixify

ifeq ($(DONT_BUILD_DEPS),)
    ifneq ($(MAKECMDGOALS),)
        DONT_BUILD_DEPS := $(if $(filter-out $(NODEPS),$(MAKECMDGOALS)),,true)
    endif
endif

ifeq ($(DONT_BUILD_DEPS),)
    -include $(addprefix $(OBJDIR)/,$(addsuffix .dep,$(notdir $(basename $(filter-out $(NODEPS_SRC),$(wildcard *.cpp))))))
endif

#-----------------------------------------------------------------------------
# Generic utilities, available at all levels of directory.
#-----------------------------------------------------------------------------

# Build a tarbal of the source tree.

ifeq ($(SOURCE_TARBALL),)
    SOURCE_TARBALL = $(INSTALLERDIR)/tsduck-$(shell $(GET_TSDUCK_VERSION)).tgz
endif
TARNAME = $(notdir $(basename $(SOURCE_TARBALL)))

PHONY: tarball
tarball:
	rm -rf $(TMPROOT)
	mkdir -p $(TMPROOT)/$(TARNAME)
	cd $(ROOTDIR); \
	    $(TAR) -cpf - sample/sample-*/japanese-tables.bin \
	    $(patsubst %,--exclude '%',.git $(shell cat $(ROOTDIR)/.gitignore)) \
	    . | $(TAR) -C $(TMPROOT)/$(TARNAME) -xpf -
	$(MAKE) -C $(TMPROOT)/$(TARNAME) clean
	$(TAR) -C $(TMPROOT) -czf $(SOURCE_TARBALL) -p --owner=0 --group=0 $(TARNAME)
	rm -rf $(TMPROOT)

# Cleanup Windows oddities in source files.
# Many IDE's indent with tabs, and tabs are 4 chars wide.
# Tabs shall not be expanded in Makefiles.

.PHONY: unixify
unixify:
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  expand -t 4 $$f >$$f.tmp; \
	  $(CHMOD) --reference=$$f $$f.tmp; \
	  mv -f $$f.tmp $$f; \
	done
	for f in $$(find . -name \*.c -o -name \*.cpp -o -name \*.h -o -name Makefile\* -o -name \*.sh -o -name \*.dox -o -name \*.md -o -name \*.xml -o -name \*.txt); do \
	  dos2unix $(if $(FREEBSD)$(DRAGONFLYBSD),,-q) $$f; \
	  $(SED) -i -e 's/  *$$//' $$f; \
	done

# Display make variables for debug purposes.

.PHONY: listvars
listvars:
	@true
	$(foreach v, \
	  $(sort $(filter-out .% ^% @% _% *% \%% <% +% ?% BASH% LS_COLORS SSH% VTE% XDG% F_%,$(.VARIABLES))), \
	  $(info $(v) = "$($(v))"))

# Display predefined macros for C++

.PHONY: cxxmacros
cxxmacros:
	@$(CPP) $(CXXFLAGS) -x c++ -dM /dev/null | sort
