#
# Build Makefile for GNU Make
#

# Notes:
#  As of 6 July 2005, the engine should handle optimisations being enabled.
#  If things seem to be going wrong, lower or disable optimisations, then
#  try again. If things are still going wrong, call me.
#   

# Use colored output
PRETTY_OUTPUT = 1

# Engine options - these may be overridden by game makefiles
#  SUPERBUILD     - enables voxels
#  POLYMOST       - enables Polymost renderer
#  USE_OPENGL     - enables OpenGL support in Polymost
#  NOASM          - disables the use of inline assembly pragmas
#  LINKED_GTK     - enables compile-time linkage to GTK
#

SUPERBUILD ?= 1
POLYMOST ?= 0		
POLYMER ?= 0
USE_OPENGL ?= 0
NOASM ?= 1
LINKED_GTK ?= 0
BUILD32_ON_64 ?= 0
NEDMALLOC ?= 0

ifeq (0,$(USE_OPENGL))
    POLYMER = 0
endif

# Debugging/Build options
#  RELEASE - 1 = no debugging
#  EFENCE  - 1 = compile with Electric Fence for malloc() debugging
#  OPTLEVEL	- 0..3 = GCC optimization strategy 
#
RELEASE?=1
DEBUGANYWAY?=0
KRANDDEBUG?=0
EFENCE?=0
OPTLEVEL?=3
PROFILER?=0

ifneq (0,$(KRANDDEBUG))
    RELEASE=0
endif
ifneq (0,$(PROFILER))
    DEBUGANYWAY=1
endif

# SDK locations for Windows - adjust to match your setup
#
DXROOT=c:/sdks/directx/dx61
FMODROOTWIN=c:/sdks/fmodapi374win/api

# Build locations - OBJ gets overridden to the game-specific objects dir
#
OBJ?=obj.gnu
SRC=src
RSRC=rsrc
INC=include
SDL_INC=/opt/FunKey-sdk/arm-funkey-linux-musleabihf/sysroot/usr/include/

# ENETROOT=$(SRC)/enet

# Filename extensions - these won't need to change
#
o=o
res=o
asm=nasm

# Debugging and release
#
ifneq ($(RELEASE),0)
    # Debugging disabled
    debug=-fomit-frame-pointer -funswitch-loops -O$(OPTLEVEL) $(F_NO_STACK_PROTECTOR)
    LIBS=-lm
else
    # Debugging enabled
    debug=-ggdb -O0 -DDEBUGGINGAIDS -DNOSDLPARACHUTE
    LIBS=-lm $(L_SSP) -Wl,--enable-auto-import -rdynamic
    ifneq (0,$(KRANDDEBUG))
        debug+=-DKRANDDEBUG=1
    endif
    ifneq (0,$(PROFILER))
        LIBS+= -lprofiler
        debug+= -pg
    endif
endif

ifneq (0,$(DEBUGANYWAY))
    debug+=-ggdb
endif

CHAINPREFIX ?= /opt/FunKey-sdk
CROSS_COMPILE ?= $(CHAINPREFIX)/bin/arm-funkey-linux-musleabihf-

CC    ?= $(CROSS_COMPILE)gcc
CXX   ?= $(CROSS_COMPILE)g++
#CC=gcc
#CXX=g++
#AS=nasm
#RC=windres
#AR=ar
#RANLIB=ranlib
RANLIB ?= $(CROSS_COMPILE)ranlib
ARCH=-fstrength-reduce -fthread-jumps -fexpensive-optimizations -fomit-frame-pointer -frename-registers -pipe -g0 -ffast-math

OURCFLAGS=$(debug) -W -Wall -Wimplicit -w \
	-Wno-char-subscripts -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-DKSFORBUILD -I$(INC) -I$(SDL_INC) -D_FORTIFY_SOURCE=2 \
    $(F_JUMP_TABLES) $(ARCH)

OURCXXFLAGS=-fno-exceptions -fno-rtti
ASFLAGS=-s #-g
EXESUFFIX=

UTILLIBS=-lpthread

include Makefile.shared

ENGINEOBJS=
ifeq (0,$(NOASM))
  ENGINEOBJS+= $(OBJ)/a.$o
endif

ifneq ($(PLATFORM),WINDOWS)
    LIBS+= -ldl -pthread
endif


ENGINEOBJS+= \
	$(OBJ)/a-c.$o \
	$(OBJ)/baselayer.$o \
	$(OBJ)/cache1d.$o \
	$(OBJ)/compat.$o \
	$(OBJ)/crc32.$o \
	$(OBJ)/defs.$o \
	$(OBJ)/engine.$o \
	$(OBJ)/polymost.$o \
	$(OBJ)/hightile.$o \
	$(OBJ)/textfont.$o \
	$(OBJ)/smalltextfont.$o \
	$(OBJ)/kplib.$o \
	$(OBJ)/quicklz.$o \
	$(OBJ)/md4.$o \
	$(OBJ)/osd.$o \
	$(OBJ)/pragmas.$o \
	$(OBJ)/scriptfile.$o \
	$(OBJ)/mutex.$o

