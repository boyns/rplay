include ../../Makefile.config

CPPFLAGS=	$(CC_OPTIONS) -I../../include -I/usr/X11R6/include

.c.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

LDFLAGS=	$(LD_OPTIONS) -L../../librplay -L/usr/X11R6/lib -lrplay -lforms -lXpm -lX11 -lm

OBJS=		pos.o

pos:		$(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)
