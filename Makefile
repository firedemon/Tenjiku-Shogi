#CFLAGS= -DDEBUG -DDEBUG -DMOVELOG -DSTACKLOG  -lgdbm -g -pg  -fbounds-checking
CFLAGS=  -g   -DCENTIPLY -DQUIESCE
#CFLAGS=-g -DEVAL_INFLUENCE -DCENTIPLY -fbounds-checking -DDEBUG
#CFLAGS=-O3 -DEVAL_INFLUENCE -DCENTIPLY -fomit-frame-pointer -mpentiumpro -march=pentiumpro -malign-functions=4 -funroll-loops -fexpensive-optimizations -malign-double -fschedule-insns2 -mwide-multiply
# -DCENTIPLY
#LDFLAGS = -lgdbm 
#CFLAGS=  -g -pg
LDFLAGS =  -lgdbm -lreadline
# LDFLAGS =  -lgdbm -lreadline -lncurses


# all: tenjiku tenjiku-nox11 tenjiku.book.in TeX
all: tenjiku

tenjiku.book.in: book.pl book.dat
	perl book.pl book.dat > tenjiku.book.in

tenjiku: board.o data-ansi.o eval.o main-ansi.o search.o book.o kbhit.o
	gcc  ${CFLAGS} -o tenjiku board.o data-ansi.o eval.o main-ansi.o search.o book.o kbhit.o ${LDFLAGS}

book.o: book.c data.h defs.h protos.h Makefile
	gcc  ${CFLAGS} -c book.c

board.o: board.c data.h defs.h protos.h Makefile
	gcc  ${CFLAGS} -c board.c

data-ansi.o: data-ansi.c data.h defs.h protos.h Makefile
	gcc ${CFLAGS} -c data-ansi.c

eval.o: eval.c data.h defs.h protos.h Makefile
	gcc ${CFLAGS} -c eval.c

kbhit.o: kbhit.c
	gcc ${CFLAGS} -c kbhit.c

main-ansi.o: main-ansi.c data.h defs.h protos.h Makefile
	gcc ${CFLAGS} -c main-ansi.c

search.o: search.c data.h defs.h protos.h Makefile
	gcc ${CFLAGS}  -c search.c

###################################################3
###################################################3

clean:
	rm -f tenjiku tenjiku-nox11 *.log *.o *~ gmon.out
