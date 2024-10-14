CFILES = src/ecgsyn.c src/opt.c src/dfour1.c src/ran1.c
CFLAGS = -O

CC = gcc

ecgsyn:		$(CFILES) src/opt.h
	$(CC) $(CFLAGS) -o ecgsyn $(CFILES) -lm

clean:
	rm -f *~ *.o *.obj
