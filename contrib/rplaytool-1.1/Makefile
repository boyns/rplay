## -- Begin Makefile configuration --

## Your favorite K&R C compiler.
CC = gcc

## rplaytool requires XView.  OPENWINHOME should be set to the
## directory that contains the XView include files and the
## XView libraries.
OPENWINHOME = /usr/openwin

## Where is `rplay.h' and `librplay.a' installed?
RPLAY_INCLUDE = /usr/local/include
RPLAY_LIB = /usr/local/lib

## Libraries you'll need for Solaris and maybe others.
#EXTRA_LIBS = -lsocket -lnsl

## The OpenWindows Developer's Guide is only required if you're
## going to modify this program.   
GUIDEHOME = /usr/guide

## -- End Makefile configuration --

PROGRAM = rplaytool
SOURCES.c = rptp.c misc.c getopt.c getopt1.c
SOURCES.h = 
SOURCES.G = rplaytool.G
STUBS.G = rplaytool.G

# Derived parameters.

SOURCES = \
	$(SOURCES.G) \
	$(SOURCES.h) \
	$(SOURCES.l) \
	$(SOURCES.y) \
	$(SOURCES.cps) \
	$(SOURCES.c)

TARGETS.c = \
	$(SOURCES.G:%.G=%_ui.c) \
	$(STUBS.G:%.G=%_stubs.c)

TARGETS.h = \
	$(SOURCES.G:%.G=%_ui.h) \
	$(SOURCES.l:%.l=%.h) \
	$(SOURCES.y:%.y=%.h) \
	$(SOURCES.cps:%.cps=%.h)

TARGETS.other = \
	$(SOURCES.G:%.G=%.info)

TARGETS = \
	$(TARGETS.other) \
	$(TARGETS.h) \
	$(TARGETS.c)

OBJECTS = \
	$(SOURCES.c:%.c=%.o) \
	$(TARGETS.c:%.c=%.o)

# Compiler flags.

CFLAGS += -g 
CPPFLAGS += -I$(RPLAY_INCLUDE) -I$(GUIDEHOME)/include -I$(OPENWINHOME)/include -DMAIN
LDFLAGS += -L$(RPLAY_LIB) -L$(GUIDEHOME)/lib -L$(OPENWINHOME)/lib
##LDLIBS += -lrplay -lguidexv -lguide -lxview -lolgx -lX11
LDLIBS += -lrplay -lxview -lolgx -lX11 $(EXTRA_LIBS)

# Standard targets.

all:	 $(TARGETS.other) $(PROGRAM)
objects: $(SOURCES.c) $(TARGETS.c) $(TARGETS.h) $(OBJECTS)
sources: $(SOURCES)
targets: $(SOURCES) $(TARGETS)

$(PROGRAM): $(SOURCES.c) $(TARGETS.c) $(TARGETS.h) $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

# Targets to be used by Saber-C.

saber_src:
	#load $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
		$(SOURCES.c) $(TARGETS.c) $(LDLIBS)

saber_obj:
	#load $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $(OBJECTS) $(LDLIBS)

clean:
	$(RM) $(OBJECTS) *.BAK *.delta core $(PROGRAM) *~

guideclean:
	$(RM) $(SOURCES.G:%.G=%_ui.c) $(TARGETS.other) $(TARGETS.h) $(OBJECTS) *.BAK *.delta core $(PROGRAM) *~

%_ui.c: %.G
	$(GUIDEHOME)/bin/gxv $*

%_ui.h: %_ui.c
	@touch $@

%_stubs.c: %_ui.c
	@touch $@

%.info: %_ui.c
	@touch $@
