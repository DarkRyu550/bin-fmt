CC=clang
CFLAGS=-Wall -Wextra -Werror -g -O0

LD=clang
LFLAGS=
LIBS=

OBJS=bf.o
bf: $(OBJS)
	$(LD) $(LFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o bf
