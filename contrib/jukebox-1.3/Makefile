#
# Makefile for JUKEBOX 1.3
# (C) 1993 Raphael Quinet (quinet@montefiore.ulg.ac.be)
#
# NOTE : you should "make" this program *before* you "make clean" in the
# main rplay directory ("../..").

#include <std_disclaimer.h>

include ../../Makefile.config
LIBRPLAY_DIR=   ../../librplay
INCLUDE_DIR=    ../../include

TARGET=	jukebox

SRCS = jukebox.c

OBJS = jukebox.o


all: 		$(TARGET)

$(TARGET): 	$(OBJS)
		$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

install:	$(TARGET)
		@echo Installing $(TARGET) in $(INSTALL_BIN_DIR)
		$(BIN_INSTALL) $(TARGET) $(INSTALL_BIN_DIR)

install.man:
		@echo Installing the man page in $(INSTALL_BIN_DIR)
		$(INSTALL) man1/$(TARGET).1 $(INSTALL_MAN_DIR)/man1
		chmod a+r $(INSTALL_MAN_DIR)/man1/$(TARGET).1

world: 		all install install.man

clean:
		$(RM) $(TARGET) $(OBJS) core a.out *~ *.bak man1/*.bak man1/*~ 

depend:
		$(MAKEDEPEND) -- $(CFLAGS) -- $(SRCS)

tar: 		clean
		(cd .. ; tar -cvf $(TARGET).tar $(TARGET) ; gzip -9 $(TARGET).tar )

zip: 		clean
		(cd .. ; zip -9 -o -y -r $(TARGET).zip $(TARGET) )

