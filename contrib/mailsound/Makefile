include ../../Makefile.config

CPPFLAGS=	$(CC_OPTIONS) -I../../include

.c.o:
		$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

LDFLAGS=	$(LD_OPTIONS) -L../../librplay -lrplay

OBJS=		mailsound.o

mailsound:	$(OBJS)
		$(CC) -o $@ $(OBJS) $(LDFLAGS)