ifeq (1,$(USE_OPENGL))
    ENGINEOBJS+=  $(OBJ)/mdsprite.$o
    ENGINEOBJS+=  $(OBJ)/glbuild.$o
endif

ifeq (1,$(POLYMER))
    ENGINEOBJS+=  $(OBJ)/polymer.$o
endif

ifeq (1,$(NEDMALLOC))
    ENGINEOBJS+=  $(OBJ)/nedmalloc.$o
endif

EDITOROBJS=$(OBJ)/build.$o \
	$(OBJ)/config.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
    ASFLAGS+= -f elf
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINEOBJS  += $(OBJ)/StartupWinController.editor.$o \
            $(OBJ)//osxbits.$o
    ASFLAGS     += -f Mach-O
    OURCFLAGS   += -fno-pic
    TARGETOPTS  += -DNOASM
endif
ifeq ($(PLATFORM),BSD)
    ASFLAGS+= -f elf
    OURCFLAGS+= -I/usr/X11R6/include
endif
ifeq ($(PLATFORM),WINDOWS)
    OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include -I$(FMODROOTWIN)/inc # -I$(ENETROOT)/include
    LIBS+= -lwsock32 -lws2_32 -lwinmm #-L$(ENETROOT)
    ASFLAGS+= -DUNDERSCORES -f win32
endif
ifeq ($(PLATFORM),BEOS)
    ASFLAGS+= -f elf
    TARGETOPTS+= -DNOASM
endif
ifeq ($(PLATFORM),SYLLABLE)
    ASFLAGS+= -f elf
endif

ifeq ($(RENDERTYPE),SDL)
    ENGINEOBJS+= $(OBJ)/sdlayer.$o
    OURCFLAGS+= $(subst -Dmain=SDL_main,,$(SDLCONFIG_CFLAGS))

    ifeq (1,$(HAVE_GTK2))
        OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
        ENGINEOBJS+= $(OBJ)/gtkbits.$o
        ifeq ($(LINKED_GTK),0)
            ENGINEOBJS+= $(OBJ)/dynamicgtk.$o
        endif
        EDITOROBJS+= $(OBJ)/startgtk.editor.$o
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    ENGINEOBJS+= $(OBJ)/winlayer.$o $(OBJ)/rawinput.$o
    EDITOROBJS+= $(OBJ)/startwin.editor.$o
endif

ifneq (0,$(EFENCE))
    LIBS+= -lefence
    OURCFLAGS+= -DEFENCE
endif

OURCFLAGS+= $(BUILDCFLAGS)

.SILENT:
.PHONY: clean veryclean all utils enginelib editorlib

# TARGETS

UTILS=kextract$(EXESUFFIX) kgroup$(EXESUFFIX) transpal$(EXESUFFIX) wad2art$(EXESUFFIX) wad2map$(EXESUFFIX)

# all: $(OBJ)/$(ENGINELIB) $(OBJ)/$(EDITORLIB)
utils: $(UTILS)

enginelib: $(OBJ)/$(ENGINELIB)
$(OBJ)/$(ENGINELIB): $(ENGINEOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

editorlib: $(OBJ)/$(EDITORLIB)
$(OBJ)/$(EDITORLIB): $(EDITOROBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

pragmacheck$(EXESUFFIX): $(OBJ)/pragmacheck.$o $(OBJ)/pragmas.$o
	$(CC) $(subst -Dmain=app_main,,$(OURCFLAGS)) -o $@ $^

kextract$(EXESUFFIX): $(OBJ)/kextract.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
kgroup$(EXESUFFIX): $(OBJ)/kgroup.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
transpal$(EXESUFFIX): $(OBJ)/transpal.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
wad2art$(EXESUFFIX): $(OBJ)/wad2art.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
wad2map$(EXESUFFIX): $(OBJ)/wad2map.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
generateicon$(EXESUFFIX): $(OBJ)/generateicon.$o $(OBJ)/kplib.$o
	$(CC) -o $@ $^ $(UTILLIBS)
cacheinfo$(EXESUFFIX): $(OBJ)/cacheinfo.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(UTILLIBS)
enumdisplay$(EXESUFFIX): src/misc/enumdisplay.c
	$(CC) -g -Os -o $@ $^ $(UTILLIBS) -I$(DXROOT)/include -lgdi32
mapdump$(EXESUFFIX): $(OBJ)/mapdump.$o
	$(CC) -o $@ $^ $(UTILLIBS)

# DEPENDENCIES
include Makefile.deps

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	if $(AS) $(ASFLAGS) $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.m
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cxx
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(INC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/*

veryclean: clean
	-rm -f $(ENGINELIB) $(EDITORLIB) $(UTILS)

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done

# DO NOT DELETE


