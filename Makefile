LIB=libmill.a

OFILES=\
	chan.o\
	cr.o\
	debug.o\
	list.o\
	poller.o\
	slist.o\
	stack.o\
	timer.o\

CC=gcc
CFLAGS=-fvisibility=hidden -DMILL_EXPORTS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -c

%.o: %.c
	$(CC) $(CFLAGS) $*.c

$(LIB): $(OFILES)
	ar rvc $(LIB) *.o

clean: 
	rm -f *.o $(LIB)

install: $(LIB)
	cp $(LIB) /usr/local/lib
	cp libmill.h /usr/local/include